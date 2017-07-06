#include "../../mednafen-types.h"

void (*z80_writebyte)(uint16 a, uint8 b);
uint8 (*z80_readbyte)(uint16 a);
void (*z80_writeport)(uint16 a, uint8 b);
uint8 (*z80_readport)(uint16 a);
