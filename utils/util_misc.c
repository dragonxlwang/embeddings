#ifndef UTIL_MISC
#define UTIL_MISC

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// util_misc.c shall not include other util_XXXX files but can be included
// Parameters:
//  log_debug_mode        : debug mode 0 print only error, 2 print all

// w k r g y b m c l
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
#define ARR_CLONE(d, s, l)                                        \
  ({                                                              \
    d = (__typeof__(s[0]) *)malloc(l * sizeof(__typeof__(s[0]))); \
    int i;                                                        \
    for (i = 0; i < l; i++) d[i] = s[i];                          \
  })
#define STR_CLONE(d, s)                                 \
  ({                                                    \
    d = (char *)malloc((strlen(s) + 1) * sizeof(char)); \
    strcpy(d, s);                                       \
  })

char *sconcat(char *sa, char *sb, int la, int lb) {
  // if la = -1 (or lb), use whole string for concatenating
  int i;
  if (la == -1) la = strlen(sa);
  if (lb == -1) lb = strlen(sb);
  char *s = (char *)malloc(la + lb);
  for (i = 0; i < la; i++) s[i] = sa[i];
  for (i = 0; i < lb; i++) s[i + la] = sb[i];
  return s;
}

char *sformat(char *fmt, ...) {
  // sort of like sprintf, but allocate string with malloc. max length 4096
  char s[0x1000];
  char *ss;
  va_list al;
  va_start(al, fmt);
  vsprintf(s, fmt, al);
  va_end(al);
  STR_CLONE(ss, s);
  return ss;
}

char *vsformatc(char fg_color_code, char bg_color_code, const char *fmt,
                va_list al) {
  char f = (fg_color_code >= 'A' && fg_color_code <= 'Z')
               ? fg_color_code + 'a' - 'A'
               : fg_color_code;
  char b = (bg_color_code >= 'A' && bg_color_code <= 'Z')
               ? bg_color_code + 'a' - 'A'
               : bg_color_code;
  char *fg, *bg;
  switch (f) {
    case 'w':
      fg = ANSI_COLOR_WHITE;
      break;
    case 'k':
      fg = ANSI_COLOR_BLACK;
      break;
    case 'r':
      fg = ANSI_COLOR_RED;
      break;
    case 'g':
      fg = ANSI_COLOR_GREEN;
      break;
    case 'y':
      fg = ANSI_COLOR_YELLOW;
      break;
    case 'b':
      fg = ANSI_COLOR_BLUE;
      break;
    case 'm':
      fg = ANSI_COLOR_MAGENTA;
      break;
    case 'c':
      fg = ANSI_COLOR_CYAN;
      break;
    case 'l':
      fg = ANSI_COLOR_LGRAY;
      break;
    default:
      exit(1);
  }
  switch (b) {
    case 'k':
      bg = ANSI_COLOR_BG_BLACK;
      break;
    case 'r':
      bg = ANSI_COLOR_BG_RED;
      break;
    case 'g':
      bg = ANSI_COLOR_BG_GREEN;
      break;
    case 'y':
      bg = ANSI_COLOR_BG_YELLOW;
      break;
    case 'b':
      bg = ANSI_COLOR_BG_BLUE;
      break;
    case 'm':
      bg = ANSI_COLOR_BG_MAGENTA;
      break;
    case 'c':
      bg = ANSI_COLOR_BG_CYAN;
      break;
    case 'l':
      bg = ANSI_COLOR_BG_LGRAY;
      break;
    default:
      exit(1);
  }
  char s[0x1000];
  char *ss;
  strcat(s, fg);
  strcat(s, bg);
  vsprintf(s + strlen(s), fmt, al);
  strcat(s, ANSI_COLOR_RESET);
  STR_CLONE(ss, s);
  return ss;
}

char *sformatc(char fg_color_code, char bg_color_code, const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  char *s = vsformatc(fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  return s;
}

void printfc(char fg_color_code, char bg_color_code, const char *fmt, ...) {
  va_list al;
  va_start(al, fmt);
  char *s = vsformatc(fg_color_code, bg_color_code, fmt, al);
  va_end(al);
  printf("%s", s);
  free(s);
  return;
}

#endif /* ifndef UTIL_MISC */
