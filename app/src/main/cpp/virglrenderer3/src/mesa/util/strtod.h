/*
 * strtod.h - Shim for Mesa 24.3.4 TGSI compatibility.
 * On Android, we can use the standard strtod/strtof directly.
 */
#ifndef STRTOD_H
#define STRTOD_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline double
_mesa_strtod(const char *s, char **end)
{
   return strtod(s, end);
}

static inline float
_mesa_strtof(const char *s, char **end)
{
   return strtof(s, end);
}

#ifdef __cplusplus
}
#endif

#endif /* STRTOD_H */

