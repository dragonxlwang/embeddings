#ifndef PEEK
#define PEEK

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "variables.c"

int PEEK_TOP_K = 200;

real PeekEval(real *scr, real *tar, int **peek_wids, int *peek_wnum, int size) {
  // evaluate on peek set
  // to speed up, we constrain competitive target words from top 10k words
  int i, j, k, lt, rt, md;
  int topk, *ids, l, pn = 0, h0n;
  topk = PEEK_TOP_K;
  real *p = (real *)malloc(topk * sizeof(real));
  real h0[NUP], h[NUP], hw, avgp = 0;
  for (i = 0; i < size; i++) {
    ids = peek_wids[i];
    l = peek_wnum[i];
    h0n = 0;
    NumFillZeroVec(h0, N);
    for (j = 0; j < SMALLER(l, C); j++) {
      NumVecAddCVec(h0, scr + ids[j] * N, 1, N);
      h0n++;
    }
    for (j = 0; j < l; j++) {
      lt = j - C - 1;
      rt = j + C;
      md = j;
      if (rt < l) {
        NumVecAddCVec(h0, scr + ids[rt] * N, 1, N);
        h0n++;
      }
      if (lt >= 0) {
        NumVecAddCVec(h0, scr + ids[lt] * N, -1, N);
        h0n--;
      }
      hw = 1.0 / (h0n - 1.0);
      NumAddCVecDVec(h0, scr + ids[md] * N, hw, -hw, N, h);
      if (ids[j] < topk) {
        for (k = 0; k < topk; k++) p[k] = NumVecDot(h, tar + k * N, N);
        NumSoftMax(p, topk);
        avgp += p[ids[j]];
        pn++;
      }
    }
  }
  avgp /= pn;
  free(p);
  return avgp;
}

int PeekBuildSentSet(char *text_file_path, real sample_rate,
                     struct Vocabulary *vcb, int ***peek_wids_ptr,
                     int **peek_wnum_ptr) {
  // sample by sample_rate amount of sentences
  FILE *fin = fopen(text_file_path, "rb");
  if (!fin) {
    LOG(0, "Error\n");
    exit(1);
  }
  int size = 0;
  int cap = 10000;
  int **peek_wids = (int **)malloc(cap * sizeof(int *));
  int *peek_wnum = (int *)malloc(cap * sizeof(int));
  int wids[SUP], wnum;
  long long int sid = 0;
  while (1) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    if (sid++ > 100 && NumRand() < sample_rate && wnum >= 5) {
      ARR_CLONE(peek_wids[size], wids, wnum);
      peek_wnum[size] = wnum;
      size++;
      if (size == cap) {
        cap <<= 1;  // double resize
        peek_wids = (int **)realloc(peek_wids, cap * sizeof(int *));
        peek_wnum = (int *)realloc(peek_wnum, cap * sizeof(int));
      }
    }
    if (feof(fin)) break;
  }
  *peek_wids_ptr = (int **)realloc(peek_wids, size * sizeof(int *));  // shrink
  *peek_wnum_ptr = (int *)realloc(peek_wnum, size * sizeof(int));
  LOG(1, "Peek Set: %d / %lld\n", size, sid);
  return size;
}

void PeekSaveSentSet(char *fp, int **peek_wids, int *peek_wnum, int size) {
  int i, j;
  FILE *fout = fopen(fp, "wb");
  fprintf(fout, "%d\n", size);
  for (i = 0; i < size; i++) {
    fprintf(fout, "%d", peek_wnum[i]);
    for (j = 0; j < peek_wnum[i]; j++) fprintf(fout, " %d", peek_wids[i][j]);
    fprintf(fout, "\n");
  }
  LOG(1, "Peek File: %s\n", fp);
  return;
}

void PeekSaveTextSentSet(char *fp, int **peek_wids, int *peek_wnum, int size,
                         struct Vocabulary *vcb) {
  int i, j;
  FILE *fout = fopen(fp, "wb");
  fprintf(fout, "%d", PEEK_TOP_K);
  for (i = 0; i < PEEK_TOP_K; i++) fprintf(fout, " %s", VocabGetWord(vcb, i));
  fprintf(fout, "\n");
  fprintf(fout, "%d\n", size);
  for (i = 0; i < size; i++) {
    fprintf(fout, "%d", peek_wnum[i]);
    for (j = 0; j < peek_wnum[i]; j++)
      fprintf(fout, " %s", VocabGetWord(vcb, peek_wids[i][j]));
    fprintf(fout, "\n");
  }
  LOG(1, "Peek File: %s\n", fp);
  return;
}

int PeekLoadSentSet(char *fp, int ***peek_wids_ptr, int **peek_wnum_ptr) {
  int i, j;
  int size;
  FILE *fin = fopen(fp, "rb");
  if (!fin) {
    LOG(0, "Error\n");
    exit(1);
  }
  fscanf(fin, "%d\n", &size);
  int **peek_wids = (int **)malloc(size * sizeof(int *));
  int *peek_wnum = (int *)malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    fscanf(fin, "%d", peek_wnum + i);
    peek_wids[i] = (int *)malloc(peek_wnum[i] * sizeof(int));
    for (j = 0; j < peek_wnum[i]; j++) fscanf(fin, " %d", peek_wids[i] + j);
    fscanf(fin, "\n");
  }
  *peek_wids_ptr = peek_wids;
  *peek_wnum_ptr = peek_wnum;
  return size;
}
#endif /* ifndef PEEK */
