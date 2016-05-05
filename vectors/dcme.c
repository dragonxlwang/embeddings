//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////

#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "variables.c"

// model parameter is shared across all threads
struct Model {
  real* scr;  // source vector;
  real* tar;  // target vector;
} * model;

// each thread worker maintains a bookkeeping
struct Bookkeeping {
  real* dd;   // list(K):vector(V) dual distribution
  real* ent;  // list(K):
  real* ww;   // list(K):vector(N) W * dd, synthesis word in prime
  real* hh;   // list(K):vector(N) avg(h_k), sufficient stats
  int* hn;    // list(K): |h_k|
  int* ii;    // list(K):vector(Q) active word index in each dual distribution
};
real gd_ss;     // gradient descent step size
real shrink_w;  // shrink weight

// current progress for each worker
real* progress;

// vocabulary
struct Vocabulary* vcb;

// start time
clock_t start_clock_t;

// some helper functions for debugging
real GetProgress() {
  int i;
  real p = 0;
  for (i = 0; i < V_THREAD_NUM; i++) p += progress[i];
  p /= (V_THREAD_NUM * V_ITER_NUM);
  return p;
}

int preceed_newline_flag = 1;
int simple_ppb_lock = 0;
void ThreadPrintProgBar(int dbg_lvl, int tid, real p) {
  if (simple_ppb_lock) return;
  simple_ppb_lock = 1;
  if (preceed_newline_flag) {
    LOG(dbg_lvl, "\n");
    preceed_newline_flag = 0;
  }
  // print progress
  int i, bar_len = 80;
  clock_t cur_clock_t = clock();
  real pct = p * 100;
  int bar = p * bar_len;
  LOG(dbg_lvl, "\33[2K\r[%7.4lf%%]: ", pct);
  for (i = 0; i < bar; i++) LOG(dbg_lvl, "+");
  LOG(dbg_lvl, "~");
  for (i = bar + 1; i < bar_len; i++) LOG(dbg_lvl, "=");
  LOG(dbg_lvl, " (tid = %d)", tid);
  double elapsed_time = (double)(cur_clock_t - start_clock_t) / CLOCKS_PER_SEC;
  LOG(dbg_lvl, " time: %e / %e", elapsed_time, elapsed_time / V_THREAD_NUM);
  LOG(dbg_lvl, " gdss: %e", gd_ss);
  simple_ppb_lock = 0;
  return;
}

void DebugModel(struct Bookkeeping* b) {
  /* LOG(0, "hh=%.3e ", NumVecNorm(b->hh, K * N)); */
  real ww = NumVecNorm(b->ww, K * N);
  real dd = NumVecPNorm(b->dd, K * V, 1);
  real scr = NumVecNorm(model->scr, V * N);
  real ss = NumMatMaxRowNorm(model->scr, V, N);
  real tar = NumVecNorm(model->tar, V * N);
  real tt = NumMatMaxRowNorm(model->tar, V, N);
  LOG(0, "ww=%.2e/", ww);
  LOGC(0, 'r', 'k', "%.2ett ", ww / tt);
  LOG(0, "dd=%s ", NumEqual(dd, 100) ? "100" : "error");
  LOG(0, "scr=%.2e/%.2e ", scr, scr / ss);
  LOG(0, "tar=%.2e/", tar);
  LOGC(0, 'r', 'k', "%.2ett ", tar / tt);
  LOGC(0, 'y', 'k', "tt=%.2e", tt);
  int i;
  heap* h = HeapCreate(5);
  for (i = 0; i < V; i++) {
    HeapPush(h, i, NumVecNorm(model->tar + i * N, N));
  }
  int size = HeapSort(h);
  for (i = 0; i < size; i++) {
    LOGC(0, 'c', 'k', " %s", vcb->id2wd[h->d[i].key]);
    LOG(0, ":%.2lf", h->d[i].val);
  }
  HeapDestroy(h);
  return;
}

struct Bookkeeping* BookkeepingCreate() {
  struct Bookkeeping* b =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hn = NumNewHugeIntVec(K);
  NumFillValVec(b->hh, N * K, 0);
  NumFillValIntVec(b->hn, K, 0);
  NumFillValVec(b->ent, K, 0);
  NumRandFillVec(b->ww, N * K, -1e-1, 1e-1);
  NumFillValVec(b->dd, V * K, 1.0 / V);
  return b;
}

void ModelInit() {
  model = (struct Model*)malloc(sizeof(struct Model));
  model->scr = NumNewHugeVec(V * N);
  model->tar = NumNewHugeVec(V * N);
  NumRandFillVec(model->scr, V * N, -1e-1, 1e-1);
  NumRandFillVec(model->tar, V * N, -1e-1, 1e-1);
  return;
}

