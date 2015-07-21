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
#include "../masmem.h"

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef min
#define min(a,b) ((a)<(b)?(a):(b))
#endif

static void MonoPlot(ngpgfx_t *gfx, uint16_t *cfb_scanline, uint8_t *zbuffer,
      uint8 x, uint8* palette_ptr, uint16 pal_hi, uint8 index, uint8 depth)
{
	uint8_t data8, *zbuf;
   uint16_t *scan, data16;

	/* Clip */
	if (index == 0 || x < gfx->winx || x >= (gfx->winw + gfx->winx) || x >= SCREEN_WIDTH)
		return;

   zbuf = &zbuffer[x];

	/*Depth check, <= to stop later sprites overwriting pixels! */
	if (depth <= *zbuf)
      return;
	*zbuf = depth;

	/*Get the colour of the pixel */
   data8 = palette_ptr[(pal_hi ? 3 : 0) + index - 1];

	uint16 r = (data8 & 7) << 1;
	uint16 g = (data8 & 7) << 5;
	uint16 b = (data8 & 7) << 9;

   scan = &cfb_scanline[x];
   data16 = ~(r | g | b);
	if (gfx->negative)
      data16 = ~(data16);

   *scan++ = data16;
}

static void drawMonoPattern(ngpgfx_t *gfx, 
      uint16_t *cfb_scanline, uint8_t *zbuffer, uint8 screenx, uint16 tile, uint8 tiley, uint16 mirror, 
      uint8* palette_ptr, uint16 pal, uint8 depth)
{
	/* Get the data for th e "tiley'th" line of "tile". */
	uint16_t data = LoadU16_LE((uint16*)(gfx->CharacterRAM + (tile * 16) + (tiley * 2)));

	if (mirror)
	{
      /* Horizontal Flip */
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 7, palette_ptr, pal, (data & 0xC000) >> 0xE, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 6, palette_ptr, pal, (data & 0x3000) >> 0xC, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 5, palette_ptr, pal, (data & 0x0C00) >> 0xA, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 4, palette_ptr, pal, (data & 0x0300) >> 0x8, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 3, palette_ptr, pal, (data & 0x00C0) >> 0x6, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 2, palette_ptr, pal, (data & 0x0030) >> 0x4, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 1, palette_ptr, pal, (data & 0x000C) >> 0x2, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 0, palette_ptr, pal, (data & 0x0003) >> 0x0, depth);
	}
	else
	{
      /*Normal */
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 0, palette_ptr, pal, (data & 0xC000) >> 0xE, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 1, palette_ptr, pal, (data & 0x3000) >> 0xC, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 2, palette_ptr, pal, (data & 0x0C00) >> 0xA, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 3, palette_ptr, pal, (data & 0x0300) >> 0x8, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 4, palette_ptr, pal, (data & 0x00C0) >> 0x6, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 5, palette_ptr, pal, (data & 0x0030) >> 0x4, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 6, palette_ptr, pal, (data & 0x000C) >> 0x2, depth);
		MonoPlot(gfx, cfb_scanline, zbuffer, screenx + 7, palette_ptr, pal, (data & 0x0003) >> 0x0, depth);
	}
}

static void draw_mono_scroll1(ngpgfx_t *gfx, uint16_t *cfb_scanline,
      uint8_t *zbuffer, uint8 depth, int ngpc_scanline)
{
	unsigned i;
	uint8_t line = ngpc_scanline + gfx->scroll1y;
	uint8_t row = line & 7;	/* Which row? */

	/* Draw Foreground scroll plane (Scroll 1) */
	for (i = 0; i < 32; i++)
	{
		uint16_t data16 = LoadU16_LE((uint16*)(gfx->ScrollVRAM + ((i + ((line >> 3) << 5)) << 1)));
		
		//Draw the line of the tile
		drawMonoPattern(gfx, cfb_scanline, zbuffer, (i << 3) - gfx->scroll1x, data16 & 0x01FF, 
			(data16 & 0x4000) ? 7 - row : row, data16 & 0x8000, gfx->SCRP1PLT,
			data16 & 0x2000, depth);
	}
}

static void draw_mono_scroll2(ngpgfx_t *gfx, uint16_t *cfb_scanline, uint8_t *zbuffer, uint8 depth, int ngpc_scanline)
{
   unsigned i;
	uint8_t line = ngpc_scanline + gfx->scroll2y;
	uint8_t row = line & 7;	//Which row?

	/* Draw Background scroll plane (Scroll 2) */
	for (i = 0; i < 32; i++)
	{
		uint16_t data16 = LoadU16_LE((uint16*)(gfx->ScrollVRAM + 0x0800 + ((i + ((line >> 3) << 5)) << 1)));
		
		/* Draw the line of the tile */
		drawMonoPattern(gfx, cfb_scanline, zbuffer, (i << 3) - gfx->scroll2x, data16 & 0x01FF, 
			(data16 & 0x4000) ? 7 - row : row, data16 & 0x8000, gfx->SCRP2PLT,
			data16 & 0x2000, depth);
	}
}

