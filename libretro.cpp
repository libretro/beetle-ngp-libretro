#include "mednafen/mednafen.h"
#include <libretro.h>
#include <streams/file_stream.h>

#include "libretro_core_options.h"

#include "mednafen/ngp/neopop.h"
#include "mednafen/ngp/sound.h"



// core options
static int RETRO_SAMPLE_RATE = 44100;

static int RETRO_PIX_BYTES = 2;
static int RETRO_PIX_DEPTH = 15;


// ====================================================


static MDFNGI *game;
static uint8_t input_buf;
static MDFN_Surface *surf;
static bool initial_ports_hookup = false;


extern void SetInput(unsigned port, const char *type, uint8 *ptr);
extern void DoSimpleCommand(int cmd);
extern void Emulate(EmulateSpecStruct *espec);

extern MDFNGI *MDFNI_LoadGame(const char *name);
extern MDFNGI *MDFNI_LoadGameData(const uint8_t *data, size_t size);
extern void MDFNI_CloseGame(void);
extern void MDFNGI_reset(MDFNGI *gameinfo);

extern uint8 CPUExRAM[16384];


static void hookup_ports(bool force)
{
   if (initial_ports_hookup && !force)
      return;

   SetInput(0, "gamepad", &input_buf);

   initial_ports_hookup = true;
}


// ====================================================


struct retro_perf_callback perf_cb;
retro_get_cpu_features_t perf_get_cpu_features_cb = NULL;
retro_log_printf_t log_cb;
static retro_video_refresh_t video_cb;
static retro_audio_sample_batch_t audio_batch_cb;
static retro_environment_t environ_cb;
static retro_input_poll_t input_poll_cb;
static retro_input_state_t input_state_cb;

static bool overscan;

static bool failed_init;

static bool libretro_supports_bitmasks = false;

extern "C" char retro_base_directory[1024];
std::string retro_base_name;
char retro_save_directory[1024];
char error_message[1024];


#define MEDNAFEN_CORE_NAME_MODULE "ngp"
#define MEDNAFEN_CORE_NAME "Beetle NeoPop"
#define MEDNAFEN_CORE_VERSION "v1.23.0.0"
#define MEDNAFEN_CORE_EXTENSIONS "ngp|ngc|ngpc|npc"
#define MEDNAFEN_CORE_TIMING_FPS 60.25
#define MEDNAFEN_CORE_GEOMETRY_BASE_W 160 
#define MEDNAFEN_CORE_GEOMETRY_BASE_H 152
#define MEDNAFEN_CORE_GEOMETRY_MAX_W 160
#define MEDNAFEN_CORE_GEOMETRY_MAX_H 152
#define MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO (20.0 / 19.0)
#define FB_WIDTH 160
#define FB_HEIGHT 152

#define FB_MAX_HEIGHT FB_HEIGHT

const char *mednafen_core_str = MEDNAFEN_CORE_NAME;

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

static void check_system_specs(void)
{
   unsigned level = 0;
   environ_cb(RETRO_ENVIRONMENT_SET_PERFORMANCE_LEVEL, &level);
}

static void check_variables(void)
{
   struct retro_variable var = {0};

   var.key = "ngp_language";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      // user must manually restart core for change to happen
      if (!strcmp(var.value, "japanese"))
         setting_ngp_language = 0;
      else if (!strcmp(var.value, "english"))
         setting_ngp_language = 1;
   }

   var.key = "ngp_sound_sample_rate";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      static bool once = false;

      if(!once)
      {
         RETRO_SAMPLE_RATE = atoi(var.value);
         once = true;
      }
   }

   var.key = "ngp_gfx_colors";
   var.value = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var) && var.value)
   {
      static bool once = false;

      if(!once)
      {
         if (strcmp(var.value, "16bit") == 0)
         {
            RETRO_PIX_BYTES = 2;
            RETRO_PIX_DEPTH = 16;
         }
         else if (strcmp(var.value, "24bit") == 0)
         {
            RETRO_PIX_BYTES = 4;
            RETRO_PIX_DEPTH = 24;
         }
         once = true;
      }
   }
}

