#include <stdio.h>
#include <stdint.h>
#include "system.h"

#include "../general.h"

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
