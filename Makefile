DEBUG = 0
FRONTEND_SUPPORTS_RGB565 = 1

CORE_DIR := .

# system platform
ifeq ($(platform),)
   platform = unix
   ifeq ($(shell uname -a),)
      EXE_EXT = .exe
      platform = win
   else ifneq ($(findstring MINGW,$(shell uname -a)),)
      platform = win
   else ifneq ($(findstring win,$(shell uname -a)),)
      platform = win
   else ifneq ($(findstring Darwin,$(shell uname -a)),)
      platform = osx
   endif
else ifneq (,$(findstring armv,$(platform)))
   override platform += unix
else ifneq (,$(findstring rpi,$(platform)))
   override platform += unix
endif

ifeq ($(shell uname -p),powerpc)
   arch = ppc
else
   arch = intel
endif


NEED_BPP = 16
NEED_BLIP = 1
NEED_STEREO_SOUND = 1

prefix := /usr
libdir := $(prefix)/lib

LIBRETRO_DIR := libretro

TARGET_NAME := mednafen_ngp

# GIT HASH
GIT_VERSION := " $(shell git rev-parse --short HEAD || echo unknown)"
ifneq ($(GIT_VERSION)," unknown")
   FLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"
endif

# Unix
ifneq (,$(findstring unix,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.so
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T

   # Raspberry Pi
   ifneq (,$(findstring rpi,$(platform)))
      FLAGS += -fomit-frame-pointer -ffast-math -marm
      ifneq (,$(findstring rpi1,$(platform)))
         FLAGS += -march=armv6j -mfpu=vfp -mfloat-abi=hard
      else ifneq (,$(findstring rpi2,$(platform)))
         FLAGS += -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard
      else ifneq (,$(findstring rpi3,$(platform)))
         FLAGS += -mcpu=cortex-a53 -mfpu=neon-fp-armv8 -mfloat-abi=hard
      endif
   endif

# OS X
else ifeq ($(platform), osx)
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   ifeq ($(arch),ppc)
      ENDIANNESS_DEFINES := -DMSB_FIRST
      OLD_GCC := 1
   endif
   OSXVER = `sw_vers -productVersion | cut -d. -f 2`
   OSX_LT_MAVERICKS = `(( $(OSXVER) <= 9)) && echo "YES"`
   ifeq ($(OSX_LT_MAVERICKS),"YES")
      fpic += -mmacosx-version-min=10.5
   endif

# iOS
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
   ifeq ($(IOSSDK),)
      IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
   endif
   CC = cc -arch armv7 -isysroot $(IOSSDK)
   CXX = c++ -arch armv7 -isysroot $(IOSSDK)
   OSXVER = `sw_vers -productVersion | cut -d. -f 2`
   IPHONEMINVER :=
   ifeq ($(platform),$(filter $(platform),ios9 ios-arm64))
      IPHONEMINVER = -miphoneos-version-min=8.0
   else
      IPHONEMINVER = -miphoneos-version-min=5.0
   endif
   LDFLAGS += $(IPHONEMINVER)
   FLAGS += $(IPHONEMINVER)
   CC += $(IPHONEMINVER)
   CXX += $(IPHONEMINVER)

# QNX
else ifeq ($(platform), qnx)
   TARGET := $(TARGET_NAME)_libretro_$(platform).so
   fpic := -fPIC
   SHARED := -lcpp -lm -shared -Wl,--no-undefined -Wl,--version-script=link.T
   CC = qcc -Vgcc_ntoarmv7le
   CXX = QCC -Vgcc_ntoarmv7le_cpp
   AR = QCC -Vgcc_ntoarmv7le
   FLAGS += -D__BLACKBERRY_QNX__ -marm -mcpu=cortex-a9 -mfpu=neon -mfloat-abi=softfp


# PS3
else ifneq (,$(filter $(platform), ps3 sncps3 psl1ght))
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   ENDIANNESS_DEFINES := -DMSB_FIRST
   STATIC_LINKING = 1

   # sncps3
   ifneq (,$(findstring sncps3,$(platform)))
      TARGET := $(TARGET_NAME)_libretro_ps3.a
      CC = $(CELL_SDK)/host-win32/sn/bin/ps3ppusnc.exe
      CXX = $(CC)
      AR = $(CELL_SDK)/host-win32/sn/bin/ps3snarl.exe
      OLD_GCC := 1
      NO_GCC := 1
      CXXFLAGS += -Xc+=exceptions
      FLAGS += -DARCH_POWERPC_ALTIVEC

   # PS3
   else ifneq (,$(findstring ps3,$(platform)))
      CC = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-gcc.exe
      CXX = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-g++.exe
      AR = $(CELL_SDK)/host-win32/ppu/bin/ppu-lv2-ar.exe
      OLD_GCC := 1
      FLAGS += -DARCH_POWERPC_ALTIVEC

   # Lightweight PS3 Homebrew SDK
   else ifneq (,$(findstring psl1ght,$(platform)))
      CC = $(PS3DEV)/ppu/bin/ppu-gcc$(EXE_EXT)
      CXX = $(PS3DEV)/ppu/bin/ppu-g++$(EXE_EXT)
      AR = $(PS3DEV)/ppu/bin/ppu-ar$(EXE_EXT)
   endif

# PSP
else ifeq ($(platform), psp1)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = psp-gcc$(EXE_EXT)
   CXX = psp-g++$(EXE_EXT)
   AR = psp-ar$(EXE_EXT)
   FLAGS += -DPSP -G0
   STATIC_LINKING = 1

# Vita
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = arm-vita-eabi-gcc$(EXE_EXT)
   CXX = arm-vita-eabi-g++$(EXE_EXT)
   AR = arm-vita-eabi-ar$(EXE_EXT)
   FLAGS += -DVITA
   STATIC_LINKING = 1

# CTR (3DS)
else ifeq ($(platform), ctr)
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITARM)/bin/arm-none-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITARM)/bin/arm-none-eabi-g++$(EXE_EXT)
   AR = $(DEVKITARM)/bin/arm-none-eabi-ar$(EXE_EXT)
   ENDIANNESS_DEFINES := -DLSB_FIRST
   FLAGS += -DARM11 -D_3DS
   FLAGS += -march=armv6k -mtune=mpcore -mfloat-abi=hard
   FLAGS += -mword-relocations
   FLAGS += -fomit-frame-pointer -fstrict-aliasing -ffast-math
   FLAGS += -fno-rtti
   FLAGS += -fno-exceptions -DDISABLE_EXCEPTIONS
   STATIC_LINKING = 1

