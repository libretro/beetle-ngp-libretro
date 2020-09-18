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

	TLCS900h_disassemble_reg.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

22 JUL 2002 - neopop_uk
=======================================
- Fixed disassembly of Link, it's second operand is signed.

28 JUL 2002 - neopop_uk
=======================================
- Better disassembly of MUL/MULS/DIV/DIVS - operand size is shown.
  
//---------------------------------------------------------------------------
*/

#include <stdio.h>
#include "TLCS900h_disassemble.h"
#include "TLCS900h_registers.h"
#include "TLCS900h_interpret.h"

//=========================================================================

static void LDi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "LD %s,0x%02X", str_r, get8_dis());	break;
	case 1:	sprintf(instr, "LD %s,0x%04X", str_r, get16_dis());	break;
	case 2: sprintf(instr, "LD %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void PUSH(void)
{
	sprintf(instr, "PUSH %s", str_r);
}

static void POP(void)
{
	sprintf(instr, "POP %s", str_r);
}

static void CPL(void)
{
	sprintf(instr, "CPL %s", str_r);
}

static void NEG(void)
{
	sprintf(instr, "NEG %s", str_r);
}

static void MULi(void)
{
	get_rr_Name();

	switch(size)
	{
	case 0:	sprintf(instr, "MUL.b %s,0x%02X", str_r, get8_dis());	break;
	case 1: sprintf(instr, "MUL.w %s,0x%04X", str_r, get16_dis());	break;
	}
}

static void MULSi(void)
{
   get_rr_Name();

   switch(size)
   {
      case 0:
         sprintf(instr, "MULS.b %s,0x%02X", str_r, get8_dis());
         break;
      case 1:
         sprintf(instr, "MULS.w %s,0x%04X", str_r, get16_dis());
         break;
   }
}

static void DIVi(void)
{
   get_rr_Name();

   switch(size)
   {
      case 0:
         sprintf(instr, "DIV.b %s,0x%02X", str_r, get8_dis());
         break;
      case 1:
         sprintf(instr, "DIV.w %s,0x%04X", str_r, get16_dis());
         break;
   }
}

static void DIVSi(void)
{
	get_rr_Name();

	switch(size)
	{
	case 0:	sprintf(instr, "DIVS.b %s,0x%02X", str_r, get8_dis());	break;
	case 1: sprintf(instr, "DIVS.w %s,0x%04X", str_r, get16_dis());	break;
	}
}

static void LINK(void)
{
	sprintf(instr, "LINK %s, %d", str_r, (int16)get16_dis());
}

static void UNLK(void)
{
	sprintf(instr, "UNLK %s", str_r);
}

static void BS1F(void)
{
	sprintf(instr, "BS1F A,%s", str_r);
}

static void BS1B(void)
{
	sprintf(instr, "BS1B A,%s", str_r);
}

static void DAA(void)
{
	sprintf(instr, "DAA %s", str_r);
}

static void EXTZ(void)
{
	sprintf(instr, "EXTZ %s", str_r);
}

static void EXTS(void)
{
	sprintf(instr, "EXTS %s", str_r);
}

static void PAA(void)
{
	sprintf(instr, "PAA %s", str_r);
}

static void MIRR(void)
{
	sprintf(instr, "MIRR %s", str_r);
}

static void MULA(void)
{
	sprintf(instr, "MULA %s", str_r);
}

static void DJNZ(void)
{
	sprintf(instr, "DJNZ %s,0x%06X", str_r, (int8)get8_dis() + pc);
}

static void ANDCFi(void)
{
	sprintf(instr, "ANDCF %d,%s", get8_dis() & 0xF, str_r);
}

static void ORCFi(void)
{
	sprintf(instr, "ORCF %d,%s", get8_dis() & 0xF, str_r);
}

static void XORCFi(void)
{
	sprintf(instr, "XORCF %d,%s", get8_dis() & 0xF, str_r);
}

static void LDCFi(void)
{
	sprintf(instr, "LDCF %d,%s", get8_dis() & 0xF, str_r);
}

static void STCFi(void)
{
	sprintf(instr, "STCF %d,%s", get8_dis() & 0xF, str_r);
}

static void ANDCFA(void)
{
	sprintf(instr, "ANDCF A,%s", str_r);
}

static void ORCFA(void)
{
	sprintf(instr, "ORCF A,%s", str_r);
}

static void XORCFA(void)
{
	sprintf(instr, "XORCF A,%s", str_r);
}

static void LDCFA(void)
{
	sprintf(instr, "LDCF A,%s", str_r);
}

static void STCFA(void)
{
	sprintf(instr, "STCF A,%s", str_r);
}

static void LDCcrr(void)
{
	uint8 cr = get8_dis();
	sprintf(instr, "LDC %s,%s", crName[size][cr >> size], str_r);
}

static void LDCrcr(void)
{
	uint8 cr = get8_dis();
	sprintf(instr, "LDC %s,%s", str_r, crName[size][cr >> size]);
}

static void RES(void)
{
	sprintf(instr, "RES %d,%s", get8_dis() & 0xF, str_r);
}

static void SET(void)
{
	sprintf(instr, "SET %d,%s", get8_dis() & 0xF, str_r);
}

static void CHG(void)
{
	sprintf(instr, "CHG %d,%s", get8_dis() & 0xF, str_r);
}

static void BIT(void)
{
	sprintf(instr, "BIT %d,%s", get8_dis() & 0xF, str_r);
}

static void TSET(void)
{
	sprintf(instr, "TSET %d,%s", get8_dis() & 0xF, str_r);
}

static void MINC1(void)
{
	sprintf(instr, "MINC1 %d,%s", get16_dis()+1, str_r);
}

static void MINC2(void)
{
	sprintf(instr, "MINC2 %d,%s", get16_dis()+2, str_r);
}

static void MINC4(void)
{
	sprintf(instr, "MINC4 %d,%s", get16_dis()+4, str_r);
}

static void MDEC1(void)
{
	sprintf(instr, "MDEC1 %d,%s", get16_dis()+1, str_r);
}

static void MDEC2(void)
{
	sprintf(instr, "MDEC2 %d,%s", get16_dis()+2, str_r);
}

static void MDEC4(void)
{
	sprintf(instr, "MDEC4 %d,%s", get16_dis()+4, str_r);
}

static void MUL(void)
{
	get_RR_Name();
	switch(size)
	{
	case 0:	sprintf(instr, "MUL.b %s,%s", str_R, str_r);	break;
	case 1: sprintf(instr, "MUL.w %s,%s", str_R, str_r);	break;
	}
}

static void MULS(void)
{
	get_RR_Name();
	switch(size)
	{
	case 0:	sprintf(instr, "MULS.b %s,%s", str_R, str_r);	break;
	case 1: sprintf(instr, "MULS.w %s,%s", str_R, str_r);	break;
	}
}

static void DIV(void)
{
	get_RR_Name();
	switch(size)
	{
	case 0:	sprintf(instr, "DIV.b %s,%s", str_R, str_r);	break;
	case 1: sprintf(instr, "DIV.w %s,%s", str_R, str_r);	break;
	}
}

static void DIVS(void)
{
	get_RR_Name();
	switch(size)
	{
	case 0:	sprintf(instr, "DIVS.b %s,%s", str_R, str_r);	break;
	case 1: sprintf(instr, "DIVS.w %s,%s", str_R, str_r);	break;
	}
}

static void INC(void)
{
	uint8 val = (second & 7);
	if (val == 0)
		val = 8;

	sprintf(instr, "INC %d,%s", val, str_r);
}

static void DEC(void)
{
	uint8 val = (second & 7);
	if (val == 0)
		val = 8;

	sprintf(instr, "DEC %d,%s", val, str_r);
}

static void SCC(void)
{
	sprintf(instr, "SCC %s,%s", ccName[second & 0xF], str_r);
}

static void ADD(void)
{
	sprintf(instr, "ADD %s,%s", str_R, str_r);
}

static void LDRr(void)
{
	sprintf(instr, "LD %s,%s", str_R, str_r);
}

static void LDrR(void)
{
	sprintf(instr, "LD %s,%s", str_r, str_R);
}

static void ADC(void)
{
	sprintf(instr, "ADC %s,%s", str_R, str_r);
}

static void SUB(void)
{
	sprintf(instr, "SUB %s,%s", str_R, str_r);
}

static void SBC(void)
{
	sprintf(instr, "SBC %s,%s", str_R, str_r);
}

static void LDr3(void)
{
	sprintf(instr, "LD %s,%d", str_r, second & 7);
}

static void EX(void)
{
	sprintf(instr, "EX %s,%s", str_R, str_r);
}

static void AND(void)
{
	sprintf(instr, "AND %s,%s", str_R, str_r);
}

static void ADDi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "ADD %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "ADD %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "ADD %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void ADCi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "ADC %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "ADC %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "ADC %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void SUBi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "SUB %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "SUB %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "SUB %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void SBCi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "SBC %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "SBC %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "SBC %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void ANDi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "AND %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "AND %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "AND %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void XORi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "XOR %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "XOR %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "XOR %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void ORi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "OR %s,0x%02X", str_r, get8_dis());		break;
	case 1:	sprintf(instr, "OR %s,0x%04X", str_r, get16_dis());		break;
	case 2:	sprintf(instr, "OR %s,0x%08X", str_r, get32_dis());		break;
	}
}

