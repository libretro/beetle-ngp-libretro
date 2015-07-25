#ifndef __MDFN_PSX_MASMEM_H
#define __MDFN_PSX_MASMEM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

static INLINE uint16_t LoadU16_RBO(const uint16_t *a)
{
#ifdef ARCH_POWERPC
   uint16_t tmp;
   __asm__ ("lhbrx %0, %y1" : "=r"(tmp) : "Z"(*a));
   return(tmp);
#else
   return((*a << 8) | (*a >> 8));
#endif
}

static INLINE uint32_t LoadU32_RBO(const uint32_t *a)
{
#ifdef ARCH_POWERPC
   uint32_t tmp;

   __asm__ ("lwbrx %0, %y1" : "=r"(tmp) : "Z"(*a));

   return(tmp);
#else
   uint32_t tmp = *a;
   return((tmp << 24) | ((tmp & 0xFF00) << 8) | ((tmp >> 8) & 0xFF00) | (tmp >> 24));
#endif
}

static INLINE void StoreU16_RBO(uint16_t *a, const uint16_t v)
{
#ifdef ARCH_POWERPC
   __asm__ ("sthbrx %0, %y1" : : "r"(v), "Z"(*a));
#else
   uint16_t tmp = (v << 8) | (v >> 8);
   *a = tmp;
#endif
}

static INLINE void StoreU32_RBO(uint32_t *a, const uint32_t v)
{
#ifdef ARCH_POWERPC
   __asm__ ("stwbrx %0, %y1" : : "r"(v), "Z"(*a));
#else
   uint32_t tmp = (v << 24) | ((v & 0xFF00) << 8) | ((v >> 8) & 0xFF00) | (v >> 24);
   *a = tmp;
#endif
}

#ifdef __cplusplus
}
#endif

#endif
