#ifndef _GENERAL_H
#define _GENERAL_H

#include <string.h>

typedef enum
{
 MDFNMKF_STATE = 0,
 MDFNMKF_SNAP,
 MDFNMKF_SAV,
 MDFNMKF_CHEAT,
 MDFNMKF_PALETTE,
 MDFNMKF_IPS,
 MDFNMKF_MOVIE,
 MDFNMKF_AUX,
 MDFNMKF_SNAP_DAT,
 MDFNMKF_CHEAT_TMP,
 MDFNMKF_FIRMWARE
} MakeFName_Type;

#ifdef __cplusplus
extern "C" {
#endif
void MDFN_MakeFName(MakeFName_Type type, char *s, size_t len,
      int id1, const char *cd1);
#ifdef __cplusplus
}
#endif

#endif