static void CPi(void)
{
	switch(size)
	{
	case 0:	sprintf(instr, "CP %s,0x%02X", str_r, get8_dis());	break;
	case 1:	sprintf(instr, "CP %s,0x%04X", str_r, get16_dis());	break;
	case 2:	sprintf(instr, "CP %s,0x%08X", str_r, get32_dis());	break;
	}
}

static void XOR(void)
{
	sprintf(instr, "XOR %s,%s", str_R, str_r);
}

static void CPr3(void)
{
	sprintf(instr,"CP %s,%d", str_r, second&7);
}

static void OR(void)
{
	sprintf(instr, "OR %s,%s", str_R, str_r);
}

static void RLCi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "RLC %d,%s", val, str_r);
}

static void RRCi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "RRC %d,%s", val, str_r);
}

static void RLi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "RL %d,%s", val, str_r);
}

static void RRi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "RR %d,%s", val, str_r);
}

static void SLAi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "SLA %d,%s", val, str_r);
}

static void SRAi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "SRA %d,%s", val, str_r);
}

static void SLLi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "SLL %d,%s", val, str_r);
}

static void SRLi(void)
{
	uint8 val = get8_dis() & 0xF;
	if (val == 0) val = 16;
	sprintf(instr, "SRL %d,%s", val, str_r);
}

