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
  real* www;  // debug use
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
  /* real ww = NumVecNorm(b->ww, K * N); */
  /* LOG(0, " ww=%.2e/", ww); */
  /* LOGC(0, 'y', 'k', "%.2ett ", ww / tt); */
  /* heap* h = HeapCreate(5); */
  /* for (i = 0; i < V; i++) HeapPush(h, i, NumVecNorm(model->tar + i * N, N));
   */
  /* HeapSort(h); */
  /* LOGC(0, 'c', 'k', "tt=%.2e ", tt); */
  /* for (i = 0; i < 5; i++) { */
  /*   LOGC(0, 'b', 'k', "%s", vcb->id2wd[h->d[i].key]); */
  /*   LOG(0, ":%lf ", h->d[i].val); */
  /* } */
  real scr = NumVecNorm(model->scr, V * N);
  real ss = NumMatMaxRowNorm(model->scr, V, N);
  real tar = NumVecNorm(model->tar, V * N);
  real tt = NumMatMaxRowNorm(model->tar, V, N);
  LOG(0, " scr=%.2e/%.2ess", scr, scr / ss);
  LOG(0, " tar=%.2e/%.2ett", tar, tar / tt);
  LOGC(0, 'r', 'k', " tt=%.2e", tt);
  real eem = NumVecMean(b->ent, K);
  real ees = NumVecStd(b->ent, K);
  LOGC(0, 'w', 'r', " ENT:%.2e/%.2e", eem, ees);
  real avgp = PeekEval(model->scr, model->tar, peek_wids, peek_wnum, peek_size);
  LOGC(0, 'r', 'c', " PEEK:%.2e", avgp);
#endif
  ppb_lock = 0;
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
  b->tw = NumNewHugeIntVec(Q * K);
  NumRandFillVec(b->hh, N * K, -1e-1, 1e-1);
  NumFillValIntVec(b->hn, K, 1);
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

