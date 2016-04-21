#ifndef UTIL_MISC
#define UTIL_MISC

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parameters:
//  log_debug_mode        : debug mode 0 print only error, 2 print all

#define ANSI_COLOR_WHITE "\x1b[00m"
#define ANSI_COLOR_BLACK "\x1b[30m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_LGRAY "\x1b[37m"
#define ANSI_COLOR_BG_BLACK "\x1b[40m"
#define ANSI_COLOR_BG_RED "\x1b[41m"
#define ANSI_COLOR_BG_GREEN "\x1b[42m"
#define ANSI_COLOR_BG_YELLOW "\x1b[43m"
#define ANSI_COLOR_BG_BLUE "\x1b[44m"
#define ANSI_COLOR_BG_MAGENTA "\x1b[45m"
#define ANSI_COLOR_BG_CYAN "\x1b[46m"
#define ANSI_COLOR_BG_LGRAY "\x1b[47m"
#define ANSI_COLOR_RESET "\x1b[0m"

int log_debug_mode = 2;

// print only if log_dbg_level is small enough -- 0 being always printed
#define LOG(log_dbg_level, ...)            \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      printf(__VA_ARGS__);                 \
      fflush(stdout);                      \
    }                                      \
  })
#define ARR_CLONE(d, s, l)                                       \
  ({                                                             \
    d = (__typeof__(s[0])*)malloc(l * sizeof(__typeof__(s[0]))); \
    int i;                                                       \
    for (i = 0; i < l; i++) d[i] = s[i];                         \
  })

void printfc(char fg_color_code, char bg_color_code, const char* format, ...) {
  char f = (fg_color_code >= 'A' && fg_color_code <= 'Z')
               ? fg_color_code + 'a' - 'A'
               : fg_color_code;
  char b = (bg_color_code >= 'A' && bg_color_code <= 'Z')
               ? bg_color_code + 'a' - 'A'
               : bg_color_code;
  va_list args;
  va_start(args, format);
  switch (f) {
    case 'w':
      printf(ANSI_COLOR_WHITE);
      break;
    case 'k':
      printf(ANSI_COLOR_BLACK);
      break;
    case 'r':
      printf(ANSI_COLOR_RED);
      break;
    case 'g':
      printf(ANSI_COLOR_GREEN);
      break;
    case 'y':
      printf(ANSI_COLOR_YELLOW);
      break;
    case 'b':
      printf(ANSI_COLOR_BLUE);
      break;
    case 'm':
      printf(ANSI_COLOR_MAGENTA);
      break;
    case 'c':
      printf(ANSI_COLOR_CYAN);
      break;
    case 'l':
      printf(ANSI_COLOR_LGRAY);
      break;
  }
  switch (b) {
    case 'k':
      printf(ANSI_COLOR_BG_BLACK);
      break;
    case 'r':
      printf(ANSI_COLOR_BG_RED);
      break;
    case 'g':
      printf(ANSI_COLOR_BG_GREEN);
      break;
    case 'y':
      printf(ANSI_COLOR_BG_YELLOW);
      break;
    case 'b':
      printf(ANSI_COLOR_BG_BLUE);
      break;
    case 'm':
      printf(ANSI_COLOR_BG_MAGENTA);
      break;
    case 'c':
      printf(ANSI_COLOR_BG_CYAN);
      break;
    case 'l':
      printf(ANSI_COLOR_BG_LGRAY);
      break;
  }
  va_end(args);
  vprintf(format, args);
  printf(ANSI_COLOR_RESET);
}

#endif /* ifndef UTIL_MISC */
