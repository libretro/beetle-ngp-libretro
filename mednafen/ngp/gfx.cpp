//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

#include "neopop.h"
#include "mem.h"
#include "gfx.h"
#include "interrupt.h"
#include "dma.h"
#include "TLCS-900h/TLCS900h_registers.h"

static void ngpgfx_delayed_settings(ngpgfx_t *gfx)
{
	//Window dimensions
	gfx->winx = gfx->WBA_H;
	gfx->winy = gfx->WBA_V;
	gfx->winw = gfx->WSI_H;
	gfx->winh = gfx->WSI_V;

	/* Scroll Planes (Confirmed delayed) */
	gfx->scroll1x = gfx->S1SO_H;
	gfx->scroll1y = gfx->S1SO_V;

	gfx->scroll2x = gfx->S2SO_H;
	gfx->scroll2y = gfx->S2SO_V;

	/* Sprite offset (Confirmed delayed) */
	gfx->scrollsprx = gfx->PO_H;
	gfx->scrollspry = gfx->PO_V;

	/* Plane Priority (Confirmed delayed) */
	gfx->planeSwap = gfx->P_F & 0x80;

	/* Background colour register (Confirmed delayed) */
	gfx->bgc = gfx->BG_COL;

	/* 2D Control register (Confirmed delayed) */
	gfx->oowc     = gfx->CONTROL_2D & 7;
	gfx->negative = gfx->CONTROL_2D & 0x80;
}

void ngpgfx_reset(ngpgfx_t *gfx)
{
   memset(gfx->SPPLT,    0x7, sizeof(gfx->SPPLT));
   memset(gfx->SCRP1PLT, 0x7, sizeof(gfx->SCRP1PLT));
   memset(gfx->SCRP2PLT, 0x7, sizeof(gfx->SCRP2PLT));

   gfx->raster_line    = 0;
   gfx->S1SO_H         = 0;
   gfx->S1SO_V         = 0;
   gfx->S2SO_H         = 0;
   gfx->S2SO_V         = 0;
   gfx->WBA_H          = 0;
   gfx->WBA_V          = 0;

   gfx->WSI_H          = 0xFF;
   gfx->WSI_V          = 0xFF;

   gfx->C_OVR          = 0;
   gfx->BLNK           = 0;

   gfx->PO_H           = 0;
   gfx->PO_V           = 0;
   gfx->P_F            = 0;

   gfx->BG_COL         = 0x7;
   gfx->CONTROL_2D     = 0;
   gfx->CONTROL_INT    = 0;
   gfx->SCREEN_PERIOD  = 0xC6;
   gfx->K2GE_MODE      = 0;

   ngpgfx_delayed_settings(gfx);
}

void ngpgfx_power(ngpgfx_t *gfx)
{
   ngpgfx_reset(gfx);

   memset(gfx->ScrollVRAM, 0, sizeof(gfx->ScrollVRAM));
   memset(gfx->CharacterRAM, 0, sizeof(gfx->CharacterRAM));
   memset(gfx->SpriteVRAM, 0, sizeof(gfx->SpriteVRAM));
   memset(gfx->SpriteVRAMColor, 0, sizeof(gfx->SpriteVRAMColor));
   memset(gfx->ColorPaletteRAM, 0, sizeof(gfx->ColorPaletteRAM));
}


bool ngpgfx_hint(ngpgfx_t *gfx)
{
   /* H_Int / Delayed settings */
   if ((gfx->raster_line < SCREEN_HEIGHT-1 || gfx->raster_line == gfx->SCREEN_PERIOD))
   {
      /* Get delayed settings */
      ngpgfx_delayed_settings(gfx);

      /* Allowed? */
      if (gfx->CONTROL_INT & 0x40)
         return 1;
   }

   return 0;
}

void ngpgfx_set_pixel_format(ngpgfx_t *gfx)
{
   unsigned i;

   for(i = 0; i < 4096; i++)
   {
      int r = (i & 0xF) * 17;
      int g = ((i >> 4) & 0xF) * 17;
      int b = ((i >> 8) & 0xF) * 17;

      gfx->ColorMap[i] = MAKECOLOR(r, g, b, 0);
   }
}

