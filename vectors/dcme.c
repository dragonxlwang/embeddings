//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////
#ifndef DCME
#define DCME

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
#include "../vectors/model.c"
#include "../vectors/peek.c"
#include "../vectors/variables.c"

typedef struct DcmeBookkeeping {  // each thread worker maintains a bookkeeping
  real* dd;                       // list(K):vector(V) dual distribution
  real* ent;                      // list(K):
  real* ww;                       // list(K):vector(N) W * dd, synthesis word
  real* hh;                       // list(K):vector(N) avg(h_k)
  int* hn;                        // list(K): |h_k|
  int* tw;                        // list(K):list(Q) top words
  real* twps;                     // list(K): top words probability sum
  real* wwow;                     // list(K):vector(N)
} DcmeBookkeeping;

#ifdef DEBUG
int sid_dcme_dual_update_cnt = 0;
int sid_dcme_dual_update_cluster_cnt[1000] = {0};
#endif

int sid_dcme_ppb_lock = 0;
void DcmeThreadPrintProgBar(int dbg_lvl, int tid, real p, DcmeBookkeeping* b) {
  if (sid_dcme_ppb_lock) return;
  sid_dcme_ppb_lock = 1;
  char* mdis =
      ModelDebugInfoStr(model, p, tid, start_clock_t, V_THREAD_NUM, gd_ss);
#ifdef DEBUG
  char* ddis = malloc(0x1000);
  real eem = NumVecMean(b->ent, K);
  real ees = NumVecStd(b->ent, K);
  real avgp = PeekEvalSingleThread(model, ps, C);
  pair* dps = sortedi(sid_dcme_dual_update_cluster_cnt, K, 1);
  sprintfc(ddis, 'g', 'k', "ENT:%.2e\u00b1%.2e", eem, ees);
  saprintf(ddis, " ");
  saprintfc(ddis, 'c', 'r', "PEEK:%.2e", avgp);
  saprintf(ddis, "\n");
  int j, k;
  for (j = 0; j < K; j++) {
    if (j % 10 == 0 && j != 0) saprintf(ddis, "\n");
    k = dps[j].key;
    saprintfc(ddis, 'y', 'k', "[%02d]:%.3lf:", k,
              (real)sid_dcme_dual_update_cluster_cnt[k] /
                  (sid_dcme_dual_update_cnt + 1));
    saprintfc(ddis, 'c', 'k', "%.2e", b->ent[k]);
    saprintf(ddis, " ");
  }
  free(dps);
  LOGCR(dbg_lvl);
  if (V_MODEL_DECOR_FILE_PATH) LOG(dbg_lvl, "[%s]: ", V_MODEL_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", mdis);
  LOG(dbg_lvl, " ");
  LOG(dbg_lvl, "%s", ddis);
  LOGCR(dbg_lvl);
  free(mdis);
  free(ddis);
#else
  LOGCLR(dbg_lvl);
  if (V_MODEL_DECOR_FILE_PATH) LOG(dbg_lvl, "[%s]: ", V_MODEL_DECOR_FILE_PATH);
  LOG(dbg_lvl, "%s", mdis);
  free(mdis);
#endif
  sid_dcme_ppb_lock = 0;
  return;
}

DcmeBookkeeping* DcmeBookkeepingCreate() {
  DcmeBookkeeping* b = (DcmeBookkeeping*)malloc(sizeof(DcmeBookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hn = NumNewHugeIntVec(K);
  b->tw = NumNewHugeIntVec(Q * K);
  b->twps = NumNewHugeVec(K);
  b->wwow = NumNewHugeVec(N * K);
  NumRandFillVec(b->hh, N * K, -model_init_amp, model_init_amp);
  NumFillValIntVec(b->hn, K, 1);
  return b;
}

void DcmeBookkeepingFree(DcmeBookkeeping* b) {
  free(b->dd);
  free(b->ent);
  free(b->ww);
  free(b->hh);
  free(b->hn);
  free(b->tw);
  free(b->twps);
  free(b->wwow);
  free(b);
  return;
}

int DcmeDualDecode(real* h, DcmeBookkeeping* b) {
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

void DcmeDualUpdate(int zz, DcmeBookkeeping* b, heap* twh) {
  int j, k;
  NumMulMatVec(model->tar, b->hh + zz * N, V, N, b->dd + zz * V);  // dd
  b->ent[zz] = NumSoftMax(b->dd + zz * V, b->hn[zz], V);           // ent (sm)
  HeapEmpty(twh);                                                  // tw reset
  for (j = 0; j < V; j++) HeapPush(twh, j, b->dd[zz * V + j]);     // tw add
  for (j = 0; j < Q; j++) b->tw[zz * Q + j] = twh->d[j].key;       // tw load
  qsort(b->tw + zz * Q, Q, sizeof(int), cmp_int);                  // tw sort
  if (!V_MICRO_ME) {
    NumMulVecMat(b->dd + zz * V, model->tar, V, N, b->ww + zz * N);  // ww
  } else {
    NumFillZeroVec(b->wwow + zz * N, N);
    NumFillZeroVec(b->ww + zz * N, N);
    b->twps[zz] = 0;
    for (j = 0, k = 0; j < V; j++) {  // ww, wwow: two way merge of tw and ow
      if (j == b->tw[zz * Q + k]) {   // topic words
        NumVecAddCVec(b->ww + zz * N, model->tar + j * N, b->dd[zz * V + j], N);
        b->twps[zz] += b->dd[zz * N + j];
        k++;
      } else
        NumVecAddCVec(b->wwow + zz * N, model->tar + j * N, b->dd[zz * V + j],
                      N);
    }
    NumVecAddCVec(b->ww + zz * N, b->wwow + zz * N, 1, N);  // ww: adding ow
  }
  return;
}

void DcmeDualReset(int zz, DcmeBookkeeping* b) {
  // dual reset distribution (hh and hn) of zz
  int dropt = 1;                         // several options are available:
  if (dropt == 1) {                      //  1) clean reset
    b->hn[zz] = 0;                       //
    NumFillZeroVec(b->hh + zz * N, N);   //
  } else if (dropt == 2) {               //  2) half reset
    b->hn[zz] *= 0.5;                    //
    NumVecMulC(b->hh + zz * N, 0.5, N);  //
  } else if (dropt == 3) {               //  3) decay/overshoot reset
    real v = 0.1;                        //
    b->hn[zz] *= v;                      //
    NumVecMulC(b->hh + zz * N, v, N);    //
  }                                      //  4) no reset
  return;
}

int Update(int* ids, int l, DcmeBookkeeping* b, heap* twh) {
  // dual/primal update for one sentence (ids, l)
  int offline_performed = 0;
  int i, j, k, lt, rt, md;
  real h0[NUP], h[NUP], hw[SUP], wd[NUP * SUP], w0[NUP], w[NUP], twp[QUP];
  int zz, h0n = 0;
  int window = C;    // int window = NumRand() * C + 1;
  int twidx, twlen;  // target ward idx, micro me dim
  real twps;         // sum of tw words
  NumFillZeroVec(h0, N);
  NumFillZeroVec(w0, N);
  for (i = 0; i < SMALLER(l, window); i++) {
    NumVecAddCVec(h0, model->scr + ids[i] * N, 1, N);  // h0++
    h0n++;                                             // h0n++
  }
  for (i = 0; i < l; i++) {
    lt = i - window - 1;
    rt = i + window;
    md = i;
    if (rt < l) {
      NumVecAddCVec(h0, model->scr + ids[rt] * N, 1, N);   // h0++
      h0n++;                                               // h0n++
    }                                                      //
    if (lt >= 0) {                                         //
      NumVecAddCVec(h0, model->scr + ids[lt] * N, -1, N);  // h0--
      h0n--;                                               // h0n--
    }                                                      //
    hw[md] = 1.0 / (h0n - 1.0);                            // hw
    NumAddCVecDVec(h0, model->scr + ids[md] * N, hw[md], -hw[md], N, h);  // h
    zz = DcmeDualDecode(h, b);                 // dcd z
    NumVecAddCVec(b->hh + zz * N, h, 1.0, N);  // b->hh
    b->hn[zz]++;                               // b->hn
    if (!V_MICRO_ME) {
      NumAddCVecDVec(model->tar + ids[i] * N, b->ww + zz * N, 1, -1, N,
                     wd + i * N);
      // up tar (pos), wd[i] <- w - ww
      ModelGradUpdate(model, 1, ids[i], -gd_ss, h);
      ModelVecRegularize(model, 1, ids[i], V_MODEL_PROJ_BALL_NORM, -1);
      for (k = 0; k < Q; k++) {  // up tar (neg, top words)
        j = b->tw[zz * Q + k];
        ModelGradUpdate(model, 1, j, b->dd[zz * V + j] * gd_ss, h);
        ModelVecRegularize(model, 1, j, V_MODEL_PROJ_BALL_NORM, -1);
      }
    } else {
      twidx = Q;
      for (k = 0; k < Q; k++) {  // micro ME
        j = b->tw[zz * Q + k];
        twp[k] = NumVecDot(model->tar + j * N, h, N);
        if (j == ids[i]) twidx = j;
      }
      if (twidx == Q) {
        twp[Q] = NumVecDot(model->tar + ids[i] * N, h, N);
        twlen = Q + 1;
        twps = b->twps[zz] + b->dd[zz * V + ids[i]];  // merge
      } else {
        twlen = Q;
        twps = b->twps[zz];
      }
      NumSoftMax(twp, 1.0, twlen);
      NumVecMulC(twp, twps, twlen);
      if (twidx == Q) twp[twidx] -= b->dd[zz * V + ids[i]];  // split
      if (V_MICRO_ME_SCR_UPDATE) {
        NumAddCVecDVec(model->tar + ids[i] * N, b->wwow + zz * N, 1, -1, N,
                       wd + i * N);  // wd[i] <- w - wwow - wwtw
        for (k = 0; k < twlen; k++) {
          if (k == Q)
            j = ids[i];
          else
            j = b->tw[zz * Q + k];
          NumVecAddCVec(wd + i * N, model->tar + j * N, -twp[k], N);
        }
      } else {
        NumAddCVecDVec(model->tar + ids[i] * N, b->ww + zz * N, 1, -1, N,
                       wd + i * N);
      }
      // up tar (pos)
      ModelGradUpdate(model, 1, ids[i], -gd_ss, h);
      ModelVecRegularize(model, 1, ids[i], V_MODEL_PROJ_BALL_NORM, -1);
      for (k = 0; k < twlen; k++) {  // up tar (neg, top words)
        if (k == Q)
          j = ids[i];
        else
          j = b->tw[zz * Q + k];
        ModelGradUpdate(model, 1, j, twp[k] * gd_ss, h);
        ModelVecRegularize(model, 1, j, V_MODEL_PROJ_BALL_NORM, -1);
      }
    }
    lt = i - 2 * window - 1;
    rt = i;
    md = i - window;
    NumVecAddCVec(w0, wd + rt * N, hw[rt], N);                  // w0++
    while (md < l) {                                            //
      if (lt >= 0) NumVecAddCVec(w0, wd + lt * N, -hw[lt], N);  // w0--
      if (md >= 0) {                                            //
        NumAddCVecDVec(w0, wd + md * N, 1, -hw[md], N, w);      // w
        ModelGradUpdate(model, 0, ids[md], -gd_ss, w);          // up m->scr
        ModelVecRegularize(model, 0, ids[md], V_MODEL_PROJ_BALL_NORM, -1);
      }
      if (i == l - 1) {
        md++;
        lt++;
      } else
        break;
    }
    if (b->hn[zz] >= V * V_OFFLINE_INTERVAL_VOCAB_RATIO) {  // update offline
      offline_performed = 1;                                //
      for (j = 0, k = 0; j < V; j++) {        // up m->tar (neg, other words)
        if (k < Q && j == b->tw[zz * Q + k])  // join two sorted list
          k++;
        else {
          ModelGradUpdate(model, 1, j, b->dd[zz * V + j] * gd_ss,
                          b->hh + zz * N);
          ModelVecRegularize(model, 1, j, V_MODEL_PROJ_BALL_NORM, -1);
        }
      }
      if (V_L2_REGULARIZATION_WEIGHT != 0)
        ModelShrink(model,
                    V_L2_REGULARIZATION_WEIGHT);  // optional model shrink
      DcmeDualUpdate(zz, b, twh);  // dual update based on hh and hn
      DcmeDualReset(zz, b);        // reset hh and hn
#ifdef DEBUG
      sid_dcme_dual_update_cnt++;
      sid_dcme_dual_update_cluster_cnt[zz]++;
#endif
    }
  }
  return offline_performed;
}

void* DcmeThreadTrain(void* arg) {
  int k;
  int tid = (long)arg;
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  DcmeBookkeeping* b = DcmeBookkeepingCreate();
  heap* twh = HeapCreate(Q);
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  long int fpos = fbeg;
  int wids[SUP], wnum;
  real p = 0;
  int iter_num = 0;
  for (k = 0; k < K; k++) {  // initialize dual
    DcmeDualUpdate(k, b, twh);
    DcmeDualReset(k, b);
  }
  fseek(fin, fbeg, SEEK_SET);  // training
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    fpos = ftell(fin);
    if (wnum > 1 && Update(wids, wnum, b, twh)) {
      progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);  // prog
      p = ModelTrainProgress(progress, V_THREAD_NUM, V_ITER_NUM);        //
      gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);                   // gdss
      DcmeThreadPrintProgBar(2, tid, p, b);                              // info
    }
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_MODEL)
        ModelSave(model, iter_num, V_MODEL_SAVE_PATH);
      iter_num++;
    }
  }
  HeapFree(twh);
  DcmeBookkeepingFree(b);
  fclose(fin);
  pthread_exit(NULL);
  return 0;
}

#endif /* ifndef DCME */
