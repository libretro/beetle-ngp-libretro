/*
 FIXME:
  File format is not endian-safe.

  Still possible for corrupt/malicious save game data to cause a crash, from blindly reading past the end of the buffer.
*/

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
#include "mem.h"

//-----------------------------------------------------------------------------
// Local Definitions
//-----------------------------------------------------------------------------
//This value is used to verify flash data - it is set to the
//version number that the flash description was modified for.

#define FLASH_VALID_ID		0x0053

//Number of different flash blocks, this should be enough.

#define FLASH_MAX_BLOCKS	256

typedef struct
{
   //Flash Id
   uint16 valid_flash_id;		// = FLASH_VALID_ID

   uint16 block_count;			//Number of flash data blocks

   uint32 total_file_length;		// header + block[0 - block_count]

} FlashFileHeader;

typedef struct
{
   uint32 start_address;		// 24 bit address
   uint16 data_length;		// length of following data

   //Followed by data_length bytes of the actual data.

} FlashFileBlockHeader;

//-----------------------------------------------------------------------------
// Local Data
//-----------------------------------------------------------------------------
static FlashFileBlockHeader	blocks[FLASH_MAX_BLOCKS];
static uint16 block_count;

//=============================================================================

//-----------------------------------------------------------------------------
// optimise_blocks()
//-----------------------------------------------------------------------------
void flash_optimise_blocks(void)
{
   int i, j;

   // Bubble Sort by address
   for (i = 0; i < block_count - 1; i++)
   {
      for (j = i+1; j < block_count; j++)
      {
         //Swap?
         if (blocks[i].start_address > blocks[j].start_address)
         {
            uint32 temp32;
            uint16 temp16;

            temp32 = blocks[i].start_address;
            blocks[i].start_address = blocks[j].start_address;
            blocks[j].start_address = temp32;

            temp16 = blocks[i].data_length;
            blocks[i].data_length = blocks[j].data_length;
            blocks[j].data_length = temp16;
         }
      }
   }

   //Join contiguous blocks
   //Only advance 'i' if required, this will allow subsequent
   //blocks to be compared to the newly expanded block.
   for (i = 0; i < block_count - 1; /**/)
   {
      //Next block lies within (or borders) this one?
      if (blocks[i+1].start_address <=
            (blocks[i].start_address + blocks[i].data_length))
      {
         //Extend the first block
         blocks[i].data_length = (uint16)(MAX(blocks[i + 0].start_address + blocks[i + 0].data_length,
									          blocks[i + 1].start_address + blocks[i + 1].data_length) - blocks[i].start_address);

         //Remove the next one.
         for (j = i+2; j < block_count; j++)
         {
            blocks[j-1].start_address = blocks[j].start_address;
            blocks[j-1].data_length = blocks[j].data_length;
         }
         block_count --;
      }
      else
      {
         i++;	// Try the next block
      }
   }

   //for(i = 0; i < block_count; i++)
   // printf("block: 0x%08x 0x%04x\n", blocks[i].start_address, blocks[i].data_length);
}

static void do_flash_read(const uint8 *flashdata)
{
   FlashFileHeader header;
   const uint8 *fileptr;
   uint16 i;
   uint32 j;
   bool PREV_memory_unlock_flash_write = memory_unlock_flash_write; // kludge, hack, FIXME

   memcpy(&header, flashdata, sizeof(header));

   if(header.block_count > FLASH_MAX_BLOCKS)
   {
      MDFN_PrintError("FLASH header block_count(%u) > FLASH_MAX_BLOCKS!", header.block_count);
   }

   //Read header
   block_count = header.block_count;
   fileptr = flashdata + sizeof(FlashFileHeader);

   //Copy blocks
   memory_unlock_flash_write = true;
   for (i = 0; i < block_count; i++)
   {
      FlashFileBlockHeader* current = (FlashFileBlockHeader*)fileptr;
      fileptr += sizeof(FlashFileBlockHeader);

      blocks[i].start_address = current->start_address;
      blocks[i].data_length = current->data_length;

      //Copy data
      for (j = 0; j < blocks[i].data_length; j++)
      {
         storeB(blocks[i].start_address + j, *fileptr);
         fileptr++;
      }
   }
   memory_unlock_flash_write = PREV_memory_unlock_flash_write;

   flash_optimise_blocks();		//Optimise

#if 0
	//Output block list...
	for (i = 0; i < block_count; i++)
		printf("flash block: %06X, %d bytes\n", 
			blocks[i].start_address, blocks[i].data_length);
#endif
}


