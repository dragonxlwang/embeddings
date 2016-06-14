#ifndef EVAL_LOAD_MODEL
#define EVAL_LOAD_MODEL
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"
#include "../vectors/variables.c"

real *scr, *tar;
struct Vocabulary* vcb;

void ModelLoad() {
  vcb = TextLoadVocab(V_VOCAB_FILE_PATH, V, V_VOCAB_HIGH_FREQ_CUTOFF);
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

void W2vModelLoad() {
  int i, j, k;
  char* file_name = "~/workspace/w2v/giga_vectors.bin";
  file_name = FilePathExpand(file_name);
  char word[WUP];
  FILE* fin1 = fopen(sformat("%s.syn0", file_name), "rb");
  FILE* fin2 = fopen(sformat("%s.syn1neg", file_name), "rb");
  if (fin1 == NULL || fin2 == NULL) {
    printf("Input file not found\n");
    exit(1);
  }
  fscanf(fin1, "%d", &V);
  fscanf(fin1, "%d", &N);
  fscanf(fin2, "%d", &i);
  fscanf(fin2, "%d", &i);
  vcb = VocabCreate(V);
  scr = NumNewHugeVec(V * N);
  tar = NumNewHugeVec(V * N);
  float vec[NUP];
  int dup_cnt = 0;
  for (i = 0; i < V; i++) {
    j = 0;  // read words
    while (1) {
      word[j] = fgetc(fin1);
      fgetc(fin2);
      if (feof(fin1) || (word[j] == ' ')) break;
      if ((j < WUP) && (word[j] != '\n')) j++;
    }
    word[j] = 0;
    slower(word);
    if (VocabGetId(vcb, word) != -1) {  // ignore case, take lemma with larger f
      dup_cnt++;
      fread(vec, sizeof(float), N, fin1);
      fread(vec, sizeof(float), N, fin2);
      continue;
    } else
      j = VocabAdd(vcb, word, 1);
    fread(vec, sizeof(float), N, fin1);
    for (k = 0; k < N; k++) scr[j * N + k] = vec[k];
    fread(vec, sizeof(float), N, fin2);
    for (k = 0; k < N; k++) tar[j * N + k] = vec[k];
  }
  V = vcb->size;
  fclose(fin1);
  fclose(fin2);
  printf("dup_cnt = %d, V = %d\n", dup_cnt, V);
}

#endif /* ifndef EVAL_LOAD_MODEL */
