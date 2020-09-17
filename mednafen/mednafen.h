#ifndef _MEDNAFEN_H
#define _MEDNAFEN_H

#include "mednafen-types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "git.h"

#ifdef _WIN32
#define strcasecmp _stricmp
#endif

extern MDFNGI EmulatedNGP;

#include "settings.h"

void MDFN_LoadGameCheats(void *override);
void MDFN_FlushGameCheats(int nosave);

#include "mednafen-driver.h"

#endif
