/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "include/boolean.h"
#include "settings.h"

uint32_t setting_ngp_language = 0;

uint64_t MDFN_GetSettingUI(const char *name)
{
   fprintf(stderr, "unhandled setting UI: %s\n", name);
   return 0;
}

int64_t MDFN_GetSettingI(const char *name)
{
   fprintf(stderr, "unhandled setting I: %s\n", name);
   return 0;
}

double MDFN_GetSettingF(const char *name)
{
   fprintf(stderr, "unhandled setting F: %s\n", name);
   return 0;
}

bool MDFN_GetSettingB(const char *name)
{
   if (!strcmp("cheats", name))
      return 0;
   /* LIBRETRO */
   if (!strcmp("libretro.cd_load_into_ram", name))
      return 0;
   if (!strcmp("ngp.language", name))
      return setting_ngp_language;
   /* FILESYS */
   if (!strcmp("filesys.untrusted_fip_check", name))
      return 0;
   if (!strcmp("filesys.disablesavegz", name))
      return 1;
   fprintf(stderr, "unhandled setting B: %s\n", name);
   return 0;
}

extern char retro_base_directory[1024];

const char *MDFN_GetSettingS(const char *name)
{
#if defined(WANT_GBA_EMU)
   if (!strcmp("gba.bios", name))
      return setting_gba_hle ? "" : "gba_bios.bin";
#elif defined(WANT_PCFX_EMU)
   if (!strcmp("pcfx.bios", name))
      return "pcfx.bios";
   if (!strcmp("pcfx.fxscsi", name))
      return "pcfx.fxscsi";
#elif defined(WANT_WSWAN_EMU)
   if (!strcmp("wswan.name", name))
      return "Mednafen";
#endif
   /* FILESYS */
   if (!strcmp("filesys.path_firmware", name))
      return retro_base_directory;
   if (!strcmp("filesys.path_palette", name))
      return retro_base_directory;
   if (!strcmp("filesys.path_sav", name))
      return retro_base_directory;
   if (!strcmp("filesys.path_state", name))
      return retro_base_directory;
   if (!strcmp("filesys.path_cheat", name))
      return retro_base_directory;
   fprintf(stderr, "unhandled setting S: %s\n", name);
   return 0;
}

bool MDFNI_SetSetting(const char *name, const char *value, bool NetplayOverride)
{
   return false;
}

bool MDFNI_SetSettingB(const char *name, bool value)
{
   return false;
}

bool MDFNI_SetSettingUI(const char *name, uint64_t value)
{
   return false;
}
