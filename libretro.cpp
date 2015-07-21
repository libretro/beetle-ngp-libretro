#include "mednafen/mednafen.h"
#include "mednafen/mempatcher.h"
#include "mednafen/git.h"
#include "mednafen/general.h"
#include "mednafen/md5.h"
#include "libretro.h"

static MDFNGI *game;

struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;
retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static bool overscan;
static double last_sound_rate;
static MDFN_PixelFormat last_pixel_format;

static MDFN_Surface *surf;

static bool failed_init;

static void hookup_ports(bool force);

static bool initial_ports_hookup = false;

std::string retro_base_directory;
std::string retro_base_name;
std::string retro_save_directory;

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

#include "mednafen/ngp/neopop.h"
#include "mednafen/general.h"
#include "mednafen/md5.h"
#include "mednafen/FileStream.h"

#include "mednafen/ngp/TLCS-900h/TLCS900h_interpret.h"
#include "mednafen/ngp/TLCS-900h/TLCS900h_registers.h"
#include "mednafen/ngp/Z80_interface.h"
#include "mednafen/ngp/interrupt.h"
#include "mednafen/ngp/mem.h"
#include "mednafen/ngp/gfx.h"
#include "mednafen/ngp/sound.h"
#include "mednafen/ngp/dma.h"
#include "mednafen/ngp/bios.h"
#include "mednafen/ngp/flash.h"

extern uint8 CPUExRAM[16384];

ngpgfx_t *NGPGfx;

COLOURMODE system_colour = COLOURMODE_AUTO;

uint8 NGPJoyLatch;

bool system_comms_read(uint8* buffer)
{
 return(0);
}

