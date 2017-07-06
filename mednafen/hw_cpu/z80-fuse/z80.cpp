/* z80.c: z80 supplementary functions
   Copyright (c) 1999-2003 Philip Kendall

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

#include "z80.h"
#include "z80_macros.h"

#include "../../state.h"

/* Whether a half carry occurred or not can be determined by looking at
   the 3rd bit of the two arguments and the result; these are hashed
   into this table in the form r12, where r is the 3rd bit of the
   result, 1 is the 3rd bit of the 1st argument and 2 is the
   third bit of the 2nd argument; the tables differ for add and subtract
   operations */
const uint8 halfcarry_add_table[] =
  { 0, FLAG_H, FLAG_H, FLAG_H, 0, 0, 0, FLAG_H };
const uint8 halfcarry_sub_table[] =
  { 0, 0, FLAG_H, 0, FLAG_H, 0, FLAG_H, FLAG_H };


/* Some more tables; initialised in z80_init_tables() */

/* This is what everything acts on! */
processor z80;

static void z80_init_tables(void);

/* Set up the z80 emulation */
void z80_init( void )
{
   z80_init_tables();
}

/* Initialise the tables used to set flags */
static void z80_init_tables(void)
{
  int i;

  for(i = 0; i < 0x100; i++)
  {
     int j        = i;
     int k        = 0;
     uint8 parity = 0;

     sz53_table[i]= i & ( FLAG_3 | FLAG_5 | FLAG_S );

     for(; k < 8; k++)
     {
        parity ^= j & 1;
        j >>=1;
     }

     parity_table[i] = ( parity ? 0 : FLAG_P );
     sz53p_table[i]  = sz53_table[i] | parity_table[i];
  }

  sz53_table[0]  |= FLAG_Z;
  sz53p_table[0] |= FLAG_Z;

}

/* Reset the z80 */
void z80_reset( void )
{
  AF =BC =DE =HL =0;
  AF_=BC_=DE_=HL_=0;
  IX=IY=0;
  I=R=R7=0;
  SP=PC=0;
  IFF1=IFF2=IM=0;
  z80.halted=0;

  z80.interrupts_enabled_at = -1;
  z80_tstates = last_z80_tstates = 0;
}

/* Process a z80 maskable interrupt */
int z80_interrupt( void )
{
   if (!IFF1)
      return 0;			/* Did not accept an interrupt */

   /* If interrupts have just been enabled, don't accept the interrupt now,
      but check after the next instruction has been executed */
   if(z80_tstates == z80.interrupts_enabled_at)
      return 0;

   if(z80.halted)
   {
      PC++;
      z80.halted = 0;
   }

   IFF1 = IFF2 = 0;

   Z80_WB_MACRO( --SP, PCH );
   Z80_WB_MACRO( --SP, PCL );

   R++;

   switch(IM)
   {
      case 0:
         PC = 0x0038;
         z80_tstates += 7;
         break;
      case 1:
         PC = 0x0038;
         z80_tstates += 7;
         break;
      case 2: 
         {
            uint16 inttemp=(0x100*I)+0xff;
            PCL = Z80_RB_MACRO(inttemp++); PCH = Z80_RB_MACRO(inttemp);
            z80_tstates += 7;
            break;
         }
   }

   return 1;			/* Accepted an interrupt */
}

/* Process a z80 non-maskable interrupt */
void z80_nmi(void)
{
  if(z80.halted)
  {
     PC++;
     z80.halted = 0;
  }

  IFF1 = 0;

  Z80_WB_MACRO( --SP, PCH );
  Z80_WB_MACRO( --SP, PCL );

  /* FIXME: how is R affected? */

  /* FIXME: how does contention apply here? */
  z80_tstates += 11;
  PC = 0x0066;
}

int z80_state_action(void *data, int load, int data_only, const char *section_name)
{
   uint8 r_register;

   SFORMAT StateRegs[] =
   {
      { &(z80.af.w), sizeof(z80.af.w), 0x80000000, "AF" },
      { &(z80.bc.w), sizeof(z80.bc.w), 0x80000000, "BC" },
      { &(z80.de.w), sizeof(z80.de.w), 0x80000000, "DE" },
      { &(z80.hl.w), sizeof(z80.hl.w), 0x80000000, "HL" },
      { &(z80.af_.w), sizeof(z80.af_.w), 0x80000000, "AF_" },
      { &(z80.bc_.w), sizeof(z80.bc_.w), 0x80000000, "BC_" },
      { &(z80.de_.w), sizeof(z80.de_.w), 0x80000000, "DE_" },
      { &(z80.hl_.w), sizeof(z80.hl_.w), 0x80000000, "HL_" },
      { &(z80.ix.w), sizeof(z80.ix.w), 0x80000000, "IX" },
      { &(z80.iy.w), sizeof(z80.iy.w), 0x80000000, "IY" },
      { &(z80.i), sizeof(z80.i), 0x80000000, "I" },
      { &(z80.sp.w), sizeof(z80.sp.w), 0x80000000, "SP" },
      { &(z80.pc.w), sizeof(z80.pc.w), 0x80000000, "PC" },
      { &(z80.iff1), sizeof(z80.iff1), 0x80000000, "IFF1" },
      { &(z80.iff2), sizeof(z80.iff2), 0x80000000, "IFF2" },
      { &(z80.im), sizeof(z80.im), 0x80000000, "IM" },
      { &(r_register), sizeof(r_register), 0x80000000, "R" },

      { &(z80.interrupts_enabled_at), sizeof(z80.interrupts_enabled_at), 0x80000000, "interrupts_enabled_at" },
      { &(z80.halted), sizeof(z80.halted), 0x80000000, "halted" },

      { &((z80_tstates)), sizeof((z80_tstates)), 0x80000000, "z80_tstates" },
      { &((last_z80_tstates)), sizeof((last_z80_tstates)), 0x80000000, "last_z80_tstates" },

      { 0, 0, 0, 0 }
   };

   if(!load)
      r_register = (z80.r7 & 0x80) | (z80.r & 0x7f);

   if(!MDFNSS_StateAction(data, load, data_only, StateRegs, section_name))
      return(0);

   if(load)
   {
      z80.r7 = r_register & 0x80;
      z80.r = r_register & 0x7F;
   }

   return(1);
}

