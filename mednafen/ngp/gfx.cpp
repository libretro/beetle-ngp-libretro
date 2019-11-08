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

#define gfx NGPGfx

void NGPGfx_reset(void)
{
   memset(gfx->SPPLT, 0x7, sizeof(gfx->SPPLT));
   memset(gfx->SCRP1PLT, 0x7, sizeof(gfx->SCRP1PLT));
   memset(gfx->SCRP2PLT, 0x7, sizeof(gfx->SCRP2PLT));

   gfx->raster_line = 0;
   gfx->S1SO_H = 0;
   gfx->S1SO_V = 0;
   gfx->S2SO_H = 0;
   gfx->S2SO_V = 0;
   gfx->WBA_H = 0;
   gfx->WBA_V = 0;

   gfx->WSI_H = 0xFF;
   gfx->WSI_V = 0xFF;

   gfx->C_OVR = 0;
   gfx->BLNK = 0;

   gfx->PO_H = 0;
   gfx->PO_V = 0;
   gfx->P_F = 0;

   gfx->BG_COL = 0x7;
   gfx->CONTROL_2D = 0;
   gfx->CONTROL_INT = 0;
   gfx->SCREEN_PERIOD = 0xC6;
   gfx->K2GE_MODE = 0;

   NGPGfx_delayed_settings();
}

void NGPGfx_power(void)
{
   NGPGfx_reset();

   memset(gfx->ScrollVRAM, 0, sizeof(gfx->ScrollVRAM));
   memset(gfx->CharacterRAM, 0, sizeof(gfx->CharacterRAM));
   memset(gfx->SpriteVRAM, 0, sizeof(gfx->SpriteVRAM));
   memset(gfx->SpriteVRAMColor, 0, sizeof(gfx->SpriteVRAMColor));
   memset(gfx->ColorPaletteRAM, 0, sizeof(gfx->ColorPaletteRAM));
}

void NGPGfx_delayed_settings(void)
{
   //Window dimensions
   gfx->winx = gfx->WBA_H;
   gfx->winy = gfx->WBA_V;
   gfx->winw = gfx->WSI_H;
   gfx->winh = gfx->WSI_V;

   // Scroll Planes (Confirmed delayed)
   gfx->scroll1x = gfx->S1SO_H;
   gfx->scroll1y = gfx->S1SO_V;

   gfx->scroll2x = gfx->S2SO_H;
   gfx->scroll2y = gfx->S2SO_V;

   // Sprite offset (Confirmed delayed)
   gfx->scrollsprx = gfx->PO_H;
   gfx->scrollspry = gfx->PO_V;

   // Plane Priority (Confirmed delayed)
   gfx->planeSwap = gfx->P_F & 0x80;

   // Background colour register (Confirmed delayed)
   gfx->bgc = gfx->BG_COL;

   // 2D Control register (Confirmed delayed)
   gfx->oowc = gfx->CONTROL_2D & 7;
   gfx->negative = gfx->CONTROL_2D & 0x80;
}

bool NGPGfx_hint(void)
{
   // H_Int / Delayed settings
   if ((gfx->raster_line < SCREEN_HEIGHT-1 || gfx->raster_line == gfx->SCREEN_PERIOD))
   {
      NGPGfx_delayed_settings(); // Get delayed settings

      // Allowed?
      if (gfx->CONTROL_INT & 0x40)
         return (1);
   }
   return (0);
}

void NGPGfx_set_pixel_format(void)
{
   for(int x = 0; x < 4096; x++)
   {
      int r = (x & 0xF) * 17;
      int g = ((x >> 4) & 0xF) * 17;
      int b = ((x >> 8) & 0xF) * 17;

      gfx->ColorMap[x] = MAKECOLOR(r, g, b, 0);
   }
}

bool NGPGfx_draw(MDFN_Surface *surface, bool skip)
{
   bool ret = 0;

   // Draw the scanline
   if (gfx->raster_line < SCREEN_HEIGHT && !skip)
   {
      if (!gfx->K2GE_MODE) NGPGfx_draw_scanline_colour(gfx->layer_enable_setting, gfx->raster_line);
      else                 NGPGfx_draw_scanline_mono(gfx->layer_enable_setting, gfx->raster_line);

      uint16 *dest = surface->pixels + surface->pitch * gfx->raster_line;
      for (int x = 0; x < SCREEN_WIDTH; x++)
	  {
         dest[x] = gfx->ColorMap[gfx->cfb_scanline[x] & 4095];
	  }
   }
   gfx->raster_line++;

   // V_Int?
   if (gfx->raster_line == SCREEN_HEIGHT)
   {
      gfx->BLNK = 1;
      ret = 1;

      if(gfx->CONTROL_INT & 0x80) // (statusIFF() <= 4
         TestIntHDMA(5, 0x0B);
   }

   // End of V_Int
   if(gfx->raster_line == gfx->SCREEN_PERIOD + 1) // Last scanline + 1
   {
      gfx->raster_line = 0;
      gfx->C_OVR = 0;
      gfx->BLNK = 0;
   }

   return (ret);
}

