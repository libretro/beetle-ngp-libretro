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

/*
//---------------------------------------------------------------------------
//=========================================================================

	TLCS900h_disassemble_extra.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

02 SEP 2002 - neopop_uk
=======================================
- Added the undocumented type 0x13 R32 address mode.

//---------------------------------------------------------------------------
*/

//=========================================================================

#include <stdio.h>
#include "TLCS900h_disassemble.h"
#include "TLCS900h_interpret.h"

#define RCN_fetch(_a, _b) (regCodeName[(_a)][(_b)] ? regCodeName[(_a)][(_b)] : "-UNK-")

//=========================================================================

char extra[256];	//Print the mnemonic for the addressing mode here.

//=========================================================================

static void EXTRA_ExXWA(void) { sprintf(extra, "XWA"); }

static void EXTRA_ExXBC(void)	{sprintf(extra, "XBC");}
static void EXTRA_ExXDE(void)	{sprintf(extra, "XDE");}
static void EXTRA_ExXHL(void)	{sprintf(extra, "XHL");}
static void EXTRA_ExXIX(void)	{sprintf(extra, "XIX");}
static void EXTRA_ExXIY(void)	{sprintf(extra, "XIY");}
static void EXTRA_ExXIZ(void)	{sprintf(extra, "XIZ");}
static void EXTRA_ExXSP(void)	{sprintf(extra, "XSP");}

static void EXTRA_ExXWAd(void)	{sprintf(extra, "XWA %+d", (int8)get8_dis());}
static void EXTRA_ExXBCd(void)	{sprintf(extra, "XBC %+d", (int8)get8_dis());}
static void EXTRA_ExXDEd(void)	{sprintf(extra, "XDE %+d", (int8)get8_dis());}
static void EXTRA_ExXHLd(void)	{sprintf(extra, "XHL %+d", (int8)get8_dis());}
static void EXTRA_ExXIXd(void)	{sprintf(extra, "XIX %+d", (int8)get8_dis());}
static void EXTRA_ExXIYd(void)	{sprintf(extra, "XIY %+d", (int8)get8_dis());}
static void EXTRA_ExXIZd(void)	{sprintf(extra, "XIZ %+d", (int8)get8_dis());}
static void EXTRA_ExXSPd(void)	{sprintf(extra, "XSP %+d", (int8)get8_dis());}

static void EXTRA_Ex8(void)		{sprintf(extra, "0x%02X", get8_dis());}
static void EXTRA_Ex16(void)		{sprintf(extra, "0x%04X", get16_dis());}
static void EXTRA_Ex24(void)		{sprintf(extra, "0x%06X", get24_dis());}

static void EXTRA_ExR32(void)
{
	uint8 data = get8_dis();

	if (data == 0x03)
	{
		uint8 rIndex, r32;
		r32 = get8_dis();	//r32, upper 6 bits
		rIndex = get8_dis();	//r8 / r16
		sprintf(extra, "%s + %s", 
			RCN_fetch(2, r32 >> 2), RCN_fetch(0, rIndex >> 0));
		return;
	}

	if (data == 0x07)
	{
		uint8 rIndex, r32;
		r32 = get8_dis();	//r32, upper 6 bits
		rIndex = get8_dis();	//r8 / r16
		sprintf(extra, "%s + %s", 
			RCN_fetch(2, r32 >> 2), RCN_fetch(1, rIndex >> 1));
		return;
	}

	//Undocumented mode.
	if (data == 0x13)
	{
		sprintf(extra, "pc %+d", (int16)get16_dis()); 
		return;
	}

	if ((data & 3) == 1)
		sprintf(extra, "%s %+d", RCN_fetch(2, data >> 2), (int16)get16_dis()); 
	else
		sprintf(extra, "%s", RCN_fetch(2, data >> 2)); 
}

static void EXTRA_ExDec(void)
{
	uint8 data = get8_dis();
	uint8 r32 = data & 0xFC;

	switch(data & 3)
	{
	case 0:	sprintf(extra, "1--%s", RCN_fetch(2, r32 >> 2));	break;
	case 1:	sprintf(extra, "2--%s", RCN_fetch(2, r32 >> 2));	break;
	case 2:	sprintf(extra, "4--%s", RCN_fetch(2, r32 >> 2));	break;
	}
}

static void EXTRA_ExInc(void)
{
   uint8 data = get8_dis();
   uint8 r32 = data & 0xFC;

   switch(data & 3)
   {
      case 0:
         sprintf(extra, "%s++1", RCN_fetch(2, r32 >> 2));
         break;
      case 1:
         sprintf(extra, "%s++2", RCN_fetch(2, r32 >> 2));
         break;
      case 2:
         sprintf(extra, "%s++4", RCN_fetch(2, r32 >> 2));
         break;
   }
}