bool system_comms_poll(uint8* buffer)
{
 return(0);
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

static void Emulate(EmulateSpecStruct *espec)
{
	bool MeowMeow = 0;

	espec->DisplayRect.x = 0;
	espec->DisplayRect.y = 0;
	espec->DisplayRect.w = 160;
	espec->DisplayRect.h = 152;

   if(espec->VideoFormatChanged)
      ngpgfx_set_pixel_format(NGPGfx);

	if(espec->SoundFormatChanged)
	 MDFNNGPC_SetSoundRate(espec->SoundRate);


	NGPJoyLatch = *chee;
	storeB(0x6F82, *chee);

	MDFNMP_ApplyPeriodicCheats();

	ngpc_soundTS = 0;
	NGPFrameSkip = espec->skip;

	do
	{
#if 0
         int32 timetime;

	 if(main_timeaccum == 0)
	 {
	  main_timeaccum = TLCS900h_interpret();
          if(main_timeaccum > 255)
	  {
	   main_timeaccum = 255;
           printf("%d\n", main_timeaccum);
	  }
	 }

	 timetime = std::min<int32>(main_timeaccum, 24);
	 main_timeaccum -= timetime;
#else
	 int32 timetime = (uint8)TLCS900h_interpret();	// This is sooo not right, but it's replicating the old behavior(which is necessary
							// now since I've fixed the TLCS900h core and other places not to truncate cycle counts
							// internally to 8-bits).  Switch to the #if 0'd block of code once we fix cycle counts in the
							// TLCS900h core(they're all sorts of messed up), and investigate if certain long
							// instructions are interruptable(by interrupts) and later resumable, RE Rockman Battle
							// & Fighters voice sample playback.
#endif
	 //if(timetime > 255)
	 // printf("%d\n", timetime);

	 // Note: Don't call updateTimers with a time/tick/cycle/whatever count greater than 255.
	 MeowMeow |= updateTimers(espec->surface, timetime);

	 z80_runtime += timetime;

         while(z80_runtime > 0)
	 {
	  int z80rantime = Z80_RunOP();

	  if(z80rantime < 0) // Z80 inactive, so take up all run time!
	  {
	   z80_runtime = 0;
	   break;
	  }

	  z80_runtime -= z80rantime << 1;

	 }
	} while(!MeowMeow);


	espec->MasterCycles = ngpc_soundTS;
	espec->SoundBufSize = MDFNNGPCSOUND_Flush(espec->SoundBuf, espec->SoundBufMaxSize);
}

static bool TestMagic(const char *name, MDFNFILE *fp)
{
 if(strcasecmp(GET_FEXTS_PTR(fp), "ngp") && strcasecmp(GET_FEXTS_PTR(fp), "ngpc") && strcasecmp(GET_FEXTS_PTR(fp), "ngc") && strcasecmp(GET_FEXTS_PTR(fp), "npc"))
  return(FALSE);

 return(TRUE);
}

static int Load(const char *name, MDFNFILE *fp)
{
 if(!(ngpc_rom.data = (uint8 *)MDFN_malloc(GET_FSIZE_PTR(fp), _("Cart ROM"))))
  return(0);

 ngpc_rom.length = GET_FSIZE_PTR(fp);
 memcpy(ngpc_rom.data, GET_FDATA_PTR(fp), GET_FSIZE_PTR(fp));

 md5_context md5;
 md5.starts();
 md5.update(ngpc_rom.data, ngpc_rom.length);
 md5.finish(MDFNGameInfo->MD5);

 rom_loaded();
 MDFN_printf(_("ROM:       %dKiB\n"), (ngpc_rom.length + 1023) / 1024);
 MDFN_printf(_("ROM MD5:   0x%s\n"), md5_context::asciistr(MDFNGameInfo->MD5, 0).c_str());

 MDFNMP_Init(1024, 1024 * 1024 * 16 / 1024);

 NGPGfx = (ngpgfx_t*)calloc(1, sizeof(*NGPGfx));
 NGPGfx->layer_enable = 1 | 2 | 4;

 MDFNGameInfo->fps = (uint32)((uint64)6144000 * 65536 * 256 / 515 / 198); // 3072000 * 2 * 10000 / 515 / 198
 MDFNGameInfo->GameSetMD5Valid = FALSE;

 MDFNNGPCSOUND_Init();

 MDFNMP_AddRAM(16384, 0x4000, CPUExRAM);

 SetFRM(); // Set up fast read memory mapping

 bios_install();

 //main_timeaccum = 0;
 z80_runtime = 0;

 reset();

 return(1);
}

static void CloseGame(void)
{
 rom_unload();
 if (NGPGfx)
    free(NGPGfx);
 NGPGfx = NULL;
}

static void SetInput(int port, const char *type, void *ptr)
{
 if(!port) chee = (uint8 *)ptr;
}

static int StateAction(StateMem *sm, int load, int data_only)
{
 SFORMAT StateRegs[] =
 {
  SFVAR(z80_runtime),
  SFARRAY(CPUExRAM, 16384),
  SFVAR(FlashStatusEnable),
  SFEND
 };

 SFORMAT TLCS_StateRegs[] =
 {
  SFVARN(pc, "PC"),
  SFVARN(sr, "SR"),
  SFVARN(f_dash, "F_DASH"),
  SFARRAY32N(gpr, 4, "GPR"),
  SFARRAY32N(gprBank[0], 4, "GPRB0"),
  SFARRAY32N(gprBank[1], 4, "GPRB1"),
  SFARRAY32N(gprBank[2], 4, "GPRB2"),
  SFARRAY32N(gprBank[3], 4, "GPRB3"),
  SFEND
 };

 if(!MDFNSS_StateAction(sm, load, data_only, StateRegs, "MAIN"))
  return(0);

 if(!MDFNSS_StateAction(sm, load, data_only, TLCS_StateRegs, "TLCS"))
  return(0);

 if(!MDFNNGPCDMA_StateAction(sm, load, data_only))
  return(0);

 if(!MDFNNGPCSOUND_StateAction(sm, load, data_only))
  return(0);

 if(!ngpgfx_StateAction(NGPGfx, sm, load, data_only))
  return(0);

 if(!MDFNNGPCZ80_StateAction(sm, load, data_only))
  return(0);

 if(!int_timer_StateAction(sm, load, data_only))
  return(0);

 if(!BIOSHLE_StateAction(sm, load, data_only))
  return(0);

 if(!FLASH_StateAction(sm, load, data_only))
  return(0);

 if(load)
 {
  RecacheFRM();
  changedSP();
 }
 return(1);
}

static void DoSimpleCommand(int cmd)
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
 { "japanese", 0, gettext_noop("Japanese") },
 { "0", 0 },

 { "english", 1, gettext_noop("English") },
 { "1", 1 },

 { NULL, 0 },
};

