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
#include "peek.c"
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
  int* tw;    // list(K):list(Q) top words
};

real gd_ss;  // gradient descent step size
int max_hn;  // when hn exceeds max_hn, performs offline update for one dist

// current progress for each worker
real* progress;

// vocabulary
struct Vocabulary* vcb;

// start time
clock_t start_clock_t;

// peek set
int** peek_wids;
int* peek_wnum;
int peek_size;

// some helper functions for debugging
real GetProgress() {
  real p = NumSumVec(progress, V_THREAD_NUM);
  p /= (V_THREAD_NUM * V_ITER_NUM);
  return p;
}

#ifdef DEBUG
int dual_update_cnt = 0;
int dual_update_cluster_cnt[1000] = {0};
#endif
int ppb_lock = 0;
void ThreadPrintProgBar(int dbg_lvl, int tid, real p, struct Bookkeeping* b) {
  if (ppb_lock) return;
  ppb_lock = 1;
  // print progress
  int i, bar_len = 10;
  clock_t cur_clock_t = clock();
  real pct = p * 100;
  int bar = p * bar_len;
#ifdef DEBUG
  LOGC(dbg_lvl, 'y', 'k', "\n[%7.4lf%%]: ", pct);
#else
  LOGC(dbg_lvl, 'y', 'k', "\33[2K\r[%7.4lf%%]: ", pct);
#endif
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
  real scr = NumVecNorm(model->scr, V * N);
  real ss = NumMatMaxRowNorm(model->scr, V, N);
  real tar = NumVecNorm(model->tar, V * N);
  real tt = NumMatMaxRowNorm(model->tar, V, N);
  LOG(0, " scr=%.2e/%.2ess", scr, scr / ss);
  LOG(0, " tar=%.2e/%.2ett", tar, tar / tt);
  LOGC(0, 'r', 'k', " ss=%.2e", ss);
  LOGC(0, 'r', 'k', " tt=%.2e", tt);
  real eem = NumVecMean(b->ent, K);
  real ees = NumVecStd(b->ent, K);
  LOGC(0, 'w', 'r', " ENT:%.2e/%.2e", eem, ees);
  real avgp = PeekEval(model->scr, model->tar, peek_wids, peek_wnum, peek_size);
  LOGC(0, 'r', 'c', " PEEK:%.2e", avgp);
#endif
#ifdef DEBUG
  /* FILE* fout_tmp = fopen("tmp.txt", "wb"); */
  /* pair* pairs = sorted(b->dd + zz * V, V, 1); */
  /* for (j = 0; j < V; j++) { */
  /*   fprintf(fout_tmp, "%d: %8d, %-20s %.6e\n", j, pairs[j].key, */
  /*           VocabGetWord(vcb, pairs[j].key), pairs[j].val); */
  /* } */
  /* fclose(fout_tmp); */

  /* printf("TT=%11.6e hh=%11.6e ww=%11.6e ent=%11.6e\n", */
  /*        NumMatMaxRowNorm(m->tar, V, N), NumVecNorm(b->hh + zz * N, N), */
  /*        NumVecNorm(b->ww + zz * N, N), b->ent[zz]); */
  printf("\n");
  pair* dps = sortedi(dual_update_cluster_cnt, K, 1);
  int j, k;
  for (j = 0; j < K; j++) {
    if (j % 5 == 0 && j != 0) printf("\n");
    k = dps[j].key;
    LOGC(0, 'y', 'k', "[%02d]:%.3lf:", k,
         (real)dual_update_cluster_cnt[k] / (dual_update_cnt + 1));
    LOGC(0, 'c', 'k', "%.2e", b->ent[k]);
  }
  free(dps);
/* pair* ps = sorted(b->dd + zz * V, V, 1); */
/* int printed_adw_cnt = 0; */
/* for (j = 0; j < V; j++) { */
/*   k = ps[j].key; */
/*   if (b->dd[zz * V + k] > 10.0 / V) { */
/*     // abonormal distribution weight */
/*     LOGC(0, 'g', 'k', "[TW:%02d-W:%04d] %15s = %.6lf\n", zz, k, */
/*          VocabGetWord(vcb, k), b->dd[zz * V + k]); */
/*     if (printed_adw_cnt++ >= 50) break; */
/*   } */
/* } */
/* free(ps); */
#endif
  ppb_lock = 0;
  return;
}

real init_amp = 1e-3;  // to avoid numeric instability use a small value
struct Bookkeeping* BookkeepingCreate() {
  struct Bookkeeping* b =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hn = NumNewHugeIntVec(K);
  b->tw = NumNewHugeIntVec(Q * K);
  NumRandFillVec(b->hh, N * K, -init_amp, init_amp);
  NumFillValIntVec(b->hn, K, 1);
  return b;
}

