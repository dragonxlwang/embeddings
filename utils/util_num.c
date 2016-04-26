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

// Parameters:
//  NUM_EXP_TABLE_LEN          : exp table size
//  NUM_EXP_HIGH               : exp table high end
//  NUM_EXP_LOW                : exp table low end
//  NUM_MAX_PRINT_ELEM         : number of printable element in an array
//  NUM_EPS                    : machine precision
// Init:
//  NumInit()

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

#define NUM_EXP_TABLE_LEN 0xFFFFF
real NUM_EXP_TABLE[NUM_EXP_TABLE_LEN];
real NUM_EXP_HIGH = 1.0;
real NUM_EXP_LOW = -15;
real NumExp(real x) {
  int i = (x - NUM_EXP_LOW) / (NUM_EXP_HIGH - NUM_EXP_LOW) * NUM_EXP_TABLE_LEN;
  if (i < 0)
    return 0;
  else
    return NUM_EXP_TABLE[i];
}

unsigned long long int RANDOM_SEED = 0x0F0F0F0FL;
real NumRand() {
  RANDOM_SEED = ((RANDOM_SEED * 0x5DEECE66DL + 0x0A) & 0xFFFFFFFFFFFFL);
  return ((real)RANDOM_SEED / ((real)0xFFFFFFFFFFFFL));
}

void NumRandFillVec(real *arr, int l, real lb, real ub) {
  int i;
  for (i = 0; i < l; i++) arr[i] = lb + NumRand() * (ub - lb);
  return;
}

int NUM_MAX_PRINT_ELEM = 10;
real NUM_EPS = 1e-6;

void NumPrintArr(char *name, real *arr, int l) {
  int i = 0;
  char name10[11] = {0};
  for (i = 0; i < 10; i++) {
    if (i < strlen(name))
      name10[i] = name[i];
    else
      name10[i] = ' ';
  }
  printfc('g', 'k', "%10s: ", name10);
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

real *NumNewHugeVec(long long int elem_num) {
  real *ptr;
  if (posix_memalign((void **)&ptr, 128, elem_num * sizeof(real))) {
    LOG(0, "[NumNewHugeVec]: memory allocation failed!\n");
    exit(1);
  }
  return ptr;
}

int *NumNewHugeIntVec(long long int elem_num) {
  int *ptr;
  if (posix_memalign((void **)&ptr, 128, elem_num * sizeof(int))) {
    LOG(0, "[NumNewHugeVec]: memory allocation failed!\n");
    exit(1);
  }
  return ptr;
}

real *NumCLoneHugeVec(real *vec, long long int elem_num) {
  real *ptr = NumNewHugeVec(elem_num);
  memcpy(ptr, vec, elem_num * sizeof(real));
  return ptr;
}

int *NumCLoneHugeIntVec(int *vec, long long int elem_num) {
  int *ptr = NumNewHugeIntVec(elem_num);
  memcpy(ptr, vec, elem_num * sizeof(int));
  return ptr;
}

real *NumCloneVec(real *vec, int elem_num) {
  real *ptr = malloc(elem_num * sizeof(real));
  memcpy(ptr, vec, elem_num * sizeof(real));
  return ptr;
}

int *NumCloneIntVec(int *vec, int elem_num) {
  int *ptr = malloc(elem_num * sizeof(int));
  memcpy(ptr, vec, elem_num * sizeof(int));
  return ptr;
}

void NumReadVec(real *ptr, long long int elem_num, FILE *fin) {
  long long int actual_read_size = fread(ptr, sizeof(real), elem_num, fin);
  if (actual_read_size != elem_num) {
    LOG(0, "[NumReadVec]: read error!");
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

void NumVecAddCVec(real *a, real *b, real c, int l) {
  // in place a <- a + c * b
  int i;
  for (i = 0; i < l; i++) a[i] += c * b[i];
  return;
}

void NumVecMulC(real *a, real c, int l) {
  // in place a <- c * a
  int i;
  for (i = 0; i < l; i++) a[i] *= c;
  return;
}

void NumMulMatVec(real *m, real *a, int r, int l, real *x) {
  // x <- m * a; m: r * l, a: l
  int i, j, k = 0;
  for (i = 0; i < r; i++) x[i] = 0;
  for (j = 0; j < l; j++)
    for (i = 0; i < r; i++) x[i] += m[k++] * a[j];
  return;
}

void NumAddCVecDVec(real *a, real *b, real c, real d, int l, real *x) {
  // x <- c * a + d * b
  int i;
  for (i = 0; i < l; i++) x[i] = c * a[i] + d * b[i];
  return;
}

real NumSoftMax(real *a, int l) {
  // return entropy
  int i;
  real c = MAX(a, l);
  real s = 0, e = 0, t;
  for (i = 0; i < l; i++) {
    a[i] -= c;
    t = NumExp(a[i]);
    e += t * a[i];
    s += t;
    a[i] = t;
  }
  for (i = 0; i < l; i++) a[i] /= s;
  e = -e / s + log(s);
  return e;
}

void NumFillValVec(real *a, int l, real v) {
  int i;
  for (i = 0; i < l; i++) a[i] = v;
  return;
}

void NumFillValIntVec(int *a, int l, int v) {
  int i;
  for (i = 0; i < l; i++) a[i] = v;
  return;
}

real NumSumVec(real *a, int l) {
  int i;
  real s = 0;
  for (i = 0; i < l; i++) s += a[i];
  return s;
}

int NumSumIntVec(int *a, int l) {
  int i;
  int s = 0;
  for (i = 0; i < l; i++) s += a[i];
  return s;
}

void NumInit() {
  // Initialize exp table;
  int i;
  real x;
  for (i = 0; i < NUM_EXP_TABLE_LEN; i++) {
    x = NUM_EXP_LOW + i * (NUM_EXP_HIGH - NUM_EXP_LOW) / NUM_EXP_TABLE_LEN;
    NUM_EXP_TABLE[i] = exp(x);
  }
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