static MDFNSetting NGPSettings[] =
{
 { "ngp.language", MDFNSF_EMU_STATE | MDFNSF_UNTRUSTED_SAFE, gettext_noop("Language games should display text in."), NULL, MDFNST_ENUM, "english", NULL, NULL, NULL, NULL, LanguageList },
 { NULL }
};


bool system_io_flash_read(uint8* buffer, uint32 bufferLength)
{
 try
 {
  FileStream fp(MDFN_MakeFName(MDFNMKF_SAV, 0, "flash").c_str(), FileStream::MODE_READ);

  fp.read(buffer, bufferLength);
 }
 catch(std::exception &e)
 {
  //if(ene.Errno() == ENOENT)  . asdf
  return(0);
 }

 return(1);
}

bool system_io_flash_write(uint8* buffer, uint32 bufferLength)
{
 try
 {
  FileStream fp(MDFN_MakeFName(MDFNMKF_SAV, 0, "flash").c_str(), FileStream::MODE_WRITE);

  fp.write(buffer, bufferLength);
 }
 catch(std::exception &e)
 {
  return(0);
 }

 return(1);
}

static void SetLayerEnableMask(uint64 mask)
{
 ngpgfx_SetLayerEnableMask(NGPGfx, mask);
}

static const InputDeviceInputInfoStruct IDII[] =
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
 { ".ngp", gettext_noop("Neo Geo Pocket ROM Image") },
 { ".ngc", gettext_noop("Neo Geo Pocket Color ROM Image") },
 { NULL, NULL }
};

MDFNGI EmulatedNGP =
{
 "ngp",
 "Neo Geo Pocket (Color)",
 KnownExtensions,
 MODPRIO_INTERNAL_HIGH,
 NULL,
 &InputInfo,
 Load,
 TestMagic,
 NULL,
 NULL,
 CloseGame,
 SetLayerEnableMask,
 "Background Scroll\0Foreground Scroll\0Sprites\0",
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 false,
 StateAction,
 Emulate,
 SetInput,
 DoSimpleCommand,
 NGPSettings,
 MDFN_MASTERCLOCK_FIXED(6144000),
 0,

 false, // Multires possible?

 160,   // lcm_width
 152,   // lcm_height
 NULL,  // Dummy

 160,	// Nominal width
 152,	// Nominal height

 160,	// Framebuffer width
 152,	// Framebuffer height

 2,     // Number of output sound channels
};


static void set_basename(const char *path)
{
   const char *base = strrchr(path, '/');
   if (!base)
      base = strrchr(path, '\\');

   if (base)
      retro_base_name = base + 1;
   else
      retro_base_name = path;

   retro_base_name = retro_base_name.substr(0, retro_base_name.find_last_of('.'));
}

#define MEDNAFEN_CORE_NAME_MODULE "ngp"
#define MEDNAFEN_CORE_NAME "Mednafen NeoPop"
#define MEDNAFEN_CORE_VERSION "v0.9.36.1"
#define MEDNAFEN_CORE_EXTENSIONS "ngp|ngc"
#define MEDNAFEN_CORE_TIMING_FPS 60.25
#define MEDNAFEN_CORE_GEOMETRY_BASE_W 160 
#define MEDNAFEN_CORE_GEOMETRY_BASE_H 152
#define MEDNAFEN_CORE_GEOMETRY_MAX_W 160
#define MEDNAFEN_CORE_GEOMETRY_MAX_H 152
#define MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO (4.0 / 3.0)
#define FB_WIDTH 160
#define FB_HEIGHT 152



