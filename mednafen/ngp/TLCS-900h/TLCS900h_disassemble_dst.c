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

	TLCS900h_disassemble_dst.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

//---------------------------------------------------------------------------
*/

//=========================================================================

#include <stdio.h>
#include "TLCS900h_disassemble.h"
#include "TLCS900h_registers.h"
#include "TLCS900h_interpret.h"

//=========================================================================

static void DST_LDBi(void)
{
	sprintf(instr, "LD (%s),0x%02X", extra, get8_dis());
}

static void DST_LDWi(void)
{
	sprintf(instr, "LD (%s),0x%04X", extra, get16_dis());
}

static void DST_POPB(void)
{
	sprintf(instr, "POP.b (%s)", extra);
}

static void DST_POPW(void)
{
	sprintf(instr, "POP.w (%s)", extra);
}

static void DST_LDBm16(void)
{
	sprintf(instr, "LD.b (%s),(0x%04X)", extra, get16_dis());
}

static void DST_LDWm16(void)
{
	sprintf(instr, "LD.w (%s),(0x%04X)", extra, get16_dis());
}

static void DST_LDAW(void)
{
	sprintf(instr, "LDA %s,%s", gprName[second & 7][1], extra);
}

static void DST_LDAL(void)
{
	sprintf(instr, "LDA %s,%s", gprName[second & 7][2], extra);
}

static void DST_ANDCFA(void)
{
	sprintf(instr, "ANDCF A,(%s)", extra);
}

static void DST_ORCFA(void)
{
	sprintf(instr, "ORCF A,(%s)", extra);
}

static void DST_XORCFA(void)
{
	sprintf(instr, "XORCF A,(%s)", extra);
}

static void DST_LDCFA(void)
{
	sprintf(instr, "LDCF A,(%s)", extra);
}

static void DST_STCFA(void)
{
	sprintf(instr, "STCF A,(%s)", extra);
}

static void DST_LDBR(void)
{
	sprintf(instr, "LD (%s),%s", extra, gprName[second&7][0]);
}

static void DST_LDWR(void)
{
	sprintf(instr, "LD (%s),%s", extra, gprName[second&7][1]);
}

static void DST_LDLR(void)
{
	sprintf(instr, "LD (%s),%s", extra, gprName[second&7][2]);
}

static void DST_ANDCF(void)
{
	sprintf(instr, "ANDCF %d,(%s)", second & 7, extra);
}

static void DST_ORCF(void)
{
	sprintf(instr, "ORCF %d,(%s)", second & 7, extra);
}

static void DST_XORCF(void)
{
	sprintf(instr, "XORCF %d,(%s)", second & 7, extra);
}

static void DST_LDCF(void)
{
	sprintf(instr, "LDCF %d,(%s)", second & 7, extra);
}

static void DST_STCF(void)
{
	sprintf(instr, "STCF %d,(%s)", second & 7, extra);
}

static void DST_TSET(void)
{
	sprintf(instr, "TSET %d,(%s)", second & 7, extra);
}

static void DST_RES(void)
{
	sprintf(instr, "RES %d,(%s)", second & 7, extra);
}

static void DST_SET(void)
{
	sprintf(instr, "SET %d,(%s)", second & 7, extra);
}

static void DST_CHG(void)
{
	sprintf(instr, "CHG %d,(%s)", second & 7, extra);
}

static void DST_BIT(void)
{
	sprintf(instr, "BIT %d,(%s)", second & 7, extra);
}

static void DST_JP(void)
{
	sprintf(instr, "JP %s,%s", ccName[second & 0xF], extra);
}

static void DST_CALL(void)
{
	sprintf(instr, "CALL %s,%s", ccName[second & 0xF], extra);
}

static void DST_RET(void)
{
	sprintf(instr, "RET %s", ccName[second & 0xF]);
}

//=========================================================================

