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
};

real gd_ss;     // gradient descent step size
real shrink_w;  // shrink weight
int max_hn;     // when hn exceeds max_hn, performs offline update for one dist

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
void ThreadPrintProgBar(int dbg_lvl, int tid, real p, struct Bookkeeping* b) {
  if (simple_ppb_lock) return;
  simple_ppb_lock = 1;
  if (preceed_newline_flag) {
    LOG(dbg_lvl, "\n");
    preceed_newline_flag = 0;
  }
#ifdef DEBUG
  real ww = NumVecNorm(b->ww, K * N);
  real scr = NumVecNorm(model->scr, V * N);
  real ss = NumMatMaxRowNorm(model->scr, V, N);
  real tar = NumVecNorm(model->tar, V * N);
  real tt = NumMatMaxRowNorm(model->tar, V, N);
#endif
  // print progress
  int i, bar_len = 10;
  clock_t cur_clock_t = clock();
  real pct = p * 100;
  int bar = p * bar_len;
  LOGC(dbg_lvl, 'y', 'k', "\33[2K\r[%7.4lf%%]: ", pct);
  for (i = 0; i < bar; i++) LOGC(dbg_lvl, 'r', 'k', "+");
  LOGC(dbg_lvl, 'r', 'k', "~");
  for (i = bar + 1; i < bar_len; i++) LOGC(dbg_lvl, 'c', 'k', "=");
  LOGC(dbg_lvl, 'g', 'k', " (tid=%02d)", tid);
  double elapsed_time = (double)(cur_clock_t - start_clock_t) / CLOCKS_PER_SEC;
  char* ht = strtime(elapsed_time / V_THREAD_NUM);
  LOG(dbg_lvl, " TIME:%.2e/", elapsed_time);
  LOGC(dbg_lvl, 'c', 'k', "%s", ht);
  LOG(dbg_lvl, " GDSS:%.4e", gd_ss);
  free(ht);
#ifdef DEBUG
  LOG(0, " ww=%.2e/", ww);
  LOGC(0, 'y', 'k', "%.2ett ", ww / tt);
  LOG(0, "scr=%.2e/%.2e ", scr, scr / ss);
  LOG(0, "tar=%.2e/", tar);
  LOGC(0, 'y', 'k', "%.2ett ", tar / tt);
  LOGC(0, 'c', 'k', "tt=%.2e", tt);
#endif
  simple_ppb_lock = 0;
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
  NumFillZeroVec(b->hh, N * K);
  NumFillZeroIntVec(b->hn, K);
  NumFillZeroVec(b->ent, K);
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

void ModelGradUpdate(struct Model* m, int p, int i, real c, real* g,
                     int shrink) {
  // model minimization update: p=0: scr; p=1: tar
  // use c = +/- 1 to reverse optimization direction
  int j;
  if (V_L2_REGULARIZATION_WEIGHT != 0 && shrink) {  // L2 regularization
    for (j = 0; j < N; j++) {
      m->scr[i * N + j] *= shrink_w;
      m->tar[i * N + j] *= shrink_w;
    }
  }
  if (p == 0)  // update scr
    for (j = 0; j < N; j++) m->scr[i * N + j] -= c * gd_ss * g[j];
  else  // update tar
    for (j = 0; j < N; j++) m->tar[i * N + j] -= c * gd_ss * g[j];

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

int Update(int* ids, int l, struct Bookkeeping* b, struct Model* m,
           int primal) {
  // update for one sentence (ids, l)
  // primal = 0: used for burn-in
  // primal = 1: primal and dual update
  int offline_performed = 0;
  int i, j, k;
  real h0[NUP], h[NUP], w0[NUP], w[NUP];
  int z[SUP];
  memset(h0, 0, N * sizeof(real));
  memset(w0, 0, N * sizeof(real));
  for (i = 0; i < l; i++) NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(h0, m->scr + ids[i] * N, 1, -1, N, h);  // h
    z[i] = DualDecode(h, b);                               // dual decode: z
    if (primal) {                                          // primal part 1
      NumVecAddCVec(w0, m->tar + ids[i] * N, 1, N);        // w - ww
      NumVecAddCVec(w0, b->ww + z[i] * N, -1, N);
      ModelGradUpdate(m, 1, ids[i], -1, h, 0);  // primal online update: m->tar
    }
    if (b->hn[z[i]] >= max_hn) {  // offline
      offline_performed = 1;
      k = z[i];
      if (primal)                // primal part 2
        for (j = 0; j < V; j++)  // primal offline update: m->tar (neg sampling)
          ModelGradUpdate(m, 1, j, b->dd[k * V + j], b->hh + k * N, 1);
      NumVecMulC(b->hh + k * N, 1.0 / b->hn[k], N);  // hh
      for (j = 0; j < V; j++)                        // dd
        b->dd[k * V + j] = NumVecDot(m->tar + j * N, b->hh + k * N, N);
      b->ent[k] = NumSoftMax(b->dd + k * V, V);                  // ent
      NumMulVecMat(b->dd + k * V, m->tar, V, N, b->ww + k * N);  // ww
      b->hn[k] = 0;                                              // reset hn
      NumFillZeroVec(b->hh + k * N, N);                          // reset hh
    }
  }
  if (primal) {  // primal part 3
    for (i = 0; i < l; i++) {
      NumAddCVecDVec(w0, m->tar + ids[i] * N, 1, -1, N, w);  // w - ww
      NumVecAddCVec(w, b->ww + z[i] * N, 1, N);
      ModelGradUpdate(m, 0, ids[i], -1, w, 0);  // update m->scr
    }
  }
  return offline_performed;
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
  long long int burn_in_left = V * V_BURN_IN_INTERVAL_VOCAB_RATIO;
  fseek(fin, fbeg, SEEK_SET);
  while (burn_in_left > 0) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    burn_in_left -= wnum;
    if (Update(wids, wnum, b, model, 0)) {
      // nop
    }
    if (feof(fin) || ftell(fin) >= fend) {
      fseek(fin, fbeg, SEEK_SET);
    }
  }
  if (preceed_newline_flag) LOG(2, "$");
  // training
  real p = 0;
  int iter_num = 0;
  fseek(fin, fbeg, SEEK_SET);
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    if (Update(wids, wnum, b, model, 1)) {
      // ftell cost running time roughly 15/14 times of an addition operation
      fpos = ftell(fin);
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);
      // adjust gd_ss
      p = GetProgress();
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);
      // debug info
      ThreadPrintProgBar(2, tid, p, b);
      if (feof(fin) || fpos >= fend) {
        fseek(fin, fbeg, SEEK_SET);
        ModelSave(iter_num);
        iter_num++;
      }
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
  NumFillZeroVec(progress, V_THREAD_NUM);
  VariableInit();
  NumInit();
  vcb = TextLoadVocab(V_TEXT_VOCAB_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  // overwrite V by actual vocabulary size
  V = vcb->size;
  LOG(1, "Actual V: %d\n", V);
  ModelInit();
  shrink_w = 1 - V_L2_REGULARIZATION_WEIGHT;
  max_hn = V * V_OFFLINE_INTERVAL_VOCAB_RATIO;
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
