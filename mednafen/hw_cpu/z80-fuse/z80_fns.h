#include "../../mednafen-types.h"

#ifndef FLAG_V
#define FLAG_V	0x04
#endif

void (*z80_writebyte)(uint16 a, uint8 b);
uint8 (*z80_readbyte)(uint16 a);
void (*z80_writeport)(uint16 a, uint8 b);
uint8 (*z80_readport)(uint16 a);

uint64 z80_tstates;
uint64 last_z80_tstates;
uint8 sz53_table[0x100]; /* The S, Z, 5 and 3 bits of the index */
uint8 parity_table[0x100]; /* The parity of the lookup value */
uint8 sz53p_table[0x100]; /* OR the above two tables together */

/* Similarly, overflow can be determined by looking at the 7th bits; again
   the hash into this table is r12 */
const uint8 overflow_add_table[] = { 0, 0, 0, FLAG_V, FLAG_V, 0, 0, 0 };
const uint8 overflow_sub_table[] = { 0, FLAG_V, 0, 0, 0, 0, FLAG_V, 0 };
