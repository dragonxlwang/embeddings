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
#define QUP 100      // upper bound for micro maximal entropy
#define WUP 100      // upper bound for character number in a word
#define SUP 200      // upper bound for word number in a sentence

char *V_TEXT_FILE_PATH = "~/data/gigaword/giga_nyt.txt";
/* char *V_TEXT_FILE_PATH = "~/data/text8/text8"; */
char *V_VOCAB_FILE_PATH = NULL;  // don't set it if can be inferred from above
char *V_MODEL_SAVE_PATH = NULL;
char *V_PEEK_FILE_PATH = NULL;
/* int V_THREAD_NUM = 1; */
int V_THREAD_NUM = 20;
int V_ITER_NUM = 10;
// every this number times vocabulary online updates perform one offline update
real V_OFFLINE_INTERVAL_VOCAB_RATIO = 1;
// Initial grad descent step size
real V_INIT_GRAD_DESCENT_STEP_SIZE = 1e-4;
/* real V_INIT_GRAD_DESCENT_STEP_SIZE = 0; */
// Model Shrink: l-2 regularization:
real V_L2_REGULARIZATION_WEIGHT = 0;  // 1e-3;
// Peek sampling rate
real V_PEEK_SAMPLE_RATE = 1e-5;
// Model Shrink: if proj model to unit ball
int V_MODEL_PROJ_UNIT_BALL = 0;
// Vocab loading option: cut-off high frequent (stop) words
int V_VOCAB_HIGH_FREQ_CUTOFF = 80;
// if cache model per iteration
int V_CACHE_INTERMEDIATE_MODEL = 0;
// if overwrite vocab file
int V_VOCAB_OVERWRITE = 0;
// if overwrite peek file
int V_PEEK_OVERWRITE = 0;
int N = 100;  // embedding dimension
int K = 20;   // number of dual cluster
/* int V = 1000;  // vocabulary size cap, set to -1 if no limit */
int V = 1000000;  // vocabulary size cap, set to -1 if no limit
int C = 5;        // context length
int Q = 10;       // Number of negative words updated online

void PrintConfigInfo() {
  LOG(1, "Input File                                        : %s\n",
      V_TEXT_FILE_PATH);
  LOG(1, "Vocab File                                        : %s\n",
      V_VOCAB_FILE_PATH);
  LOG(1, "Model Save                                        : %s\n",
      V_MODEL_SAVE_PATH);
  LOG(1, "Peek File                                         : %s\n",
      V_PEEK_FILE_PATH);
  LOG(1, "Thread Num                                        : %d\n",
      V_THREAD_NUM);
  LOG(1, "Iterations                                        : %d\n",
      V_ITER_NUM);
  LOG(1, "Offline interval / online update / vocabulary size: %lf\n",
      (double)V_OFFLINE_INTERVAL_VOCAB_RATIO);
  LOG(1, "Initial Grad Descent Step Size                    : %lf\n",
      (double)V_INIT_GRAD_DESCENT_STEP_SIZE);
  LOG(1, "L2 Regularization Weight                          : %lf\n",
      (double)V_L2_REGULARIZATION_WEIGHT);
  LOG(1, "Peek Sampling Rate                                : %lf\n",
      (double)V_PEEK_SAMPLE_RATE);
  LOG(1, "Model Inside Unit Ball                            : %d\n",
      V_MODEL_PROJ_UNIT_BALL);
  LOG(1, "Vocabulary high-freq words cut-off                : %d\n",
      V_VOCAB_HIGH_FREQ_CUTOFF);
  LOG(1, "Cache intermediate models                         : %d\n",
      V_CACHE_INTERMEDIATE_MODEL);
  LOG(1, "Overwrite vocabulary file                         : %d\n",
      V_VOCAB_OVERWRITE);
  LOG(1, "Overwrite peek file                               : %d\n",
      V_PEEK_OVERWRITE);
  LOG(1, "[N] -- word embedding dim                 : %d\n", N);
  LOG(1, "[K] -- dual cluster number                : %d\n", K);
  LOG(1, "[V] -- vocabulary size cap                : %d\n", V);
  LOG(1, "[C] -- context length                     : %d\n", C);
  LOG(1, "[Q] -- micro maximal entropy size         : %d\n", Q);
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
  x = (QUP > Q);
  LOG(1, "        QUP > Q: %s (%d > %d)\n", x == 1 ? "yes" : "no", QUP, Q);
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
  if (!V_PEEK_FILE_PATH)
    V_PEEK_FILE_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "pek");
  else
    V_PEEK_FILE_PATH = FilePathExpand(V_PEEK_FILE_PATH);
  // define util_text constant variables
  TEXT_MAX_WORD_LEN = WUP;
  TEXT_MAX_SENT_WCT = SUP;
  // use perm file instead of original
  /* V_TEXT_FILE_PATH = sformat("%s.perm", V_TEXT_FILE_PATH); */
  PrintConfigInfo();
  return;
}

#endif /* ifndef CONSTANTS */