void ModelInit() {
  model = (struct Model*)malloc(sizeof(struct Model));
  model->scr = NumNewHugeVec(V * N);
  model->tar = NumNewHugeVec(V * N);
  NumRandFillVec(model->scr, V * N, -init_amp, init_amp);
  NumRandFillVec(model->tar, V * N, -init_amp, init_amp);
  return;
}

int saved_iter_num = -1;
void ModelSave(int iter_num) {
  char* mfp;
  if (iter_num == -1) {                      // save final model
    STR_CLONE(mfp, V_MODEL_SAVE_PATH);       // so that free(mfp) can work
  } else {                                   //
    if (iter_num <= saved_iter_num) return;  // avoid thread racing
    saved_iter_num = iter_num;
    mfp = sformat("%s.part%d", V_MODEL_SAVE_PATH, iter_num);
  }
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
  free(b->tw);
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
  if (p == 0) {  // update scr
    NumVecAddCVec(m->scr + i * N, g, -c * gd_ss, N);
  } else {  // update tar
    NumVecAddCVec(m->tar + i * N, g, -c * gd_ss, N);
  }
  return;
}

void ModelShrink(struct Model* m) {
  int i;
  if (V_L2_REGULARIZATION_WEIGHT) {
    NumVecMulC(model->scr, 1 - V_L2_REGULARIZATION_WEIGHT, V * N);
    NumVecMulC(model->tar, 1 - V_L2_REGULARIZATION_WEIGHT, V * N);
  } else if (V_MODEL_PROJ_UNIT_BALL) {
    for (i = 0; i < V; i++) {
      NumVecProjUnitBall(m->scr + i * N, N);
      NumVecProjUnitBall(m->tar + i * N, N);
    }
  }
  return;
}

int DualDecode(real* h, struct Bookkeeping* b) {
  // fit the sentence with the best dual distribution (z)
  // which maximizes h * w(z) + entropy(z)
  real s = 0, t;
  int z = 0, k;
  for (k = 0; k < K; k++) {
    t = NumVecDot(h, b->ww + k * N, N) + b->ent[k];
    if (k == 0 || t > s) {
      s = t;
      z = k;
    }
  }
  return z;
}

void DualUpdate(int zz, struct Bookkeeping* b, struct Model* m, heap* twh) {
  int j;
  NumMulMatVec(m->tar, b->hh + zz * N, V, N, b->dd + zz * V);   // dd
  NumVecMulC(b->dd + zz * V, 1.0 / b->hn[zz], V);               // dd/hn
  b->ent[zz] = NumSoftMax(b->dd + zz * V, V);                   // ent (sm)
  HeapEmpty(twh);                                               // tw reset
  for (j = 0; j < V; j++) HeapPush(twh, j, b->dd[zz * V + j]);  // tw add
  for (j = 0; j < Q; j++) b->tw[zz * Q + j] = twh->d[j].key;    // tw dump
  qsort(b->tw + zz * Q, Q, sizeof(int), cmp_int);               // tw sort
  NumMulVecMat(b->dd + zz * V, m->tar, V, N, b->ww + zz * N);   // ww
  return;
}

void DualReset(int zz, struct Bookkeeping* b) {  // use previous round as prior
  b->hn[zz] *= 0.1;                              // reset hn by decay
  NumVecMulC(b->hh + zz * N, 0.1, N);            // reset hh by decay
  /* b->hn[zz] = 0;                      // reset hn to half */
  /* NumFillZeroVec(b->hh + zz * N, N);  // reset hh to half */
}

