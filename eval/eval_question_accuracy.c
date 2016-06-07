#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_file.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "../vectors/variables.c"

char* EV_QUESTION_FILE_PATH = "~/data/w2v/questions-words.txt";

void EvalQuestionAccuracy(real* e, struct Vocabulary* vcb, int V) {
  EV_QUESTION_FILE_PATH = FilePathExpand(EV_QUESTION_FILE_PATH);
  char st1[WUP], st2[WUP], st3[WUP], st4[WUP];
  real vec[NUP];
  FILE* fin = fopen(EV_QUESTION_FILE_PATH, "rb");
  int TOPCNT = 1;  // top count: 10
  int TCN = 0;     // group count
  int CCN = 0;     // group correct
  int QID = 0;     // group id
  int CACN = 0;    // total correct
  int TACN = 0;    // total count
  int SEAC = 0;    // semantic correct
  int SECN = 0;    // semantic count
  int SYAC = 0;    // syntactic correct
  int SYCN = 0;    // syntactic count
  int TQ = 0;      // total question count
  int TQS = 0;     // questions  passed unk test
  int a, b1, b2, b3, b4, c;
  heap* h = HeapCreate(TOPCNT);
  real len;
  real max_len = 0, sum_len = 0;
  printf("\n");
  for (a = 0; a < V; a++) {
    len = NumVecNorm(e + a * N, N);
    if (len > max_len) max_len = len;
    sum_len += len;
    NumVecMulC(e + a * N, 1.0 / len, N);
  }
  printf("\nsum_len=%lf, max_len=%lf\n", sum_len, max_len);
  while (1) {
    HeapEmpty(h);
    fscanf(fin, "%s", st1);
    for (a = 0; a < strlen(st1); a++) st1[a] = LOWER(st1[a]);
    if ((!strcmp(st1, ":")) || (!strcmp(st1, "EXIT")) || feof(fin)) {
      if (TCN == 0) TCN = 1;
      if (QID != 0) {
        printf("ACCURACY TOP%d: %.2f %%  (%d / %d)\n", TOPCNT,
               CCN / (float)TCN * 100, CCN, TCN);
        printf(
            "Total accuracy: %.2f %%   "
            "Semantic accuracy: %.2f %%   "
            "Syntactic accuracy: %.2f %% \n",
            CACN / (float)TACN * 100, SEAC / (float)SECN * 100,
            SYAC / (float)SYCN * 100);
      }
      QID++;
      fscanf(fin, "%s", st1);
      if (feof(fin)) break;
      printf("%s:\n", st1);
      TCN = 0;
      CCN = 0;
      continue;
    }
    if (!strcmp(st1, "EXIT")) break;
    fscanf(fin, "%s", st2);
    for (a = 0; a < strlen(st2); a++) st2[a] = LOWER(st2[a]);
    fscanf(fin, "%s", st3);
    for (a = 0; a < strlen(st3); a++) st3[a] = LOWER(st3[a]);
    fscanf(fin, "%s", st4);
    for (a = 0; a < strlen(st4); a++) st4[a] = LOWER(st4[a]);
    b1 = VocabGetId(vcb, st1);
    b2 = VocabGetId(vcb, st2);
    b3 = VocabGetId(vcb, st3);
    b4 = VocabGetId(vcb, st4);
    TQ++;
    if (b1 == -1 || b2 == -1 || b3 == -1 || b4 == -1) continue;  // UNK
    if (b1 >= V || b2 >= V || b3 >= V || b4 >= V) continue;      // exceed V
    TQS++;
    NumAddCVecDVec(e + b2 * N, e + b1 * N, 1, -1, N, vec);
    NumVecAddCVec(vec, e + b3 * N, 1, N);
    for (c = 0; c < V; c++) {
      if (c == b1 || c == b2 || c == b3) continue;
      HeapPush(h, c, NumVecDot(vec, e + c * N, N));
    }
    for (c = 0; c < h->size; c++)
      if (h->d[c].key == b4) break;
    if (c != h->size) {
      CCN++;
      CACN++;
      if (QID <= 5)
        SEAC++;
      else
        SYAC++;
    }
    if (QID <= 5)
      SECN++;
    else
      SYCN++;
    TCN++;
    TACN++;
  }
  printf("Questions seen / total: %d %d   %.2f %% \n", TQS, TQ,
         TQS / (float)TQ * 100);
  fclose(fin);
  HeapDestroy(h);
  return;
}

real *tar, *scr;
void ModelLoad() {
  /* char* EV_MODEL_FILE_PATH = FilePathExpand("~/data/text8/text8.mdl"); */
  char* EV_MODEL_FILE_PATH = V_MODEL_SAVE_PATH;
  FILE* fin = fopen(EV_MODEL_FILE_PATH, "rb");
  if (!fin) {
    LOG(0, "Error!\n");
    exit(1);
  }
  scr = NumNewHugeVec(N * V);
  if (fread(scr, sizeof(real), N * V, fin) != N * V) {
    LOG(0, "Error!\n");
    exit(1);
  }
  tar = NumNewHugeVec(N * V);
  if (fread(tar, sizeof(real), N * V, fin) != N * V) {
    LOG(0, "Error!\n");
    exit(1);
  }
  return;
}

int main() {
  VariableInit();
  NumInit();
  struct Vocabulary* vcb =
      TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
  ModelLoad();
  EvalQuestionAccuracy(scr, vcb, SMALLER(10000, V));
  return 0;
}
