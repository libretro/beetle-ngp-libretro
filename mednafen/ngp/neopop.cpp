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
#include "mednafen/general.h"
#include <streams/file_stream.h>

#include "Z80_interface.h"
#include "interrupt.h"
#include "mem.h"
#include "gfx.h"
#include "sound.h"
#include "dma.h"
#include "bios.h"
#include "flash.h"

extern uint8 CPUExRAM[16384];

NGPGfx_t *NGPGfx = NULL;

COLOURMODE system_colour = COLOURMODE_AUTO;

uint8 NGPJoyLatch;

bool system_comms_read(uint8* buffer)
{
   return (0);
}

bool system_comms_poll(uint8* buffer)
{
   return (0);
}

void system_comms_write(uint8 data)
{
   return;
}

static uint8 *chee;

bool NGPFrameSkip;
int32 ngpc_soundTS = 0;
//static int32 main_timeaccum;
static int32 z80_runtime;

void Emulate(EmulateSpecStruct *espec)
{
   bool MeowMeow        = 0;

   espec->DisplayRect.x = 0;
   espec->DisplayRect.y = 0;
   espec->DisplayRect.w = 160;
   espec->DisplayRect.h = 152;

   if(espec->VideoFormatChanged)
      NGPGfx_set_pixel_format();

   if(espec->SoundFormatChanged)
      MDFNNGPC_SetSoundRate(espec->SoundRate);


   NGPJoyLatch = *chee;
   storeB(0x6F82, *chee);

   MDFNMP_ApplyPeriodicCheats();

   ngpc_soundTS         = 0;
   NGPFrameSkip         = espec->skip;

   do
   {
      int32 timetime = (uint8)TLCS900h_interpret();	// This is sooo not right, but it's replicating the old behavior(which is necessary
                                                    // now since I've fixed the TLCS900h core and other places not to truncate cycle counts
                                                    // internally to 8-bits).  Switch to the #if 0'd block of code once we fix cycle counts in the
                                                    // TLCS900h core(they're all sorts of messed up), and investigate if certain long
                                                    // instructions are interruptable(by interrupts) and later resumable, RE Rockman Battle
                                                    // & Fighters voice sample playback.

      //if(timetime > 255)
      // printf("%d\n", timetime);

	  // Note: Don't call updateTimers with a time/tick/cycle/whatever count greater than 255.
      MeowMeow |= updateTimers(espec->surface, timetime);

      z80_runtime += timetime;

      while(z80_runtime > 0)
      {
         int z80rantime = Z80_RunOP();

         if (z80rantime < 0) // Z80 inactive, so take up all run time!
         {
            z80_runtime = 0;
            break;
         }

         z80_runtime -= z80rantime << 1;

      }
   }while(!MeowMeow);


   espec->MasterCycles = ngpc_soundTS;
   espec->SoundBufSize = MDFNNGPCSOUND_Flush(espec->SoundBuf, espec->SoundBufMaxSize);
}

static bool TestMagic(MDFNFILE* gf)
{
   if(gf->ext != "ngp" && gf->ext != "ngpc" && gf->ext != "ngc" && gf->ext != "npc")
      return false;

   return true;
}

static MDFN_COLD void Cleanup(void)
{
   rom_unload();

   if (NGPGfx != NULL)
   {
      free(NGPGfx);
      NGPGfx = NULL;
   }
}

