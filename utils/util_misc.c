#ifndef UTIL_MISC
#define UTIL_MISC

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// util_misc.c shall not include other util_XXXX files but can be included
// sid_module_name prefix indicate functions/variables shall not be accessed
//    directly; for variables, a suffix '_' is used.
// Parameters:
//  log_debug_mode        : debug mode 0 print only error, 2 print all

/***
 *      #####
 *     #     # ##### #####  # #    #  ####
 *     #         #   #    # # ##   # #    #
 *      #####    #   #    # # # #  # #
 *           #   #   #####  # #  # # #  ###
 *     #     #   #   #   #  # #   ## #    #
 *      #####    #   #    # # #    #  ####
 *
 */
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

char *log_affix_str = NULL;
// print only if log_dbg_level is small enough -- 0 being always printed
#define LOG(log_dbg_level, ...)            \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      if (log_affix_str) {                 \
        printf("%s: ", log_affix_str);     \
      }                                    \
      printf(__VA_ARGS__);                 \
      fflush(stdout);                      \
    }                                      \
  })
#define LOGC(log_dbg_level, ...)           \
  ({                                       \
    if (log_debug_mode >= log_dbg_level) { \
      if (log_affix_str) {                 \
        printf("%s: ", log_affix_str);     \
      }                                    \
      printfc(__VA_ARGS__);                \
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
#define LOWER(c) (((c) >= 'A' && (c) <= 'Z') ? (c) - 'A' + 'a' : (c))
#define UPPER(c) (((c) >= 'a' && (c) <= 'z') ? (c) - 'a' + 'A' : (c))
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
  char s[0x1000] = {0};
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
  char s[0x1000] = {0};
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

char *strtime(int s) {
  int ss = s;
  int mm = ss / 60;
  ss %= 60;
  int hh = mm / 60;
  mm %= 60;
  int dd = hh / 24;
  hh %= 24;
  return sformat("%02d:%02d:%02d:%02d", dd, hh, mm, ss);
}

/***
 *     #######
 *     #       # #      ######
 *     #       # #      #
 *     #####   # #      #####
 *     #       # #      #
 *     #       # #      #
 *     #       # ###### ######
 *
 */
int fexists(const char *filename) {
  // however, this also return true if filename is a directory
  // which might not be desired
  FILE *file = fopen(filename, "r");
  if (file) fclose(file);
  return (file != 0);
}

/***
 *        #
 *       # #   #       ####   ####  #####  # ##### #    # #    #
 *      #   #  #      #    # #    # #    # #   #   #    # ##  ##
 *     #     # #      #      #    # #    # #   #   ###### # ## #
 *     ####### #      #  ### #    # #####  #   #   #    # #    #
 *     #     # #      #    # #    # #   #  #   #   #    # #    #
 *     #     # ######  ####   ####  #    # #   #   #    # #    #
 *
 */
#ifndef real
#define real double
#endif

#define SMALLER(a, b) ((a) <= (b) ? (a) : (b))
#define LARGER(a, b) ((a) >= (b) ? (a) : (b))
#define MAX(arr, l)                                        \
  ({                                                       \
    __typeof__(arr[0]) x = arr[0];                         \
    int i;                                                 \
    for (i = 1; i < l; i++) x = (x < arr[i]) ? arr[i] : x; \
    x;                                                     \
  })
#define MIN(arr, l)                                        \
  ({                                                       \
    __typeof__(arr[0]) x = arr[0];                         \
    int i;                                                 \
    for (i = 1; i < l; i++) x = (x > arr[i]) ? arr[i] : x; \
    x;                                                     \
  })
#define ABS(x) ((x) < 0 ? -(x) : (x))
#define ABSMAX(arr, l)                                               \
  ({                                                                 \
    __typeof__(arr[0]) x = ABS(arr[0]);                              \
    int i;                                                           \
    for (i = 1; i < l; i++) x = (x < ABS(arr[i])) ? ABS(arr[i]) : x; \
    x;                                                               \
  })
#define SWAP(x, y)       \
  ({                     \
    __typeof__(x) z = x; \
    x = y;               \
    y = z;               \
  })

int sign(real x) { return (x == 0 ? 0 : (x < 0 ? -1 : 1)); }
int signi(int x) { return (x == 0 ? 0 : (x < 0 ? -1 : 1)); }

void range(int *idx, int size) {
  int i;
  for (i = 0; i < size; i++) idx[i] = i;
  return;
}

typedef struct pair {
  int key;
  real val;
} pair;
pair *array2tuples(real *arr, int size) {
  int i;
  pair *p = (pair *)malloc(sizeof(pair) * size);
  for (i = 0; i < size; i++) {
    p[i].key = i;
    p[i].val = arr[i];
  }
  return p;
}
pair *array2tuplesi(int *arr, int size) {
  int i;
  pair *p = (pair *)malloc(sizeof(pair) * size);
  for (i = 0; i < size; i++) {
    p[i].key = i;
    p[i].val = arr[i];
  }
  return p;
}
int cmp(const void *x, const void *y) {
  return sign(((real *)x) - ((real *)y));
}
int cmp_reverse(const void *x, const void *y) {
  return sign(((real *)y) - ((real *)x));
}
int cmp_int(const void *x, const void *y) {
  return sign(((int *)x) - ((int *)y));
}
int cmp_int_reverse(const void *x, const void *y) {
  return sign(((int *)y) - ((int *)x));
}
int cmp_pair(const void *x, const void *y) {
  return sign(((pair *)x)->val - ((pair *)y)->val);
}
int cmp_pair_reverse(const void *x, const void *y) {
  return sign(((pair *)y)->val - ((pair *)x)->val);
}
pair *sort_tuples(pair *p, int size, int reverse) {
  if (reverse)
    qsort(p, size, sizeof(pair), cmp_pair_reverse);
  else
    qsort(p, size, sizeof(pair), cmp_pair);
  return p;
}
pair *sorted(real *arr, int size, int reverse) {
  return sort_tuples(array2tuples(arr, size), size, reverse);
}
pair *sortedi(int *arr, int size, int reverse) {
  return sort_tuples(array2tuplesi(arr, size), size, reverse);
}

typedef struct heap {
  pair *d;  // hold data pairsof keys and vals
  int k;    // only retain max k elements
  int size;
} heap;
// min heap: select k max number from an array
void sid_misc_heap_sift_up(pair *h, int size) {
  int c = size - 1;
  int p = (size - 2) >> 1;
  while (p >= 0 && h[c].val < h[p].val) {
    SWAP(h[c].key, h[p].key);
    SWAP(h[c].val, h[p].val);
    c = p--;
    p >>= 1;
  }
  return;
}
void sid_misc_heap_sift_down(pair *h, int size) {
  int c = 1;
  int p = 0;
  while (1) {
    if (c + 1 < size && h[c].val > h[c + 1].val) c++;
    if (c >= size || h[p].val <= h[c].val) return;
    SWAP(h[c].key, h[p].key);
    SWAP(h[c].val, h[p].val);
    p = c;
    c <<= 1;
    c++;
  }
}
heap *HeapCreate(int k) {
  heap *h = (heap *)malloc(sizeof(heap));
  h->d = (pair *)malloc(k * sizeof(pair));
  h->k = k;
  h->size = 0;
  return h;
}
void HeapDestroy(heap *h) {
  free(h->d);
  free(h);
  return;
}
void HeapEmpty(heap *h) {
  h->size = 0;
  return;
}
int HeapPush(heap *h, int key, real val) {
  if (h->size < h->k) {
    h->d[h->size].key = key;
    h->d[h->size].val = val;
    h->size++;
    sid_misc_heap_sift_up(h->d, h->size);
  } else if (val > h->d[0].val) {
    h->d[0].key = key;
    h->d[0].val = val;
    sid_misc_heap_sift_down(h->d, h->size);
  } else
    return 0;
  return 1;
}
int HeapPop(heap *h) {
  h->size--;
  SWAP(h->d[h->size].key, h->d[0].key);
  SWAP(h->d[h->size].val, h->d[0].val);
  sid_misc_heap_sift_down(h->d, h->size);
  return h->d[h->size].key;
}
int HeapSort(heap *h) {
  int size = h->size;
  while (h->size > 0) HeapPop(h);
  return size;
}
#endif /* ifndef UTIL_MISC */