int NGPGfx_StateAction(StateMem *sm, int load, int data_only)
{
   SFORMAT StateRegs[] =
   {
      SFVAR(gfx->raster_line),
      SFVAR(gfx->S1SO_H), SFVAR(gfx->S1SO_V), SFVAR(gfx->S2SO_H), SFVAR(gfx->S2SO_V),
      SFVAR(gfx->WBA_H), SFVAR(gfx->WBA_V), SFVAR(gfx->WSI_H), SFVAR(gfx->WSI_V),
      SFVAR(gfx->C_OVR), SFVAR(gfx->BLNK),
      SFVAR(gfx->PO_H), SFVAR(gfx->PO_V),
      SFVAR(gfx->P_F),
      SFVAR(gfx->BG_COL),
      SFVAR(gfx->CONTROL_2D),
      SFVAR(gfx->CONTROL_INT),
      SFVAR(gfx->SCREEN_PERIOD),
      SFVAR(gfx->K2GE_MODE),

      SFVAR(gfx->SPPLT),
      SFVAR(gfx->SCRP1PLT),
      SFVAR(gfx->SCRP2PLT),

      SFVAR(gfx->winx), SFVAR(gfx->winw),
      SFVAR(gfx->winy), SFVAR(gfx->winh),
      SFVAR(gfx->scroll1x), SFVAR(gfx->scroll1y),
      SFVAR(gfx->scroll2x), SFVAR(gfx->scroll2y),
      SFVAR(gfx->scrollsprx), SFVAR(gfx->scrollspry),
      SFVAR(gfx->planeSwap),
      SFVAR(gfx->bgc), SFVAR(gfx->oowc),

      SFVAR(gfx->negative),

      SFVAR(gfx->ScrollVRAM),
      SFVAR(gfx->CharacterRAM),
      SFVAR(gfx->SpriteVRAM),
      SFVAR(gfx->SpriteVRAMColor),
      SFVAR(gfx->ColorPaletteRAM),

      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "GFX", false))
      return(0);

   return(1);
}

void NGPGfx_SetLayerEnableMask(uint64 mask)
{
   gfx->layer_enable_setting = mask;
}

//extern uint32 ngpc_soundTS;
void NGPGfx_write8(uint32 address, uint8 data)
{
   //if(address >= 0x8032 && address <= 0x8035)
      //printf("%08x %02x %d\n", address, data, ngpc_soundTS);

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
      //default: printf("HuhW: %08x\n", address); break;

      case 0x8000: gfx->CONTROL_INT = data & 0xC0; break;
      case 0x8002: gfx->WBA_H = data; break;
      case 0x8003: gfx->WBA_V = data; break;
      case 0x8004: gfx->WSI_H = data; break;
      case 0x8005: gfx->WSI_V = data; break;
      case 0x8006: gfx->SCREEN_PERIOD = data; break;
      case 0x8012: gfx->CONTROL_2D = data & 0x87; break;
      case 0x8020: gfx->PO_H = data; break;
      case 0x8021: gfx->PO_V = data; break;
      case 0x8030: gfx->P_F = data & 0x80; break;
      case 0x8032: gfx->S1SO_H = data; break;
      case 0x8033: gfx->S1SO_V = data; break;
      case 0x8034: gfx->S2SO_H = data; break;
      case 0x8035: gfx->S2SO_V = data; break;

      case 0x8101: gfx->SPPLT[0] = data & 0x7; break;
      case 0x8102: gfx->SPPLT[1] = data & 0x7; break;
      case 0x8103: gfx->SPPLT[2] = data & 0x7; break;

      case 0x8105: gfx->SPPLT[3] = data & 0x7; break;
      case 0x8106: gfx->SPPLT[4] = data & 0x7; break;
      case 0x8107: gfx->SPPLT[5] = data & 0x7; break;

      case 0x8109: gfx->SCRP1PLT[0] = data & 0x7; break;
      case 0x810a: gfx->SCRP1PLT[1] = data & 0x7; break;
      case 0x810b: gfx->SCRP1PLT[2] = data & 0x7; break;

      case 0x810d: gfx->SCRP1PLT[3] = data & 0x7; break;
      case 0x810e: gfx->SCRP1PLT[4] = data & 0x7; break;
      case 0x810f: gfx->SCRP1PLT[5] = data & 0x7; break;

      case 0x8111: gfx->SCRP2PLT[0] = data & 0x7; break;
      case 0x8112: gfx->SCRP2PLT[1] = data & 0x7; break;
      case 0x8113: gfx->SCRP2PLT[2] = data & 0x7; break;

      case 0x8115: gfx->SCRP2PLT[3] = data & 0x7; break;
      case 0x8116: gfx->SCRP2PLT[4] = data & 0x7; break;
      case 0x8117: gfx->SCRP2PLT[5] = data & 0x7; break;

      case 0x8118: gfx->BG_COL = data & 0xC7; break;

      case 0x87e0: if(data == 0x52) 
                   {
                      puts("GEreset");
                      NGPGfx_reset(); 
                   }
                   break;
      case 0x87e2: gfx->K2GE_MODE = data & 0x80; break;
   }
}

