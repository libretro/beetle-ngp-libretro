#ifndef MDFN_SETTINGS_H
#define MDFN_SETTINGS_H

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t setting_ngp_language;

uint64_t MDFN_GetSettingUI(const char *name);
int64_t MDFN_GetSettingI(const char *name);
double MDFN_GetSettingF(const char *name);
bool MDFN_GetSettingB(const char *name);

bool MDFNI_SetSettingB(const char *name, bool value);
bool MDFNI_SetSettingUI(const char *name, uint64_t value);

#ifdef __cplusplus
}
#endif

#endif