static void CP(void)
{
	sprintf(instr, "CP %s,%s", str_R, str_r);
}

static void RLCA(void)
{
	sprintf(instr, "RLC A,%s", str_r);
}

static void RRCA(void)
{
	sprintf(instr, "RRC A,%s", str_r);
}

static void RLA(void)
{
	sprintf(instr, "RL A,%s", str_r);
}

static void RRA(void)
{
	sprintf(instr, "RR A,%s", str_r);
}

static void SLAA(void)
{
	sprintf(instr, "SLA A,%s", str_r);
}

static void SRAA(void)
{
	sprintf(instr, "SRA A,%s", str_r);
}

static void SLLA(void)
{
	sprintf(instr, "SLL A,%s", str_r);
}

static void SRLA(void)
{
	sprintf(instr, "SRL A,%s", str_r);
}

//=========================================================================

//Secondary (REG) Instruction decode
static void (*decode[256])() = 
{
/*0*/	0,		0,		0,		LDi,	PUSH,	POP,	CPL,	NEG,
		MULi,	MULSi,	DIVi,	DIVSi,	LINK,	UNLK,	BS1F,	BS1B,
/*1*/	DAA,	0,		EXTZ,	EXTS,	PAA,	0,		MIRR,	0,
		0,		MULA,	0,		0,		DJNZ,	0,		0,		0,
/*2*/	ANDCFi,	ORCFi,	XORCFi,	LDCFi,	STCFi,	0,		0,		0,
		ANDCFA,	ORCFA,	XORCFA,	LDCFA,	STCFA,	0,		LDCcrr,	LDCrcr,
/*3*/	RES,	SET,	CHG,	BIT,	TSET,	0,		0,		0,
		MINC1,	MINC2,	MINC4,	0,		MDEC1,	MDEC2,	MDEC4,	0,
/*4*/	MUL,	MUL,	MUL,	MUL,	MUL,	MUL,	MUL,	MUL,
		MULS,	MULS,	MULS,	MULS,	MULS,	MULS,	MULS,	MULS,
/*5*/	DIV,	DIV,	DIV,	DIV,	DIV,	DIV,	DIV,	DIV,
		DIVS,	DIVS,	DIVS,	DIVS,	DIVS,	DIVS,	DIVS,	DIVS,
/*6*/	INC,	INC,	INC,	INC,	INC,	INC,	INC,	INC,
		DEC,	DEC,	DEC,	DEC,	DEC,	DEC,	DEC,	DEC,
/*7*/	SCC,	SCC,	SCC,	SCC,	SCC,	SCC,	SCC,	SCC,
		SCC,	SCC,	SCC,	SCC,	SCC,	SCC,	SCC,	SCC,
/*8*/	ADD,	ADD,	ADD,	ADD,	ADD,	ADD,	ADD,	ADD,
		LDRr,	LDRr,	LDRr,	LDRr,	LDRr,	LDRr,	LDRr,	LDRr,
/*9*/	ADC,	ADC,	ADC,	ADC,	ADC,	ADC,	ADC,	ADC,
		LDrR,	LDrR,	LDrR,	LDrR,	LDrR,	LDrR,	LDrR,	LDrR,
/*A*/	SUB,	SUB,	SUB,	SUB,	SUB,	SUB,	SUB,	SUB,
		LDr3,	LDr3,	LDr3,	LDr3,	LDr3,	LDr3,	LDr3,	LDr3,
/*B*/	SBC,	SBC,	SBC,	SBC,	SBC,	SBC,	SBC,	SBC,
		EX,		EX,		EX,		EX,		EX,		EX,		EX,		EX,
/*C*/	AND,	AND,	AND,	AND,	AND,	AND,	AND,	AND,
		ADDi,	ADCi,	SUBi,	SBCi,	ANDi,	XORi,	ORi,	CPi,
/*D*/	XOR,	XOR,	XOR,	XOR,	XOR,	XOR,	XOR,	XOR,
		CPr3,	CPr3,	CPr3,	CPr3,	CPr3,	CPr3,	CPr3,	CPr3,
/*E*/	OR,		OR,		OR,		OR,		OR,		OR,		OR,		OR,
		RLCi,	RRCi,	RLi,	RRi,	SLAi,	SRAi,	SLLi,	SRLi,
/*F*/	CP,		CP,		CP,		CP,		CP,		CP,		CP,		CP,
		RLCA,	RRCA,	RLA,	RRA,	SLAA,	SRAA,	SLLA,	SRLA
};

//=============================================================================

void TLCS900h_disassemble_reg(int opsize)
{
	second = get8_dis();	//Get the second opcode
	size = opsize;

	//Prepare 'Big R'
	sprintf(str_R, "%s", gprName[second & 7][size]);

	//Prepare 'little r'
	if (brCode)
		sprintf(str_r, "%s", extra);
	else
		sprintf(str_r, "%s", gprName[first & 7][opsize]); 

	if (decode[second])
		(*decode[second])();
	else
		sprintf(instr, "unknown reg instr. %02X", second);
}
