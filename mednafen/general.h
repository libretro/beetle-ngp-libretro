#ifndef _GENERAL_H
#define _GENERAL_H

#include <string.h>

typedef enum
{
 MDFNMKF_STATE = 0,
 MDFNMKF_SAV,
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