# Xbox 360
else ifeq ($(platform), xenon)
   TARGET := $(TARGET_NAME)_libretro_xenon360.a
   CC = xenon-gcc$(EXE_EXT)
   CXX = xenon-g++$(EXE_EXT)
   AR = xenon-ar$(EXE_EXT)
   ENDIANNESS_DEFINES += -D__LIBXENON__ -m32 -D__ppc__ -DMSB_FIRST
   STATIC_LINKING = 1

# Nintendo Game Cube / Wii / WiiU
else ifneq (,$(filter $(platform), ngc wii wiiu))
   TARGET := $(TARGET_NAME)_libretro_$(platform).a
   CC = $(DEVKITPPC)/bin/powerpc-eabi-gcc$(EXE_EXT)
   CXX = $(DEVKITPPC)/bin/powerpc-eabi-g++$(EXE_EXT)
   AR = $(DEVKITPPC)/bin/powerpc-eabi-ar$(EXE_EXT)
   ENDIANNESS_DEFINES += -DGEKKO -mcpu=750 -meabi -mhard-float -DMSB_FIRST
   FLAGS += -U__INT32_TYPE__ -U __UINT32_TYPE__ -D__INT32_TYPE__=int
   EXTRA_INCLUDES := -I$(DEVKITPRO)/libogc/include
   STATIC_LINKING = 1

   # Nintendo WiiU
   ifneq (,$(findstring wiiu,$(platform)))
      ENDIANNESS_DEFINES += -DWIIU -DHW_RVL -mwup

   # Nintendo Wii
   else ifneq (,$(findstring wii,$(platform)))
      ENDIANNESS_DEFINES += -DHW_RVL -mrvl

   # Nintendo Game Cube
   else ifneq (,$(findstring ngc,$(platform)))
      ENDIANNESS_DEFINES += -DHW_DOL -mrvl
   endif

# Emscripten
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_$(platform).bc
   STATIC_LINKING = 1