#define FB_MAX_HEIGHT FB_HEIGHT

const char *mednafen_core_str = MEDNAFEN_CORE_NAME;

static void check_system_specs(void)
{
   unsigned level = 0;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

void retro_init(void)
{
   struct retro_log_callback log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else 
      log_cb = NULL;

   MDFNI_InitializeModule();

   const char *dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      retro_base_directory = dir;
      // Make sure that we don't have any lingering slashes, etc, as they break Windows.
      size_t last = retro_base_directory.find_last_not_of("/\\");
      if (last != std::string::npos)
         last++;

      retro_base_directory = retro_base_directory.substr(0, last);

      MDFNI_Initialize(retro_base_directory.c_str());
   }
   else
   {
      /* TODO: Add proper fallback */
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "System directory is not defined. Fallback on using same dir as ROM for system directory later ...\n");
      failed_init = true;
   }
   
   if (environ_cb(RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY, &dir) && dir)
   {
	  // If save directory is defined use it, otherwise use system directory
      retro_save_directory = *dir ? dir : retro_base_directory;
      // Make sure that we don't have any lingering slashes, etc, as they break Windows.
      size_t last = retro_save_directory.find_last_not_of("/\\");
      if (last != std::string::npos)
         last++;

      retro_save_directory = retro_save_directory.substr(0, last);      
   }
   else
   {
      /* TODO: Add proper fallback */
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "Save directory is not defined. Fallback on using SYSTEM directory ...\n");
	  retro_save_directory = retro_base_directory;
   }      

#if defined(FRONTEND_SUPPORTS_RGB565)
   enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;
   if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565) && log_cb)
      log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");
#endif

   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;
   else
      perf_get_cpu_features_cb = NULL;

   check_system_specs();
}

void retro_reset(void)
{
   game->DoSimpleCommand(MDFN_MSC_RESET);
}

bool retro_load_game_special(unsigned, const struct retro_game_info *, size_t)
{
   return false;
}

static void set_volume (uint32_t *ptr, unsigned number)
{
   switch(number)
   {
      case 0:
      default:
         *ptr = number;
         break;
   }
}

static void check_variables(void)
{
   struct retro_variable var = {0};

   var.key = "ngp_language";

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      if (strcmp(var.value, "japanese") == 0)
         setting_ngp_language = 0;
      else if (strcmp(var.value, "english") == 0)
         setting_ngp_language = 1;    
      retro_reset();
   }
}

#define MAX_PLAYERS 1
#define MAX_BUTTONS 7
static uint8_t input_buf;


static void hookup_ports(bool force)
{
   MDFNGI *currgame = game;

   if (initial_ports_hookup && !force)
      return;

   currgame->SetInput(0, "gamepad", &input_buf);

   initial_ports_hookup = true;
}

bool retro_load_game(const struct retro_game_info *info)
{
   if (failed_init)
      return false;

   struct retro_input_descriptor desc[] = {
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT,  "D-Pad Left" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP,    "D-Pad Up" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN,  "D-Pad Down" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "D-Pad Right" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B,     "A" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A,     "B" },
      { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Option" },

      { 0 },
   };

   environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, desc);

   overscan = false;
   environ_cb(RETRO_ENVIRONMENT_GET_OVERSCAN, &overscan);

   set_basename(info->path);

   game = MDFNI_LoadGame(MEDNAFEN_CORE_NAME_MODULE, info->path);
   if (!game)
      return false;

   MDFN_PixelFormat pix_fmt(MDFN_COLORSPACE_RGB, 16, 8, 0, 24);
   memset(&last_pixel_format, 0, sizeof(MDFN_PixelFormat));
   
   surf = new MDFN_Surface(NULL, FB_WIDTH, FB_HEIGHT, FB_WIDTH, pix_fmt);

   hookup_ports(true);

   check_variables();

   return game;
}