static void EXTRA_ExRCB(void)
{
	uint8 data = get8_dis();
	sprintf(extra, "%s", RCN_fetch(0, data >> 0));
	brCode = true;
}

static void EXTRA_ExRCW(void)
{
	uint8 data = get8_dis();
	sprintf(extra, "%s", RCN_fetch(1, data >> 1));
	brCode = true;
}

static void EXTRA_ExRCL(void)
{
	uint8 data = get8_dis();
	sprintf(extra, "%s", RCN_fetch(2, data >> 2));
	brCode = true;
}

//=========================================================================

//Address Mode & Register Code
static void (*decodeExtra[256])() = 
{
/*0*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*1*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*2*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*3*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*4*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*5*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*6*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*7*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*8*/	EXTRA_ExXWA,	EXTRA_ExXBC,	EXTRA_ExXDE,	EXTRA_ExXHL,	EXTRA_ExXIX,	EXTRA_ExXIY,	EXTRA_ExXIZ,	EXTRA_ExXSP,
		EXTRA_ExXWAd,	EXTRA_ExXBCd,	EXTRA_ExXDEd,	EXTRA_ExXHLd,	EXTRA_ExXIXd,	EXTRA_ExXIYd,	EXTRA_ExXIZd,	EXTRA_ExXSPd,
/*9*/	EXTRA_ExXWA,	EXTRA_ExXBC,	EXTRA_ExXDE,	EXTRA_ExXHL,	EXTRA_ExXIX,	EXTRA_ExXIY,	EXTRA_ExXIZ,	EXTRA_ExXSP,
		EXTRA_ExXWAd,	EXTRA_ExXBCd,	EXTRA_ExXDEd,	EXTRA_ExXHLd,	EXTRA_ExXIXd,	EXTRA_ExXIYd,	EXTRA_ExXIZd,	EXTRA_ExXSPd,
/*A*/	EXTRA_ExXWA,	EXTRA_ExXBC,	EXTRA_ExXDE,	EXTRA_ExXHL,	EXTRA_ExXIX,	EXTRA_ExXIY,	EXTRA_ExXIZ,	EXTRA_ExXSP,
		EXTRA_ExXWAd,	EXTRA_ExXBCd,	EXTRA_ExXDEd,	EXTRA_ExXHLd,	EXTRA_ExXIXd,	EXTRA_ExXIYd,	EXTRA_ExXIZd,	EXTRA_ExXSPd,
/*B*/	EXTRA_ExXWA,	EXTRA_ExXBC,	EXTRA_ExXDE,	EXTRA_ExXHL,	EXTRA_ExXIX,	EXTRA_ExXIY,	EXTRA_ExXIZ,	EXTRA_ExXSP,
		EXTRA_ExXWAd,	EXTRA_ExXBCd,	EXTRA_ExXDEd,	EXTRA_ExXHLd,	EXTRA_ExXIXd,	EXTRA_ExXIYd,	EXTRA_ExXIZd,	EXTRA_ExXSPd,
/*C*/	EXTRA_Ex8,	EXTRA_Ex16,	EXTRA_Ex24,	EXTRA_ExR32,	EXTRA_ExDec,	EXTRA_ExInc,	0,		EXTRA_ExRCB,
		0,		0,		0,		0,		0,		0,		0,		0,
/*D*/	EXTRA_Ex8,	EXTRA_Ex16,	EXTRA_Ex24,	EXTRA_ExR32,	EXTRA_ExDec,	EXTRA_ExInc,	0,		EXTRA_ExRCW,
		0,		0,		0,		0,		0,		0,		0,		0,
/*E*/	EXTRA_Ex8,	EXTRA_Ex16,	EXTRA_Ex24,	EXTRA_ExR32,	EXTRA_ExDec,	EXTRA_ExInc,	0,		EXTRA_ExRCL,
		0,		0,		0,		0,		0,		0,		0,		0,
/*F*/	EXTRA_Ex8,	EXTRA_Ex16,	EXTRA_Ex24,	EXTRA_ExR32,	EXTRA_ExDec,	EXTRA_ExInc,	0,		0,
		0,		0,		0,		0,		0,		0,		0,		0
};

void TLCS900h_disassemble_extra(void)
{
	//Is any extra data used by this instruction?
	if (decodeExtra[first])
		(*decodeExtra[first])();
}