#GCW0
else ifeq ($(platform), gcw0)
   TARGET := $(TARGET_NAME)_libretro.so
   CC = /opt/gcw0-toolchain/usr/bin/mipsel-linux-gcc
   CXX = /opt/gcw0-toolchain/usr/bin/mipsel-linux-g++
   AR = /opt/gcw0-toolchain/usr/bin/mipsel-linux-ar
   fpic := -fPIC
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
   FLAGS += -ffast-math -march=mips32 -mtune=mips32r2 -mhard-float

# Windows MSVC 2003 Xbox 1
else ifeq ($(platform), xbox1_msvc2003)
TARGET := $(TARGET_NAME)_libretro_xdk1.lib
MSVCBINDIRPREFIX = $(XDK)/xbox/bin/vc71
CC  = "$(MSVCBINDIRPREFIX)/CL.exe"
CXX  = "$(MSVCBINDIRPREFIX)/CL.exe"
LD   = "$(MSVCBINDIRPREFIX)/lib.exe"

export INCLUDE := $(XDK)/xbox/include
export LIB := $(XDK)/xbox/lib
PSS_STYLE :=2
CFLAGS   += -D_XBOX -D_XBOX1
CXXFLAGS += -D_XBOX -D_XBOX1
STATIC_LINKING=1
HAS_GCC := 0
# Windows MSVC 2010 Xbox 360
else ifeq ($(platform), xbox360_msvc2010)
TARGET := $(TARGET_NAME)_libretro_xdk360.lib
MSVCBINDIRPREFIX = $(XEDK)/bin/win32
CC  = "$(MSVCBINDIRPREFIX)/cl.exe"
CXX  = "$(MSVCBINDIRPREFIX)/cl.exe"
LD   = "$(MSVCBINDIRPREFIX)/lib.exe"

export INCLUDE := $(XEDK)/include/xbox
export LIB := $(XEDK)/lib/xbox
PSS_STYLE :=2
ENDIANNESS_DEFINES := -DMSB_FIRST
CFLAGS   += -D_XBOX -D_XBOX360
CXXFLAGS += -D_XBOX -D_XBOX360
STATIC_LINKING=1
HAS_GCC := 0

# Windows MSVC 2010 x64
else ifeq ($(platform), windows_msvc2010_x64)
	CC  = cl.exe
	CXX = cl.exe

      NO_GCC := 1
PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin/amd64"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
LIB := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/lib/amd64")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib/x64
WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib/x64

WindowsSdkDirInc := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include
WindowsSdkDirInc ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include


INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
export INCLUDE := $(INCLUDE)
export LIB := $(LIB);$(WindowsSdkDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
# Windows MSVC 2010 x86
else ifeq ($(platform), windows_msvc2010_x86)
	CC  = cl.exe
	CXX = cl.exe

      NO_GCC := 1
PATH := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../IDE")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS100COMNTOOLS)../../VC/lib")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS100COMNTOOLS)../../VC/include")

WindowsSdkDir := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib
WindowsSdkDir ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')lib

WindowsSdkDirInc := $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.0A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include
WindowsSdkDirInc ?= $(shell reg query "HKLM\SOFTWARE\Microsoft\Microsoft SDKs\Windows\v7.1A" -v "InstallationFolder" | grep -o '[A-Z]:\\.*')Include


INCFLAGS_PLATFORM = -I"$(WindowsSdkDirInc)"
export INCLUDE := $(INCLUDE)
export LIB := $(LIB);$(WindowsSdkDir)
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL

# Windows MSVC 2005 x86
else ifeq ($(platform), windows_msvc2005_x86)
	CC  = cl.exe
	CXX = cl.exe