int Load(const char *name, MDFNFILE *fp, const uint8_t *data, size_t size)
{
   if ((data != NULL) && (size != 0)) {
      if (!(ngpc_rom.data = (uint8 *)malloc(size)))
         return(0);
      ngpc_rom.length = size;
      memcpy(ngpc_rom.data, data, size);
   }
   else
   {
      if(!(ngpc_rom.data = (uint8 *)malloc(fp->size)))
         return(0);

      ngpc_rom.length = fp->size;
      memcpy(ngpc_rom.data, fp->data, fp->size);
   }

   rom_loaded();
   MDFN_printf("ROM:       %uKiB\n", (ngpc_rom.length + 1023) / 1024);
   //MDFN_printf(_("ROM MD5:   0x%s\n"), md5_context::asciistr(MDFNGameInfo->MD5, 0).c_str());
   FLASH_LoadNV();

   MDFNMP_Init(1024, 1024 * 1024 * 16 / 1024);

   NGPGfx = (NGPGfx_t*)calloc(1, sizeof(*NGPGfx));
   NGPGfx->layer_enable_setting = 1 | 2 | 4;

   MDFNGameInfo->fps = (uint32)((uint64)6144000 * 65536 * 256 / 515 / 198); // 3072000 * 2 * 10000 / 515 / 198

   MDFNNGPCSOUND_Init();

   MDFNMP_AddRAM(16384, 0x4000, CPUExRAM);

   SetFRM(); // Set up fast read memory mapping

   bios_install();

   //main_timeaccum = 0;
   z80_runtime = 0;

   reset();

   return(1);
}

MDFN_COLD void CloseGame(void)
{
   FLASH_SaveNV();
   Cleanup();
}

void SetInput(unsigned port, const char *type, uint8 *ptr)
{
   if(!port)
      chee = ptr;
}

int StateAction(StateMem *sm, const unsigned load, const bool data_only)
{
   SFORMAT StateRegs[] =
   {
      SFVAR(z80_runtime),
      SFVAR(CPUExRAM),
      SFVAR(FlashStatusEnable),
      SFEND
   };

   SFORMAT TLCS_StateRegs[] =
   {
      SFVARN(pc, "PC"),
      SFVARN(sr, "SR"),
      SFVARN(f_dash, "F_DASH"),
      SFVARN(gpr, "GPR"),
      SFVARN(gprBank[0], "GPRB0"),
      SFVARN(gprBank[1], "GPRB1"),
      SFVARN(gprBank[2], "GPRB2"),
      SFVARN(gprBank[3], "GPRB3"),
      SFEND
   };

   if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "MAIN", false)) return(0);
   if(!MDFNSS_StateAction(sm, load, data_only, TLCS_StateRegs, "TLCS", false)) return(0);
   if(!MDFNNGPCDMA_StateAction(sm, load, data_only)) return(0);
   if(!MDFNNGPCSOUND_StateAction(sm, load, data_only)) return(0);
   if(!NGPGfx_StateAction(sm, load, data_only)) return(0);
   if(!MDFNNGPCZ80_StateAction(sm, load, data_only)) return(0);
   if(!int_timer_StateAction(sm, load, data_only)) return(0);
   if(!BIOSHLE_StateAction(sm, load, data_only)) return(0);
   if(!FLASH_StateAction(sm, load, data_only)) return(0);

   if(load)
   {
      RecacheFRM();
      changedSP();
   }
   return(1);
}

void DoSimpleCommand(int cmd)
{
   switch(cmd)
   {
      case MDFN_MSC_POWER:
      case MDFN_MSC_RESET: reset();
                           break;
   }
}

static const MDFNSetting_EnumList LanguageList[] =
{
 { "japanese", 0, "Japanese" },
 { "0", 0 },

 { "english", 1, "English" },
 { "1", 1 },

 { NULL, 0 },
};

const MDFNSetting NGPSettings[] =
{
 { "ngp.language", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, "Language games should display text in.", NULL, MDFNST_ENUM, "english", NULL, NULL, NULL, NULL, LanguageList },
 { NULL }
};


bool system_io_flash_read(uint8* buffer, uint32 bufferLength)
{
   std::string pathStr = MDFN_MakeFName(MDFNMKF_SAV, 0, "flash");
   RFILE *flash_fp = filestream_open(pathStr.c_str(),
                                     RETRO_VFS_FILE_ACCESS_READ,
                                     RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!flash_fp)
      return 0;

   filestream_read(flash_fp, buffer, bufferLength);
   filestream_close(flash_fp);

   return 1;
}