void retro_unload_game()
{
   if (!game)
      return;

   MDFNI_CloseGame();
}



static void update_input(void)
{
   MDFNGI *currgame = (MDFNGI*)game;
   input_buf = 0;

   static unsigned map[] = {
      RETRO_DEVICE_ID_JOYPAD_UP, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_DOWN, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_LEFT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_RIGHT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_B,
      RETRO_DEVICE_ID_JOYPAD_A,
      RETRO_DEVICE_ID_JOYPAD_START,
   };

   for (unsigned i = 0; i < MAX_BUTTONS; i++)
      input_buf |= map[i] != -1u &&
         input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, map[i]) ? (1 << i) : 0;

#ifdef MSB_FIRST
   union {
      uint8_t b[2];
      uint16_t s;
   } u;
   u.s = input_buf;
   input_buf = u.b[0] | u.b[1] << 8;
#endif

}

static uint64_t video_frames, audio_frames;


void retro_run()
{
   int32 SoundBufMaxSize;
   int32 SoundBufSize;
   unsigned width, height;
   const int16 *SoundBuf;
   static int16_t sound_buf[0x10000];
   static MDFN_Rect rects[FB_MAX_HEIGHT];
   const uint16_t *pix;
   bool updated = false;
   MDFNGI *curgame = game;
   EmulateSpecStruct spec = {0};

   input_poll_cb();

   update_input();

   rects[0].w = ~0;

   spec.surface = surf;
   spec.SoundRate = 44100;
   spec.SoundBuf = sound_buf;
   spec.LineWidths = rects;
   spec.SoundBufMaxSize = sizeof(sound_buf) / 2;
   spec.SoundVolume = 1.0;
   spec.soundmultiplier = 1.0;
   spec.SoundBufSize = 0;
   spec.VideoFormatChanged = false;
   spec.SoundFormatChanged = false;

   if (memcmp(&last_pixel_format, &spec.surface->format, sizeof(MDFN_PixelFormat)))
   {
      spec.VideoFormatChanged = TRUE;

      last_pixel_format = spec.surface->format;
   }

   if (spec.SoundRate != last_sound_rate)
   {
      spec.SoundFormatChanged = true;
      last_sound_rate = spec.SoundRate;
   }

   curgame->Emulate(&spec);

   SoundBuf = spec.SoundBuf + spec.SoundBufSizeALMS * curgame->soundchan;
   SoundBufSize = spec.SoundBufSize - spec.SoundBufSizeALMS;
   SoundBufMaxSize = spec.SoundBufMaxSize - spec.SoundBufSizeALMS;

   spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

   width  = spec.DisplayRect.w;
   height = spec.DisplayRect.h;

   pix = surf->pixels16;
   video_cb(pix, width, height, FB_WIDTH << 1);

   video_frames++;
   audio_frames += spec.SoundBufSize;

   audio_batch_cb(spec.SoundBuf, spec.SoundBufSize);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = MEDNAFEN_CORE_NAME;
   info->library_version  = MEDNAFEN_CORE_VERSION;
   info->need_fullpath    = true;
   info->valid_extensions = MEDNAFEN_CORE_EXTENSIONS;
   info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = MEDNAFEN_CORE_TIMING_FPS;
   info->timing.sample_rate    = 44100;
   info->geometry.base_width   = MEDNAFEN_CORE_GEOMETRY_BASE_W;
   info->geometry.base_height  = MEDNAFEN_CORE_GEOMETRY_BASE_H;
   info->geometry.max_width    = MEDNAFEN_CORE_GEOMETRY_MAX_W;
   info->geometry.max_height   = MEDNAFEN_CORE_GEOMETRY_MAX_H;
   info->geometry.aspect_ratio = MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO;
}

void retro_deinit()
{
   delete surf;
   surf = NULL;

   if (log_cb)
   {
      log_cb(RETRO_LOG_INFO, "[%s]: Samples / Frame: %.5f\n",
            mednafen_core_str, (double)audio_frames / video_frames);
      log_cb(RETRO_LOG_INFO, "[%s]: Estimated FPS: %.5f\n",
            mednafen_core_str, (double)video_frames * 44100 / audio_frames);
   }
}