PATH := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin"):$(PATH)
PATH := $(PATH):$(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../IDE")
INCLUDE := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/include")
LIB := $(shell IFS=$$'\n'; cygpath -w "$(VS80COMNTOOLS)../../VC/lib")
BIN := $(shell IFS=$$'\n'; cygpath "$(VS80COMNTOOLS)../../VC/bin")

WindowsSdkDir := $(INETSDK)

export INCLUDE := $(INCLUDE);$(INETSDK)/Include;libretro-common/include/compat/msvc
export LIB := $(LIB);$(WindowsSdkDir);$(INETSDK)/Lib
TARGET := $(TARGET_NAME)_libretro.dll
PSS_STYLE :=2
LDFLAGS += -DLL
CFLAGS += -D_CRT_SECURE_NO_DEPRECATE
LIBS =
FLAGS += -DHAVE__MKDIR

# Windows
else
   TARGET := $(TARGET_NAME)_libretro.dll
   CC = gcc
   CXX = g++
   SHARED := -shared -Wl,--no-undefined -Wl,--version-script=link.T
   LDFLAGS += -static-libgcc -static-libstdc++ -lwinmm
   FLAGS += -DHAVE__MKDIR
endif

include Makefile.common

ifneq (,$(findstring msvc,$(platform)))
WARNINGS :=
else
WARNINGS := -Wall \
	-Wno-sign-compare \
	-Wno-unused-variable \
	-Wno-unused-function \
	-Wno-uninitialized \
	$(NEW_GCC_WARNING_FLAGS) \
	-Wno-strict-aliasing
endif

ifeq ($(NO_GCC),1)
   WARNINGS :=
endif

ifeq ($(DEBUG), 1)
ifneq (,$(findstring msvc,$(platform)))
	ifeq ($(STATIC_LINKING),1)
	CFLAGS += -MTd
	CXXFLAGS += -MTd
else
	CFLAGS += -MDd
	CXXFLAGS += -MDd
endif

CFLAGS += -Od -Zi -DDEBUG -D_DEBUG
CXXFLAGS += -Od -Zi -DDEBUG -D_DEBUG
	else
	CFLAGS += -O0 -g -DDEBUG
	CXXFLAGS += -O0 -g -DDEBUG
endif
else
ifneq (,$(findstring msvc,$(platform)))
ifeq ($(STATIC_LINKING),1)
	CFLAGS += -MT
	CXXFLAGS += -MT
else
	CFLAGS += -MD
	CXXFLAGS += -MD
endif

CFLAGS += -O2 -DNDEBUG
CXXFLAGS += -O2 -DNDEBUG
else
	CFLAGS += -O2 -DNDEBUG
	CXXFLAGS += -O2 -DNDEBUG
endif
endif


OBJECTS := $(SOURCES_CXX:.cpp=.o) $(SOURCES_C:.c=.o)

all: $(TARGET)

LDFLAGS += $(fpic) $(SHARED)
FLAGS   += $(fpic) $(NEW_GCC_FLAGS)
FLAGS   += $(INCFLAGS) $(INCFLAGS_PLATFORM)

FLAGS += $(ENDIANNESS_DEFINES) -DSIZEOF_DOUBLE=8 $(WARNINGS) -DMEDNAFEN_VERSION=\"0.9.31\" -DPACKAGE=\"mednafen\" -DMEDNAFEN_VERSION_NUMERIC=931 -DPSS_STYLE=1 -DMPC_FIXED_POINT -DSTDC_HEADERS -D__STDC_LIMIT_MACROS -D__LIBRETRO__ -D_LOW_ACCURACY_ $(EXTRA_INCLUDES)

ifneq (,$(findstring msvc,$(platform)))
FLAGS += -DINLINE="_inline"
else
FLAGS += -DINLINE="inline"
endif

CXXFLAGS += $(FLAGS)
CFLAGS += $(FLAGS)

OBJOUT   = -o
LINKOUT  = -o 

ifneq (,$(findstring msvc,$(platform)))
	OBJOUT = -Fo
	LINKOUT = -out:
ifeq ($(STATIC_LINKING),1)
	LD ?= lib.exe
	STATIC_LINKING=0
else
	LD = link.exe
endif
else
	LD = $(CXX)
endif

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	$(LD) $(LINKOUT)$@ $^ $(LDFLAGS)
endif

%.o: %.cpp
	$(CXX) -c $(OBJOUT)$@ $< $(CPPFLAGS) $(CXXFLAGS)

%.o: %.c
	$(CC) -c $(OBJOUT)$@ $< $(CPPFLAGS) $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJECTS)

install:
	install -D -m 755 $(TARGET) $(DESTDIR)$(libdir)/$(LIBRETRO_DIR)/$(TARGET)

uninstall:
	rm $(DESTDIR)$(libdir)/$(LIBRETRO_DIR)/$(TARGET)

.PHONY: clean