bool system_io_flash_write(uint8 *buffer, uint32 bufferLength)
{
   std::string pathStr = MDFN_MakeFName(MDFNMKF_SAV, 0, "flash");
   RFILE *flash_fp = filestream_open(pathStr.c_str(),
                                     RETRO_VFS_FILE_ACCESS_WRITE,
                                     RETRO_VFS_FILE_ACCESS_HINT_NONE);

   if (!flash_fp)
      return 0;

   filestream_write(flash_fp, buffer, bufferLength);
   filestream_close(flash_fp);

   return 1;
}

static void SetLayerEnableMask(uint64 mask)
{
   NGPGfx_SetLayerEnableMask(mask);
}

static InputDeviceInputInfoStruct IDII[] =
{
 { "up", "UP ↑", 0, IDIT_BUTTON, "down" },
 { "down", "DOWN ↓", 1, IDIT_BUTTON, "up" },
 { "left", "LEFT ←", 2, IDIT_BUTTON, "right" },
 { "right", "RIGHT →", 3, IDIT_BUTTON, "left" },
 { "a", "A", 5, IDIT_BUTTON_CAN_RAPID,  NULL },
 { "b", "B", 6, IDIT_BUTTON_CAN_RAPID, NULL },
 { "option", "OPTION", 4, IDIT_BUTTON, NULL },
};

static InputDeviceInfoStruct InputDeviceInfo[] =
{
 {
  "gamepad",
  "Gamepad",
  NULL,
  NULL,
  sizeof(IDII) / sizeof(InputDeviceInputInfoStruct),
  IDII,
 }
};

static const InputPortInfoStruct PortInfo[] =
{
 { "builtin", "Built-In", sizeof(InputDeviceInfo) / sizeof(InputDeviceInfoStruct), InputDeviceInfo, "gamepad" }
};

static InputInfoStruct InputInfo =
{
 sizeof(PortInfo) / sizeof(InputPortInfoStruct),
 PortInfo
};

static const FileExtensionSpecStruct KnownExtensions[] =
{
 { ".ngp", "Neo Geo Pocket ROM Image" },
 { ".ngc", "Neo Geo Pocket Color ROM Image" },
 { NULL, NULL }
};

MDFNGI EmulatedNGP = {};

// ====================================================

MDFNGI *MDFNGameInfo = &EmulatedNGP;

void MDFNGI_reset(MDFNGI *gameinfo)
{
   gameinfo->Settings = NGPSettings;
   gameinfo->MasterClock = MDFN_MASTERCLOCK_FIXED(6144000);
   gameinfo->fps = 0;
   gameinfo->multires = false; // Multires possible?

   gameinfo->lcm_width = 160;
   gameinfo->lcm_height = 152;
   gameinfo->dummy_separator = NULL;

   gameinfo->nominal_width = 160;
   gameinfo->nominal_height = 152;

   gameinfo->fb_width = 160;
   gameinfo->fb_height = 152;

   gameinfo->soundchan = 2;
}

MDFNGI *MDFNI_LoadGame(const char *name)
{
   MDFNFILE *GameFile = file_open(name);

   if(!GameFile)
      goto error;

   if(Load(name, GameFile, NULL, 0) <= 0)
      goto error;

   file_close(GameFile);
   GameFile     = NULL;

   return MDFNGameInfo;

error:
   if (GameFile)
      file_close(GameFile);
   GameFile     = NULL;
   MDFNGI_reset(MDFNGameInfo);
   return(0);
}

MDFNGI *MDFNI_LoadGameData(const uint8_t *data, size_t size)
{
   if(Load("", NULL, data, size) <= 0)
      goto error;
   return MDFNGameInfo;

error:
   MDFNGI_reset(MDFNGameInfo);
   return(0);
}

void MDFNI_CloseGame(void)
{
   if(!MDFNGameInfo)
      return;

   CloseGame();
   MDFNGI_reset(MDFNGameInfo);
}
