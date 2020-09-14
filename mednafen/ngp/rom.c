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
#include "flash.h"
#include "interrupt.h"
#include "TLCS-900h/TLCS900h_disassemble.h"
#include "../mednafen.h"

#ifdef MSB_FIRST
#define HTOLE16(l)      ((((l)>>8) & 0xff) | (((l)<<8) & 0xff00))
#else
#define HTOLE16(l) (l)
#endif

#define MATCH_CATALOG(c, s)	(rom_header->catalog == HTOLE16(c) \
				 && rom_header->subCatalog == (s))

RomInfo ngpc_rom;
RomHeader* rom_header = NULL;

static void rom_hack(void)
{
   //=============================
   // SPECIFIC ROM HACKS !
   //=============================

   if (MATCH_CATALOG(0, 16))
   {
      /* "Neo-Neo! V1.0 (PD)" */
      ngpc_rom.data[0x23] = 0x10;	// Fix rom header
   }

   if (MATCH_CATALOG(4660, 161))
   {
      /* "Cool Cool Jam SAMPLE (U)" */
      ngpc_rom.data[0x23] = 0x10;	// Fix rom header
   }

   if (MATCH_CATALOG(51, 33))
   {
      /* "Dokodemo Mahjong (J)" */
      ngpc_rom.data[0x23] = 0x00;	// Fix rom header
   }
}

static void rom_display_header(void)
{
#if 0
   printf("Name:    %s\n", ngpc_rom.name);
   printf("System:  ");

   if(rom_header->mode & 0x10)
      printf("Color");
   else
      printf("Greyscale");

   printf("\n");

   printf("Catalog:  %d (sub %d)\n",
         le16toh(rom_header->catalog),
         rom_header->subCatalog);

   printf("Starting PC:  0x%06X\n", le32toh(rom_header->startPC) & 0xFFFFFF);
#endif
}

void rom_loaded(void)
{
   int i;

   ngpc_rom.orig_data = (uint8 *)malloc(ngpc_rom.length);
   memcpy(ngpc_rom.orig_data, ngpc_rom.data, ngpc_rom.length);

   /* Extract the header */
   rom_header = (RomHeader*)(ngpc_rom.data);

   /* ROM Name */
   for(i = 0; i < 12; i++)
   {
      ngpc_rom.name[i] = ' ';
      if (rom_header->name[i] >= 32 && rom_header->name[i] < 128)
         ngpc_rom.name[i] = rom_header->name[i];
   }
   ngpc_rom.name[i] = 0;

   /* Apply a hack if required! */
   rom_hack();	

   rom_display_header();

   flash_read();
}

void rom_unload(void)
{
   if (ngpc_rom.data)
   {
      int i;

      flash_commit();

      free(ngpc_rom.data);
      ngpc_rom.data = NULL;
      ngpc_rom.length = 0;
      rom_header = 0;

      for (i = 0; i < 16; i++)
         ngpc_rom.name[i] = 0;
   }		

   if(ngpc_rom.orig_data)
   {
      free(ngpc_rom.orig_data);
      ngpc_rom.orig_data = NULL;
   }
}