void NGPGfx_write16(uint32 address, uint16 data)
{
   NGPGfx_write8(address, data & 0xFF);
   NGPGfx_write8(address + 1, data >> 8);
}

#if 0
namespace TLCS900H
{
   extern uint32 pc;
};
#endif

uint8 NGPGfx_read8(uint32 address)
{
#if 0
   if(address >= 0x8200 && address <= 0xbfff)
   {
      printf("[GFX] Read8: %08x -- %08x\n", address, TLCS900H::pc);
      if(pc == 0x0028dd3d) //21)
      {
         TLCS900H::pc = 0x28DD15;
         for(int x = 0; x < 256; x++)
            puts(TLCS900h_disassemble());

         abort();
      }
   }
#endif

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
      //default: printf("Huh: %08x\n", address); break;
      case 0x8000: return(gfx->CONTROL_INT);
      case 0x8002: return(gfx->WBA_H);
      case 0x8003: return(gfx->WBA_V);
      case 0x8004: return(gfx->WSI_H);
      case 0x8005: return(gfx->WSI_V);
      case 0x8006: return(gfx->SCREEN_PERIOD);

      case 0x8008: return( (uint8)((abs(TIMER_HINT_RATE - (int)timer_hint)) >> 2) ); //RAS.H read (Simulated horizontal raster position)
      case 0x8009: return(gfx->raster_line);
      case 0x8010: return((gfx->C_OVR ? 0x80 : 0x00) | (gfx->BLNK ? 0x40 : 0x00));
      case 0x8012: return(gfx->CONTROL_2D);
      case 0x8020: return(gfx->PO_H);
      case 0x8021: return(gfx->PO_V);
      case 0x8030: return(gfx->P_F);
      case 0x8032: return(gfx->S1SO_H);
      case 0x8033: return(gfx->S1SO_V);
      case 0x8034: return(gfx->S2SO_H);
      case 0x8035: return(gfx->S2SO_V);

      case 0x8101: return(gfx->SPPLT[0]); break;
      case 0x8102: return(gfx->SPPLT[1]); break;
      case 0x8103: return(gfx->SPPLT[2]); break;

      case 0x8105: return(gfx->SPPLT[3]); break;
      case 0x8106: return(gfx->SPPLT[4]); break;
      case 0x8107: return(gfx->SPPLT[5]); break;

      case 0x8108: return(gfx->SCRP1PLT[0]); break;
      case 0x8109: return(gfx->SCRP1PLT[1]); break;
      case 0x810a: return(gfx->SCRP1PLT[2]); break;

      case 0x810d: return(gfx->SCRP1PLT[3]); break;
      case 0x810e: return(gfx->SCRP1PLT[4]); break;
      case 0x810f: return(gfx->SCRP1PLT[5]); break;

      case 0x8111: return(gfx->SCRP2PLT[0]); break;
      case 0x8112: return(gfx->SCRP2PLT[1]); break;
      case 0x8113: return(gfx->SCRP2PLT[2]); break;

      case 0x8115: return(gfx->SCRP2PLT[3]); break;
      case 0x8116: return(gfx->SCRP2PLT[4]); break;
      case 0x8117: return(gfx->SCRP2PLT[5]); break;

      case 0x8118: return(gfx->BG_COL);

      case 0x87e2: return(gfx->K2GE_MODE);
   }

   return (0);
}

uint16 NGPGfx_read16(uint32 address)
{
   uint16 ret;

   ret = NGPGfx_read8(address);
   ret |= NGPGfx_read8(address + 1) << 8;

   return(ret);
}

#undef gfx
