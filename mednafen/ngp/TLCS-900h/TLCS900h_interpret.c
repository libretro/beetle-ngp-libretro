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

	TLCS900h_interpret.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

26 JUL 2002 - neopop_uk
=======================================
- Fixed a nasty bug that only affects [src]"EX (mem), XWA", 
	it was executing "EX F,F'" instead - Very bad! 

28 JUL 2002 - neopop_uk
=======================================
- Added generic DIV and DIVS functions

30 AUG 2002 - neopop_uk
=======================================
- Fixed detection of R32+d16 addressing mode.

02 SEP 2002 - neopop_uk
=======================================
- Added the undocumented type 0x13 R32 address mode.

09 SEP 2002 - neopop_uk
=======================================
- Extra cycles for addressing modes.

//---------------------------------------------------------------------------
*/

#include "TLCS900h_registers.h"
#include "../mem.h"
#include "../bios.h"
#include "TLCS900h_interpret.h"
#include "TLCS900h_interpret_single.h"
#include "TLCS900h_interpret_src.h"
#include "TLCS900h_interpret_dst.h"
#include "TLCS900h_interpret_reg.h"

//=========================================================================

uint32	mem;		//Result of addressing mode
int		size;		//operand size, 0 = Byte, 1 = Word, 2 = Long

uint8		first;		//The first byte
uint8		R;			//big R
uint8		second;		//The second opcode

bool	brCode;		//Register code used?
uint8		rCode;		//The code

int32		cycles;		//How many state changes?
int32		cycles_extra;	//How many extra state changes?

//=========================================================================

uint16 fetch16(void)
{
	uint16 a = loadW(pc);
	pc += 2;
	return a;
}

uint32 fetch24(void)
{
	uint32 b, a = loadW(pc);
	pc += 2;
	b = loadB(pc++);
	return (b << 16) | a;
}

uint32 fetch32(void)
{
	uint32 a = loadL(pc);
	pc += 4;
	return a;
}

//=============================================================================

void parityB(uint8 value)
{
	uint8 count = 0, i;

	for (i = 0; i < 8; i++)
	{
		if (value & 1) count++;
		value >>= 1;
	}

	// if (count & 1) == false, means even, thus SET
	SETFLAG_V((count & 1) == 0);
}

void parityW(uint16 value)
{
	uint8 count = 0, i;

	for (i = 0; i < 16; i++)
	{
		if (value & 1) count++;
		value >>= 1;
	}

	// if (count & 1) == false, means even, thus SET
	SETFLAG_V((count & 1) == 0);
}

//=========================================================================

void push8(uint8 data)
{
   REGXSP -= 1;
   storeB(REGXSP, data);
}

void push16(uint16 data)
{
   REGXSP -= 2;
   storeW(REGXSP, data);
}

void push32(uint32 data)
{
   REGXSP -= 4;
   storeL(REGXSP, data);
}

uint8 pop8(void)
{
   uint8 temp = loadB(REGXSP);
   REGXSP += 1;
   return temp;
}

uint16 pop16(void)
{
   uint16 temp = loadW(REGXSP);
   REGXSP += 2;
   return temp;
}

uint32 pop32(void)
{
   uint32 temp = loadL(REGXSP);
   REGXSP += 4;
   return temp;
}

//=============================================================================

uint16 generic_DIV_B(uint16 val, uint8 div)
{
	if (div == 0)
	{ 
		SETFLAG_V1
		return (val << 8) | ((val >> 8) ^ 0xFF);
	}
	else
	{
		uint16 quo = val / (uint16)div;
		uint16 rem = val % (uint16)div;
		if (quo > 0xFF) SETFLAG_V1 else SETFLAG_V0
		return (quo & 0xFF) | ((rem & 0xFF) << 8);
	}
}

uint32 generic_DIV_W(uint32 val, uint16 div)
{
	if (div == 0)
	{ 
		SETFLAG_V1
		return (val << 16) | ((val >> 16) ^ 0xFFFF);
	}
	else
	{
		uint32 quo = val / (uint32)div;
		uint32 rem = val % (uint32)div;
		if (quo > 0xFFFF) SETFLAG_V1 else SETFLAG_V0
		return (quo & 0xFFFF) | ((rem & 0xFFFF) << 16);
	}
}

//=============================================================================

uint16 generic_DIVS_B(int16 val, int8 div)
{
	if (div == 0)
	{
		SETFLAG_V1
		return (val << 8) | ((val >> 8) ^ 0xFF);
	}
	else
	{
		int16 quo = val / (int16)div;
		int16 rem = val % (int16)div;
		if (quo > 0xFF) SETFLAG_V1 else SETFLAG_V0
		return (quo & 0xFF) | ((rem & 0xFF) << 8);
	}
}

