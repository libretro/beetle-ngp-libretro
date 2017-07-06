/* z80_ops.c: Process the next opcode
   Copyright (c) 1999-2005 Philip Kendall, Witold Filipczyk

   $Id: z80_ops.c 4624 2012-01-09 20:59:35Z pak21 $

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include <boolean.h>

#include "z80.h"
#include "z80_macros.h"

bool NGPFrameSkip;
int32_t ngpc_soundTS = 0;

int iline = 0;

void z80_set_interrupt(int set)
{
   iline = set;
}

int z80_do_opcode( void )
{
   int ret;
   uint8 opcode;

   if(iline)
   {
      if(z80_interrupt())
      {
         int ret = z80_tstates - last_z80_tstates;
         last_z80_tstates = z80_tstates;
         return ret;
      }
   }

   opcode = Z80_RB_MACRO( PC ); 
   z80_tstates++;

   PC++; 
   R++;

   switch(opcode) 
   {
      case 0x00:		/* NOP */
         break;
      case 0x01:		/* LD BC,nnnn */
         C=Z80_RB_MACRO(PC++);
         B=Z80_RB_MACRO(PC++);
         break;
      case 0x02:		/* LD (BC),A */
         Z80_WB_MACRO(BC,A);
         break;
      case 0x03:		/* INC BC */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         BC++;
         break;
      case 0x04:		/* INC B */
         INC(B);
         break;
      case 0x05:		/* DEC B */
         DEC(B);
         break;
      case 0x06:		/* LD B,nn */
         B = Z80_RB_MACRO( PC++ );
         break;
      case 0x07:		/* RLCA */
         A = ( A << 1 ) | ( A >> 7 );
         F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
            ( A & ( FLAG_C | FLAG_3 | FLAG_5 ) );
         break;
      case 0x08:		/* EX AF,AF' */
         {
            uint16 wordtemp = AF; AF = AF_; AF_ = wordtemp;
         }
         break;
      case 0x09:		/* ADD HL,BC */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         ADD16(HL,BC);
         break;
      case 0x0a:		/* LD A,(BC) */
         A=Z80_RB_MACRO(BC);
         break;
      case 0x0b:		/* DEC BC */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         BC--;
         break;
      case 0x0c:		/* INC C */
         INC(C);
         break;
      case 0x0d:		/* DEC C */
         DEC(C);
         break;
      case 0x0e:		/* LD C,nn */
         C = Z80_RB_MACRO( PC++ );
         break;
      case 0x0f:		/* RRCA */
         F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) | ( A & FLAG_C );
         A = ( A >> 1) | ( A << 7 );
         F |= ( A & ( FLAG_3 | FLAG_5 ) );
         break;
      case 0x10:		/* DJNZ offset */
         contend_read_no_mreq( IR, 1 );
         B--;
         if(B) {
            JR();
         } else {
            contend_read( PC, 3 );
         }
         PC++;
         break;
      case 0x11:		/* LD DE,nnnn */
         E=Z80_RB_MACRO(PC++);
         D=Z80_RB_MACRO(PC++);
         break;
      case 0x12:		/* LD (DE),A */
         Z80_WB_MACRO(DE,A);
         break;
      case 0x13:		/* INC DE */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         DE++;
         break;
      case 0x14:		/* INC D */
         INC(D);
         break;
      case 0x15:		/* DEC D */
         DEC(D);
         break;
      case 0x16:		/* LD D,nn */
         D = Z80_RB_MACRO( PC++ );
         break;
      case 0x17:		/* RLA */
         {
            uint8 bytetemp = A;
            A = ( A << 1 ) | ( F & FLAG_C );
            F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
               ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp >> 7 );
         }
         break;
      case 0x18:		/* JR offset */
         JR();
         PC++;
         break;
      case 0x19:		/* ADD HL,DE */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         ADD16(HL,DE);
         break;
      case 0x1a:		/* LD A,(DE) */
         A=Z80_RB_MACRO(DE);
         break;
      case 0x1b:		/* DEC DE */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         DE--;
         break;
      case 0x1c:		/* INC E */
         INC(E);
         break;
      case 0x1d:		/* DEC E */
         DEC(E);
         break;
      case 0x1e:		/* LD E,nn */
         E = Z80_RB_MACRO( PC++ );
         break;
      case 0x1f:		/* RRA */
         {
            uint8 bytetemp = A;
            A = ( A >> 1 ) | ( F << 7 );
            F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
               ( A & ( FLAG_3 | FLAG_5 ) ) | ( bytetemp & FLAG_C ) ;
         }
         break;
      case 0x20:		/* JR NZ,offset */
         if( ! ( F & FLAG_Z ) ) {
            JR();
         } else {
            contend_read( PC, 3 );
         }
         PC++;
         break;
      case 0x21:		/* LD HL,nnnn */
         L=Z80_RB_MACRO(PC++);
         H=Z80_RB_MACRO(PC++);
         break;
      case 0x22:		/* LD (nnnn),HL */
         LD16_NNRR(L,H);
      case 0x23:		/* INC HL */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         HL++;
         break;
      case 0x24:		/* INC H */
         INC(H);
         break;
      case 0x25:		/* DEC H */
         DEC(H);
         break;
      case 0x26:		/* LD H,nn */
         H = Z80_RB_MACRO( PC++ );
         break;
      case 0x27:		/* DAA */
         {
            uint8 add = 0, carry = ( F & FLAG_C );
            if( ( F & FLAG_H ) || ( ( A & 0x0f ) > 9 ) ) add = 6;
            if( carry || ( A > 0x99 ) ) add |= 0x60;
            if( A > 0x99 ) carry = FLAG_C;
            if( F & FLAG_N ) {
               SUB(add);
            } else {
               ADD(add);
            }
            F = ( F & ~( FLAG_C | FLAG_P ) ) | carry | parity_table[A];
         }
         break;
      case 0x28:		/* JR Z,offset */
         if( F & FLAG_Z ) {
            JR();
         } else {
            contend_read( PC, 3 );
         }
         PC++;
         break;
      case 0x29:		/* ADD HL,HL */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         ADD16(HL,HL);
         break;
      case 0x2a:		/* LD HL,(nnnn) */
         LD16_RRNN(L,H);
      case 0x2b:		/* DEC HL */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         HL--;
         break;
      case 0x2c:		/* INC L */
         INC(L);
         break;
      case 0x2d:		/* DEC L */
         DEC(L);
         break;
      case 0x2e:		/* LD L,nn */
         L = Z80_RB_MACRO( PC++ );
         break;
      case 0x2f:		/* CPL */
         A ^= 0xff;
         F = ( F & ( FLAG_C | FLAG_P | FLAG_Z | FLAG_S ) ) |
            ( A & ( FLAG_3 | FLAG_5 ) ) | ( FLAG_N | FLAG_H );
         break;
      case 0x30:		/* JR NC,offset */
         if( ! ( F & FLAG_C ) ) {
            JR();
         } else {
            contend_read( PC, 3 );
         }
         PC++;
         break;
      case 0x31:		/* LD SP,nnnn */
         SPL=Z80_RB_MACRO(PC++);
         SPH=Z80_RB_MACRO(PC++);
         break;
      case 0x32:		/* LD (nnnn),A */
         {
            uint16 wordtemp = Z80_RB_MACRO( PC++ );
            wordtemp|=Z80_RB_MACRO(PC++) << 8;
            Z80_WB_MACRO(wordtemp,A);
         }
         break;
      case 0x33:		/* INC SP */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         SP++;
         break;
      case 0x34:		/* INC (HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            contend_read_no_mreq( HL, 1 );
            INC(bytetemp);
            Z80_WB_MACRO(HL,bytetemp);
         }
         break;
      case 0x35:		/* DEC (HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            contend_read_no_mreq( HL, 1 );
            DEC(bytetemp);
            Z80_WB_MACRO(HL,bytetemp);
         }
         break;
      case 0x36:		/* LD (HL),nn */
         Z80_WB_MACRO(HL,Z80_RB_MACRO(PC++));
         break;
      case 0x37:		/* SCF */
         F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
            ( A & ( FLAG_3 | FLAG_5          ) ) |
            FLAG_C;
         break;
      case 0x38:		/* JR C,offset */
         if( F & FLAG_C ) {
            JR();
         } else {
            contend_read( PC, 3 );
         }
         PC++;
         break;
      case 0x39:		/* ADD HL,SP */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         ADD16(HL,SP);
         break;
      case 0x3a:		/* LD A,(nnnn) */
         {
            uint16 wordtemp;
            wordtemp = Z80_RB_MACRO(PC++);
            wordtemp|= ( Z80_RB_MACRO(PC++) << 8 );
            A=Z80_RB_MACRO(wordtemp);
         }
         break;
      case 0x3b:		/* DEC SP */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         SP--;
         break;
      case 0x3c:		/* INC A */
         INC(A);
         break;
      case 0x3d:		/* DEC A */
         DEC(A);
         break;
      case 0x3e:		/* LD A,nn */
         A = Z80_RB_MACRO( PC++ );
         break;
      case 0x3f:		/* CCF */
         F = ( F & ( FLAG_P | FLAG_Z | FLAG_S ) ) |
            ( ( F & FLAG_C ) ? FLAG_H : FLAG_C ) | ( A & ( FLAG_3 | FLAG_5 ) );
         break;
      case 0x40:		/* LD B,B */
         break;
      case 0x41:		/* LD B,C */
         B=C;
         break;
      case 0x42:		/* LD B,D */
         B=D;
         break;
      case 0x43:		/* LD B,E */
         B=E;
         break;
      case 0x44:		/* LD B,H */
         B=H;
         break;
      case 0x45:		/* LD B,L */
         B=L;
         break;
      case 0x46:		/* LD B,(HL) */
         B=Z80_RB_MACRO(HL);
         break;
      case 0x47:		/* LD B,A */
         B=A;
         break;
      case 0x48:		/* LD C,B */
         C=B;
         break;
      case 0x49:		/* LD C,C */
         break;
      case 0x4a:		/* LD C,D */
         C=D;
         break;
      case 0x4b:		/* LD C,E */
         C=E;
         break;
      case 0x4c:		/* LD C,H */
         C=H;
         break;
      case 0x4d:		/* LD C,L */
         C=L;
         break;
      case 0x4e:		/* LD C,(HL) */
         C=Z80_RB_MACRO(HL);
         break;
      case 0x4f:		/* LD C,A */
         C=A;
         break;
      case 0x50:		/* LD D,B */
         D=B;
         break;
      case 0x51:		/* LD D,C */
         D=C;
         break;
      case 0x52:		/* LD D,D */
         break;
      case 0x53:		/* LD D,E */
         D=E;
         break;
      case 0x54:		/* LD D,H */
         D=H;
         break;
      case 0x55:		/* LD D,L */
         D=L;
         break;
      case 0x56:		/* LD D,(HL) */
         D=Z80_RB_MACRO(HL);
         break;
      case 0x57:		/* LD D,A */
         D=A;
         break;
      case 0x58:		/* LD E,B */
         E=B;
         break;
      case 0x59:		/* LD E,C */
         E=C;
         break;
      case 0x5a:		/* LD E,D */
         E=D;
         break;
      case 0x5b:		/* LD E,E */
         break;
      case 0x5c:		/* LD E,H */
         E=H;
         break;
      case 0x5d:		/* LD E,L */
         E=L;
         break;
      case 0x5e:		/* LD E,(HL) */
         E=Z80_RB_MACRO(HL);
         break;
      case 0x5f:		/* LD E,A */
         E=A;
         break;
      case 0x60:		/* LD H,B */
         H=B;
         break;
      case 0x61:		/* LD H,C */
         H=C;
         break;
      case 0x62:		/* LD H,D */
         H=D;
         break;
      case 0x63:		/* LD H,E */
         H=E;
         break;
      case 0x64:		/* LD H,H */
         break;
      case 0x65:		/* LD H,L */
         H=L;
         break;
      case 0x66:		/* LD H,(HL) */
         H=Z80_RB_MACRO(HL);
         break;
      case 0x67:		/* LD H,A */
         H=A;
         break;
      case 0x68:		/* LD L,B */
         L=B;
         break;
      case 0x69:		/* LD L,C */
         L=C;
         break;
      case 0x6a:		/* LD L,D */
         L=D;
         break;
      case 0x6b:		/* LD L,E */
         L=E;
         break;
      case 0x6c:		/* LD L,H */
         L=H;
         break;
      case 0x6d:		/* LD L,L */
         break;
      case 0x6e:		/* LD L,(HL) */
         L=Z80_RB_MACRO(HL);
         break;
      case 0x6f:		/* LD L,A */
         L=A;
         break;
      case 0x70:		/* LD (HL),B */
         Z80_WB_MACRO(HL,B);
         break;
      case 0x71:		/* LD (HL),C */
         Z80_WB_MACRO(HL,C);
         break;
      case 0x72:		/* LD (HL),D */
         Z80_WB_MACRO(HL,D);
         break;
      case 0x73:		/* LD (HL),E */
         Z80_WB_MACRO(HL,E);
         break;
      case 0x74:		/* LD (HL),H */
         Z80_WB_MACRO(HL,H);
         break;
      case 0x75:		/* LD (HL),L */
         Z80_WB_MACRO(HL,L);
         break;
      case 0x76:		/* HALT */
         z80.halted=1;
         PC--;
         break;
      case 0x77:		/* LD (HL),A */
         Z80_WB_MACRO(HL,A);
         break;
      case 0x78:		/* LD A,B */
         A=B;
         break;
      case 0x79:		/* LD A,C */
         A=C;
         break;
      case 0x7a:		/* LD A,D */
         A=D;
         break;
      case 0x7b:		/* LD A,E */
         A=E;
         break;
      case 0x7c:		/* LD A,H */
         A=H;
         break;
      case 0x7d:		/* LD A,L */
         A=L;
         break;
      case 0x7e:		/* LD A,(HL) */
         A=Z80_RB_MACRO(HL);
         break;
      case 0x7f:		/* LD A,A */
         break;
      case 0x80:		/* ADD A,B */
         ADD(B);
         break;
      case 0x81:		/* ADD A,C */
         ADD(C);
         break;
      case 0x82:		/* ADD A,D */
         ADD(D);
         break;
      case 0x83:		/* ADD A,E */
         ADD(E);
         break;
      case 0x84:		/* ADD A,H */
         ADD(H);
         break;
      case 0x85:		/* ADD A,L */
         ADD(L);
         break;
      case 0x86:		/* ADD A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            ADD(bytetemp);
         }
         break;
      case 0x87:		/* ADD A,A */
         ADD(A);
         break;
      case 0x88:		/* ADC A,B */
         ADC(B);
         break;
      case 0x89:		/* ADC A,C */
         ADC(C);
         break;
      case 0x8a:		/* ADC A,D */
         ADC(D);
         break;
      case 0x8b:		/* ADC A,E */
         ADC(E);
         break;
      case 0x8c:		/* ADC A,H */
         ADC(H);
         break;
      case 0x8d:		/* ADC A,L */
         ADC(L);
         break;
      case 0x8e:		/* ADC A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            ADC(bytetemp);
         }
         break;
      case 0x8f:		/* ADC A,A */
         ADC(A);
         break;
      case 0x90:		/* SUB A,B */
         SUB(B);
         break;
      case 0x91:		/* SUB A,C */
         SUB(C);
         break;
      case 0x92:		/* SUB A,D */
         SUB(D);
         break;
      case 0x93:		/* SUB A,E */
         SUB(E);
         break;
      case 0x94:		/* SUB A,H */
         SUB(H);
         break;
      case 0x95:		/* SUB A,L */
         SUB(L);
         break;
      case 0x96:		/* SUB A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            SUB(bytetemp);
         }
         break;
      case 0x97:		/* SUB A,A */
         SUB(A);
         break;
      case 0x98:		/* SBC A,B */
         SBC(B);
         break;
      case 0x99:		/* SBC A,C */
         SBC(C);
         break;
      case 0x9a:		/* SBC A,D */
         SBC(D);
         break;
      case 0x9b:		/* SBC A,E */
         SBC(E);
         break;
      case 0x9c:		/* SBC A,H */
         SBC(H);
         break;
      case 0x9d:		/* SBC A,L */
         SBC(L);
         break;
      case 0x9e:		/* SBC A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            SBC(bytetemp);
         }
         break;
      case 0x9f:		/* SBC A,A */
         SBC(A);
         break;
      case 0xa0:		/* AND A,B */
         AND(B);
         break;
      case 0xa1:		/* AND A,C */
         AND(C);
         break;
      case 0xa2:		/* AND A,D */
         AND(D);
         break;
      case 0xa3:		/* AND A,E */
         AND(E);
         break;
      case 0xa4:		/* AND A,H */
         AND(H);
         break;
      case 0xa5:		/* AND A,L */
         AND(L);
         break;
      case 0xa6:		/* AND A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            AND(bytetemp);
         }
         break;
      case 0xa7:		/* AND A,A */
         AND(A);
         break;
      case 0xa8:		/* XOR A,B */
         XOR(B);
         break;
      case 0xa9:		/* XOR A,C */
         XOR(C);
         break;
      case 0xaa:		/* XOR A,D */
         XOR(D);
         break;
      case 0xab:		/* XOR A,E */
         XOR(E);
         break;
      case 0xac:		/* XOR A,H */
         XOR(H);
         break;
      case 0xad:		/* XOR A,L */
         XOR(L);
         break;
      case 0xae:		/* XOR A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            XOR(bytetemp);
         }
         break;
      case 0xaf:		/* XOR A,A */
         XOR(A);
         break;
      case 0xb0:		/* OR A,B */
         OR(B);
         break;
      case 0xb1:		/* OR A,C */
         OR(C);
         break;
      case 0xb2:		/* OR A,D */
         OR(D);
         break;
      case 0xb3:		/* OR A,E */
         OR(E);
         break;
      case 0xb4:		/* OR A,H */
         OR(H);
         break;
      case 0xb5:		/* OR A,L */
         OR(L);
         break;
      case 0xb6:		/* OR A,(HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            OR(bytetemp);
         }
         break;
      case 0xb7:		/* OR A,A */
         OR(A);
         break;
      case 0xb8:		/* CP B */
         CP(B);
         break;
      case 0xb9:		/* CP C */
         CP(C);
         break;
      case 0xba:		/* CP D */
         CP(D);
         break;
      case 0xbb:		/* CP E */
         CP(E);
         break;
      case 0xbc:		/* CP H */
         CP(H);
         break;
      case 0xbd:		/* CP L */
         CP(L);
         break;
      case 0xbe:		/* CP (HL) */
         {
            uint8 bytetemp = Z80_RB_MACRO( HL );
            CP(bytetemp);
         }
         break;
      case 0xbf:		/* CP A */
         CP(A);
         break;
      case 0xc0:		/* RET NZ */
         contend_read_no_mreq( IR, 1 );
         if( ! ( F & FLAG_Z ) ) { RET(); }
         break;
      case 0xc1:		/* POP BC */
         POP16(C,B);
         break;
      case 0xc2:		/* JP NZ,nnnn */
         if( ! ( F & FLAG_Z ) ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xc3:		/* JP nnnn */
         JP();
         break;
      case 0xc4:		/* CALL NZ,nnnn */
         if( ! ( F & FLAG_Z ) ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xc5:		/* PUSH BC */
         contend_read_no_mreq( IR, 1 );
         PUSH16(C,B);
         break;
      case 0xc6:		/* ADD A,nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            ADD(bytetemp);
         }
         break;
      case 0xc7:		/* RST 00 */
         contend_read_no_mreq( IR, 1 );
         RST(0x00);
         break;
      case 0xc8:		/* RET Z */
         contend_read_no_mreq( IR, 1 );
         if( F & FLAG_Z ) { RET(); }
         break;
      case 0xc9:		/* RET */
         RET();
         break;
      case 0xca:		/* JP Z,nnnn */
         if( F & FLAG_Z ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xcb:		/* shift CB */
         {
            uint8 opcode2;
            opcode2 = Z80_RB_MACRO( PC );
            z80_tstates++;
            PC++;
            R++;
            switch(opcode2)
            {
               case 0x00:		/* RLC B */
                  RLC(B);
                  break;
               case 0x01:		/* RLC C */
                  RLC(C);
                  break;
               case 0x02:		/* RLC D */
                  RLC(D);
                  break;
               case 0x03:		/* RLC E */
                  RLC(E);
                  break;
               case 0x04:		/* RLC H */
                  RLC(H);
                  break;
               case 0x05:		/* RLC L */
                  RLC(L);
                  break;
               case 0x06:		/* RLC (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     RLC(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x07:		/* RLC A */
                  RLC(A);
                  break;
               case 0x08:		/* RRC B */
                  RRC(B);
                  break;
               case 0x09:		/* RRC C */
                  RRC(C);
                  break;
               case 0x0a:		/* RRC D */
                  RRC(D);
                  break;
               case 0x0b:		/* RRC E */
                  RRC(E);
                  break;
               case 0x0c:		/* RRC H */
                  RRC(H);
                  break;
               case 0x0d:		/* RRC L */
                  RRC(L);
                  break;
               case 0x0e:		/* RRC (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     RRC(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x0f:		/* RRC A */
                  RRC(A);
                  break;
               case 0x10:		/* RL B */
                  RL(B);
                  break;
               case 0x11:		/* RL C */
                  RL(C);
                  break;
               case 0x12:		/* RL D */
                  RL(D);
                  break;
               case 0x13:		/* RL E */
                  RL(E);
                  break;
               case 0x14:		/* RL H */
                  RL(H);
                  break;
               case 0x15:		/* RL L */
                  RL(L);
                  break;
               case 0x16:		/* RL (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     RL(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x17:		/* RL A */
                  RL(A);
                  break;
               case 0x18:		/* RR B */
                  RR(B);
                  break;
               case 0x19:		/* RR C */
                  RR(C);
                  break;
               case 0x1a:		/* RR D */
                  RR(D);
                  break;
               case 0x1b:		/* RR E */
                  RR(E);
                  break;
               case 0x1c:		/* RR H */
                  RR(H);
                  break;
               case 0x1d:		/* RR L */
                  RR(L);
                  break;
               case 0x1e:		/* RR (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     RR(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x1f:		/* RR A */
                  RR(A);
                  break;
               case 0x20:		/* SLA B */
                  SLA(B);
                  break;
               case 0x21:		/* SLA C */
                  SLA(C);
                  break;
               case 0x22:		/* SLA D */
                  SLA(D);
                  break;
               case 0x23:		/* SLA E */
                  SLA(E);
                  break;
               case 0x24:		/* SLA H */
                  SLA(H);
                  break;
               case 0x25:		/* SLA L */
                  SLA(L);
                  break;
               case 0x26:		/* SLA (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     SLA(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x27:		/* SLA A */
                  SLA(A);
                  break;
               case 0x28:		/* SRA B */
                  SRA(B);
                  break;
               case 0x29:		/* SRA C */
                  SRA(C);
                  break;
               case 0x2a:		/* SRA D */
                  SRA(D);
                  break;
               case 0x2b:		/* SRA E */
                  SRA(E);
                  break;
               case 0x2c:		/* SRA H */
                  SRA(H);
                  break;
               case 0x2d:		/* SRA L */
                  SRA(L);
                  break;
               case 0x2e:		/* SRA (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     SRA(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x2f:		/* SRA A */
                  SRA(A);
                  break;
               case 0x30:		/* SLL B */
                  SLL(B);
                  break;
               case 0x31:		/* SLL C */
                  SLL(C);
                  break;
               case 0x32:		/* SLL D */
                  SLL(D);
                  break;
               case 0x33:		/* SLL E */
                  SLL(E);
                  break;
               case 0x34:		/* SLL H */
                  SLL(H);
                  break;
               case 0x35:		/* SLL L */
                  SLL(L);
                  break;
               case 0x36:		/* SLL (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     SLL(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x37:		/* SLL A */
                  SLL(A);
                  break;
               case 0x38:		/* SRL B */
                  SRL(B);
                  break;
               case 0x39:		/* SRL C */
                  SRL(C);
                  break;
               case 0x3a:		/* SRL D */
                  SRL(D);
                  break;
               case 0x3b:		/* SRL E */
                  SRL(E);
                  break;
               case 0x3c:		/* SRL H */
                  SRL(H);
                  break;
               case 0x3d:		/* SRL L */
                  SRL(L);
                  break;
               case 0x3e:		/* SRL (HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO(HL);
                     contend_read_no_mreq( HL, 1 );
                     SRL(bytetemp);
                     Z80_WB_MACRO(HL,bytetemp);
                  }
                  break;
               case 0x3f:		/* SRL A */
                  SRL(A);
                  break;
               case 0x40:		/* BIT 0,B */
                  BIT( 0, B );
                  break;
               case 0x41:		/* BIT 0,C */
                  BIT( 0, C );
                  break;
               case 0x42:		/* BIT 0,D */
                  BIT( 0, D );
                  break;
               case 0x43:		/* BIT 0,E */
                  BIT( 0, E );
                  break;
               case 0x44:		/* BIT 0,H */
                  BIT( 0, H );
                  break;
               case 0x45:		/* BIT 0,L */
                  BIT( 0, L );
                  break;
               case 0x46:		/* BIT 0,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 0, bytetemp );
                  }
                  break;
               case 0x47:		/* BIT 0,A */
                  BIT( 0, A );
                  break;
               case 0x48:		/* BIT 1,B */
                  BIT( 1, B );
                  break;
               case 0x49:		/* BIT 1,C */
                  BIT( 1, C );
                  break;
               case 0x4a:		/* BIT 1,D */
                  BIT( 1, D );
                  break;
               case 0x4b:		/* BIT 1,E */
                  BIT( 1, E );
                  break;
               case 0x4c:		/* BIT 1,H */
                  BIT( 1, H );
                  break;
               case 0x4d:		/* BIT 1,L */
                  BIT( 1, L );
                  break;
               case 0x4e:		/* BIT 1,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 1, bytetemp );
                  }
                  break;
               case 0x4f:		/* BIT 1,A */
                  BIT( 1, A );
                  break;
               case 0x50:		/* BIT 2,B */
                  BIT( 2, B );
                  break;
               case 0x51:		/* BIT 2,C */
                  BIT( 2, C );
                  break;
               case 0x52:		/* BIT 2,D */
                  BIT( 2, D );
                  break;
               case 0x53:		/* BIT 2,E */
                  BIT( 2, E );
                  break;
               case 0x54:		/* BIT 2,H */
                  BIT( 2, H );
                  break;
               case 0x55:		/* BIT 2,L */
                  BIT( 2, L );
                  break;
               case 0x56:		/* BIT 2,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 2, bytetemp );
                  }
                  break;
               case 0x57:		/* BIT 2,A */
                  BIT( 2, A );
                  break;
               case 0x58:		/* BIT 3,B */
                  BIT( 3, B );
                  break;
               case 0x59:		/* BIT 3,C */
                  BIT( 3, C );
                  break;
               case 0x5a:		/* BIT 3,D */
                  BIT( 3, D );
                  break;
               case 0x5b:		/* BIT 3,E */
                  BIT( 3, E );
                  break;
               case 0x5c:		/* BIT 3,H */
                  BIT( 3, H );
                  break;
               case 0x5d:		/* BIT 3,L */
                  BIT( 3, L );
                  break;
               case 0x5e:		/* BIT 3,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 3, bytetemp );
                  }
                  break;
               case 0x5f:		/* BIT 3,A */
                  BIT( 3, A );
                  break;
               case 0x60:		/* BIT 4,B */
                  BIT( 4, B );
                  break;
               case 0x61:		/* BIT 4,C */
                  BIT( 4, C );
                  break;
               case 0x62:		/* BIT 4,D */
                  BIT( 4, D );
                  break;
               case 0x63:		/* BIT 4,E */
                  BIT( 4, E );
                  break;
               case 0x64:		/* BIT 4,H */
                  BIT( 4, H );
                  break;
               case 0x65:		/* BIT 4,L */
                  BIT( 4, L );
                  break;
               case 0x66:		/* BIT 4,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 4, bytetemp );
                  }
                  break;
               case 0x67:		/* BIT 4,A */
                  BIT( 4, A );
                  break;
               case 0x68:		/* BIT 5,B */
                  BIT( 5, B );
                  break;
               case 0x69:		/* BIT 5,C */
                  BIT( 5, C );
                  break;
               case 0x6a:		/* BIT 5,D */
                  BIT( 5, D );
                  break;
               case 0x6b:		/* BIT 5,E */
                  BIT( 5, E );
                  break;
               case 0x6c:		/* BIT 5,H */
                  BIT( 5, H );
                  break;
               case 0x6d:		/* BIT 5,L */
                  BIT( 5, L );
                  break;
               case 0x6e:		/* BIT 5,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 5, bytetemp );
                  }
                  break;
               case 0x6f:		/* BIT 5,A */
                  BIT( 5, A );
                  break;
               case 0x70:		/* BIT 6,B */
                  BIT( 6, B );
                  break;
               case 0x71:		/* BIT 6,C */
                  BIT( 6, C );
                  break;
               case 0x72:		/* BIT 6,D */
                  BIT( 6, D );
                  break;
               case 0x73:		/* BIT 6,E */
                  BIT( 6, E );
                  break;
               case 0x74:		/* BIT 6,H */
                  BIT( 6, H );
                  break;
               case 0x75:		/* BIT 6,L */
                  BIT( 6, L );
                  break;
               case 0x76:		/* BIT 6,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 6, bytetemp );
                  }
                  break;
               case 0x77:		/* BIT 6,A */
                  BIT( 6, A );
                  break;
               case 0x78:		/* BIT 7,B */
                  BIT( 7, B );
                  break;
               case 0x79:		/* BIT 7,C */
                  BIT( 7, C );
                  break;
               case 0x7a:		/* BIT 7,D */
                  BIT( 7, D );
                  break;
               case 0x7b:		/* BIT 7,E */
                  BIT( 7, E );
                  break;
               case 0x7c:		/* BIT 7,H */
                  BIT( 7, H );
                  break;
               case 0x7d:		/* BIT 7,L */
                  BIT( 7, L );
                  break;
               case 0x7e:		/* BIT 7,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     BIT( 7, bytetemp );
                  }
                  break;
               case 0x7f:		/* BIT 7,A */
                  BIT( 7, A );
                  break;
               case 0x80:		/* RES 0,B */
                  B &= 0xfe;
                  break;
               case 0x81:		/* RES 0,C */
                  C &= 0xfe;
                  break;
               case 0x82:		/* RES 0,D */
                  D &= 0xfe;
                  break;
               case 0x83:		/* RES 0,E */
                  E &= 0xfe;
                  break;
               case 0x84:		/* RES 0,H */
                  H &= 0xfe;
                  break;
               case 0x85:		/* RES 0,L */
                  L &= 0xfe;
                  break;
               case 0x86:		/* RES 0,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xfe );
                  }
                  break;
               case 0x87:		/* RES 0,A */
                  A &= 0xfe;
                  break;
               case 0x88:		/* RES 1,B */
                  B &= 0xfd;
                  break;
               case 0x89:		/* RES 1,C */
                  C &= 0xfd;
                  break;
               case 0x8a:		/* RES 1,D */
                  D &= 0xfd;
                  break;
               case 0x8b:		/* RES 1,E */
                  E &= 0xfd;
                  break;
               case 0x8c:		/* RES 1,H */
                  H &= 0xfd;
                  break;
               case 0x8d:		/* RES 1,L */
                  L &= 0xfd;
                  break;
               case 0x8e:		/* RES 1,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xfd );
                  }
                  break;
               case 0x8f:		/* RES 1,A */
                  A &= 0xfd;
                  break;
               case 0x90:		/* RES 2,B */
                  B &= 0xfb;
                  break;
               case 0x91:		/* RES 2,C */
                  C &= 0xfb;
                  break;
               case 0x92:		/* RES 2,D */
                  D &= 0xfb;
                  break;
               case 0x93:		/* RES 2,E */
                  E &= 0xfb;
                  break;
               case 0x94:		/* RES 2,H */
                  H &= 0xfb;
                  break;
               case 0x95:		/* RES 2,L */
                  L &= 0xfb;
                  break;
               case 0x96:		/* RES 2,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xfb );
                  }
                  break;
               case 0x97:		/* RES 2,A */
                  A &= 0xfb;
                  break;
               case 0x98:		/* RES 3,B */
                  B &= 0xf7;
                  break;
               case 0x99:		/* RES 3,C */
                  C &= 0xf7;
                  break;
               case 0x9a:		/* RES 3,D */
                  D &= 0xf7;
                  break;
               case 0x9b:		/* RES 3,E */
                  E &= 0xf7;
                  break;
               case 0x9c:		/* RES 3,H */
                  H &= 0xf7;
                  break;
               case 0x9d:		/* RES 3,L */
                  L &= 0xf7;
                  break;
               case 0x9e:		/* RES 3,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xf7 );
                  }
                  break;
               case 0x9f:		/* RES 3,A */
                  A &= 0xf7;
                  break;
               case 0xa0:		/* RES 4,B */
                  B &= 0xef;
                  break;
               case 0xa1:		/* RES 4,C */
                  C &= 0xef;
                  break;
               case 0xa2:		/* RES 4,D */
                  D &= 0xef;
                  break;
               case 0xa3:		/* RES 4,E */
                  E &= 0xef;
                  break;
               case 0xa4:		/* RES 4,H */
                  H &= 0xef;
                  break;
               case 0xa5:		/* RES 4,L */
                  L &= 0xef;
                  break;
               case 0xa6:		/* RES 4,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xef );
                  }
                  break;
               case 0xa7:		/* RES 4,A */
                  A &= 0xef;
                  break;
               case 0xa8:		/* RES 5,B */
                  B &= 0xdf;
                  break;
               case 0xa9:		/* RES 5,C */
                  C &= 0xdf;
                  break;
               case 0xaa:		/* RES 5,D */
                  D &= 0xdf;
                  break;
               case 0xab:		/* RES 5,E */
                  E &= 0xdf;
                  break;
               case 0xac:		/* RES 5,H */
                  H &= 0xdf;
                  break;
               case 0xad:		/* RES 5,L */
                  L &= 0xdf;
                  break;
               case 0xae:		/* RES 5,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xdf );
                  }
                  break;
               case 0xaf:		/* RES 5,A */
                  A &= 0xdf;
                  break;
               case 0xb0:		/* RES 6,B */
                  B &= 0xbf;
                  break;
               case 0xb1:		/* RES 6,C */
                  C &= 0xbf;
                  break;
               case 0xb2:		/* RES 6,D */
                  D &= 0xbf;
                  break;
               case 0xb3:		/* RES 6,E */
                  E &= 0xbf;
                  break;
               case 0xb4:		/* RES 6,H */
                  H &= 0xbf;
                  break;
               case 0xb5:		/* RES 6,L */
                  L &= 0xbf;
                  break;
               case 0xb6:		/* RES 6,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0xbf );
                  }
                  break;
               case 0xb7:		/* RES 6,A */
                  A &= 0xbf;
                  break;
               case 0xb8:		/* RES 7,B */
                  B &= 0x7f;
                  break;
               case 0xb9:		/* RES 7,C */
                  C &= 0x7f;
                  break;
               case 0xba:		/* RES 7,D */
                  D &= 0x7f;
                  break;
               case 0xbb:		/* RES 7,E */
                  E &= 0x7f;
                  break;
               case 0xbc:		/* RES 7,H */
                  H &= 0x7f;
                  break;
               case 0xbd:		/* RES 7,L */
                  L &= 0x7f;
                  break;
               case 0xbe:		/* RES 7,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp & 0x7f );
                  }
                  break;
               case 0xbf:		/* RES 7,A */
                  A &= 0x7f;
                  break;
               case 0xc0:		/* SET 0,B */
                  B |= 0x01;
                  break;
               case 0xc1:		/* SET 0,C */
                  C |= 0x01;
                  break;
               case 0xc2:		/* SET 0,D */
                  D |= 0x01;
                  break;
               case 0xc3:		/* SET 0,E */
                  E |= 0x01;
                  break;
               case 0xc4:		/* SET 0,H */
                  H |= 0x01;
                  break;
               case 0xc5:		/* SET 0,L */
                  L |= 0x01;
                  break;
               case 0xc6:		/* SET 0,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x01 );
                  }
                  break;
               case 0xc7:		/* SET 0,A */
                  A |= 0x01;
                  break;
               case 0xc8:		/* SET 1,B */
                  B |= 0x02;
                  break;
               case 0xc9:		/* SET 1,C */
                  C |= 0x02;
                  break;
               case 0xca:		/* SET 1,D */
                  D |= 0x02;
                  break;
               case 0xcb:		/* SET 1,E */
                  E |= 0x02;
                  break;
               case 0xcc:		/* SET 1,H */
                  H |= 0x02;
                  break;
               case 0xcd:		/* SET 1,L */
                  L |= 0x02;
                  break;
               case 0xce:		/* SET 1,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x02 );
                  }
                  break;
               case 0xcf:		/* SET 1,A */
                  A |= 0x02;
                  break;
               case 0xd0:		/* SET 2,B */
                  B |= 0x04;
                  break;
               case 0xd1:		/* SET 2,C */
                  C |= 0x04;
                  break;
               case 0xd2:		/* SET 2,D */
                  D |= 0x04;
                  break;
               case 0xd3:		/* SET 2,E */
                  E |= 0x04;
                  break;
               case 0xd4:		/* SET 2,H */
                  H |= 0x04;
                  break;
               case 0xd5:		/* SET 2,L */
                  L |= 0x04;
                  break;
               case 0xd6:		/* SET 2,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x04 );
                  }
                  break;
               case 0xd7:		/* SET 2,A */
                  A |= 0x04;
                  break;
               case 0xd8:		/* SET 3,B */
                  B |= 0x08;
                  break;
               case 0xd9:		/* SET 3,C */
                  C |= 0x08;
                  break;
               case 0xda:		/* SET 3,D */
                  D |= 0x08;
                  break;
               case 0xdb:		/* SET 3,E */
                  E |= 0x08;
                  break;
               case 0xdc:		/* SET 3,H */
                  H |= 0x08;
                  break;
               case 0xdd:		/* SET 3,L */
                  L |= 0x08;
                  break;
               case 0xde:		/* SET 3,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x08 );
                  }
                  break;
               case 0xdf:		/* SET 3,A */
                  A |= 0x08;
                  break;
               case 0xe0:		/* SET 4,B */
                  B |= 0x10;
                  break;
               case 0xe1:		/* SET 4,C */
                  C |= 0x10;
                  break;
               case 0xe2:		/* SET 4,D */
                  D |= 0x10;
                  break;
               case 0xe3:		/* SET 4,E */
                  E |= 0x10;
                  break;
               case 0xe4:		/* SET 4,H */
                  H |= 0x10;
                  break;
               case 0xe5:		/* SET 4,L */
                  L |= 0x10;
                  break;
               case 0xe6:		/* SET 4,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x10 );
                  }
                  break;
               case 0xe7:		/* SET 4,A */
                  A |= 0x10;
                  break;
               case 0xe8:		/* SET 5,B */
                  B |= 0x20;
                  break;
               case 0xe9:		/* SET 5,C */
                  C |= 0x20;
                  break;
               case 0xea:		/* SET 5,D */
                  D |= 0x20;
                  break;
               case 0xeb:		/* SET 5,E */
                  E |= 0x20;
                  break;
               case 0xec:		/* SET 5,H */
                  H |= 0x20;
                  break;
               case 0xed:		/* SET 5,L */
                  L |= 0x20;
                  break;
               case 0xee:		/* SET 5,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x20 );
                  }
                  break;
               case 0xef:		/* SET 5,A */
                  A |= 0x20;
                  break;
               case 0xf0:		/* SET 6,B */
                  B |= 0x40;
                  break;
               case 0xf1:		/* SET 6,C */
                  C |= 0x40;
                  break;
               case 0xf2:		/* SET 6,D */
                  D |= 0x40;
                  break;
               case 0xf3:		/* SET 6,E */
                  E |= 0x40;
                  break;
               case 0xf4:		/* SET 6,H */
                  H |= 0x40;
                  break;
               case 0xf5:		/* SET 6,L */
                  L |= 0x40;
                  break;
               case 0xf6:		/* SET 6,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x40 );
                  }
                  break;
               case 0xf7:		/* SET 6,A */
                  A |= 0x40;
                  break;
               case 0xf8:		/* SET 7,B */
                  B |= 0x80;
                  break;
               case 0xf9:		/* SET 7,C */
                  C |= 0x80;
                  break;
               case 0xfa:		/* SET 7,D */
                  D |= 0x80;
                  break;
               case 0xfb:		/* SET 7,E */
                  E |= 0x80;
                  break;
               case 0xfc:		/* SET 7,H */
                  H |= 0x80;
                  break;
               case 0xfd:		/* SET 7,L */
                  L |= 0x80;
                  break;
               case 0xfe:		/* SET 7,(HL) */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO( HL, bytetemp | 0x80 );
                  }
                  break;
               case 0xff:		/* SET 7,A */
                  A |= 0x80;
                  break;
            }
         }
         break;
      case 0xcc:		/* CALL Z,nnnn */
         if( F & FLAG_Z ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xcd:		/* CALL nnnn */
         CALL();
         break;
      case 0xce:		/* ADC A,nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            ADC(bytetemp);
         }
         break;
      case 0xcf:		/* RST 8 */
         contend_read_no_mreq( IR, 1 );
         RST(0x08);
         break;
      case 0xd0:		/* RET NC */
         contend_read_no_mreq( IR, 1 );
         if( ! ( F & FLAG_C ) ) { RET(); }
         break;
      case 0xd1:		/* POP DE */
         POP16(E,D);
         break;
      case 0xd2:		/* JP NC,nnnn */
         if( ! ( F & FLAG_C ) ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xd3:		/* OUT (nn),A */
         { 
            uint16 outtemp;
            outtemp = Z80_RB_MACRO( PC++ ) + ( A << 8 );
            Z80_WP_MACRO( outtemp, A );
         }
         break;
      case 0xd4:		/* CALL NC,nnnn */
         if( ! ( F & FLAG_C ) ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xd5:		/* PUSH DE */
         contend_read_no_mreq( IR, 1 );
         PUSH16(E,D);
         break;
      case 0xd6:		/* SUB nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            SUB(bytetemp);
         }
         break;
      case 0xd7:		/* RST 10 */
         contend_read_no_mreq( IR, 1 );
         RST(0x10);
         break;
      case 0xd8:		/* RET C */
         contend_read_no_mreq( IR, 1 );
         if( F & FLAG_C ) { RET(); }
         break;
      case 0xd9:		/* EXX */
         {
            uint16 wordtemp;
            wordtemp = BC; BC = BC_; BC_ = wordtemp;
            wordtemp = DE; DE = DE_; DE_ = wordtemp;
            wordtemp = HL; HL = HL_; HL_ = wordtemp;
         }
         break;
      case 0xda:		/* JP C,nnnn */
         if( F & FLAG_C ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xdb:		/* IN A,(nn) */
         { 
            uint16 intemp;
            intemp = Z80_RB_MACRO( PC++ ) + ( A << 8 );
            A=Z80_RP_MACRO( intemp );
         }
         break;
      case 0xdc:		/* CALL C,nnnn */
         if( F & FLAG_C ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xdd:		/* shift DD */
         {
            uint8 opcode2;
            opcode2 = Z80_RB_MACRO( PC );
            z80_tstates++;
            PC++;
            R++;
            switch(opcode2) {
#define REGISTER  IX
#define REGISTERL IXL
#define REGISTERH IXH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
            }
         }
         break;
      case 0xde:		/* SBC A,nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            SBC(bytetemp);
         }
         break;
      case 0xdf:		/* RST 18 */
         contend_read_no_mreq( IR, 1 );
         RST(0x18);
         break;
      case 0xe0:		/* RET PO */
         contend_read_no_mreq( IR, 1 );
         if( ! ( F & FLAG_P ) ) { RET(); }
         break;
      case 0xe1:		/* POP HL */
         POP16(L,H);
         break;
      case 0xe2:		/* JP PO,nnnn */
         if( ! ( F & FLAG_P ) ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xe3:		/* EX (SP),HL */
         {
            uint8 bytetempl, bytetemph;
            bytetempl = Z80_RB_MACRO( SP );
            bytetemph = Z80_RB_MACRO( SP + 1 ); contend_read_no_mreq( SP + 1, 1 );
            Z80_WB_MACRO( SP + 1, H );
            Z80_WB_MACRO( SP,     L  );
            contend_write_no_mreq( SP, 1 ); contend_write_no_mreq( SP, 1 );
            L=bytetempl; H=bytetemph;
         }
         break;
      case 0xe4:		/* CALL PO,nnnn */
         if( ! ( F & FLAG_P ) ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xe5:		/* PUSH HL */
         contend_read_no_mreq( IR, 1 );
         PUSH16(L,H);
         break;
      case 0xe6:		/* AND nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            AND(bytetemp);
         }
         break;
      case 0xe7:		/* RST 20 */
         contend_read_no_mreq( IR, 1 );
         RST(0x20);
         break;
      case 0xe8:		/* RET PE */
         contend_read_no_mreq( IR, 1 );
         if( F & FLAG_P ) { RET(); }
         break;
      case 0xe9:		/* JP HL */
         PC=HL;		/* NB: NOT INDIRECT! */
         break;
      case 0xea:		/* JP PE,nnnn */
         if( F & FLAG_P ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xeb:		/* EX DE,HL */
         {
            uint16 wordtemp=DE; DE=HL; HL=wordtemp;
         }
         break;
      case 0xec:		/* CALL PE,nnnn */
         if( F & FLAG_P ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xed:		/* shift ED */
         {
            uint8 opcode2;
            opcode2 = Z80_RB_MACRO( PC );
            z80_tstates++;
            PC++;
            R++;
            switch(opcode2)
            {
               case 0x40:		/* IN B,(C) */
                  Z80_IN( B, BC );
                  break;
               case 0x41:		/* OUT (C),B */
                  Z80_WP_MACRO( BC, B );
                  break;
               case 0x42:		/* SBC HL,BC */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  SBC16(BC);
                  break;
               case 0x43:		/* LD (nnnn),BC */
                  LD16_NNRR(C,B);
               case 0x44:
               case 0x4c:
               case 0x54:
               case 0x5c:
               case 0x64:
               case 0x6c:
               case 0x74:
               case 0x7c:		/* NEG */
                  {
                     uint8 bytetemp=A;
                     A=0;
                     SUB(bytetemp);
                  }
                  break;
               case 0x45:
               case 0x4d:
               case 0x55:
               case 0x5d:
               case 0x65:
               case 0x6d:
               case 0x75:
               case 0x7d:		/* RETN */
                  IFF1=IFF2;
                  RET();
                  break;
               case 0x46:
               case 0x4e:
               case 0x66:
               case 0x6e:		/* IM 0 */
                  IM=0;
                  break;
               case 0x47:		/* LD I,A */
                  contend_read_no_mreq( IR, 1 );
                  I=A;
                  break;
               case 0x48:		/* IN C,(C) */
                  Z80_IN( C, BC );
                  break;
               case 0x49:		/* OUT (C),C */
                  Z80_WP_MACRO( BC, C );
                  break;
               case 0x4a:		/* ADC HL,BC */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  ADC16(BC);
                  break;
               case 0x4b:		/* LD BC,(nnnn) */
                  LD16_RRNN(C,B);
               case 0x4f:		/* LD R,A */
                  contend_read_no_mreq( IR, 1 );
                  R=R7=A;
                  break;
               case 0x50:		/* IN D,(C) */
                  Z80_IN( D, BC );
                  break;
               case 0x51:		/* OUT (C),D */
                  Z80_WP_MACRO( BC, D );
                  break;
               case 0x52:		/* SBC HL,DE */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  SBC16(DE);
                  break;
               case 0x53:		/* LD (nnnn),DE */
                  LD16_NNRR(E,D);
               case 0x56:
               case 0x76:		/* IM 1 */
                  IM=1;
                  break;
               case 0x57:		/* LD A,I */
                  contend_read_no_mreq( IR, 1 );
                  A=I;
                  F = ( F & FLAG_C ) | sz53_table[A] | ( IFF2 ? FLAG_V : 0 );
                  break;
               case 0x58:		/* IN E,(C) */
                  Z80_IN( E, BC );
                  break;
               case 0x59:		/* OUT (C),E */
                  Z80_WP_MACRO( BC, E );
                  break;
               case 0x5a:		/* ADC HL,DE */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  ADC16(DE);
                  break;
               case 0x5b:		/* LD DE,(nnnn) */
                  LD16_RRNN(E,D);
               case 0x5e:
               case 0x7e:		/* IM 2 */
                  IM=2;
                  break;
               case 0x5f:		/* LD A,R */
                  contend_read_no_mreq( IR, 1 );
                  A=(R&0x7f) | (R7&0x80);
                  F = ( F & FLAG_C ) | sz53_table[A] | ( IFF2 ? FLAG_V : 0 );
                  break;
               case 0x60:		/* IN H,(C) */
                  Z80_IN( H, BC );
                  break;
               case 0x61:		/* OUT (C),H */
                  Z80_WP_MACRO( BC, H );
                  break;
               case 0x62:		/* SBC HL,HL */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  SBC16(HL);
                  break;
               case 0x63:		/* LD (nnnn),HL */
                  LD16_NNRR(L,H);
               case 0x67:		/* RRD */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO(HL,  ( A << 4 ) | ( bytetemp >> 4 ) );
                     A = ( A & 0xf0 ) | ( bytetemp & 0x0f );
                     F = ( F & FLAG_C ) | sz53p_table[A];
                  }
                  break;
               case 0x68:		/* IN L,(C) */
                  Z80_IN( L, BC );
                  break;
               case 0x69:		/* OUT (C),L */
                  Z80_WP_MACRO( BC, L );
                  break;
               case 0x6a:		/* ADC HL,HL */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  ADC16(HL);
                  break;
               case 0x6b:		/* LD HL,(nnnn) */
                  LD16_RRNN(L,H);
               case 0x6f:		/* RLD */
                  {
                     uint8 bytetemp = Z80_RB_MACRO( HL );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     Z80_WB_MACRO(HL, (bytetemp << 4 ) | ( A & 0x0f ) );
                     A = ( A & 0xf0 ) | ( bytetemp >> 4 );
                     F = ( F & FLAG_C ) | sz53p_table[A];
                  }
                  break;
               case 0x70:		/* IN F,(C) */
                  {
                     uint8 bytetemp;
                     Z80_IN( bytetemp, BC );
                  }
                  break;
               case 0x71:		/* OUT (C),0 */
                  Z80_WP_MACRO( BC, 0 );
                  break;
               case 0x72:		/* SBC HL,SP */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  SBC16(SP);
                  break;
               case 0x73:		/* LD (nnnn),SP */
                  LD16_NNRR(SPL,SPH);
               case 0x78:		/* IN A,(C) */
                  Z80_IN( A, BC );
                  break;
               case 0x79:		/* OUT (C),A */
                  Z80_WP_MACRO( BC, A );
                  break;
               case 0x7a:		/* ADC HL,SP */
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  contend_read_no_mreq( IR, 1 );
                  ADC16(SP);
                  break;
               case 0x7b:		/* LD SP,(nnnn) */
                  LD16_RRNN(SPL,SPH);
               case 0xa0:		/* LDI */
                  {
                     uint8 bytetemp=Z80_RB_MACRO( HL );
                     BC--;
                     Z80_WB_MACRO(DE,bytetemp);
                     contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                     DE++; HL++;
                     bytetemp += A;
                     F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
                        ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
                  }
                  break;
               case 0xa1:		/* CPI */
                  {
                     uint8 value = Z80_RB_MACRO( HL ), bytetemp = A - value,
                           lookup = ( (        A & 0x08 ) >> 3 ) |
                              ( (  (value) & 0x08 ) >> 2 ) |
                              ( ( bytetemp & 0x08 ) >> 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 );
                     HL++; BC--;
                     F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
                        halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
                        ( bytetemp & FLAG_S );
                     if(F & FLAG_H) bytetemp--;
                     F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
                  }
                  break;
               case 0xa2:		/* INI */
                  {
                     uint8 initemp, initemp2;

                     contend_read_no_mreq( IR, 1 );
                     initemp = Z80_RP_MACRO( BC );
                     Z80_WB_MACRO( HL, initemp );

                     B--; HL++;
                     initemp2 = initemp + C + 1;
                     F = ( initemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];
                  }
                  break;
               case 0xa3:		/* OUTI */
                  {
                     uint8 outitemp, outitemp2;

                     contend_read_no_mreq( IR, 1 );
                     outitemp = Z80_RB_MACRO( HL );
                     B--;	/* This does happen first, despite what the specs say */
                     Z80_WP_MACRO(BC,outitemp);

                     HL++;
                     outitemp2 = outitemp + L;
                     F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];
                  }
                  break;
               case 0xa8:		/* LDD */
                  {
                     uint8 bytetemp=Z80_RB_MACRO( HL );
                     BC--;
                     Z80_WB_MACRO(DE,bytetemp);
                     contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                     DE--; HL--;
                     bytetemp += A;
                     F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
                        ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
                  }
                  break;
               case 0xa9:		/* CPD */
                  {
                     uint8 value = Z80_RB_MACRO( HL ), bytetemp = A - value,
                           lookup = ( (        A & 0x08 ) >> 3 ) |
                              ( (  (value) & 0x08 ) >> 2 ) |
                              ( ( bytetemp & 0x08 ) >> 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 );
                     HL--; BC--;
                     F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
                        halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
                        ( bytetemp & FLAG_S );
                     if(F & FLAG_H) bytetemp--;
                     F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
                  }
                  break;
               case 0xaa:		/* IND */
                  {
                     uint8 initemp, initemp2;

                     contend_read_no_mreq( IR, 1 );
                     initemp = Z80_RP_MACRO( BC );
                     Z80_WB_MACRO( HL, initemp );

                     B--; HL--;
                     initemp2 = initemp + C - 1;
                     F = ( initemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];
                  }
                  break;
               case 0xab:		/* OUTD */
                  {
                     uint8 outitemp, outitemp2;

                     contend_read_no_mreq( IR, 1 );
                     outitemp = Z80_RB_MACRO( HL );
                     B--;	/* This does happen first, despite what the specs say */
                     Z80_WP_MACRO(BC,outitemp);

                     HL--;
                     outitemp2 = outitemp + L;
                     F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];
                  }
                  break;
               case 0xb0:		/* LDIR */
                  {
                     uint8 bytetemp=Z80_RB_MACRO( HL );
                     Z80_WB_MACRO(DE,bytetemp);
                     contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                     BC--;
                     bytetemp += A;
                     F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
                        ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
                     if(BC) {
                        contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                        contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                        contend_write_no_mreq( DE, 1 );
                        PC-=2;
                     }
                     HL++; DE++;
                  }
                  break;
               case 0xb1:		/* CPIR */
                  {
                     uint8 value = Z80_RB_MACRO( HL ), bytetemp = A - value,
                           lookup = ( (        A & 0x08 ) >> 3 ) |
                              ( (  (value) & 0x08 ) >> 2 ) |
                              ( ( bytetemp & 0x08 ) >> 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 );
                     BC--;
                     F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
                        halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
                        ( bytetemp & FLAG_S );
                     if(F & FLAG_H) bytetemp--;
                     F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
                     if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
                        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                        contend_read_no_mreq( HL, 1 );
                        PC-=2;
                     }
                     HL++;
                  }
                  break;
               case 0xb2:		/* INIR */
                  {
                     uint8 initemp, initemp2;

                     contend_read_no_mreq( IR, 1 );
                     initemp = Z80_RP_MACRO( BC );
                     Z80_WB_MACRO( HL, initemp );

                     B--;
                     initemp2 = initemp + C + 1;
                     F = ( initemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];

                     if( B ) {
                        contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
                        contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
                        contend_write_no_mreq( HL, 1 );
                        PC -= 2;
                     }
                     HL++;
                  }
                  break;
               case 0xb3:		/* OTIR */
                  {
                     uint8 outitemp, outitemp2;

                     contend_read_no_mreq( IR, 1 );
                     outitemp = Z80_RB_MACRO( HL );
                     B--;	/* This does happen first, despite what the specs say */
                     Z80_WP_MACRO(BC,outitemp);

                     HL++;
                     outitemp2 = outitemp + L;
                     F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];

                     if( B ) {
                        contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
                        contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
                        contend_read_no_mreq( BC, 1 );
                        PC -= 2;
                     }
                  }
                  break;
               case 0xb8:		/* LDDR */
                  {
                     uint8 bytetemp=Z80_RB_MACRO( HL );
                     Z80_WB_MACRO(DE,bytetemp);
                     contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                     BC--;
                     bytetemp += A;
                     F = ( F & ( FLAG_C | FLAG_Z | FLAG_S ) ) | ( BC ? FLAG_V : 0 ) |
                        ( bytetemp & FLAG_3 ) | ( (bytetemp & 0x02) ? FLAG_5 : 0 );
                     if(BC) {
                        contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                        contend_write_no_mreq( DE, 1 ); contend_write_no_mreq( DE, 1 );
                        contend_write_no_mreq( DE, 1 );
                        PC-=2;
                     }
                     HL--; DE--;
                  }
                  break;
               case 0xb9:		/* CPDR */
                  {
                     uint8 value = Z80_RB_MACRO( HL ), bytetemp = A - value,
                           lookup = ( (        A & 0x08 ) >> 3 ) |
                              ( (  (value) & 0x08 ) >> 2 ) |
                              ( ( bytetemp & 0x08 ) >> 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                     contend_read_no_mreq( HL, 1 );
                     BC--;
                     F = ( F & FLAG_C ) | ( BC ? ( FLAG_V | FLAG_N ) : FLAG_N ) |
                        halfcarry_sub_table[lookup] | ( bytetemp ? 0 : FLAG_Z ) |
                        ( bytetemp & FLAG_S );
                     if(F & FLAG_H) bytetemp--;
                     F |= ( bytetemp & FLAG_3 ) | ( (bytetemp&0x02) ? FLAG_5 : 0 );
                     if( ( F & ( FLAG_V | FLAG_Z ) ) == FLAG_V ) {
                        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                        contend_read_no_mreq( HL, 1 ); contend_read_no_mreq( HL, 1 );
                        contend_read_no_mreq( HL, 1 );
                        PC-=2;
                     }
                     HL--;
                  }
                  break;
               case 0xba:		/* INDR */
                  {
                     uint8 initemp, initemp2;

                     contend_read_no_mreq( IR, 1 );
                     initemp = Z80_RP_MACRO( BC );
                     Z80_WB_MACRO( HL, initemp );

                     B--;
                     initemp2 = initemp + C - 1;
                     F = ( initemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( initemp2 < initemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( initemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];

                     if( B ) {
                        contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
                        contend_write_no_mreq( HL, 1 ); contend_write_no_mreq( HL, 1 );
                        contend_write_no_mreq( HL, 1 );
                        PC -= 2;
                     }
                     HL--;
                  }
                  break;
               case 0xbb:		/* OTDR */
                  {
                     uint8 outitemp, outitemp2;

                     contend_read_no_mreq( IR, 1 );
                     outitemp = Z80_RB_MACRO( HL );
                     B--;	/* This does happen first, despite what the specs say */
                     Z80_WP_MACRO(BC,outitemp);

                     HL--;
                     outitemp2 = outitemp + L;
                     F = ( outitemp & 0x80 ? FLAG_N : 0 ) |
                        ( ( outitemp2 < outitemp ) ? FLAG_H | FLAG_C : 0 ) |
                        ( parity_table[ ( outitemp2 & 0x07 ) ^ B ] ? FLAG_P : 0 ) |
                        sz53_table[B];

                     if( B ) {
                        contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
                        contend_read_no_mreq( BC, 1 ); contend_read_no_mreq( BC, 1 );
                        contend_read_no_mreq( BC, 1 );
                        PC -= 2;
                     }
                  }
                  break;
               case 0xfb:		/* slttrap */
                  //slt_trap( HL, A );
                  break;
               default:		/* All other opcodes are NOPD */
                  break;
            }
         }
         break;
      case 0xee:		/* XOR A,nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            XOR(bytetemp);
         }
         break;
      case 0xef:		/* RST 28 */
         contend_read_no_mreq( IR, 1 );
         RST(0x28);
         break;
      case 0xf0:		/* RET P */
         contend_read_no_mreq( IR, 1 );
         if( ! ( F & FLAG_S ) ) { RET(); }
         break;
      case 0xf1:		/* POP AF */
         POP16(F,A);
         break;
      case 0xf2:		/* JP P,nnnn */
         if( ! ( F & FLAG_S ) ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xf3:		/* DI */
         IFF1=IFF2=0;
         break;
      case 0xf4:		/* CALL P,nnnn */
         if( ! ( F & FLAG_S ) ) {
            CALL();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xf5:		/* PUSH AF */
         contend_read_no_mreq( IR, 1 );
         PUSH16(F,A);
         break;
      case 0xf6:		/* OR nn */
         {
            uint8 bytetemp = Z80_RB_MACRO( PC++ );
            OR(bytetemp);
         }
         break;
      case 0xf7:		/* RST 30 */
         contend_read_no_mreq( IR, 1 );
         RST(0x30);
         break;
      case 0xf8:		/* RET M */
         contend_read_no_mreq( IR, 1 );
         if( F & FLAG_S ) { RET(); }
         break;
      case 0xf9:		/* LD SP,HL */
         contend_read_no_mreq( IR, 1 );
         contend_read_no_mreq( IR, 1 );
         SP = HL;
         break;
      case 0xfa:		/* JP M,nnnn */
         if( F & FLAG_S ) {
            JP();
         } else {
            contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
         }
         break;
      case 0xfb:		/* EI */
         /* Interrupts are not accepted immediately after an EI, but are
            accepted after the next instruction */
               IFF1 = IFF2 = 1;
            z80.interrupts_enabled_at = z80_tstates;
            //event_add( z80_tstates + 1, z80_interrupt_event );
            break;
      case 0xfc:		/* CALL M,nnnn */
            if( F & FLAG_S ) {
               CALL();
            } else {
               contend_read( PC, 3 ); contend_read( PC + 1, 3 ); PC += 2;
            }
            break;
      case 0xfd:		/* shift FD */
            {
               uint8 opcode2;
               opcode2 = Z80_RB_MACRO( PC );
               z80_tstates++;
               PC++;
               R++;
               switch(opcode2) {
#define REGISTER  IY
#define REGISTERL IYL
#define REGISTERH IYH
#include "z80_ddfd.c"
#undef REGISTERH
#undef REGISTERL
#undef REGISTER
               }
            }
            break;
      case 0xfe:		/* CP nn */
            {
               uint8 bytetemp = Z80_RB_MACRO( PC++ );
               CP(bytetemp);
            }
            break;
      case 0xff:		/* RST 38 */
            contend_read_no_mreq( IR, 1 );
            RST(0x38);
            break;
   }

   ret              = z80_tstates - last_z80_tstates;
   last_z80_tstates = z80_tstates;

   //printf("PC: %04x, %02x, time=%d\n", lastpc, opcode, ret);

   return ret;
}