int save_iter_num = -1;
void ModelSave(int iter_num) {
  if (iter_num <= save_iter_num) return;  // avoid thread racing by simple lock
  save_iter_num = iter_num;
  char* mfp = sformat("%s.part%d", V_MODEL_SAVE_PATH, iter_num);
  FILE* fout = fopen(mfp, "wb");
  if (!fout) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fwrite(model->scr, sizeof(real), V * N, fout);
  fwrite(model->tar, sizeof(real), V * N, fout);
  fclose(fout);
  free(mfp);
  return;
}

void BookkeepingFree(struct Bookkeeping* b) {
  free(b->dd);
  free(b->ent);
  free(b->ww);
  free(b->hh);
  free(b->hn);
  free(b);
  return;
}

void ModelFree() {
  free(model->scr);
  free(model->tar);
  free(model);
  return;
}

void ModelGradUpdate(struct Model* m, int p, int i, real c, real* g) {
  // model minimization update: p=0: scr; p=1: tar
  // use c = +/- 1 to reverse optimization direction
  int j;
  if (p == 0) {
    // update scr
    for (j = 0; j < N; j++) {
      m->scr[i * N + j] *= shrink_w;
      m->scr[i * N + j] -= c * gd_ss * g[j];
    }
  } else {
    // update tar
    for (j = 0; j < N; j++) {
      m->scr[i * N + j] *= shrink_w;
      m->tar[i * N + j] -= c * gd_ss * g[j];
    }
  }
  return;
}

void ModelShrink(struct Model* m, real w) {
  int i;
  real c = 1 - w;
  for (i = 0; i < V * N; i++) model->scr[i] *= c;
  for (i = 0; i < V * N; i++) model->tar[i] *= c;
  return;
}

int DualDecode(real* h, struct Bookkeeping* b) {
  // fit the sentence with the best dual distribution (z)
  real s = 0, t;
  int z = 0, k;
  for (k = 0; k < K; k++) {
    t = -NumVecDot(h, b->ww + k * N, N) - b->ent[k];
    if (k == 0 || s > t) {
      s = t;
      z = k;
    }
  }
  // update bookkeeping: hh, hn
  NumVecAddCVec(b->hh + z * N, h, 1.0, N);
  b->hn[z]++;
  return z;
}

void DualUpdateOnline(int* ids, int l, struct Bookkeeping* b, struct Model* m) {
  // dual decoding for one sentence -- this is used for burn in
  int i;
  real h0[NUP], h[NUP];
  memset(h0, 0, N * sizeof(real));
  for (i = 0; i < l; i++) NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  // dual decode
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(h0, m->scr + ids[i] * N, 1, -1, N, h);  // h
    DualDecode(h, b);                                      // z
  }
  return;
}

void PrimalDualUpdateOnline(int* ids, int l, struct Bookkeeping* b,
                            struct Model* m) {
  // update primal parameters online for one sentence (ids)
  int i;
  real h0[NUP], h[NUP], w0[NUP], w[NUP];
  int z[SUP];
  memset(h0, 0, N * sizeof(real));
  memset(w0, 0, N * sizeof(real));
  for (i = 0; i < l; i++) NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  // dual decode
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(h0, m->scr + ids[i] * N, 1, -1, N, h);  // h
    z[i] = DualDecode(h, b);                               // z
    NumVecAddCVec(w0, m->tar + ids[i] * N, 1, N);          // w - ww
    NumVecAddCVec(w0, b->ww + z[i] * N, -1, N);
    // update m->tar (1st part, online update)
    ModelGradUpdate(m, 1, ids[i], -1, h);
  }
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(w0, m->tar + ids[i] * N, 1, -1, N, w);
    NumVecAddCVec(w, b->ww + z[i] * N, 1, N);
    // update m->scr
    ModelGradUpdate(m, 0, ids[i], -1, w);
  }
}

void PrimalUpdateOffline(struct Bookkeeping* b, struct Model* m) {
  // update primal parameters offline: update m->tar (negative sampling)
  // call before DualUpdateOffline
  int i, k;
  for (k = 0; k < K; k++)
    for (i = 0; i < V; i++)
      ModelGradUpdate(m, 1, i, b->dd[k * V + i], b->hh + k * N);
  /* // l-2 regularization */
  /* ModelShrink(m, V_L2_REGULARIZATION_WEIGHT); */
  return;
}