uint32 generic_DIVS_W(int32 val, int16 div)
{
	if (div == 0)
	{
		SETFLAG_V1
		return (val << 16) | ((val >> 16) ^ 0xFFFF);
	}
	else
	{
		int32 quo = val / (int32)div;
		int32 rem = val % (int32)div;
		if (quo > 0xFFFF) SETFLAG_V1 else SETFLAG_V0
		return (quo & 0xFFFF) | ((rem & 0xFFFF) << 16);
	}
}

//=============================================================================

uint8 generic_ADD_B(uint8 dst, uint8 src)
{
	uint8 half = (dst & 0xF) + (src & 0xF);
	uint32 resultC = (uint32)dst + (uint32)src;
	uint8 result = (uint8)(resultC & 0xFF);

	SETFLAG_S(result & 0x80);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int8)dst >= 0) && ((int8)src >= 0) && ((int8)result < 0)) ||
		(((int8)dst < 0)  && ((int8)src < 0) && ((int8)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFF);

	return result;
}

uint16 generic_ADD_W(uint16 dst, uint16 src)
{
	uint16 half = (dst & 0xF) + (src & 0xF);
	uint32 resultC = (uint32)dst + (uint32)src;
	uint16 result = (uint16)(resultC & 0xFFFF);

	SETFLAG_S(result & 0x8000);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int16)dst >= 0) && ((int16)src >= 0) && ((int16)result < 0)) ||
		(((int16)dst < 0)  && ((int16)src < 0) && ((int16)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFFFF);

	return result;
}

uint32 generic_ADD_L(uint32 dst, uint32 src)
{
	uint64 resultC = (uint64)dst + (uint64)src;
	uint32 result = (uint32)(resultC & 0xFFFFFFFF);

	SETFLAG_S(result & 0x80000000);
	SETFLAG_Z(result == 0);

	if ((((int32)dst >= 0) && ((int32)src >= 0) && ((int32)result < 0)) || 
		(((int32)dst < 0)  && ((int32)src < 0) && ((int32)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}
	
	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFFFFFFFF);

	return result;
}

//=============================================================================

uint8 generic_ADC_B(uint8 dst, uint8 src)
{
	uint8 half = (dst & 0xF) + (src & 0xF) + FLAG_C;
	uint32 resultC = (uint32)dst + (uint32)src + (uint32)FLAG_C;
	uint8 result = (uint8)(resultC & 0xFF);

	SETFLAG_S(result & 0x80);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int8)dst >= 0) && ((int8)src >= 0) && ((int8)result < 0)) || 
		(((int8)dst < 0)  && ((int8)src < 0) && ((int8)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFF);

	return result;
}

uint16 generic_ADC_W(uint16 dst, uint16 src)
{
	uint16 half = (dst & 0xF) + (src & 0xF) + FLAG_C;
	uint32 resultC = (uint32)dst + (uint32)src + (uint32)FLAG_C;
	uint16 result = (uint16)(resultC & 0xFFFF);

	SETFLAG_S(result & 0x8000);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int16)dst >= 0) && ((int16)src >= 0) && ((int16)result < 0)) || 
		(((int16)dst < 0)  && ((int16)src < 0) && ((int16)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFFFF);

	return result;
}

uint32 generic_ADC_L(uint32 dst, uint32 src)
{
	uint64 resultC = (uint64)dst + (uint64)src + (uint64)FLAG_C;
	uint32 result = (uint32)(resultC & 0xFFFFFFFF);

	SETFLAG_S(result & 0x80000000);
	SETFLAG_Z(result == 0);

	if ((((int32)dst >= 0) && ((int32)src >= 0) && ((int32)result < 0)) || 
		(((int32)dst < 0)  && ((int32)src < 0) && ((int32)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}
	
	SETFLAG_N0;
	SETFLAG_C(resultC > 0xFFFFFFFF);

	return result;
}

//=============================================================================

uint8 generic_SUB_B(uint8 dst, uint8 src)
{
	uint8 half = (dst & 0xF) - (src & 0xF);
	uint32 resultC = (uint32)dst - (uint32)src;
	uint8 result = (uint8)(resultC & 0xFF);

	SETFLAG_S(result & 0x80);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int8)dst >= 0) && ((int8)src < 0) && ((int8)result < 0)) ||
		(((int8)dst < 0) && ((int8)src >= 0) && ((int8)result >= 0)))
	{
      SETFLAG_V1
   }
   else
   {
      SETFLAG_V0
   }

	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFF);

	return result;
}

uint16 generic_SUB_W(uint16 dst, uint16 src)
{
	uint16 half = (dst & 0xF) - (src & 0xF);
	uint32 resultC = (uint32)dst - (uint32)src;
	uint16 result = (uint16)(resultC & 0xFFFF);

	SETFLAG_S(result & 0x8000);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int16)dst >= 0) && ((int16)src < 0) && ((int16)result < 0)) ||
		(((int16)dst < 0) && ((int16)src >= 0) && ((int16)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFFFF);

	return result;
}