bool ngpgfx_draw(ngpgfx_t *gfx, MDFN_Surface *surface, bool skip)
{
   unsigned x;
   bool ret = 0;

   /* Draw the scanline */
   if (gfx->raster_line < SCREEN_HEIGHT && !skip)
   {
      uint16_t *dest = surface->pixels16 + surface->pitchinpix * gfx->raster_line;

      if (!gfx->K2GE_MODE)
         draw_scanline_colour(gfx, dest, gfx->layer_enable, gfx->raster_line);
      else
         draw_scanline_mono(gfx, dest, gfx->layer_enable, gfx->raster_line);

      for (x = 0; x < SCREEN_WIDTH; x++)
         dest[x] = gfx->ColorMap[dest[x] & 4095];
   }
   gfx->raster_line++;

   /* V_Int? */
   if (gfx->raster_line == SCREEN_HEIGHT)
   {
      gfx->BLNK = 1;
      ret = 1;

      if(gfx->CONTROL_INT & 0x80) /* (statusIFF() <= 4 */
         TestIntHDMA(5, 0x0B);
   }

   /* End of V_Int */
   if(gfx->raster_line == gfx->SCREEN_PERIOD + 1)
   {
      /* Last scanline + 1 */
      gfx->raster_line = 0;
      gfx->C_OVR = 0;
      gfx->BLNK = 0;
   }

   return ret;
}

int ngpgfx_StateAction(ngpgfx_t *gfx, void *data, int load, int data_only)
{
   StateMem *sm = (StateMem*)data;
   SFORMAT StateRegs[] =
   {
      SFVARN(gfx->raster_line, "raster_line"),
      SFVARN(gfx->S1SO_H, "S1SO_H"),
      SFVARN(gfx->S1SO_V, "S1SO_V"),
      SFVARN(gfx->S2SO_H, "S2SO_H"),
      SFVARN(gfx->S2SO_V, "S2SO_V"),
      SFVARN(gfx->WBA_H, "WBA_H"),
      SFVARN(gfx->WBA_V, "WBA_V"),
      SFVARN(gfx->WSI_H, "WSI_H"),
      SFVARN(gfx->WSI_V, "WSI_V"),
      SFVARN(gfx->C_OVR, "C_OVR"),
      SFVARN(gfx->BLNK, "BLNK"),
      SFVARN(gfx->PO_H, "PO_H"),
      SFVARN(gfx->PO_V, "PO_V"),
      SFVARN(gfx->P_F, "P_F"),
      SFVARN(gfx->BG_COL, "BG_COL"),
      SFVARN(gfx->CONTROL_2D, "CONTROL_2D"),
      SFVARN(gfx->CONTROL_INT, "CONTROL_INT"),
      SFVARN(gfx->SCREEN_PERIOD, "SCREEN_PERIOD"),
      SFVARN(gfx->K2GE_MODE, "K2GE_MODE"),

      SFARRAYN(gfx->SPPLT, 6, "SPPLT"),
      SFARRAYN(gfx->SCRP1PLT, 6, "SCRP1PLT"),
      SFARRAYN(gfx->SCRP2PLT, 6, "SCRP2PLT"),

      SFVARN(gfx->winx, "winx"),
      SFVARN(gfx->winw, "winw"),
      SFVARN(gfx->winy, "winy"),
      SFVARN(gfx->winh, "winh"),
      SFVARN(gfx->scroll1x, "scroll1x"),
      SFVARN(gfx->scroll1y, "scroll1y"),
      SFVARN(gfx->scroll2x, "scroll2x"),
      SFVARN(gfx->scroll2y, "scroll2y"),
      SFVARN(gfx->scrollsprx, "scrollsprx"),
      SFVARN(gfx->scrollspry, "scrollspry"),
      SFVARN(gfx->planeSwap, "planeSwap"),
      SFVARN(gfx->bgc, "bgc"),
      SFVARN(gfx->oowc, "oowc"),

      SFVARN(gfx->negative, "negative"),

      SFARRAYN(gfx->ScrollVRAM, 4096, "ScrollVRAM"),
      SFARRAYN(gfx->CharacterRAM, 8192, "CharacterRAM"),
      SFARRAYN(gfx->SpriteVRAM, 256, "SpriteVRAM"),
      SFARRAYN(gfx->SpriteVRAMColor, 0x40, "SpriteVRAMColor"),
      SFARRAYN(gfx->ColorPaletteRAM, 0x200, "ColorPaletteRAM"),

      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "GFX"))
      return(0);

   return(1);
}