int Update(int* ids, int l, struct Bookkeeping* b, struct Model* m, heap* twh) {
  // dual/primal update for one sentence (ids, l)
  int offline_performed = 0;
  int i, j, k, lt, rt, md;
  real h0[NUP], h[NUP], hw[SUP], wd[NUP * SUP], w0[NUP], w[NUP];
  int zz, h0n = 0;
  int neg_id;
  /* int window = NumRand() * C + 1; */
  int window = C;
  NumFillZeroVec(h0, N);
  NumFillZeroVec(w0, N);
  for (i = 0; i < SMALLER(l, window); i++) {
    NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
    h0n++;
  }
  for (i = 0; i < l; i++) {
    lt = i - window - 1;
    rt = i + window;
    md = i;
    if (rt < l) {
      NumVecAddCVec(h0, m->scr + ids[rt] * N, 1, N);                  // h0++
      h0n++;                                                          // h0n++
    }                                                                 //
    if (lt >= 0) {                                                    //
      NumVecAddCVec(h0, m->scr + ids[lt] * N, -1, N);                 // h0--
      h0n--;                                                          // h0n--
    }                                                                 //
    hw[md] = 1.0 / (h0n - 1.0);                                       // hw
    NumAddCVecDVec(h0, m->scr + ids[md] * N, hw[md], -hw[md], N, h);  // h
    zz = DualDecode(h, b);                                            // dcd z
    NumVecAddCVec(b->hh + zz * N, h, 1.0, N);                         // b->hh
    b->hn[zz]++;                                                      // b->hn
    NumAddCVecDVec(m->tar + ids[i] * N, b->ww + zz * N, 1, -1, N,
                   wd + i * N);            // wd[i] <- w - ww
    ModelGradUpdate(m, 1, ids[i], -1, h);  // up m->tar (pos)
    for (j = 0; j < Q; j++) {              // up m->tar (neg, top words)
      neg_id = b->tw[zz * Q + j];
      ModelGradUpdate(m, 1, neg_id, b->dd[zz * V + neg_id], h);
    }
    lt = i - 2 * window - 1;
    rt = i;
    md = i - window;
    NumVecAddCVec(w0, wd + rt * N, hw[rt], N);                  // w0++
    while (md < l) {                                            //
      if (lt >= 0) NumVecAddCVec(w0, wd + lt * N, -hw[lt], N);  // w0--
      if (md >= 0) {                                            //
        NumAddCVecDVec(w0, wd + md * N, 1, -hw[md], N, w);      // w
        ModelGradUpdate(m, 0, ids[md], -1, w);                  // up m->scr
      }
      if (i == l - 1) {
        md++;
        lt++;
      } else
        break;
    }
    if (b->hn[zz] >= max_hn) {                // update offline
      offline_performed = 1;                  //
      for (j = 0, k = 0; j < V; j++)          // up m->tar (neg, other words)
        if (k < Q && j == b->tw[zz * Q + k])  // join two sorted list
          k++;
        else
          ModelGradUpdate(m, 1, j, b->dd[zz * V + j], b->hh + zz * N);
      ModelShrink(m);             // optional model shrink
      DualUpdate(zz, b, m, twh);  // dual update based on hh and hn
      DualReset(zz, b);           // reset hh and hn
#ifdef DEBUG
      dual_update_cnt++;
      dual_update_cluster_cnt[zz]++;
#endif
    }
  }
  return offline_performed;
}

void* ThreadWork(void* arg) {
  int k;
  int tid = (long)arg;
  heap* twh = HeapCreate(Q);
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  long int fpos = fbeg;
  struct Bookkeeping* b = BookkeepingCreate();
  int wids[SUP], wnum;
  real p = 0;
  int iter_num = 0;
  for (k = 0; k < K; k++) {  // initialize dual
    DualUpdate(k, b, model, twh);
    DualReset(k, b);
  }
  fseek(fin, fbeg, SEEK_SET);  // training
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    if (wnum > 1 && Update(wids, wnum, b, model, twh)) {
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = GetProgress();                                                 // gdss
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   //
      ThreadPrintProgBar(2, tid, p, b);                                  // info
    }
    fpos = ftell(fin);
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_MODEL) ModelSave(iter_num);
      iter_num++;
    }
  }
  BookkeepingFree(b);
  fclose(fin);
  HeapDestroy(twh);
  pthread_exit(NULL);
  return 0;
}

void ScheduleWork() {
  long long int tid;
  start_clock_t = clock();
  progress = (real*)malloc(V_THREAD_NUM * sizeof(real));
  NumFillZeroVec(progress, V_THREAD_NUM);
  VariableInit();  // include initialization
  NumInit();
  // build vocab if necessary, load, and set V by smaller actual size
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    LOG(0, "[TASK]: build vocabulary\n");
    vcb = TextBuildVocab(V_TEXT_FILE_PATH, 1, -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
  }
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  V = vcb->size;
  LOG(1, "Actual V  : %d\n", V);
  // build peek set if necessary, and load
  if (!fexists(V_PEEK_FILE_PATH) || V_PEEK_OVERWRITE) {
    LOG(0, "[TASK]: build peek set\n");
    peek_size = PeekBuildSentSet(V_TEXT_FILE_PATH, V_PEEK_SAMPLE_RATE, vcb,
                                 &peek_wids, &peek_wnum);
    PeekSaveSentSet(V_PEEK_FILE_PATH, peek_wids, peek_wnum, peek_size);
    // build peek set for w2v
    PeekSaveTextSentSet(sformat("%s.txt", V_PEEK_FILE_PATH), peek_wids,
                        peek_wnum, peek_size, vcb);
  }
  peek_size = PeekLoadSentSet(V_PEEK_FILE_PATH, &peek_wids, &peek_wnum);
  LOG(1, "Peek size : %d\n", peek_size);
  // initialization
  ModelInit();
  max_hn = V * V_OFFLINE_INTERVAL_VOCAB_RATIO;
  // schedule thread jobs
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));
  for (tid = 0; tid < V_THREAD_NUM; tid++)
    pthread_create(&pt[tid], NULL, ThreadWork, (void*)tid);
  for (tid = 0; tid < V_THREAD_NUM; tid++) pthread_join(pt[tid], NULL);
  ModelSave(-1);  // save model
  ModelFree();    // free
  VocabDestroy(vcb);
  free(progress);
  free(pt);
  LOG(1, "\nTraining finished\n");
  return;
}

int main(int argc, char* argv[]) {
  ScheduleWork();
  return 0;
}