unsigned retro_get_region(void)
{
   return RETRO_REGION_NTSC; // FIXME: Regions for other cores.
}

unsigned retro_api_version(void)
{
   return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned in_port, unsigned device)
{
   MDFNGI *currgame = (MDFNGI*)game;

   if (!currgame)
      return;
}

void retro_set_environment(retro_environment_t cb)
{
   environ_cb = cb;

   static const struct retro_variable vars[] = {
      { "ngp_language", "Language (restart); english|japanese" },
      { NULL, NULL },
   };
   cb(RETRO_ENVIRONMENT_SET_VARIABLES, (void*)vars);
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
   audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
   input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
   video_cb = cb;
}

static size_t serialize_size;

size_t retro_serialize_size(void)
{
   MDFNGI *curgame = (MDFNGI*)game;
   //if (serialize_size)
   //   return serialize_size;

   if (!curgame->StateAction)
   {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[mednafen]: Module %s doesn't support save states.\n", curgame->shortname);
      return 0;
   }

   StateMem st;
   memset(&st, 0, sizeof(st));

   if (!MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL))
   {
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "[mednafen]: Module %s doesn't support save states.\n", curgame->shortname);
      return 0;
   }

   free(st.data);
   return serialize_size = st.len;
}

bool retro_serialize(void *data, size_t size)
{
   StateMem st;
   memset(&st, 0, sizeof(st));
   st.data     = (uint8_t*)data;
   st.malloced = size;

   return MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL);
}

bool retro_unserialize(const void *data, size_t size)
{
   StateMem st;
   memset(&st, 0, sizeof(st));
   st.data = (uint8_t*)data;
   st.len  = size;

   return MDFNSS_LoadSM(&st, 0, 0);
}

void *retro_get_memory_data(unsigned)
{
   return NULL;
}

size_t retro_get_memory_size(unsigned)
{
   return 0;
}

void retro_cheat_reset(void)
{}

void retro_cheat_set(unsigned, bool, const char *)
{}

#ifdef _WIN32
static void sanitize_path(std::string &path)
{
   size_t size = path.size();
   for (size_t i = 0; i < size; i++)
      if (path[i] == '/')
         path[i] = '\\';
}
#endif

// Use a simpler approach to make sure that things go right for libretro.
std::string MDFN_MakeFName(MakeFName_Type type, int id1, const char *cd1)
{
   char slash;
#ifdef _WIN32
   slash = '\\';
#else
   slash = '/';
#endif
   std::string ret;
   switch (type)
   {
      case MDFNMKF_SAV:
         ret = retro_save_directory +slash + retro_base_name +
            std::string(".") +
#ifndef _XBOX
	    md5_context::asciistr(MDFNGameInfo->MD5, 0) + std::string(".") +
#endif
            std::string(cd1);
         break;
      case MDFNMKF_FIRMWARE:
         ret = retro_base_directory + slash + std::string(cd1);
#ifdef _WIN32
   sanitize_path(ret); // Because Windows path handling is mongoloid.
#endif
         break;
      default:	  
         break;
   }

   if (log_cb)
      log_cb(RETRO_LOG_INFO, "MDFN_MakeFName: %s\n", ret.c_str());
   return ret;
}

void MDFND_DispMessage(unsigned char *str)
{
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "%s\n", str);
}

void MDFND_Message(const char *str)
{
   if (log_cb)
      log_cb(RETRO_LOG_INFO, "%s\n", str);
}

void MDFND_MidSync(const EmulateSpecStruct *)
{}

void MDFN_MidLineUpdate(EmulateSpecStruct *espec, int y)
{
 //MDFND_MidLineUpdate(espec, y);
}

void MDFND_PrintError(const char* err)
{
   if (log_cb)
      log_cb(RETRO_LOG_ERROR, "%s\n", err);
}