void ngpgfx_SetLayerEnableMask(ngpgfx_t *gfx, uint64_t mask)
{
   gfx->layer_enable = mask;
}

void ngpgfx_write8(ngpgfx_t *gfx, uint32 address, uint8 data)
{
#if 0
   if(address >= 0x8032 && address <= 0x8035)
      printf("%08x %02x %d\n", address, data, ngpc_soundTS);
#endif

   if(address >= 0x9000 && address <= 0x9fff)
      gfx->ScrollVRAM[address - 0x9000] = data;
   else if(address >= 0xa000 && address <= 0xbfff)
      gfx->CharacterRAM[address - 0xa000] = data;
   else if(address >= 0x8800 && address <= 0x88ff)
      gfx->SpriteVRAM[address - 0x8800] = data;
   else if(address >= 0x8c00 && address <= 0x8c3f)
      gfx->SpriteVRAMColor[address - 0x8c00] = data & 0x0f;
   else if(address >= 0x8200 && address <= 0x83ff)
      gfx->ColorPaletteRAM[address - 0x8200] = data;
   else switch(address)
   {
      case 0x8000:
         gfx->CONTROL_INT = data & 0xC0;
         break;
      case 0x8002:
         gfx->WBA_H = data;
         break;
      case 0x8003:
         gfx->WBA_V = data;
         break;
      case 0x8004:
         gfx->WSI_H = data;
         break;
      case 0x8005:
         gfx->WSI_V = data;
         break;
      case 0x8006:
         gfx->SCREEN_PERIOD = data;
         break;
      case 0x8012:
         gfx->CONTROL_2D = data & 0x87;
         break;
      case 0x8020:
         gfx->PO_H = data;
         break;
      case 0x8021:
         gfx->PO_V = data;
         break;
      case 0x8030:
         gfx->P_F = data & 0x80;
         break;
      case 0x8032:
         gfx->S1SO_H = data;
         break;
      case 0x8033:
         gfx->S1SO_V = data;
         break;
      case 0x8034:
         gfx->S2SO_H = data;
         break;
      case 0x8035:
         gfx->S2SO_V = data;
         break;
      case 0x8101:
         gfx->SPPLT[0] = data & 0x7;
         break;
      case 0x8102:
         gfx->SPPLT[1] = data & 0x7;
         break;
      case 0x8103:
         gfx->SPPLT[2] = data & 0x7;
         break;
      case 0x8105:
         gfx->SPPLT[3] = data & 0x7;
         break;
      case 0x8106:
         gfx->SPPLT[4] = data & 0x7;
         break;
      case 0x8107:
         gfx->SPPLT[5] = data & 0x7;
         break;
      case 0x8109:
         gfx->SCRP1PLT[0] = data & 0x7;
         break;
      case 0x810a:
         gfx->SCRP1PLT[1] = data & 0x7;
         break;
      case 0x810b:
         gfx->SCRP1PLT[2] = data & 0x7;
         break;
      case 0x810d:
         gfx->SCRP1PLT[3] = data & 0x7;
         break;
      case 0x810e:
         gfx->SCRP1PLT[4] = data & 0x7;
         break;
      case 0x810f:
         gfx->SCRP1PLT[5] = data & 0x7;
         break;
      case 0x8111:
         gfx->SCRP2PLT[0] = data & 0x7;
         break;
      case 0x8112:
         gfx->SCRP2PLT[1] = data & 0x7;
         break;
      case 0x8113:
         gfx->SCRP2PLT[2] = data & 0x7;
         break;

      case 0x8115:
         gfx->SCRP2PLT[3] = data & 0x7;
         break;
      case 0x8116:
         gfx->SCRP2PLT[4] = data & 0x7;
         break;
      case 0x8117:
         gfx->SCRP2PLT[5] = data & 0x7;
         break;

      case 0x8118:
         gfx->BG_COL = data & 0xC7;
         break;

      case 0x87e0:
         if(data == 0x52) 
         {
            puts("GEreset");
            reset(); 
         }
         break;
      case 0x87e2:
         gfx->K2GE_MODE = data & 0x80;
         break;
   }
}