void draw_scanline_mono(ngpgfx_t *gfx,
      uint16_t *cfb_scanline, int layer_enable, int ngpc_scanline)
{
   int x;
   uint16_t *scan;
	int16_t lastSpriteX;
	int16_t lastSpriteY;
	int spr;
   uint8_t zbuffer[256] = {0};

	/* Window colour */
	uint16_t r = (uint16)gfx->oowc << 1;
	uint16_t g = (uint16)gfx->oowc << 5;
	uint16_t b = (uint16)gfx->oowc << 9;
   uint16_t data16 = ~(r | g | b);

	if (gfx->negative)
		data16 = ~data16;

   x = 0;
   scan = &cfb_scanline[x];

   /* Middle */
   if (!(ngpc_scanline < gfx->winy) && ngpc_scanline < gfx->winy + gfx->winh)
   {
      for (; x < min(gfx->winx, SCREEN_WIDTH); x++)
         *scan++ = data16;

      x = min(gfx->winx + gfx->winw, SCREEN_WIDTH);
   }

   /* Bottom and Top */
   for (; x < SCREEN_WIDTH; x++)
      *scan++ = data16;

	/* Ignore above and below the window's top and bottom */
	if (ngpc_scanline >= gfx->winy && ngpc_scanline < gfx->winy + gfx->winh)
	{
      int x;
      uint16_t *scan;

      data16 = 0x0FFF;

		/* Background colour Enabled? */
		if ((gfx->bgc & 0xC0) == 0x80)
		{
			r = (uint16)(gfx->bgc & 7) << 1;
			g = (uint16)(gfx->bgc & 7) << 5;
			b = (uint16)(gfx->bgc & 7) << 9;
			data16 = ~(r | g | b);
		}

		if (gfx->negative)
         data16 = ~data16;

      x    = gfx->winx;
      scan = &cfb_scanline[x];

		/* Draw background */
		for (; x < min(gfx->winx + gfx->winw, SCREEN_WIDTH); x++)	
			*scan++ = data16;

		/* Swap Front/Back scroll planes? */
		if (gfx->planeSwap)
		{
			if(layer_enable & 1)
			 draw_mono_scroll1(gfx, cfb_scanline, zbuffer, ZDEPTH_BACKGROUND_SCROLL, ngpc_scanline);		//Swap
			if(layer_enable & 2)
			 draw_mono_scroll2(gfx, cfb_scanline, zbuffer, ZDEPTH_FOREGROUND_SCROLL, ngpc_scanline);
		}
		else
		{
			if(layer_enable & 1)
			 draw_mono_scroll2(gfx, cfb_scanline, zbuffer, ZDEPTH_BACKGROUND_SCROLL, ngpc_scanline);		//Normal
			if(layer_enable & 2)
			 draw_mono_scroll1(gfx, cfb_scanline, zbuffer, ZDEPTH_FOREGROUND_SCROLL, ngpc_scanline);
		}

		/* Draw Sprites */
		/* Last sprite position, (defaults to top-left, sure?) */
		lastSpriteX = 0;
		lastSpriteY = 0;
		if(layer_enable & 4)
		for (spr = 0; spr < 64; spr++)
		{
			uint8 priority, row;
			uint8 sx = gfx->SpriteVRAM[(spr * 4) + 2];	//X position
			uint8 sy = gfx->SpriteVRAM[(spr * 4) + 3];	//Y position
			int16 x = sx;
			int16 y = sy;
			
			data16 = LoadU16_LE((uint16*)(gfx->SpriteVRAM + (spr * 4)));
			priority = (data16 & 0x1800) >> 11;

			if (data16 & 0x0400) x = lastSpriteX + sx;	//Horizontal chain?
			if (data16 & 0x0200) y = lastSpriteY + sy;	//Vertical chain?

			//Store the position for chaining
			lastSpriteX = x;
			lastSpriteY = y;
			
			//Visible?
			if (priority == 0)
            continue;

			//Scroll the sprite
			x += gfx->scrollsprx;
			y += gfx->scrollspry;

			//Off-screen?
			if (x > 248 && x < 256)	x = x - 256; else x &= 0xFF;
			if (y > 248 && y < 256)	y = y - 256; else y &= 0xFF;

			//In range?
			if (ngpc_scanline >= y && ngpc_scanline <= y + 7)
			{
				row = (ngpc_scanline - y) & 7;	//Which row?
				drawMonoPattern(gfx, cfb_scanline, zbuffer, (uint8)x, data16 & 0x01FF, 
					(data16 & 0x4000) ? 7 - row : row, data16 & 0x8000,
					gfx->SPPLT, data16 & 0x2000, priority << 1); 
			}
		}

	}
}
