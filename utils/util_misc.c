#ifndef UTIL_MISC
#define UTIL_MISC

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int log_debug_mode = 2;

#define LOG(log_dbg_level, ...)            \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printf(__VA_ARGS__);                 \
      fflush(stdout);                      \
    }                                      \
  })

#endif /* ifndef UTIL_MISC */
