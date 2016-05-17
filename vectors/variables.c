#ifndef VARIABLES
#define VARIABLES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_file.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

// The following parameters are set in order to have variables on stack
#define NUP 200      // upper bound for N  (embedding dimension)
#define KUP 200      // upper bound for K  (dual dimension)
#define VUP 0xFFFFF  // upper bound for V (vocabulary)
#define WUP 100      // upper bound for character number in a word
#define SUP 200      // upper bound for word number in a sentence

/* char *V_TEXT_FILE_PATH = "~/data/gigaword/giga_nyt.txt"; */
char *V_TEXT_FILE_PATH = "~/data/text8/text8";
char *V_VOCAB_FILE_PATH = NULL;  // don't set it if can be inferred from above
char *V_MODEL_SAVE_PATH = NULL;
int V_THREAD_NUM = 20;
int V_ITER_NUM = 15;
// every this number times vocabulary online updates perform one offline update
real V_OFFLINE_INTERVAL_VOCAB_RATIO = 0.1;
// for dual burn in, run this number times vocabulary online updates
real V_BURN_IN_INTERVAL_VOCAB_RATIO = 5;
// Initial grad descent step size
real V_INIT_GRAD_DESCENT_STEP_SIZE = 1e-3;
// Model Shrink: l-2 regularization:
real V_L2_REGULARIZATION_WEIGHT = 0;  // 1e-3;
// Model Shrink: if proj model to unit ball
int V_MODEL_PROJ_UNIT_BALL = 0;
// Vocab loading option: cut-off high frequent (stop) words
int V_VOCAB_HIGH_FREQ_CUTOFF = 80;
// if cache model per iteration
int V_CACHE_INTERMEDIATE_MODEL = 0;
// if overwrite vocab file
int V_VOCAB_OVERWRITE = 0;
// Dimension for embedding, number of dual cluster, vocabulary size cap, and
// context length
int N = 100;
int K = 100;
int V = 100000;  // set to -1 if no limit
int C = 5;

void PrintConfigInfo() {
  LOG(1, "Input File                                        : %s\n",
      V_TEXT_FILE_PATH);
  LOG(1, "Vocab File                                        : %s\n",
      V_VOCAB_FILE_PATH);
  LOG(1, "Thread Num                                        : %d\n",
      V_THREAD_NUM);
  LOG(1, "Iterations                                        : %d\n",
      V_ITER_NUM);
  LOG(1, "Offline interval / online update / vocabulary size: %lf\n",
      (double)V_OFFLINE_INTERVAL_VOCAB_RATIO);
  LOG(1, "Burn-in interval / online update / vocabulary size: %lf\n",
      (double)V_BURN_IN_INTERVAL_VOCAB_RATIO);
  LOG(1, "Initial Grad Descent Step Size                    : %lf\n",
      (double)V_INIT_GRAD_DESCENT_STEP_SIZE);
  LOG(1, "L2 Regularization Weight                          : %lf\n",
      (double)V_L2_REGULARIZATION_WEIGHT);
  LOG(1, "Vocabulary high-freq words cut-off                : %d\n",
      V_VOCAB_HIGH_FREQ_CUTOFF);
  LOG(1, "Cache intermediate models                         : %d\n",
      V_CACHE_INTERMEDIATE_MODEL);
  LOG(1, "Overwrite vocabulary file                         : %d\n",
      V_VOCAB_OVERWRITE);
  LOG(1, "Dimension N -- word embedding dim                 : %d\n", N);
  LOG(1, "Dimension K -- dual cluster number                : %d\n", K);
  LOG(1, "Dimension V -- vocabulary size cap                : %d\n", V);
  LOG(1, "Dimension C -- context length                     : %d\n", C);
  LOG(1, "Sanity Checks:\n");
  int x = 0;
  x = (NUP > N);
  LOG(1, "        NUP > N: %s (%d > %d)\n", x == 1 ? "yes" : "no", NUP, N);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  x = (KUP > K);
  LOG(1, "        KUP > K: %s (%d > %d)\n", x == 1 ? "yes" : "no", KUP, K);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  x = (VUP > V);
  LOG(1, "        VUP > V: %s (%d > %d)\n", x == 1 ? "yes" : "no", VUP, V);
  if (x == 0) {
    LOG(0, "fail!");
    exit(1);
  }
  return;
}

// always call this function before work
void VariableInit() {
  // expand file paths
  V_TEXT_FILE_PATH = FilePathExpand(V_TEXT_FILE_PATH);
  if (!V_VOCAB_FILE_PATH)
    V_VOCAB_FILE_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "vcb");
  else
    V_VOCAB_FILE_PATH = FilePathExpand(V_VOCAB_FILE_PATH);
  if (!V_MODEL_SAVE_PATH)
    V_MODEL_SAVE_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "mdl");
  else
    V_MODEL_SAVE_PATH = FilePathExpand(V_MODEL_SAVE_PATH);
  // define util_text constant variables
  TEXT_MAX_WORD_LEN = WUP;
  TEXT_MAX_SENT_WCT = SUP;
  PrintConfigInfo();
  return;
}

#endif /* ifndef CONSTANTS */