void retro_init(void)
{
   struct retro_log_callback log;
   if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
      log_cb = log.log;
   else 
      log_cb = NULL;

   const char *dir = NULL;

   if (environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &dir) && dir)
   {
      std::string retro_base_dir_tmp;

      retro_base_dir_tmp = dir;
      // Make sure that we don't have any lingering slashes, etc, as they break Windows.
      size_t last = retro_base_dir_tmp.find_last_not_of("/\\");
      if (last != std::string::npos)
         last++;

      retro_base_dir_tmp= retro_base_dir_tmp.substr(0, last);

      strcpy(retro_base_directory, retro_base_dir_tmp.c_str());
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
      std::string retro_save_dir_tmp;

	  // If save directory is defined use it, otherwise use system directory
      retro_save_dir_tmp = *dir ? dir : retro_base_directory;
      // Make sure that we don't have any lingering slashes, etc, as they break Windows.
      size_t last = retro_save_dir_tmp.find_last_not_of("/\\");
      if (last != std::string::npos)
         last++;

      retro_save_dir_tmp = retro_save_dir_tmp.substr(0, last);      

      strcpy(retro_save_directory, retro_save_dir_tmp.c_str());
   }
   else
   {
      /* TODO: Add proper fallback */
      if (log_cb)
         log_cb(RETRO_LOG_WARN, "Save directory is not defined. Fallback on using SYSTEM directory ...\n");
      strcpy(retro_save_directory, retro_base_directory);
   }      

   check_variables();

   if (RETRO_PIX_DEPTH == 24)
   {
      enum retro_pixel_format rgb888 = RETRO_PIXEL_FORMAT_XRGB8888;

      if(!environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb888))
      {
         if(log_cb) log_cb(RETRO_LOG_ERROR, "Pixel format XRGB8888 not supported by platform.\n");
         
         RETRO_PIX_BYTES = 2;
         RETRO_PIX_DEPTH = 15;
      }
   }

#if defined(FRONTEND_SUPPORTS_RGB565)
   if (RETRO_PIX_BYTES == 2)
   {
      enum retro_pixel_format rgb565 = RETRO_PIXEL_FORMAT_RGB565;

      if (environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &rgb565))
      {
         if(log_cb) log_cb(RETRO_LOG_INFO, "Frontend supports RGB565 - will use that instead of XRGB1555.\n");

         RETRO_PIX_DEPTH = 16;
      }
   }
#endif

   perf_get_cpu_features_cb = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_PERF_INTERFACE, &perf_cb))
      perf_get_cpu_features_cb = perf_cb.get_cpu_features;

   if (environ_cb(RETRO_ENVIRONMENT_GET_INPUT_BITMASKS, NULL))
      libretro_supports_bitmasks = true;

   check_system_specs();
   MDFNGI_reset(MDFNGameInfo);
}

