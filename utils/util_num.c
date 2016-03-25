#ifndef UTIL_NUM
#define UTIL_NUM

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_misc.c"

///////////////////////
// Numeric Functions //
///////////////////////
#ifndef real
#define real double
#endif

#define MAX(arr, l)                                        \
  ({                                                       \
    __typeof__(arr[0]) x = arr[0];                         \
    int i;                                                 \
    for (i = 1; i < l; i++) x = (x < arr[i]) ? arr[i] : x; \
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

int NUM_MAX_PRINT_ELEM = 10;
double NUM_EPS = 1e-6;

void NumPrintArr(char *name, real *arr, int l) {
  int i = 0;
  char name10[11] = {0};
  for (i = 0; i < 10; i++) {
    if (i < strlen(name))
      name10[i] = name[i];
    else
      name10[i] = ' ';
  }
  printf("%10s: ", name10);
  for (i = 0; i < l; i++)
    if (i < NUM_MAX_PRINT_ELEM - 2 || l - i <= 2) {
      if (-NUM_EPS <= arr[i] && arr[i] <= NUM_EPS)
        printf("%12.6g ", 0.0);
      else
        printf("%12.6g ", arr[i]);
    }
  printf("\n");
  fflush(stdout);
  return;
}

void NumPrintMatrix(char *name, real *arr, int m, int n) {
  int i;
  for (i = 0; i < m; i++) {
    if (i < NUM_MAX_PRINT_ELEM - 2 || m - i <= 2) {
      if (i == 0)
        NumPrintArr(name, arr, n);
      else
        NumPrintArr("", arr + i * n, n);
    }
    printf("\n");
  }
  fflush(stdout);
  return;
}

void NumPrintArrAbsMaxColor(char *name, real *arr, int l) {
  int i = 0;
  char name10[11] = {0};
  for (i = 0; i < 10; i++) {
    if (i < strlen(name))
      name10[i] = name[i];
    else
      name10[i] = ' ';
  }
  printf("%10s: ", name10);
  real x = ABSMAX(arr, l);
  for (i = 0; i < l; i++) {
    if (ABS(arr[i]) == x)
      printfc('r', 'k', "%12.6g ", arr[i]);
    else if (-NUM_EPS <= arr[i] && arr[i] <= NUM_EPS)
      printf("%12.6g ", 0.0);
    else
      printf("%12.6g ", arr[i]);
  }
  printf("\n");
  fflush(stdout);
}

void NumNewHugeVec(void **ptr, long long int mem_size) {
  if (posix_memalign(ptr, 128, mem_size)) {
    LOG(0, "[NumNewHugeVec]: memory allocation failed!");
    exit(1);
  }
  return;
}

void NumReadVec(real *ptr, long long int mem_size, FILE *fin) {
  long long int actual_read_size = fread(ptr, sizeof(real), mem_size, fin);
  if (actual_read_size != mem_size) {
    printf("[NumReadVec]: read error!");
    exit(1);
  }
  return;
}

real NumVecNorm(real *arr, int l) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += arr[i] * arr[i];
  return sqrt(s);
}

real NumVecDot(real *a, real *b, int l) {
  real s = 0;
  int i;
  for (i = 0; i < l; i++) s += a[i] * b[i];
  return s;
}

real NumVecL2Dist(real *a, real *b, int l) {
  real s = 0;
  int i;
  for (i = 0; i < l; i++) s += (a[i] - b[i]) * (a[i] - b[i]);
  return s;
}

real NumVecCos(real *a, real *b, int l) {
  return NumVecDot(a, b, l) / (NumVecNorm(a, l) + NUM_EPS) /
         (NumVecNorm(b, l) + NUM_EPS);
}

#endif /* end of include guard: UTIL_NUM */

/*
void InitConstant() {
  seed = 0x0F0F0F0FL;
  SMN_SIZE = 3 * s3eM * s3eM * s3eN + 2 * s3eM * s3eM + s3eN;
  DSMN_SIZE = 3 * s3eM * s3eM * s3eN + s3eM * s3eM + 2 * s3eN;
  SYN_SIZE = s3eM * (s3eM + 1) * (s3eM + 1);
  return;
}

void Perm_Gen(int i, int j, int *word_ids, int *neg_word_ids, int l) {
  int k;
  for (k = 0; k < l; k++) neg_word_ids[k] = word_ids[k];
  SWAP(neg_word_ids[i], neg_word_ids[j]);
  return;
}

void Flip_Gen(int i, int w, int *word_ids, int *neg_word_ids, int l) {
  int k;
  for (k = 0; k < l; k++) neg_word_ids[k] = word_ids[k];
  neg_word_ids[i] = w;
  return;
}

/////////////
// Numeric //
/////////////
void InitSquashTable() {
  AllocAlignMem("s3eExpTable", (void **)&s3eExpTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  AllocAlignMem("s3eSigmTable", (void **)&s3eSigmTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  AllocAlignMem("s3eTanhTable", (void **)&s3eTanhTable,
                (long long)(s3eSquashTableSize + 1) * sizeof(real));
  int i;
  for (i = 0; i <= s3eSquashTableSize; i++) {
    real r =
        exp(((real)i / s3eSquashTableSize * 2 - 1.0) * s3eSquashTableRange);
    s3eExpTable[i] = r;
    s3eSigmTable[i] = r / (r + 1.0);
    s3eTanhTable[i] = (r * r - 1.0) / (r * r + 1.0);
  }
  return;
}
*/