void ngpgfx_write16(ngpgfx_t *gfx, uint32 address, uint16 data)
{
   ngpgfx_write8(gfx, address, data & 0xFF);
   ngpgfx_write8(gfx, address + 1, data >> 8);
}

uint8_t ngpgfx_read8(ngpgfx_t *gfx, uint32_t address)
{
   if(address >= 0x9000 && address <= 0x9fff)
      return(gfx->ScrollVRAM[address - 0x9000]);
   else if(address >= 0xa000 && address <= 0xbfff)
      return(gfx->CharacterRAM[address - 0xa000]);
   else if(address >= 0x8800 && address <= 0x88ff)
      return(gfx->SpriteVRAM[address - 0x8800]);
   else if(address >= 0x8c00 && address <= 0x8c3f)
      return(gfx->SpriteVRAMColor[address - 0x8c00]);
   else if(address >= 0x8200 && address <= 0x83ff)
      return(gfx->ColorPaletteRAM[address - 0x8200]);
   else switch(address)
   {
      case 0x8000:
         return gfx->CONTROL_INT;
      case 0x8002:
         return gfx->WBA_H;
      case 0x8003:
         return gfx->WBA_V;
      case 0x8004:
         return gfx->WSI_H;
      case 0x8005:
         return gfx->WSI_V;
      case 0x8006:
         return gfx->SCREEN_PERIOD;
      case 0x8008:
         return( (uint8)((abs(TIMER_HINT_RATE - (int)timer_hint)) >> 2) ); //RAS.H read (Simulated horizontal raster position)
      case 0x8009:
         return gfx->raster_line;
      case 0x8010:
         return((gfx->C_OVR ? 0x80 : 0x00) | (gfx->BLNK ? 0x40 : 0x00));
      case 0x8012:
         return gfx->CONTROL_2D;
      case 0x8020:
         return gfx->PO_H;
      case 0x8021:
         return gfx->PO_V;
      case 0x8030:
         return gfx->P_F;
      case 0x8032:
         return gfx->S1SO_H;
      case 0x8033:
         return gfx->S1SO_V;
      case 0x8034:
         return gfx->S2SO_H;
      case 0x8035:
         return gfx->S2SO_V;
      case 0x8101:
         return gfx->SPPLT[0];
      case 0x8102:
         return gfx->SPPLT[1];
      case 0x8103:
         return gfx->SPPLT[2];
      case 0x8105:
         return gfx->SPPLT[3];
      case 0x8106:
         return gfx->SPPLT[4];
      case 0x8107:
         return gfx->SPPLT[5];
      case 0x8108:
         return gfx->SCRP1PLT[0];
      case 0x8109:
         return gfx->SCRP1PLT[1];
      case 0x810a:
         return gfx->SCRP1PLT[2];
      case 0x810d:
         return gfx->SCRP1PLT[3];
      case 0x810e:
         return gfx->SCRP1PLT[4];
      case 0x810f:
         return gfx->SCRP1PLT[5];
      case 0x8111:
         return gfx->SCRP2PLT[0];
      case 0x8112:
         return gfx->SCRP2PLT[1];
      case 0x8113:
         return gfx->SCRP2PLT[2];
      case 0x8115:
         return gfx->SCRP2PLT[3];
      case 0x8116:
         return gfx->SCRP2PLT[4];
      case 0x8117:
         return gfx->SCRP2PLT[5];
      case 0x8118:
         return(gfx->BG_COL);
      case 0x87e2:
         return(gfx->K2GE_MODE);
   }

   return 0;
}

uint16_t ngpgfx_read16(ngpgfx_t *gfx, uint32_t address)
{
   return (ngpgfx_read8(gfx, address) | (ngpgfx_read8(gfx, address + 1) << 8));
}

