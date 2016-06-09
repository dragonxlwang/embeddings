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

// Parameters:
//  NUM_EXP_TABLE_LEN          : exp table size
//  NUM_EXP_HIGH               : exp table high end
//  NUM_EXP_LOW                : exp table low end
//  NUM_MAX_PRINT_ELEM         : number of printable element in an array
//  NUM_EPS                    : machine precision
// Init:
//  NumInit()

#define NUM_EXP_TABLE_LEN 0xFFFFF
real NUM_EXP_TABLE[NUM_EXP_TABLE_LEN];
real NUM_EXP_HIGH = 1.0;
real NUM_EXP_LOW = -15;
real NumExp(real x) {
  // output exp(x) for -NUM_EXP_LOW <= x <= NUM_EXP_HIGH
  int i = (x - NUM_EXP_LOW) / (NUM_EXP_HIGH - NUM_EXP_LOW) * NUM_EXP_TABLE_LEN;
  if (i < 0)
    return 0;
  else
    return NUM_EXP_TABLE[i];
}

unsigned long long int RANDOM_SEED = 0x0F0F0F0FL;
real NumRand() {
  // return a random number range from [0, 1)
  RANDOM_SEED = ((RANDOM_SEED * 0x5DEECE66DL + 0x0A) & 0xFFFFFFFFFFFFL);
  return ((real)RANDOM_SEED / ((real)0xFFFFFFFFFFFFL));
}

void NumRandFillVec(real *arr, int l, real lb, real ub) {
  int i;
  for (i = 0; i < l; i++) arr[i] = lb + NumRand() * (ub - lb);
  return;
}

void NumPermuteIntVec(int *arr, int l, real permute_ratio) {
  // pairwise permute array l * permute_ratio times
  int cnt = permute_ratio * l;
  int i, j;
  while (cnt-- > 0) {
    i = NumRand() * l;
    j = NumRand() * (l - 1);
    if (j >= i) j++;
    SWAP(arr[i], arr[j]);
  }
  return;
}

int NUM_MAX_PRINT_ELEM = 10;
real NUM_EPS = 1e-6;

int NumEqual(real x, real y) { return ABS(x - y) <= NUM_EPS; }

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
  int abbrv = 0;
  for (i = 0; i < l; i++)
    if (i < NUM_MAX_PRINT_ELEM - 2 || l - i <= 2) {
      if (-NUM_EPS <= arr[i] && arr[i] <= NUM_EPS)
        printf("%12.6g ", 0.0);
      else
        printf("%12.6g ", arr[i]);
    } else if (!abbrv) {
      abbrv = 1;
      printf("     ...     ");
    }
  printf("\n");
  fflush(stdout);
  return;
}

void NumPrintMatrix(char *name, real *arr, int m, int n) {
  int i;
  int abbrv = 0;
  for (i = 0; i < m; i++) {
    if (i < NUM_MAX_PRINT_ELEM - 2 || m - i <= 2) {
      if (i == 0)
        NumPrintArr(name, arr, n);
      else
        NumPrintArr("", arr + i * n, n);
    } else if (!abbrv) {
      abbrv = 1;
      printf(" ... \n");
    }
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

void NumCopyVec(real *d, real *s, int l) {
  int i;
  for (i = 0; i < l; i++) d[i] = s[i];
  return;
}

void NumCopyIntVec(int *d, real *s, int l) {
  int i;
  for (i = 0; i < l; i++) d[i] = s[i];
  return;
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

real NumVecZeroNorm(real *arr, int l) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += ((arr[i] == 0) ? 0 : 1);
  return s;
}

real NumVecPNorm(real *arr, int l, int p) {
  real s = 0;
  int i = 0;
  for (i = 0; i < l; i++) s += pow(arr[i], p);
  return pow(s, 1.0 / p);
}

real NumMatMaxRowNorm(real *mat, int m, int n) {
  int i;
  real x = 0, y;
  for (i = 0; i < m; i++) {
    y = NumVecNorm(mat + i * n, n);
    x = ((i == 0 || y > x) ? y : x);
  }
  return x;
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
  return sqrt(s);
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

void NumMulCVec(real *a, real c, int l, real *x) {
  // x <- c * a
  int i;
  for (i = 0; i < l; i++) x[i] = c * a[i];
  return;
}

void NumMulMatVec(real *m, real *a, int l, int r, real *x) {
  // x <- m * a; x: l, m: l * r, a: r
  int i, j, k = 0;
  for (i = 0; i < l; i++) x[i] = 0;
  for (i = 0; i < l; i++)
    for (j = 0; j < r; j++) x[i] += m[k++] * a[j];
  return;
}

void NumMulVecMat(real *a, real *m, int l, int r, real *x) {
  // x <- a * m; x: r, m: l * r, a: l
  int i, j, k = 0;
  for (j = 0; j < r; j++) x[j] = 0;
  for (i = 0; i < l; i++)
    for (j = 0; j < r; j++) x[j] += m[k++] * a[i];
  return;
}

void NumAddCVecDVec(real *a, real *b, real c, real d, int l, real *x) {
  // x <- c * a + d * b
  int i;
  for (i = 0; i < l; i++) x[i] = c * a[i] + d * b[i];
  return;
}

real NumSoftMax(real *a, real d, int l) {
  // return entropy, discounted by d; propto exp(a/d)
  int i;
  real c = MAX(a, l);
  real s = 0, e = 0, t;
  for (i = 0; i < l; i++) {
    a[i] = (a[i] - c) / d;  // discount by d
    t = NumExp(a[i]);
    e += t * a[i];
    s += t;
    a[i] = t;
  }
  for (i = 0; i < l; i++) a[i] /= s;
  e = -e / s + log(s);
  return (e > 0) ? e : 0;
}

void NumVecProjUnitSphere(real *a, real s, int l) {
  // project to unit sphere
  NumVecMulC(a, s / NumVecNorm(a, l), l);
  return;
}

void NumVecProjUnitBall(real *a, real s, int l) {
  // project to unit ball
  real c = s / NumVecNorm(a, l);
  if (c < 1) NumVecMulC(a, c, l);
  return;
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

void NumFillZeroVec(real *a, int l) {
  memset(a, 0, l * sizeof(real));
  return;
}

void NumFillZeroIntVec(int *a, int l) {
  memset(a, 0, l * sizeof(int));
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

int NumIsNanVec(real *a, int l) {
  int i;
  for (i = 0; i < l; i++)
    if (a[i] != a[i]) return 1;
  return 0;
}

real NumVecMean(real *a, int l) { return NumSumVec(a, l) / l; }

real NumVecVar(real *a, int l) {
  real x = NumVecNorm(a, l);
  real y = NumVecMean(a, l);
  real s = x * x / l - y * y;
  return (s > 0) ? s : 0;
}

real NumVecStd(real *a, int l) { return sqrt(NumVecVar(a, l)); }

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