void retro_reset(void)
{
   DoSimpleCommand(MDFN_MSC_RESET);
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

#define MAX_PLAYERS 1
#define MAX_BUTTONS 7


bool retro_load_game(const struct retro_game_info *info)
{
   if (!info || failed_init)
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

#ifdef LOAD_FROM_MEMORY
   game = MDFNI_LoadGameData((const uint8_t *)info->data, info->size);
#else
   game = MDFNI_LoadGame(info->path);
#endif

   if (!game)
      return false;

   MDFN_LoadGameCheats(NULL);
   MDFNMP_InstallReadPatches();

   surf = (MDFN_Surface*)calloc(1, sizeof(*surf));
   
   if (!surf)
      return false;
   
   surf->width  = FB_WIDTH;
   surf->height = FB_HEIGHT;
   surf->pitch  = FB_WIDTH;
   surf->pix_bytes = RETRO_PIX_BYTES;
   surf->pix_depth = RETRO_PIX_DEPTH;

   if (RETRO_PIX_BYTES == 2)
   {
      surf->pixels16 = (uint16_t*) calloc(1, FB_WIDTH * FB_HEIGHT * RETRO_PIX_BYTES);
      if (!surf->pixels16)
      {
         free(surf);
         return false;
      }
   }
   else if (RETRO_PIX_BYTES == 4)
   {
      surf->pixels = (uint32_t*) calloc(1, FB_WIDTH * FB_HEIGHT * RETRO_PIX_BYTES);
      if (!surf->pixels)
      {
         free(surf);
         return false;
      }
   }

   hookup_ports(true);

   NGPGfx_set_pixel_format(RETRO_PIX_DEPTH);
   MDFNNGPC_SetSoundRate(RETRO_SAMPLE_RATE);

   return game;
}

void retro_unload_game(void)
{
   if (!game)
      return;

   MDFN_FlushGameCheats(0);
   MDFNI_CloseGame();
   MDFNMP_Kill();
}

static void update_input(void)
{
   static unsigned map[] = {
      RETRO_DEVICE_ID_JOYPAD_UP, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_DOWN, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_LEFT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_RIGHT, //X Cursor horizontal-layout games
      RETRO_DEVICE_ID_JOYPAD_B,
      RETRO_DEVICE_ID_JOYPAD_A,
      RETRO_DEVICE_ID_JOYPAD_START,
   };
   unsigned i, j;
   int16_t ret = 0;
   input_buf   = 0;

   if (libretro_supports_bitmasks)
      ret = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_MASK);
   else
   {
      for (j = 0; j < (RETRO_DEVICE_ID_JOYPAD_R3+1); j++)
         if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, j))
            ret |= (1 << j);
   }

   for (i = 0; i < MAX_BUTTONS; i++)
      if ((map[i] != -1u) && (ret & (1 << map[i])))
         input_buf |= (1 << i);
}

static uint64_t video_frames, audio_frames;

void retro_run(void)
{
   int32 SoundBufSize;
   unsigned width, height;
   static int16_t sound_buf[0x10000];
   static MDFN_Rect rects[FB_MAX_HEIGHT];
   EmulateSpecStruct spec = {0};
   bool updated = false;

   input_poll_cb();

   update_input();

   rects[0].w              = ~0;

   spec.surface            = surf;
   spec.SoundRate          = RETRO_SAMPLE_RATE;
   spec.SoundBuf           = sound_buf;
   spec.LineWidths         = rects;
   spec.SoundBufMaxSize    = sizeof(sound_buf) / 2;
   spec.SoundVolume        = 1.0;
   spec.soundmultiplier    = 1.0;
   spec.SoundBufSize       = 0;
   spec.VideoFormatChanged = false;
   spec.SoundFormatChanged = false;

   Emulate(&spec);

   SoundBufSize    = spec.SoundBufSize - spec.SoundBufSizeALMS;

   spec.SoundBufSize = spec.SoundBufSizeALMS + SoundBufSize;

   width  = spec.DisplayRect.w;
   height = spec.DisplayRect.h;

   if(RETRO_PIX_BYTES == 2)
      video_cb(surf->pixels16, width, height, FB_WIDTH * RETRO_PIX_BYTES);
   else if(RETRO_PIX_BYTES == 4)
      video_cb(surf->pixels, width, height, FB_WIDTH * RETRO_PIX_BYTES);

   video_frames++;
   audio_frames += spec.SoundBufSize;

   for(int total = 0; total < spec.SoundBufSize; )
      total += audio_batch_cb(spec.SoundBuf + total*2, spec.SoundBufSize - total);

   if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE, &updated) && updated)
      check_variables();
}

void retro_get_system_info(struct retro_system_info *info)
{
   memset(info, 0, sizeof(*info));
   info->library_name     = MEDNAFEN_CORE_NAME;
#ifndef GIT_VERSION
#define GIT_VERSION ""
#endif

#ifdef LOAD_FROM_MEMORY
   info->need_fullpath    = false;
#else
   info->need_fullpath    = true;
#endif

   info->library_version  = MEDNAFEN_CORE_VERSION GIT_VERSION;
   info->valid_extensions = MEDNAFEN_CORE_EXTENSIONS;
   info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
   memset(info, 0, sizeof(*info));
   info->timing.fps            = MEDNAFEN_CORE_TIMING_FPS;
   info->timing.sample_rate    = RETRO_SAMPLE_RATE;
   info->geometry.base_width   = MEDNAFEN_CORE_GEOMETRY_BASE_W;
   info->geometry.base_height  = MEDNAFEN_CORE_GEOMETRY_BASE_H;
   info->geometry.max_width    = MEDNAFEN_CORE_GEOMETRY_MAX_W;
   info->geometry.max_height   = MEDNAFEN_CORE_GEOMETRY_MAX_H;
   info->geometry.aspect_ratio = MEDNAFEN_CORE_GEOMETRY_ASPECT_RATIO;
}