int saved_iter_num = -1;
void ModelSave(int iter_num) {
  char* mfp;
  if (iter_num == -1) {                 // save final model
    STR_CLONE(mfp, V_MODEL_SAVE_PATH);  // so that free(mfp) can work
  } else {
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

real scr_norm = -1;
real tar_norm = -1;
real vs_norm = -1;
real vt_norm = -1;
real old_norm = -1;
real new_norm = -1;
real g_norm = -1;
int vsi = 0;
int vti = 0;
long long int mgucnt = 0;
int ifprint = 0;
void ModelGradUpdate(struct Model* m, int p, int i, real c, real* g) {
#ifdef DEBUG
  real nm = 0;
  g_norm = sqrt(NumVecDot(g, g, N));
  if (ifprint) {
    if (scr_norm < 0) scr_norm = sqrt(NumVecDot(m->scr, m->scr, N * V));
    if (tar_norm < 0) tar_norm = sqrt(NumVecDot(m->tar, m->tar, N * V));
    if (vs_norm < 0) vs_norm = NumMatMaxRowNorm(m->scr, V, N);
    if (vt_norm < 0) vt_norm = NumMatMaxRowNorm(m->tar, V, N);
    if (p == 0) {
      old_norm = sqrt(NumVecDot(m->scr + i * N, m->scr + i * N, N));
      scr_norm = sqrt(scr_norm * scr_norm - old_norm * old_norm);
    } else {
      old_norm = sqrt(NumVecDot(m->tar + i * N, m->tar + i * N, N));
      tar_norm = sqrt(tar_norm * tar_norm - old_norm * old_norm);
    }
  }
#endif
  // model minimization update: p=0: scr; p=1: tar
  if (p == 0) {  // update scr
    NumVecAddCVec(m->scr + i * N, g, -c * gd_ss, N);
  } else {  // update tar
    NumVecAddCVec(m->tar + i * N, g, -c * gd_ss, N);
  }
#ifdef DEBUG
  if (ifprint) {
    if (p == 0) {
      nm = sqrt(NumVecDot(m->scr + i * N, m->scr + i * N, N));
      new_norm = nm;
      scr_norm = sqrt(scr_norm * scr_norm + nm * nm);
      if (nm > vs_norm) {
        vs_norm = nm;
        vsi = i;
      }
    } else {
      nm = sqrt(NumVecDot(m->tar + i * N, m->tar + i * N, N));
      new_norm = nm;
      tar_norm = sqrt(tar_norm * tar_norm + nm * nm);
      if (nm > vt_norm) {
        vt_norm = nm;
        vti = i;
      }
    }
    clock_t cur_clock_t = clock();
    double elapsed_time =
        (double)(cur_clock_t - start_clock_t) / CLOCKS_PER_SEC;
    char* ht = strtime(elapsed_time / V_THREAD_NUM);
    printf("scr_norm=%.4e tar_norm=%.4e ", scr_norm, tar_norm);
    LOGC(0, 'c', 'k', "%15s ss=%.4e %06d ", VocabGetWord(vcb, vsi), vs_norm,
         vsi);
    LOGC(0, 'r', 'k', "%15s tt=%.4e %06d ", VocabGetWord(vcb, vti), vt_norm,
         vti);
    LOGC(0, 'b', 'k', "%.4e~%.4e*%.2e=>%.4e ", old_norm, g_norm, c * gd_ss,
         new_norm);
    char colcode = 'y';
    if (i == vsi) colcode = 'c';
    if (i == vti) colcode = 'r';
    LOGC(0, colcode, 'k', "%-15s[%s] ", VocabGetWord(vcb, i),
         p == 0 ? "scr" : "tar");
    printf("%s %lld\n", ht, mgucnt++);
    if (mgucnt % 10000 == 0) getc(stdin);
  }
#endif
  return;
}

void ModelShrink(struct Model* m) {
  int i;
  if (V_L2_REGULARIZATION_WEIGHT != 0) {
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
  NumVecMulC(b->hh + zz * N, 1.0 / b->hn[zz], N);               // hh avg
  NumMulMatVec(m->tar, b->hh + zz * N, V, N, b->dd + zz * V);   // dd
  b->ent[zz] = NumSoftMax(b->dd + zz * V, V);                   // ent (sm)
  HeapEmpty(twh);                                               // tw reset
  for (j = 0; j < V; j++) HeapPush(twh, j, b->dd[zz * V + j]);  // tw add
  for (j = 0; j < Q; j++) b->tw[zz * Q + j] = twh->d[j].key;    // tw dump
  qsort(b->tw + zz * Q, Q, sizeof(int), cmp_int);               // tw sort
  NumMulVecMat(b->dd + zz * V, m->tar, V, N, b->ww + zz * N);   // ww
  b->hn[zz] = 0;                                                // reset hn
  NumFillZeroVec(b->hh + zz * N, N);                            // reset hh
  return;
}

int dual_update_cnt = 0;
#ifdef DEBUG
int dual_update_cluster_cnt[1000] = {0};
#endif
int Update(int* ids, int l, struct Bookkeeping* b, struct Model* m, heap* twh) {
  // dual/primal update for one sentence (ids, l)
  int offline_performed = 0;
  int i, j, k, lt, rt, md;
  real h0[NUP], h[NUP], hw[SUP], wd[NUP * SUP], w0[NUP], w[NUP];
  int zz;
  int neg_id;
  /* int window = NumRand() * C + 1; */
  int window = C;
  NumFillZeroVec(h0, N);
  NumFillZeroVec(w0, N);
  for (i = 0; i < SMALLER(l, window); i++)
    NumVecAddCVec(h0, m->scr + ids[i] * N, 1, N);
  for (i = 0; i < l; i++) {
    lt = i - window - 1;
    rt = i + window;
    md = i;
    if (lt >= 0) NumVecAddCVec(h0, m->scr + ids[lt] * N, -1, N);      // h0--
    if (rt < l) NumVecAddCVec(h0, m->scr + ids[rt] * N, 1, N);        // h0++
    hw[md] = 1.0 / (SMALLER(rt, l - 1) - LARGER(lt, 0));              // hw
    NumAddCVecDVec(h0, m->scr + ids[md] * N, hw[md], -hw[md], N, h);  // h
    zz = DualDecode(h, b);                                            // dcd z
    // update bookkeeping: hh, hn
    NumVecAddCVec(b->hh + zz * N, h, 1.0, N);
    b->hn[zz]++;
    // update primal model
    NumAddCVecDVec(m->tar + ids[i] * N, b->ww + zz * N, 1, -1, N,
                   wd + i * N);            // wd[i] <- w - ww
    ModelGradUpdate(m, 1, ids[i], -1, h);  // up m->tar (pos)
    for (j = 0; j < Q; j++) {              // up m->tar (neg)
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
    // update offline
    if (b->hn[zz] >= max_hn) {
      offline_performed = 1;
      // update primal model
      for (j = 0, k = 0; j < V; j++)  // up m->tar (neg sampling)
        if (k < Q && j == b->tw[zz * Q + k])
          k++;
        else
          ModelGradUpdate(m, 1, j, b->dd[zz * V + j], b->hh + zz * N);
      ModelShrink(m);
#ifdef DEBUG
      LOGC(0, 'c', 'k', "\n=> [%d]: dual update begins: %d\n", dual_update_cnt,
           zz);
      printf("TT: %lf\n", NumMatMaxRowNorm(m->tar, V, N));
      printf("hh norm for cluster [%d]: %lf\n", zz,
             NumVecNorm(b->hh + zz * N, N));
#endif
      DualUpdate(zz, b, m, twh);  // dual update based on hh and hn
#ifdef DEBUG
      FILE* fout_tmp = fopen("tmp.txt", "wb");
      pair* pairs = sorted(b->dd + zz * V, V, 1);
      for (j = 0; j < V; j++) {
        fprintf(fout_tmp, "%d: %8d, %-20s %.6e\n", j, pairs[j].key,
                VocabGetWord(vcb, pairs[j].key), pairs[j].val);
      }
      fclose(fout_tmp);

      printf("k = %d, ww = %.6e, ent = %.6e \n", zz,
             NumVecNorm(b->ww + zz * N, N), b->ent[zz]);
      dual_update_cluster_cnt[zz]++;
      pair* dps = sortedi(dual_update_cluster_cnt, K, 1);
      for (j = 0; j < K; j++) {
        if (j % 25 == 0 && j != 0) printf("\n");
        k = dps[j].key;
        int color_code = 'y';
        if (k == zz) color_code = 'g';
        LOGC(0, color_code, 'k', "[%02d]:%.3lf ", k,
             (real)dual_update_cluster_cnt[k] / (dual_update_cnt + 1));
      }
      printf("\n");
      free(dps);
      /* LOGC(0, 'b', 'k', "print distribution of [%d]: [y/N] ", zz); */
      /* if (getc(stdin) == 'y') { */
      pair* ps = sorted(b->dd + zz * V, V, 1);
      int printed_adw_cnt = 0;
      for (j = 0; j < V; j++) {
        k = ps[j].key;
        if (b->dd[zz * V + k] > 10.0 / V) {
          // abonormal distribution weight
          LOGC(0, 'g', 'k', "[TOC:%02d-W:%04d] %15s = %.6lf\n", zz, k,
               VocabGetWord(vcb, k), b->dd[zz * V + k]);
          if (printed_adw_cnt++ >= 50) break;
        }
      }
      free(ps);
      /* } */
      /* getc(stdin); */
      LOGC(0, 'c', 'k', "<= [%d]: dual update finishes: %d\n", dual_update_cnt,
           zz);
      dual_update_cnt++;
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
  // initialize dual variables
  for (k = 0; k < K; k++) DualUpdate(k, b, model, twh);
  // training
  fseek(fin, fbeg, SEEK_SET);
#ifdef DEBUG
  int iter_begin = 1;
#endif
  while (iter_num < V_ITER_NUM) {
#ifdef DEBUG
    if (iter_begin) {
      iter_begin = 0;
      ifprint = 0;
      /* LOGC(0, 'r', 'k', "\n[%d]: print detailed info: [y/N] ",
       * dual_update_cnt); */
      /* if (getc(stdin) == 'y') */
      /*   ifprint = 1; */
      /* else */
      /*   ifprint = 0; */
      /* getc(stdin); */
    }
#endif
    wnum = TextReadSent(fin, vcb, wids, 1, 1);
    if (wnum > 1)
      if (Update(wids, wnum, b, model, twh)) {
        // thread progress
        progress[tid] = iter_num + (double)(fpos - fbeg) / (fend - fbeg);
        // gd_ss
        p = GetProgress();
        gd_ss = V_INIT_GRAD_DESCENT_STEP_SIZE * (1 - p);
        // debug info
        ThreadPrintProgBar(2, tid, p, b);
#ifdef DEBUG
        iter_begin = 1;
#endif
      }
    fpos = ftell(fin);
    if (feof(fin) || fpos >= fend) {
      fseek(fin, fbeg, SEEK_SET);
      if (V_CACHE_INTERMEDIATE_MODEL) ModelSave(iter_num);
      iter_num++;
    }
  }
  fclose(fin);
  HeapDestroy(twh);
  BookkeepingFree(b);
  pthread_exit(NULL);
  return 0;
}

void ScheduleWork() {
  long long int tid;
  start_clock_t = clock();
  progress = (real*)malloc(V_THREAD_NUM * sizeof(real));
  NumFillZeroVec(progress, V_THREAD_NUM);
  // included initialization
  VariableInit();
  NumInit();
  // build vocab if necessary, load, and set V by smaller actual size
  if (!fexists(V_VOCAB_FILE_PATH) || V_VOCAB_OVERWRITE) {
    LOG(0, "[TASK]: build vocabulary\n");
    vcb = TextBuildVocab(V_TEXT_FILE_PATH, 1, -1);
    TextSaveVocab(V_VOCAB_FILE_PATH, vcb);
  }
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  V = vcb->size;
  LOG(1, "Actual V: %d\n", V);
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
  // initialization
  ModelInit();
  max_hn = V * V_OFFLINE_INTERVAL_VOCAB_RATIO;
  // schedule thread jobs
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));
  for (tid = 0; tid < V_THREAD_NUM; tid++)
    pthread_create(&pt[tid], NULL, ThreadWork, (void*)tid);
  for (tid = 0; tid < V_THREAD_NUM; tid++) pthread_join(pt[tid], NULL);
  // save model
  ModelSave(-1);
  // free memory
  ModelFree();
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