void FLASH_LoadNV(void)
{
   FlashFileHeader header;
   uint8* flashdata;

   //Initialise the internal flash configuration
   block_count = 0;

   //Read flash buffer header
   if (system_io_flash_read((uint8*)&header, sizeof(FlashFileHeader)) == false)
      return; //Silent failure - no flash data yet.

   //Verify correct flash id
   if (header.valid_flash_id != FLASH_VALID_ID)
   {
      MDFN_PrintError("IDS_BADFLASH");
   }

   if(header.total_file_length < sizeof(FlashFileHeader) || header.total_file_length > 16384 * 1024)
   {
      MDFN_PrintError("FLASH header total_file_length is bad!");
   }

   //Read the flash data
   flashdata = (uint8*)malloc(header.total_file_length * sizeof(uint8));
   system_io_flash_read(flashdata, header.total_file_length);

   do_flash_read(flashdata);

   free(flashdata);
}

//-----------------------------------------------------------------------------
// flash_write()
//-----------------------------------------------------------------------------
void flash_write(uint32 start_address, uint16 length)
{
   uint16 i;

   //printf("flash_write 0x%08x 0x%04x\n", start_address, length);

   //Now we need a new flash command before the next flash write will work!
   memory_flash_command = false;

   //	system_debug_message("flash write: %06X, %d bytes", start_address, length);

   for (i = 0; i < block_count; i++)
   {
      //Got this block with enough bytes to cover it
      if (blocks[i].start_address == start_address &&
            blocks[i].data_length >= length)
      {
         return; //Nothing to do, block already registered.
	  }

      //Got this block with but it's length is too short
      if (blocks[i].start_address == start_address &&
            blocks[i].data_length < length)
      {
         blocks[i].data_length = length;	//Enlarge block updating.
         return;
      }
   }

   if(block_count >= FLASH_MAX_BLOCKS)
   {
      MDFN_Notify(MDFN_NOTICE_ERROR, "[FLASH] Block list overflow!");
      return;
   }
   else
   {
      // New block needs to be added
      blocks[block_count].start_address = start_address;
      blocks[block_count].data_length = length;
      block_count++;
   }
}

static void make_flash_commit(uint8 **flashdata, uint32 *length)
{
   FlashFileHeader header;
   uint8 *fileptr;

   *flashdata = NULL;

   // No flash data?
   if (block_count == 0)
      return;

   //Optimise before writing
   flash_optimise_blocks();

   // Build a header;
   header.valid_flash_id    = FLASH_VALID_ID;
   header.block_count       = block_count;
   header.total_file_length = sizeof(FlashFileHeader);
   for (int i = 0; i < block_count; i++)
   {
      header.total_file_length += sizeof(FlashFileBlockHeader);
      header.total_file_length += blocks[i].data_length;
   }

   // Write the flash data
   *flashdata = (uint8*)malloc(header.total_file_length * sizeof(uint8_t));
   *length = header.total_file_length;

   // Copy header
   memcpy(*flashdata, &header, sizeof(FlashFileHeader));
   fileptr = *flashdata + sizeof(FlashFileHeader);

   // Copy blocks
   for (int i = 0; i < block_count; i++)
   {
      memcpy(fileptr, &blocks[i], sizeof(FlashFileBlockHeader));
      fileptr += sizeof(FlashFileBlockHeader);

      // Copy data
      for (uint32 j = 0; j < blocks[i].data_length; j++)
      {
         *fileptr = loadB(blocks[i].start_address + j);
         fileptr++;
      }
   }
}

void FLASH_SaveNV(void)
{
   uint32 FlashLength = 0;
   uint8 *flashdata = NULL;

   make_flash_commit(&flashdata, &FlashLength);

   if(FlashLength > 0)
   {
      system_io_flash_write(flashdata, FlashLength);
      free(flashdata);
   }
}

int FLASH_StateAction(StateMem *sm, const unsigned load, const bool data_only)
{
   uint32 FlashLength = 0;
   uint8 *flashdata = NULL;

   if(!load)
   {
      make_flash_commit(&flashdata, &FlashLength);
   }

   SFORMAT FINF_StateRegs[] =
   {
      SFVAR(FlashLength),
      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, FINF_StateRegs, "FINF", false))
      return 0;

   if(!FlashLength) // No flash data to save, OR no flash data to load.
   {
      if(flashdata) free(flashdata);
      return 1;
   }

   if(load)
   {
      if(FlashLength > 16384 * 1024)
         FlashLength = 16384 * 1024;

      flashdata = (uint8*)malloc(FlashLength);
      memset(flashdata, 0, FlashLength);
   }

   SFORMAT FLSH_StateRegs[] =
   {
      SFARRAYN(flashdata, FlashLength, "flashdata"),
      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, FLSH_StateRegs, "FLSH", false))
   {
      free(flashdata);
      return 0;
   }

   if(load)
   {
      memcpy(ngpc_rom.data, ngpc_rom.orig_data, ngpc_rom.length);	// Restore FLASH/ROM data to its state before any writes to FLASH the game made(or were loaded from file).
      do_flash_read(flashdata);
   }

   free(flashdata);
   return 1;
}
