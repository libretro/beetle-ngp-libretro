#ifndef _MEDNAFEN_H
#define _MEDNAFEN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mednafen-types.h"
#include "git.h"
#include "settings.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

extern MDFNGI EmulatedNGP;

void MDFN_LoadGameCheats(void *override);
void MDFN_FlushGameCheats(int nosave);

#endif
