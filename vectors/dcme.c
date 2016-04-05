//////////////////////////////////////////////////////////////////
// Copyright (c) 2016 Xiaolong Wang All Rights Reserved.        //
// Code for ``Dual Clustered Maximal Entropy for Many Classes'' //
//////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "constants.c"

int N, K, V;

// model parameter is shared across all threads
struct Model {
  real *scr;  // source vector;
  real *tar;  // target vector;
};

// each thread worker maintains a bookkeeping
struct Bookkeeping {
  real *dd;   // list(K):vector(V) dual distribution
  real *ent;  // list(K):
  real *ww;   // list(K):vector(N) W * dd, synthesis word in prime
  real *hh;   // list(K):vector(N) avg(h_k), sufficient stats
  int *hn;    // list(K): |h_k|
};
real gd_ss;  // gradient descent step size

void ModelGdUpdate(struct Model *m, int p, int i, real c, real *g) {
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

// fit the sentence with the best dual distribution
int DualDecode(real *h, struct Bookkeeping *b) {
  real s, t;
  int z;
  for (int k = 0; k < K; k++) {
    t = -NumVecDot(h, b->ww + k * N, N) - b->ent[k];
    if (k == 0 || s > t) {
      s = t;
      z = k;
    }
  }
  // update bookkeeping
  NumVecAddCVec(b->hh + z * N, h, 1.0, N);
  b->hn++;
  return z;
}

// update dual distributions and others
void DualUpate(struct Bookkeeping *b, struct Model *m) {
  // update hh
  for (int k = 0; k < K; k++) NumVecMulC(b->hh + k * N, 1.0 / b->hn[k], N);
  for (int k = 0; k < K; k++) {
    // update dd
    for (int i = 0; i < V; i++) {
      b->dd[k * V + i] = NumVecDot(m->tar + i * N, b->hh + k * N, N);
    }
    // update ent
    b->ent[k] = NumSoftMax(b->dd + k * N, N);
    // update ww
    NumMulMatVec(m->tar, b->dd + k * V, N, V, b->ww + k * N);
  }
  return;
}

// update primal parameters online
void PrimalUpdateOnline(int *ids, int l, struct Bookkeeping *b,
                        struct Model *m) {
  int i;
  real h0[NUP], h[NUP], w0[NUP], w[NUP];
  int z[SUP];
  memset(h0, 0, N * sizeof(real));
  memset(w0, 0, N * sizeof(real));
  for (i = 0; i < l; i++) NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  // dual decode
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(h0, m->scr + ids[i] * N, 1, -1, N, h);
    z[i] = DualDecode(h, b);
    NumVecAddCVec(w0, m->tar + ids[i] * N, 1, N);
    NumVecAddCVec(w0, b->ww + z[i] * N, -1, N);
    // update m->tar
    ModelGdUpdate(m, 1, ids[i], 1, h);
  }
  // update m->scr
  for (i = 0; i < l; i++) {
    NumAddCVecDVec(w0, m->tar + ids[i] * N, 1, -1, N, w);
    NumVecAddCVec(w, b->ww + z[i] * N, 1, N);
    ModelGdUpdate(m, 0, ids[i], 1, w);
  }
}

// update primal parameters offline (update m->tar)
void PrimalUpdateOffline(struct Bookkeeping *b, struct Model *m) {
  int i, k;
  for (k = 0; k < K; k++) {
    for (i = 0; i < V; i++) {
      ModelGdUpdate(m, 1, i, b->dd[k * V + i], b->hh + k * N);
    }
  }
  return;
}

void ThreadWork() {
  struct Bookkeeping *b =
      (struct Bookkeeping *)malloc(sizeof(struct Bookkeeping));
  b->dd = NumNewHugeVec(V * K);
  b->ent = NumNewHugeVec(K);
  b->ww = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(N * K);
  b->hh = NumNewHugeVec(K);

  return;
}

int main(int argc, char *argv[]) { return 0; }