void retro_deinit(void)
{
   if (surf)
      free(surf);
   surf = NULL;

   if (log_cb)
   {
      log_cb(RETRO_LOG_INFO, "[%s]: Samples / Frame: %.5f\n",
            mednafen_core_str, (double)audio_frames / video_frames);
      log_cb(RETRO_LOG_INFO, "[%s]: Estimated FPS: %.5f\n",
            mednafen_core_str, (double)video_frames * 44100 / audio_frames);
   }

   libretro_supports_bitmasks = false;
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
}

void retro_set_environment(retro_environment_t cb)
{
   struct retro_vfs_interface_info vfs_iface_info;
   environ_cb = cb;

   libretro_set_core_options(environ_cb);

   vfs_iface_info.required_interface_version = 1;
   vfs_iface_info.iface                      = NULL;
   if (environ_cb(RETRO_ENVIRONMENT_GET_VFS_INTERFACE, &vfs_iface_info))
	   filestream_vfs_init(&vfs_iface_info);
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
   StateMem st;

   st.data           = NULL;
   st.loc            = 0;
   st.len            = 0;
   st.malloced       = 0;
   st.initial_malloc = 0;

   if (!MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL))
      return 0;

   free(st.data);

   return serialize_size = st.len;
}

bool retro_serialize(void *data, size_t size)
{
   StateMem st;
   bool ret          = false;
   uint8_t *_dat     = (uint8_t*)malloc(size);

   if (!_dat)
      return false;

   /* Mednafen can realloc the buffer so we need to ensure this is safe. */
   st.data           = _dat;
   st.loc            = 0;
   st.len            = 0;
   st.malloced       = size;
   st.initial_malloc = 0;

   ret = MDFNSS_SaveSM(&st, 0, 0, NULL, NULL, NULL);

   memcpy(data, st.data, size);
   free(st.data);

   return ret;
}

bool retro_unserialize(const void *data, size_t size)
{
   StateMem st;

   st.data           = (uint8_t*)data;
   st.loc            = 0;
   st.len            = size;
   st.malloced       = 0;
   st.initial_malloc = 0;

   return MDFNSS_LoadSM(&st, 0, 0);
}

void *retro_get_memory_data(unsigned type)
{
   if(type == RETRO_MEMORY_SYSTEM_RAM)
      return CPUExRAM;
   else return NULL;
}

size_t retro_get_memory_size(unsigned type)
{
   if(type == RETRO_MEMORY_SYSTEM_RAM)
      return 16384;
   else return 0;
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
         ret = std::string(retro_save_directory) + slash + std::string(retro_base_name) +
            std::string(".") + std::string(cd1);
         break;
      case MDFNMKF_FIRMWARE:
         ret = std::string(retro_base_directory) + slash + std::string(cd1);
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


extern "C" {
void MDFN_printf(char const *format,...)
{
   if (log_cb)
   {
      va_list args;
      va_start (args, format);
      vsnprintf (error_message, 1024-1, format, args);

      log_cb(RETRO_LOG_INFO, "%s\n", error_message);

      va_end (args);
   }
}
}

void MDFN_PrintError(char const *format,...)
{
   if (log_cb)
   {
      va_list args;
      va_start (args, format);
      vsnprintf (error_message, 1024-1, format, args);

      log_cb(RETRO_LOG_ERROR, "%s\n", error_message);

      va_end (args);
   }
}

void MDFN_Notify(char const *format,...)
{
   if (log_cb)
   {
      va_list args;
      va_start (args, format);
      vsnprintf (error_message, 1024-1, format, args);

      log_cb(RETRO_LOG_INFO, "%s\n", error_message);

      va_end (args);
   }
}