//Secondary (DST) Instruction decode
static void (*decode[256])() = 
{
/*0*/	DST_LDBi,	0,		DST_LDWi,	0,		DST_POPB,	0,		DST_POPW,	0,
		0,		0,		0,		0,		0,		0,		0,		0,
/*1*/	0,		0,		0,		0,		DST_LDBm16,	0,		DST_LDWm16,	0,
		0,		0,		0,		0,		0,		0,		0,		0,
/*2*/	DST_LDAW,	DST_LDAW,	DST_LDAW,	DST_LDAW,	DST_LDAW,	DST_LDAW,	DST_LDAW,	DST_LDAW,
		DST_ANDCFA,	DST_ORCFA,	DST_XORCFA,	DST_LDCFA,	DST_STCFA,	0,		0,		0,
/*3*/	DST_LDAL,	DST_LDAL,	DST_LDAL,	DST_LDAL,	DST_LDAL,	DST_LDAL,	DST_LDAL,	DST_LDAL,
		0,		0,		0,		0,		0,		0,		0,		0,
/*4*/	DST_LDBR,	DST_LDBR,	DST_LDBR,	DST_LDBR,	DST_LDBR,	DST_LDBR,	DST_LDBR,	DST_LDBR,
		0,		0,		0,		0,		0,		0,		0,		0,
/*5*/	DST_LDWR,	DST_LDWR,	DST_LDWR,	DST_LDWR,	DST_LDWR,	DST_LDWR,	DST_LDWR,	DST_LDWR,
		0,		0,		0,		0,		0,		0,		0,		0,
/*6*/	DST_LDLR,	DST_LDLR,	DST_LDLR,	DST_LDLR,	DST_LDLR,	DST_LDLR,	DST_LDLR,	DST_LDLR,
		0,		0,		0,		0,		0,		0,		0,		0,
/*7*/	0,		0,		0,		0,		0,		0,		0,		0,
		0,		0,		0,		0,		0,		0,		0,		0,
/*8*/	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,	DST_ANDCF,
		DST_ORCF,	DST_ORCF,	DST_ORCF,	DST_ORCF,	DST_ORCF,	DST_ORCF,	DST_ORCF,	DST_ORCF,
/*9*/	DST_XORCF,	DST_XORCF,	DST_XORCF,	DST_XORCF,	DST_XORCF,	DST_XORCF,	DST_XORCF,	DST_XORCF,
		DST_LDCF,	DST_LDCF,	DST_LDCF,	DST_LDCF,	DST_LDCF,	DST_LDCF,	DST_LDCF,	DST_LDCF,
/*A*/	DST_STCF,	DST_STCF,	DST_STCF,	DST_STCF,	DST_STCF,	DST_STCF,	DST_STCF,	DST_STCF,	
		DST_TSET,	DST_TSET,	DST_TSET,	DST_TSET,	DST_TSET,	DST_TSET,	DST_TSET,	DST_TSET,
/*B*/	DST_RES,    DST_RES,    DST_RES,    DST_RES,    DST_RES,    DST_RES,    DST_RES,    DST_RES,
		DST_SET,    DST_SET,    DST_SET,    DST_SET,    DST_SET,    DST_SET,    DST_SET,    DST_SET,
/*C*/	DST_CHG,    DST_CHG,    DST_CHG,    DST_CHG,    DST_CHG,    DST_CHG,    DST_CHG,    DST_CHG,
		DST_BIT,    DST_BIT,    DST_BIT,    DST_BIT,    DST_BIT,    DST_BIT,    DST_BIT,    DST_BIT,
/*D*/	DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,
		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,		DST_JP,
/*E*/	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,
		DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,	DST_CALL,
/*F*/	DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,
		DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET,    DST_RET
};

//=============================================================================

void TLCS900h_disassemble_dst(void)
{
	second = get8_dis();	//Get the second opcode

	if (decode[second])
		(*decode[second])();
	else
		sprintf(instr, "unknown dst instr. %02X", second);
}