uint32 generic_SUB_L(uint32 dst, uint32 src)
{
	uint64 resultC = (uint64)dst - (uint64)src;
	uint32 result = (uint32)(resultC & 0xFFFFFFFF);

	SETFLAG_S(result & 0x80000000);
	SETFLAG_Z(result == 0);

	if ((((int32)dst >= 0) && ((int32)src < 0) && ((int32)result < 0)) ||
		(((int32)dst < 0) && ((int32)src >= 0) && ((int32)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}
	
	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFFFFFFFF);

	return result;
}

//=============================================================================

uint8 generic_SBC_B(uint8 dst, uint8 src)
{
	uint8 half = (dst & 0xF) - (src & 0xF) - FLAG_C;
	uint32 resultC = (uint32)dst - (uint32)src - (uint32)FLAG_C;
	uint8 result = (uint8)(resultC & 0xFF);

	SETFLAG_S(result & 0x80);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int8)dst >= 0) && ((int8)src < 0) && ((int8)result < 0)) ||
		(((int8)dst < 0) && ((int8)src >= 0) && ((int8)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFF);

	return result;
}

uint16 generic_SBC_W(uint16 dst, uint16 src)
{
	uint16 half = (dst & 0xF) - (src & 0xF) - FLAG_C;
	uint32 resultC = (uint32)dst - (uint32)src - (uint32)FLAG_C;
	uint16 result = (uint16)(resultC & 0xFFFF);

	SETFLAG_S(result & 0x8000);
	SETFLAG_Z(result == 0);
	SETFLAG_H(half > 0xF);

	if ((((int16)dst >= 0) && ((int16)src < 0) && ((int16)result < 0)) ||
		(((int16)dst < 0) && ((int16)src >= 0) && ((int16)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}

	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFFFF);

	return result;
}

uint32 generic_SBC_L(uint32 dst, uint32 src)
{
	uint64 resultC = (uint64)dst - (uint64)src - (uint64)FLAG_C;
	uint32 result = (uint32)(resultC & 0xFFFFFFFF);

	SETFLAG_S(result & 0x80000000);
	SETFLAG_Z(result == 0);

	if ((((int32)dst >= 0) && ((int32)src < 0) && ((int32)result < 0)) ||
		(((int32)dst < 0) && ((int32)src >= 0) && ((int32)result >= 0)))
	{SETFLAG_V1} else {SETFLAG_V0}
	
	SETFLAG_N1;
	SETFLAG_C(resultC > 0xFFFFFFFF);

	return result;
}

//=============================================================================

bool conditionCode(int cc)
{
   switch(cc)
   {
      case 0:
	 break;		//(F)
      case 1:
         if (FLAG_S ^ FLAG_V)
            return 1;
	 break;		//(LT)
      case 2:
         if (FLAG_Z | (FLAG_S ^ FLAG_V))
            return 1;
	 break;		//(LE)
      case 3:
         if (FLAG_C | FLAG_Z)
            return 1;
	 break;		//(ULE)
      case 4:
         if (FLAG_V)
            return 1;
	 break;		//(OV)
      case 5:
         if (FLAG_S)
            return 1;
	 break;		//(MI)
      case 6:
         if (FLAG_Z)
            return 1;
	 break;		//(Z)
      case 7:
         if (FLAG_C)
            return 1;
         break;		//(C)
      case 8:
         return 1;	//always True
      case 9:
         if (FLAG_S ^ FLAG_V)
            return 0;
         return 1;	//(GE)
      case 10:
         if (FLAG_Z | (FLAG_S ^ FLAG_V))
            return 0;
         return 1;	//(GT)
      case 11:
         if (FLAG_C | FLAG_Z)
            return 0;
         return 1;	//(UGT)
      case 12:
         if (FLAG_V)
            return 0;
         return 1;	//(NOV)
      case 13:
         if (FLAG_S)
            return 0;
         return 1;	//(PL)
      case 14:
         if (FLAG_Z)
            return 0;
         return 1;	//(NZ)
      case 15:
         if (FLAG_C)
            return 0;
         return 1;	//(NC)
   }

   return 0;
}

//=============================================================================

uint8 get_rr_Target(void)
{
	uint8 target = 0x80;

	if (size == 0 && first == 0xC7)
		return rCode;

	//Create a regCode
	switch(first & 7)
	{
	case 0: if (size == 1)	target = 0xE0;	break;
	case 1:	
		if (size == 0)	target = 0xE0;
		if (size == 1)	target = 0xE4;
		break;
	case 2: if (size == 1)	target = 0xE8;	break;
	case 3:
		if (size == 0)	target = 0xE4;
		if (size == 1)	target = 0xEC;
		break;
	case 4: if (size == 1)	target = 0xF0;	break;
	case 5:	
		if (size == 0)	target = 0xE8;
		if (size == 1)	target = 0xF4;
		break;
	case 6: if (size == 1)	target = 0xF8;	break;
	case 7:
		if (size == 0)	target = 0xEC;
		if (size == 1)	target = 0xFC;
		break;
	}

	return target;
}

uint8 get_RR_Target(void)
{
   //Create a regCode
   switch(second & 7)
   {
      case 0:
         if (size == 1)
            return 0xE0;
         break;
      case 1:	
         if (size == 0)
            return 0xE0;
         else if (size == 1)
            return 0xE4;
         break;
      case 2:
         if (size == 1)
            return 0xE8;
         break;
      case 3:
         if (size == 0)
            return 0xE4;
         else if (size == 1)
            return 0xEC;
         break;
      case 4:
         if (size == 1)
            return 0xF0;
         break;
      case 5:	
         if (size == 0)
            return 0xE8;
         else if (size == 1)
            return 0xF4;
         break;
      case 6:
         if (size == 1)
            return 0xF8;
         break;
      case 7:
         if (size == 0)
            return 0xEC;
         else if (size == 1)
            return 0xFC;
         break;
   }

   return 0x80;
}

//=========================================================================

static void ExXWA(void)		{mem = regL(0);}
static void ExXBC(void)		{mem = regL(1);}
static void ExXDE(void)		{mem = regL(2);}
static void ExXHL(void)		{mem = regL(3);}
static void ExXIX(void)		{mem = regL(4);}
static void ExXIY(void)		{mem = regL(5);}
static void ExXIZ(void)		{mem = regL(6);}
static void ExXSP(void)		{mem = regL(7);}

static void ExXWAd(void)	{mem = regL(0) + (int8)FETCH8; cycles_extra = 2;}
static void ExXBCd(void)	{mem = regL(1) + (int8)FETCH8; cycles_extra = 2;}
static void ExXDEd(void)	{mem = regL(2) + (int8)FETCH8; cycles_extra = 2;}
static void ExXHLd(void)	{mem = regL(3) + (int8)FETCH8; cycles_extra = 2;}
static void ExXIXd(void)	{mem = regL(4) + (int8)FETCH8; cycles_extra = 2;}
static void ExXIYd(void)	{mem = regL(5) + (int8)FETCH8; cycles_extra = 2;}
static void ExXIZd(void)	{mem = regL(6) + (int8)FETCH8; cycles_extra = 2;}
static void ExXSPd(void)	{mem = regL(7) + (int8)FETCH8; cycles_extra = 2;}

static void Ex8(void)
{
   mem = FETCH8;
   cycles_extra = 2;
}

static void Ex16(void)
{
   mem = fetch16();
   cycles_extra = 2;
}

static void Ex24(void)
{
   mem = fetch24();
   cycles_extra = 3;
}

static void ExR32(void)
{
	uint8 data = FETCH8;

	if (data == 0x03)
	{
		uint8 rIndex, r32;
		r32 = FETCH8;		//r32
		rIndex = FETCH8;	//r8
		mem = rCodeL(r32) + (int8)rCodeB(rIndex);
		cycles_extra = 8;
		return;
	}
	else if (data == 0x07)
	{
		uint8 rIndex, r32;
		r32 = FETCH8;		//r32
		rIndex = FETCH8;	//r16
		mem = rCodeL(r32) + (int16)rCodeW(rIndex);
		cycles_extra = 8;
		return;
	}
	//Undocumented mode!
	else if (data == 0x13)
	{
		int16 disp = fetch16();
		mem = pc + disp;
		cycles_extra = 8;	//Unconfirmed... doesn't make much difference
		return;
	}

	cycles_extra = 5;

	mem = rCodeL(data);
	if ((data & 3) == 1)
		mem += (int16)fetch16();
}

static void ExDec(void)
{
	uint8 data = FETCH8;
	uint8 r32 = data & 0xFC;

	cycles_extra = 3;

	switch(data & 3)
	{
	case 0:	rCodeL(r32) -= 1;	mem = rCodeL(r32);	break;
	case 1:	rCodeL(r32) -= 2;	mem = rCodeL(r32);	break;
	case 2:	rCodeL(r32) -= 4;	mem = rCodeL(r32);	break;
	}
}

static void ExInc(void)
{
   uint8 data = FETCH8;
   uint8 r32 = data & 0xFC;

   cycles_extra = 3;

   switch(data & 3)
   {
      case 0:
         mem = rCodeL(r32);
         rCodeL(r32) += 1;
         break;
      case 1:
         mem = rCodeL(r32);
         rCodeL(r32) += 2;
         break;
      case 2:
         mem = rCodeL(r32);
         rCodeL(r32) += 4;
         break;
   }
}

static void ExRC(void)
{
	brCode = true;
	rCode = FETCH8;
	cycles_extra = 1;
}

//=========================================================================

//Address Mode & Register Code
static void (*decodeExtra[256])(void) = 
{
/*0*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*1*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*2*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*3*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*4*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*5*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*6*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*7*/	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
/*8*/	ExXWA,	ExXBC,	ExXDE,	ExXHL,	ExXIX,	ExXIY,	ExXIZ,	ExXSP,
		ExXWAd,	ExXBCd,	ExXDEd,	ExXHLd,	ExXIXd,	ExXIYd,	ExXIZd,	ExXSPd,
/*9*/	ExXWA,	ExXBC,	ExXDE,	ExXHL,	ExXIX,	ExXIY,	ExXIZ,	ExXSP,
		ExXWAd,	ExXBCd,	ExXDEd,	ExXHLd,	ExXIXd,	ExXIYd,	ExXIZd,	ExXSPd,
/*A*/	ExXWA,	ExXBC,	ExXDE,	ExXHL,	ExXIX,	ExXIY,	ExXIZ,	ExXSP,
		ExXWAd,	ExXBCd,	ExXDEd,	ExXHLd,	ExXIXd,	ExXIYd,	ExXIZd,	ExXSPd,
/*B*/	ExXWA,	ExXBC,	ExXDE,	ExXHL,	ExXIX,	ExXIY,	ExXIZ,	ExXSP,
		ExXWAd,	ExXBCd,	ExXDEd,	ExXHLd,	ExXIXd,	ExXIYd,	ExXIZd,	ExXSPd,
/*C*/	Ex8,	Ex16,	Ex24,	ExR32,	ExDec,	ExInc,	0,		ExRC,
		0,		0,		0,		0,		0,		0,		0,		0,
/*D*/	Ex8,	Ex16,	Ex24,	ExR32,	ExDec,	ExInc,	0,		ExRC,
		0,		0,		0,		0,		0,		0,		0,		0,
/*E*/	Ex8,	Ex16,	Ex24,	ExR32,	ExDec,	ExInc,	0,		ExRC,
		0,		0,		0,		0,		0,		0,		0,		0,
/*F*/	Ex8,	Ex16,	Ex24,	ExR32,	ExDec,	ExInc,	0,		0,
		0,		0,		0,		0,		0,		0,		0,		0
};

//=========================================================================

static void e(void)  { }
static void es(void) { }
static void ed(void) { }
static void er(void) { }

//=========================================================================

//Secondary (SRC) Instruction decode
static void (*srcDecode[256])(void) = 
{
/*0*/	es,			es,			es,			es,			srcPUSH,	es,			srcRLD,		srcRRD,
		es,			es,			es,			es,			es,			es,			es,			es,
/*1*/	srcLDI,		srcLDIR,	srcLDD,		srcLDDR,	srcCPI,		srcCPIR,	srcCPD,		srcCPDR,
		es,			srcLD16m,	es,			es,			es,			es,			es,			es,
/*2*/	srcLD,		srcLD,		srcLD,		srcLD,		srcLD,		srcLD,		srcLD,		srcLD,
		es,			es,			es,			es,			es,			es,			es,			es,
/*3*/	srcEX,		srcEX,		srcEX,		srcEX,		srcEX,		srcEX,		srcEX,		srcEX,
		srcADDi,	srcADCi,	srcSUBi,	srcSBCi,	srcANDi,	srcXORi,	srcORi,		srcCPi,
/*4*/	srcMUL,		srcMUL,		srcMUL,		srcMUL,		srcMUL,		srcMUL,		srcMUL,		srcMUL,
		srcMULS,	srcMULS,	srcMULS,	srcMULS,	srcMULS,	srcMULS,	srcMULS,	srcMULS,
/*5*/	srcDIV,		srcDIV,		srcDIV,		srcDIV,		srcDIV,		srcDIV,		srcDIV,		srcDIV,
		srcDIVS,	srcDIVS,	srcDIVS,	srcDIVS,	srcDIVS,	srcDIVS,	srcDIVS,	srcDIVS,
/*6*/	srcINC,		srcINC,		srcINC,		srcINC,		srcINC,		srcINC,		srcINC,		srcINC,
		srcDEC,		srcDEC,		srcDEC,		srcDEC,		srcDEC,		srcDEC,		srcDEC,		srcDEC,
/*7*/	es,			es,			es,			es,			es,			es,			es,			es,
		srcRLC,		srcRRC,		srcRL,		srcRR,		srcSLA,		srcSRA,		srcSLL,		srcSRL,
/*8*/	srcADDRm,	srcADDRm,	srcADDRm,	srcADDRm,	srcADDRm,	srcADDRm,	srcADDRm,	srcADDRm,
		srcADDmR,	srcADDmR,	srcADDmR,	srcADDmR,	srcADDmR,	srcADDmR,	srcADDmR,	srcADDmR,
/*9*/	srcADCRm,	srcADCRm,	srcADCRm,	srcADCRm,	srcADCRm,	srcADCRm,	srcADCRm,	srcADCRm,
		srcADCmR,	srcADCmR,	srcADCmR,	srcADCmR,	srcADCmR,	srcADCmR,	srcADCmR,	srcADCmR,
/*A*/	srcSUBRm,	srcSUBRm,	srcSUBRm,	srcSUBRm,	srcSUBRm,	srcSUBRm,	srcSUBRm,	srcSUBRm,
		srcSUBmR,	srcSUBmR,	srcSUBmR,	srcSUBmR,	srcSUBmR,	srcSUBmR,	srcSUBmR,	srcSUBmR,
/*B*/	srcSBCRm,	srcSBCRm,	srcSBCRm,	srcSBCRm,	srcSBCRm,	srcSBCRm,	srcSBCRm,	srcSBCRm,
		srcSBCmR,	srcSBCmR,	srcSBCmR,	srcSBCmR,	srcSBCmR,	srcSBCmR,	srcSBCmR,	srcSBCmR,
/*C*/	srcANDRm,	srcANDRm,	srcANDRm,	srcANDRm,	srcANDRm,	srcANDRm,	srcANDRm,	srcANDRm,
		srcANDmR,	srcANDmR,	srcANDmR,	srcANDmR,	srcANDmR,	srcANDmR,	srcANDmR,	srcANDmR,
/*D*/	srcXORRm,	srcXORRm,	srcXORRm,	srcXORRm,	srcXORRm,	srcXORRm,	srcXORRm,	srcXORRm,
		srcXORmR,	srcXORmR,	srcXORmR,	srcXORmR,	srcXORmR,	srcXORmR,	srcXORmR,	srcXORmR,
/*E*/	srcORRm,	srcORRm,	srcORRm,	srcORRm,	srcORRm,	srcORRm,	srcORRm,	srcORRm,
		srcORmR,	srcORmR,	srcORmR,	srcORmR,	srcORmR,	srcORmR,	srcORmR,	srcORmR,
/*F*/	srcCPRm,	srcCPRm,	srcCPRm,	srcCPRm,	srcCPRm,	srcCPRm,	srcCPRm,	srcCPRm,
		srcCPmR,	srcCPmR,	srcCPmR,	srcCPmR,	srcCPmR,	srcCPmR,	srcCPmR,	srcCPmR
};

//Secondary (DST) Instruction decode
static void (*dstDecode[256])() = 
{
/*0*/	DST_dstLDBi,	ed,			DST_dstLDWi,	ed,			DST_dstPOPB,	ed,			DST_dstPOPW,	ed,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*1*/	ed,			ed,			ed,			ed,			DST_dstLDBm16,	ed,			DST_dstLDWm16,	ed,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*2*/	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,	DST_dstLDAW,
		DST_dstANDCFA,	DST_dstORCFA,	DST_dstXORCFA,	DST_dstLDCFA,	DST_dstSTCFA,	ed,			ed,			ed,
/*3*/	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,	DST_dstLDAL,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*4*/	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,	DST_dstLDBR,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*5*/	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,	DST_dstLDWR,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*6*/	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,	DST_dstLDLR,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*7*/	ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
		ed,			ed,			ed,			ed,			ed,			ed,			ed,			ed,
/*8*/	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,	DST_dstANDCF,
		DST_dstORCF,	DST_dstORCF,	DST_dstORCF,	DST_dstORCF,	DST_dstORCF,	DST_dstORCF,	DST_dstORCF,	DST_dstORCF,
/*9*/	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,	DST_dstXORCF,
		DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,	DST_dstLDCF,
/*A*/	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	DST_dstSTCF,	
		DST_dstTSET,	DST_dstTSET,	DST_dstTSET,	DST_dstTSET,	DST_dstTSET,	DST_dstTSET,	DST_dstTSET,	DST_dstTSET,
/*B*/	DST_dstRES,		DST_dstRES,		DST_dstRES,		DST_dstRES,		DST_dstRES,		DST_dstRES,		DST_dstRES,		DST_dstRES,
		DST_dstSET,		DST_dstSET,		DST_dstSET,		DST_dstSET,		DST_dstSET,		DST_dstSET,		DST_dstSET,		DST_dstSET,
/*C*/	DST_dstCHG,		DST_dstCHG,		DST_dstCHG,		DST_dstCHG,		DST_dstCHG,		DST_dstCHG,		DST_dstCHG,		DST_dstCHG,
		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,		DST_dstBIT,
/*D*/	DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,
		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,		DST_dstJP,
/*E*/	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,
		DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,	DST_dstCALL,
/*F*/	DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,
		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET,		DST_dstRET
};

//Secondary (REG) Instruction decode
static void (*regDecode[256])() = 
{
/*0*/	er,			er,			er,			regLDi,		regPUSH,	regPOP,		regCPL,		regNEG,
		regMULi,	regMULSi,	regDIVi,	regDIVSi,	regLINK,	regUNLK,	regBS1F,	regBS1B,
/*1*/	regDAA,		er,			regEXTZ,	regEXTS,	regPAA,		er,			regMIRR,	er,
		er,			regMULA,	er,			er,			regDJNZ,	er,			er,			er,
/*2*/	regANDCFi,	regORCFi,	regXORCFi,	regLDCFi,	regSTCFi,	er,			er,			er,
		regANDCFA,	regORCFA,	regXORCFA,	regLDCFA,	regSTCFA,	er,			regLDCcrr,	regLDCrcr,
/*3*/	regRES,		regSET,		regCHG,		regBIT,		regTSET,	er,			er,			er,
		regMINC1,	regMINC2,	regMINC4,	er,			regMDEC1,	regMDEC2,	regMDEC4,	er,
/*4*/	regMUL,		regMUL,		regMUL,		regMUL,		regMUL,		regMUL,		regMUL,		regMUL,
		regMULS,	regMULS,	regMULS,	regMULS,	regMULS,	regMULS,	regMULS,	regMULS,
/*5*/	regDIV,		regDIV,		regDIV,		regDIV,		regDIV,		regDIV,		regDIV,		regDIV,
		regDIVS,	regDIVS,	regDIVS,	regDIVS,	regDIVS,	regDIVS,	regDIVS,	regDIVS,
/*6*/	regINC,		regINC,		regINC,		regINC,		regINC,		regINC,		regINC,		regINC,
		regDEC,		regDEC,		regDEC,		regDEC,		regDEC,		regDEC,		regDEC,		regDEC,
/*7*/	regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,
		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,		regSCC,
/*8*/	regADD,		regADD,		regADD,		regADD,		regADD,		regADD,		regADD,		regADD,
		regLDRr,	regLDRr,	regLDRr,	regLDRr,	regLDRr,	regLDRr,	regLDRr,	regLDRr,
/*9*/	regADC,		regADC,		regADC,		regADC,		regADC,		regADC,		regADC,		regADC,
		regLDrR,	regLDrR,	regLDrR,	regLDrR,	regLDrR,	regLDrR,	regLDrR,	regLDrR,
/*A*/	regSUB,		regSUB,		regSUB,		regSUB,		regSUB,		regSUB,		regSUB,		regSUB,
		regLDr3,	regLDr3,	regLDr3,	regLDr3,	regLDr3,	regLDr3,	regLDr3,	regLDr3,
/*B*/	regSBC,		regSBC,		regSBC,		regSBC,		regSBC,		regSBC,		regSBC,		regSBC,
		regEX,		regEX,		regEX,		regEX,		regEX,		regEX,		regEX,		regEX,
/*C*/	regAND,		regAND,		regAND,		regAND,		regAND,		regAND,		regAND,		regAND,
		regADDi,	regADCi,	regSUBi,	regSBCi,	regANDi,	regXORi,	regORi,		regCPi,
/*D*/	regXOR,		regXOR,		regXOR,		regXOR,		regXOR,		regXOR,		regXOR,		regXOR,
		regCPr3,	regCPr3,	regCPr3,	regCPr3,	regCPr3,	regCPr3,	regCPr3,	regCPr3,
/*E*/	regOR,		regOR,		regOR,		regOR,		regOR,		regOR,		regOR,		regOR,
		regRLCi,	regRRCi,	regRLi,		regRRi,		regSLAi,	regSRAi,	regSLLi,	regSRLi,
/*F*/	regCP,		regCP,		regCP,		regCP,		regCP,		regCP,		regCP,		regCP,
		regRLCA,	regRRCA,	regRLA,		regRRA,		regSLAA,	regSRAA,	regSLLA,	regSRLA
};

//=========================================================================

static void src_B(void)
{
	second = FETCH8;			//Get the second opcode
	R = second & 7;
	size = 0;					//Byte Size

	(*srcDecode[second])();		//Call
}

static void src_W(void)
{
	second = FETCH8;			//Get the second opcode
	R = second & 7;
	size = 1;					//Word Size

	(*srcDecode[second])();		//Call
}

static void src_L(void)
{
	second = FETCH8;			//Get the second opcode
	R = second & 7;
	size = 2;					//Long Size

	(*srcDecode[second])();		//Call
}

static void dst(void)
{
	second = FETCH8;			//Get the second opcode
	R = second & 7;

	(*dstDecode[second])();		//Call
}


static void reg_B(void)
{
	second = FETCH8;			//Get the second opcode
	R = second & 7;
	size = 0;					//Byte Size

	if (!brCode)
	{
		static uint8 rCodeConversionB[8] = { 0xE1, 0xE0, 0xE5, 0xE4, 0xE9, 0xE8, 0xED, 0xEC };
		brCode = true;
		rCode  = rCodeConversionB[first & 7];
	}

	(*regDecode[second])();		//Call
}

static void reg_W(void)
{
	second = FETCH8;			//Get the second opcode
	R      = second & 7;
	size   = 1;					//Word Size

	if (!brCode)
	{
		static uint8 rCodeConversionW[8] = { 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC };
		brCode = true;
		rCode  = rCodeConversionW[first & 7];
	}

	(*regDecode[second])();		//Call
}

static void reg_L(void)
{
	second = FETCH8;			//Get the second opcode
	R      = second & 7;
	size   = 2;				//Long Size

	if (!brCode)
	{
		static uint8 rCodeConversionL[8] = { 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC };
		brCode = true;
		rCode  = rCodeConversionL[first & 7];
	}

	(*regDecode[second])();		//Call
}

//=============================================================================

//Primary Instruction decode
static void (*decode[256])(void) = 
{
/*0*/	sngNOP,		sngNORMAL,	sngPUSHSR,	sngPOPSR,	sngMAX,		sngHALT,	sngEI,		sngRETI,
		sngLD8_8,	sngPUSH8,	sngLD8_16,	sngPUSH16,	sngINCF,	sngDECF,	sngRET,		sngRETD,
/*1*/	sngRCF,		sngSCF,		sngCCF,		sngZCF,		sngPUSHA,	sngPOPA,	sngEX,		sngLDF,
		sngPUSHF,	sngPOPF,	sngJP16,	sngJP24,	sngCALL16,	sngCALL24,	sngCALR,	iBIOSHLE,
/*2*/	sngLDB,		sngLDB,		sngLDB,		sngLDB,		sngLDB,		sngLDB,		sngLDB,		sngLDB,
		sngPUSHW,	sngPUSHW,	sngPUSHW,	sngPUSHW,	sngPUSHW,	sngPUSHW,	sngPUSHW,	sngPUSHW,
/*3*/	sngLDW,		sngLDW,		sngLDW,		sngLDW,		sngLDW,		sngLDW,		sngLDW,		sngLDW,
		sngPUSHL,	sngPUSHL,	sngPUSHL,	sngPUSHL,	sngPUSHL,	sngPUSHL,	sngPUSHL,	sngPUSHL,
/*4*/	sngLDL,		sngLDL,		sngLDL,		sngLDL,		sngLDL,		sngLDL,		sngLDL,		sngLDL,
		sngPOPW,	sngPOPW,	sngPOPW,	sngPOPW,	sngPOPW,	sngPOPW,	sngPOPW,	sngPOPW,
/*5*/	e,			e,			e,			e,			e,			e,			e,			e,
		sngPOPL,	sngPOPL,	sngPOPL,	sngPOPL,	sngPOPL,	sngPOPL,	sngPOPL,	sngPOPL,
/*6*/	sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,
		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,		sngJR,
/*7*/	sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,
		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,		sngJRL,
/*8*/	src_B,		src_B,		src_B,		src_B,		src_B,		src_B,		src_B,		src_B,
		src_B,		src_B,		src_B,		src_B,		src_B,		src_B,		src_B,		src_B,
/*9*/	src_W,		src_W,		src_W,		src_W,		src_W,		src_W,		src_W,		src_W,
		src_W,		src_W,		src_W,		src_W,		src_W,		src_W,		src_W,		src_W,
/*A*/	src_L,		src_L,		src_L,		src_L,		src_L,		src_L,		src_L,		src_L,
		src_L,		src_L,		src_L,		src_L,		src_L,		src_L,		src_L,		src_L,
/*B*/	dst,		dst,		dst,		dst,		dst,		dst,		dst,		dst,
		dst,		dst,		dst,		dst,		dst,		dst,		dst,		dst,
/*C*/	src_B,		src_B,		src_B,		src_B,		src_B,		src_B,		e,			reg_B,
		reg_B,		reg_B,		reg_B,		reg_B,		reg_B,		reg_B,		reg_B,		reg_B,
/*D*/	src_W,		src_W,		src_W,		src_W,		src_W,		src_W,		e,			reg_W,
		reg_W,		reg_W,		reg_W,		reg_W,		reg_W,		reg_W,		reg_W,		reg_W,
/*E*/	src_L,		src_L,		src_L,		src_L,		src_L,		src_L,		e,			reg_L,
		reg_L,		reg_L,		reg_L,		reg_L,		reg_L,		reg_L,		reg_L,		reg_L,
/*F*/	dst,		dst,		dst,		dst,		dst,		dst,		e,			sngLDX,
		sngSWI,		sngSWI,		sngSWI,		sngSWI,		sngSWI,		sngSWI,		sngSWI,		sngSWI
};

//=============================================================================

int32 TLCS900h_interpret(void)
{
	brCode = false;
	first  = FETCH8;	//Get the first byte
	//Is any extra data used by this instruction?
	cycles_extra = 0;
	if (decodeExtra[first])
		(*decodeExtra[first])();

	(*decode[first])();	//Decode

	return cycles + cycles_extra;
}
