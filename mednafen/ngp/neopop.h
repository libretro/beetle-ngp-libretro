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

#ifndef __NEOPOP__
#define __NEOPOP__

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "TLCS-900h/TLCS900h_disassemble.h"
#include "TLCS-900h/TLCS900h_interpret_dst.h"
#include "TLCS-900h/TLCS900h_interpret.h"
#include "TLCS-900h/TLCS900h_interpret_reg.h"
#include "TLCS-900h/TLCS900h_interpret_single.h"
#include "TLCS-900h/TLCS900h_interpret_src.h"
#include "TLCS-900h/TLCS900h_registers.h"

#include "system.h"

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

typedef enum
{
	COLOURMODE_GREYSCALE,
	COLOURMODE_COLOUR,
	COLOURMODE_AUTO
} COLOURMODE;

typedef struct 
{
	uint8_t* data;		/* Pointer to the ROM data */
	uint8_t *orig_data;	/* Original data (without flash writes 
                        during emulation; necessary for save states) */

	uint32_t length;    /* Length of the rom */

	uint8_t name[16];	/* NULL-terminated string, holding the Game name */
} RomInfo;

//RomHeader
typedef struct
{
#ifdef _WIN32
#pragma pack(push, 1)
#endif
	uint8_t		licence[28];		// 0x00 - 0x1B
	uint32_t	startPC;			// 0x1C - 0x1F
	uint16_t	catalog;			// 0x20 - 0x21
	uint8_t		subCatalog;			// 0x22
	uint8_t		mode;				// 0x23
	uint8_t		name[12];			// 0x24 - 0x2F

	uint32_t	reserved1;			// 0x30 - 0x33
	uint32_t	reserved2;			// 0x34 - 0x37
	uint32_t	reserved3;			// 0x38 - 0x3B
	uint32_t	reserved4;			// 0x3C - 0x3F
#ifdef _WIN32
#pragma pack(pop)
} RomHeader;
#else
} __attribute__((__packed__)) RomHeader;
#endif

/* Core <--> System-Main Interface */

void reset(void);

#ifdef __cplusplus
extern "C" {
#endif
   extern RomInfo ngpc_rom;

   extern RomHeader* rom_header;
#ifdef __cplusplus
}
#endif

/*!	Emulate a single instruction with correct TLCS900h:Z80 timing */
void emulate(void);

/*! Call this function when a rom has just been loaded, it will perform
	the system independent actions required. */
void rom_loaded(void);

/*!	Tidy up the rom and free the resources used. */
void rom_unload(void);

/*! Used to generate a critical message for the user. After the message
	has been displayed, the function should return. The message is not
	necessarily a fatal error. */
void system_message(char* vaMessage,...);

/*! Called at the start of the vertical blanking period, this function is
	designed to perform many of the critical hardware interface updates
	Here is a list of recommended actions to take:
	
	- The frame buffer should be copied to the screen.
	- The frame rate should be throttled to 59.95hz
	- The sound chips should be polled for the next chunk of data
	- Input should be polled and the current status written to "ram[0x6F82]" */
void system_VBL(void);


/* Core <--> System-Graphics Interface */

/* Physical screen dimensions */
#define SCREEN_WIDTH    160
#define SCREEN_HEIGHT   152

extern COLOURMODE system_colour;

/* Core <--> System-Sound Interface */

extern bool mute;

void int_redo_icache(void);

extern uint8_t NGPJoyLatch;
#endif
