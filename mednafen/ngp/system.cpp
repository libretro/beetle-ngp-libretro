#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "flash.h"
#include "system.h"
#include "rom.h"

#include "../general.h"
#include "../state.h"

bool system_comms_read(uint8_t* buffer)
{
   return 0;
}

bool system_comms_poll(uint8_t* buffer)
{
   return 0;
}

void system_comms_write(uint8_t data)
{
}

bool system_io_flash_read(uint8_t* buffer, uint32_t bufferLength)
{
   const char *path = MDFN_MakeFName(MDFNMKF_SAV, 0, "flash").c_str();
   FILE *flash_fp = fopen(path, "rb");

   if (!flash_fp)
      return 0;

   fread(buffer, 1, bufferLength, flash_fp);
   fclose(flash_fp);

   return 1;
}

bool system_io_flash_write(uint8_t *buffer, uint32_t bufferLength)
{
   const char *path = MDFN_MakeFName(MDFNMKF_SAV, 0, "flash").c_str();
   FILE *flash_fp = fopen(path, "wb");

   if (!flash_fp)
      return 0;

   fwrite(buffer, 1, bufferLength, flash_fp);
   fclose(flash_fp);

   return 1;
}

int FLASH_StateAction(void *data, int load, int data_only)
{
   int32_t FlashLength = 0;
   uint8_t *flashdata = NULL;

   if(!load)
      flashdata = make_flash_commit(&FlashLength);

   SFORMAT FINF_StateRegs[] =
   {
      { &FlashLength, sizeof(FlashLength), 0x80000000, "FlashLength" },
      { 0, 0, 0, 0 }
   };

   if(!MDFNSS_StateAction(data, load, data_only, FINF_StateRegs, "FINF"))
      return 0;

   if(!FlashLength) // No flash data to save, OR no flash data to load.
   {
      if(flashdata) free(flashdata);
      return 1;
   }

   if(load)
      flashdata = (uint8_t *)malloc(FlashLength);

   SFORMAT FLSH_StateRegs[] =
   {
      { flashdata, (uint32_t)FlashLength, 0, "flashdata" },
      { 0, 0, 0, 0 }
   };

   if(!MDFNSS_StateAction(data, load, data_only, FLSH_StateRegs, "FLSH"))
   {
      free(flashdata);
      return 0;
   }

   if(load)
   {
      memcpy(ngpc_rom.data, ngpc_rom.orig_data, ngpc_rom.length);
      do_flash_read(flashdata);
   }

   free(flashdata);
   return 1;
}
