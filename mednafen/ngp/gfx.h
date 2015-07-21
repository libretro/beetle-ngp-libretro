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

#ifndef __GFX__
#define __GFX__
//=============================================================================

#include "../mednafen.h"

#define ZDEPTH_BACK_SPRITE			2
#define ZDEPTH_BACKGROUND_SCROLL	3
#define ZDEPTH_MIDDLE_SPRITE		4
#define ZDEPTH_FOREGROUND_SCROLL	5
#define ZDEPTH_FRONT_SPRITE			6

extern uint16_t cfb_scanline[256];	// __attribute__ ((aligned (8)));

typedef struct ngpgfx
{
   uint8 winx, winw;
   uint8 winy, winh;
   uint8 scroll1x, scroll1y;
   uint8 scroll2x, scroll2y;
   uint8 scrollsprx, scrollspry;
   uint8 planeSwap;
   uint8 bgc, oowc, negative;

   uint8 ScrollVRAM[4096];  // 9000-9fff
   uint8 CharacterRAM[8192]; // a000-bfff
   uint8 SpriteVRAM[256]; // 8800-88ff
   uint8 SpriteVRAMColor[0x40]; // 8C00-8C3F
   uint8 ColorPaletteRAM[0x200]; // 8200-83ff

   uint8 SPPLT[6];
   uint8 SCRP1PLT[6];
   uint8 SCRP2PLT[6];

   uint8 raster_line;
   uint8 S1SO_H, S1SO_V, S2SO_H, S2SO_V;
   uint8 WBA_H, WBA_V, WSI_H, WSI_V;
   bool C_OVR, BLNK;
   uint8 PO_H, PO_V;
   uint8 P_F;
   uint8 BG_COL;
   uint8 CONTROL_2D;
   uint8 CONTROL_INT;
   uint8 SCREEN_PERIOD;
   uint8 K2GE_MODE;

   uint16 ColorMap[4096];

   int layer_enable;
} ngpgfx_t;

void ngpgfx_set_pixel_format(ngpgfx_t *fx);

void ngpgfx_SetLayerEnableMask(ngpgfx_t *gfx, uint64_t mask);

int ngpgfx_StateAction(ngpgfx_t *gfx, void *data, int load, int data_only);

void draw_scanline_colour(ngpgfx_t *gfx, uint16_t *cfb_scanline, int layer_enable, int ngpc_scanline);

void draw_scanline_mono(ngpgfx_t *gfx,
      uint16_t *cfb_scanline, int layer_enable, int ngpc_scanline);

void ngpgfx_power(ngpgfx_t *gfx);

bool ngpgfx_hint(ngpgfx_t *gfx);

bool ngpgfx_draw(ngpgfx_t *gfx, MDFN_Surface *surface, bool skip);

uint8_t ngpgfx_read8(ngpgfx_t *gfx, uint32_t address);

uint16_t ngpgfx_read16(ngpgfx_t *gfx, uint32_t address);

void ngpgfx_write16(ngpgfx_t *gfx, uint32 address, uint16 data);

void ngpgfx_write8(ngpgfx_t *gfx, uint32 address, uint8 data);

extern ngpgfx_t *NGPGfx;

#endif

