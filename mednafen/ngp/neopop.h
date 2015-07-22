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
#include "rom.h"

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

/* Core <--> System-Main Interface */

void reset(void);

/*!	Emulate a single instruction with correct TLCS900h:Z80 timing */
void emulate(void);

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