void DualUpdateOffline(struct Bookkeeping* b, struct Model* m) {
  // update dual distributions and others (b->dd,ww,ent)
  int i, k;
  // compute hh
  for (k = 0; k < K; k++)
    if (b->hn[k]) NumVecMulC(b->hh + k * N, 1.0 / b->hn[k], N);
  for (k = 0; k < K; k++) {
    // update dd
    for (i = 0; i < V; i++)
      b->dd[k * V + i] = NumVecDot(m->tar + i * N, b->hh + k * N, N);
    // normalize dd and update ent
    b->ent[k] = NumSoftMax(b->dd + k * V, V);
    // update ww
    // tar: V * N; dd: K * (V)
    NumMulVecMat(b->dd + k * V, m->tar, V, N, b->ww + k * N);
  }
  return;
}

void DualResetOffline(struct Bookkeeping* b) {
  // dual model paramters reset (b->hh,hn)
  NumFillValVec(b->hh, N * K, 0);
  NumFillValIntVec(b->hn, K, 0);
  return;
}

void* ThreadWork(void* arg) {
  int tid = (long)arg;
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fpos;
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  struct Bookkeeping* b = BookkeepingCreate();
  LOG(2, "*");
  // burn in
  int wids[SUP], wnum;
  long long int online_cnt = 0;
  long long int offline_int = V * V_OFFLINE_INTERVAL_VOCAB_RATIO;
  long long int burn_in_left = V * V_BURN_IN_INTERVAL_VOCAB_RATIO;
  fseek(fin, fbeg, SEEK_SET);
  while (burn_in_left > 0) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    DualUpdateOnline(wids, wnum, b, model);
    online_cnt += wnum;
    burn_in_left -= wnum;
    if (online_cnt >= offline_int) {
#ifdef DEBUG
      printfc('c', 'k', "Burnin: ");
      DebugModel(b);
      printf("\n");
#endif
      // offline computation
      DualUpdateOffline(b, model);
      DualResetOffline(b);
      online_cnt = 0;
    }
    if (feof(fin) || ftell(fin) >= fend) {
      fseek(fin, fbeg, SEEK_SET);
    }
  }
  if (preceed_newline_flag) LOG(2, "$");
  // training
  real p = 0;
  int iter_num = 0;
  online_cnt = 0;
  fseek(fin, fbeg, SEEK_SET);
#ifdef DEBUG
  printf("\n");
  int tmp_cnt = 0;
#endif
  while (iter_num < V_ITER_NUM) {
// online computation
#ifdef DEBUG
/* tmp_cnt++; */
/* if (tmp_cnt >= 50) { */
/*   tmp_cnt = 0; */
/*   ThreadPrintProgBar(2, tid, p); */
/*   printf(" Online: "); */
/*   DebugModel(b); */
/*   printf("\n"); */
/* } */
#endif
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    PrimalDualUpdateOnline(wids, wnum, b, model);
    online_cnt += wnum;
    if (online_cnt >= offline_int) {
      // offline computation
      PrimalUpdateOffline(b, model);
      DualUpdateOffline(b, model);
      DualResetOffline(b);
      online_cnt = 0;
      // adjust gd_ss
      p = GetProgress();
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);
// debug info
#ifndef DEBUG
      ThreadPrintProgBar(2, tid, p);
#endif /* ifndef  */
#ifdef DEBUG
      printfc('g', 'k', "Offline: fin=%.2e ", (real)ftell(fin));
      DebugModel(b);
      printf("\n");
#endif
    }
    // ftell cost running time roughly 15/14 times of an addition operation
    fpos = ftell(fin);
    progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      ModelSave(iter_num);
      iter_num++;
    }
  }
  fclose(fin);
  BookkeepingFree(b);
  pthread_exit(NULL);
  return 0;
}

void ScheduleWork() {
  long long int tid;
  start_clock_t = clock();
  progress = (real*)malloc(V_THREAD_NUM * sizeof(real));
  NumFillValVec(progress, V_THREAD_NUM, 0);
  VariableInit();
  NumInit();
  vcb = TextLoadVocab(V_TEXT_VOCAB_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  // overwrite V by actual vocabulary size
  V = vcb->size;
  LOG(1, "Actual V: %d\n", V);
  ModelInit();
  shrink_w = 1 - V_L2_REGULARIZATION_WEIGHT;
  LOG(2, "Threads spawning: ");
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));
  for (tid = 0; tid < V_THREAD_NUM; tid++)
    pthread_create(&pt[tid], NULL, ThreadWork, (void*)tid);
  for (tid = 0; tid < V_THREAD_NUM; tid++) pthread_join(pt[tid], NULL);
  ModelFree();
  VocabDestroy(vcb);
  LOG(2, "\n");
  LOG(1, "Training finished\n");
}

int main(int argc, char* argv[]) {
  ScheduleWork();
  return 0;
}
