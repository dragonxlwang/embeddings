//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "variables.c"

int N, K, V;

// model parameter is shared across all threads
struct Model {
  real* scr;  // source vector;
  real* tar;  // target vector;
}* model;

// each thread worker maintains a bookkeeping
struct Bookkeeping {
  real* dd;   // list(K):vector(V) dual distribution
  real* ent;  // list(K):
  real* ww;   // list(K):vector(N) W * dd, synthesis word in prime
  real* hh;   // list(K):vector(N) avg(h_k), sufficient stats
  int* hn;    // list(K): |h_k|
};
real gd_ss;  // gradient descent step size

// after this times vocabulary online updates, perform one offline update
real offline_interval_vocab_ratio = 1.0;

// vocabulary
struct Vocabulary* vcb;

void ModelGdUpdate(struct Model* m, int p, int i, real c, real* g) {
  // model update: p=0: scr; p=1: tar
  int j;
  if (p == 0) {
    // update scr
    for (j = 0; j < N; j++) m->scr[i * N + j] -= c * gd_ss * g[j];
  } else {
    // update tar
    for (j = 0; j < N; j++) m->tar[i * N + j] -= c * gd_ss * g[j];
  }
  return;
}

int DualDecode(real* h, struct Bookkeeping* b) {
  // fit the sentence with the best dual distribution
  real s, t;
  int z;
  for (int k = 0; k < K; k++) {
    t = -NumVecDot(h, b->ww + k * N, N) - b->ent[k];
    if (k == 0 || s > t) {
      s = t;
      z = k;
    }
  }
  // update bookkeeping: hh, hn
  NumVecAddCVec(b->hh + z * N, h, 1.0, N);
  b->hn++;
  return z;
}

void DualUpateOffline(struct Bookkeeping* b, struct Model* m) {
  // update dual distributions and others
  // update hh
  for (int k = 0; k < K; k++) NumVecMulC(b->hh + k * N, 1.0 / b->hn[k], N);
  for (int k = 0; k < K; k++) {
    // update dd
    for (int i = 0; i < V; i++) {
      b->dd[k * V + i] = NumVecDot(m->tar + i * N, b->hh + k * N, N);
    }
    // normalize dd and update ent
    b->ent[k] = NumSoftMax(b->dd + k * N, N);
    // update ww
    NumMulMatVec(m->tar, b->dd + k * V, N, V, b->ww + k * N);
  }
  return;
}

void PrimalUpdateOnline(int* ids, int l, struct Bookkeeping* b,
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
    ModelGdUpdate(m, 1, ids[i], 1, h);
  }
  // update m->scr
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(w0, m->tar + ids[i] * N, 1, -1, N, w);
    NumVecAddCVec(w, b->ww + z[i] * N, 1, N);
    // update m->scr
    ModelGdUpdate(m, 0, ids[i], 1, w);
  }
}

void PrimalUpdateOffline(struct Bookkeeping* b, struct Model* m) {
  // update primal parameters offline: update m->tar
  // call before DualUpdateOffline
  int i, k;
  for (k = 0; k < K; k++) {
    for (i = 0; i < V; i++) {
      ModelGdUpdate(m, 1, i, b->dd[k * V + i], b->hh + k * N);
    }
  }
  return;
}

void* ThreadWork(void* arg) {
  int i, j, k;
  int tid = (long)arg;
  FILE* fin = fopen(V_TEXT_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!");
    exit(1);
  }
  fseek(fin, 0, SEEK_END);
  long int fsz = ftell(fin);
  long int fbeg = fsz / V_THREAD_NUM * tid;
  long int fend = fsz / V_THREAD_NUM * (tid + 1);
  // thread worker works sent_num sentences (sent_num) for iter iterations
  struct Bookkeeping* b =
      (struct Bookkeeping*)malloc(sizeof(struct Bookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(K);
  fseek(fin, fbeg, SEEK_SET);
  int iter_num = 0, wids[SUP], wnum;
  long long int online_cnt = 0;
  long long int offline_int = V * offline_interval_vocab_ratio;
  while (iter_num < V_ITER_NUM) {
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    online_cnt += wnum;
    PrimalUpdateOnline(wids, wnum, b, model);
    if (online_cnt >= offline_int) {
      PrimalUpdateOffline(b, model);
      DualUpateOffline(b, model);
      online_cnt = 0;
    }
    if (feof(fin) || ftell(fin) >= fend) {
      iter_num++;
      fseek(fin, fbeg, SEEK_SET);
    }
  }
  return 0;
}

void ScheduleWork() {
  long long int tid;
  vcb = TextLoadVocab(V_TEXT_VOCAB_PATH, -1, 0);
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));
  for (tid = 0; tid < V_THREAD_NUM; tid++) {
    pthread_create(&pt[tid], NULL, ThreadWork, (void*)tid);
  }
}

int main(int argc, char* argv[]) { return 0; }
