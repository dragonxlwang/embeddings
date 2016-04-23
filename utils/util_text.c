#ifndef UTIL_TEXT
#define UTIL_TEXT

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util_misc.c"

// Parameters
//  HASH_SLOTS        : at most this number of elements without crash
//  TEXT_INIT_VCB_CAP : vocabulary initial size cap
//  TEXT_MAX_WORD_LEN : maximal word length
//  TEXT_MAX_SENT_WCT : maximal number of words in one sentence
// Values
//  TEXT_CORPUS_WORD_CNT  : corpus word count
//  TEXT_CORPUS_FILE_SIZE : corpus file size

////////////////
// Vocabulary //
////////////////

int HASH_SLOTS = 0xFFFFF;  // around 1M slots by default

// BKDR Hash for string
int GetStrHash(char* str) {
  unsigned long long h = 0;
  char ch;
  while ((ch = *(str++))) h = (((h << 7) + (h << 1) + (h) + ch) & HASH_SLOTS);
  return h;
}

// vocabulary, a hash map
struct Vocabulary {
  char** id2wd;
  int* id2cnt;
  int* id2next;
  int* hash2head;
  int size;
  // initial settings
  int VCB_CAP;
};

void VocabSetHashSlots(int hs) { HASH_SLOTS = hs; }

void VocabInitStorage(struct Vocabulary* vcb, int cap) {
  vcb->id2wd = (char**)malloc(cap * sizeof(char*));
  vcb->id2cnt = (int*)malloc(cap * sizeof(int));
  vcb->id2next = (int*)malloc(cap * sizeof(int));
  vcb->hash2head = (int*)malloc(HASH_SLOTS * sizeof(int));
  if (!vcb->id2wd || !vcb->id2cnt || !vcb->id2next || !vcb->hash2head) {
    LOG(0, "[VocabInitStorage]: allocating error\n");
    exit(1);
  }
  vcb->size = 0;
  vcb->VCB_CAP = cap;
  memset(vcb->hash2head, 0xFF, HASH_SLOTS * sizeof(int));
}

void VocabClearStorage(struct Vocabulary* vcb) {
  int i;
  for (i = 0; i < vcb->size; i++) free(vcb->id2wd[i]);
  free(vcb->id2wd);
  free(vcb->id2cnt);
  free(vcb->id2next);
  free(vcb->hash2head);
  return;
}

struct Vocabulary* VocabCreate(int cap) {
  struct Vocabulary* vcb =
      (struct Vocabulary*)malloc(sizeof(struct Vocabulary));
  VocabInitStorage(vcb, cap);
  return vcb;
}

void VocabDestroy(struct Vocabulary* vcb) {
  VocabClearStorage(vcb);
  free(vcb);
  return;
}

void VocabAdd(struct Vocabulary* vcb, char* str, int cnt) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  if (id == -1) {
    id = vcb->size++;
    STR_CLONE(vcb->id2wd[id], str);
    vcb->id2cnt[id] = 0;
    vcb->id2next[id] = vcb->hash2head[h];
    vcb->hash2head[h] = id;
    if (vcb->size + 2 > vcb->VCB_CAP) {
      vcb->VCB_CAP <<= 1;
      vcb->id2wd = (char**)realloc(vcb->id2wd, vcb->VCB_CAP * sizeof(char*));
      vcb->id2cnt = (int*)realloc(vcb->id2cnt, vcb->VCB_CAP * sizeof(int));
      vcb->id2next = (int*)realloc(vcb->id2next, vcb->VCB_CAP * sizeof(int));
    }
  }
  vcb->id2cnt[id] += cnt;
  return;
}

int* value_for_compare;
int CmpVal(const void* x, const void* y) {
  return value_for_compare[*((int*)y)] - value_for_compare[*((int*)x)];
}

void VocabReduce(struct Vocabulary* vcb, int cap) {
  int i;
  int* keys = (int*)malloc(vcb->size * sizeof(int));
  for (i = 0; i < vcb->size; i++) keys[i] = i;
  value_for_compare = vcb->id2cnt;
  qsort(keys, vcb->size, sizeof(int), CmpVal);
  int size = vcb->size;
  if (cap > 0 && size > cap) size = cap;
  char** words = (char**)malloc(size * sizeof(char*));
  int* counts = (int*)malloc(size * sizeof(int));
  for (i = 0; i < size; i++) {
    STR_CLONE(words[i], vcb->id2wd[keys[i]]);
    counts[i] = vcb->id2cnt[keys[i]];
  }
  VocabClearStorage(vcb);
  VocabInitStorage(vcb, (int)(size * 1.1));
  for (i = 0; i < size; i++) VocabAdd(vcb, words[i], counts[i]);
  for (i = 0; i < size; i++) free(words[i]);
  free(keys);
  free(words);
  free(counts);
  return;
}

int VocabGetId(struct Vocabulary* vcb, char* str) {
  int h = GetStrHash(str);
  int id = vcb->hash2head[h];
  while (id != -1 && strcmp(vcb->id2wd[id], str) != 0) id = vcb->id2next[id];
  return id;  // UNK = -1 or in vocab
}

char* VocabGetWord(struct Vocabulary* vcb, int id) { return vcb->id2wd[id]; }

//////////
// Text //
//////////

int TEXT_INIT_VCB_CAP = 0x200000;         // around 2M slots by default
int TEXT_MAX_WORD_LEN = 100;              // one word has max 100 char
int TEXT_MAX_SENT_WCT = 200;              // one sentence has max 200 word
long long int TEXT_CORPUS_WORD_CNT = 0;   // corpus wourd count
long long int TEXT_CORPUS_FILE_SIZE = 0;  // corpus file size

int TextReadWord(FILE* fin, char* str) {
  // return flag: 0=hit by space or tab; 1=newline; 2=end-of-file
  int i = 0, flag = -1;
  char ch;
  while (1) {
    ch = fgetc(fin);
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == EOF) break;
    if (i < TEXT_MAX_WORD_LEN - 1) str[i++] = ch;
  }
  str[i] = '\0';
  while (1) {
    if (ch == ' ' || ch == '\t')
      flag = 0;
    else if (ch == '\n' || ch == '\r') {
      flag = 1;
      if (fin == stdin) break;
    } else if (ch == EOF) {
      flag = 2;
      break;
    } else {
      ungetc(ch, fin);
      break;
    }
    ch = fgetc(fin);
  }
  return flag;
}

int TextNormWord(char* str, int if_lower, int if_rm_trail_punc) {
  // return: when if_rm_trail_punc set 1, return 1 if str ends with .,:;?!
  int i = 0, j, flag = 0;
  char ch;
  if (if_lower)
    while (str[i] != '\0') {
      if (str[i] >= 'A' && str[i] <= 'Z') str[i] = str[i] - 'A' + 'a';
      i++;
    }
  i--;
  if (if_rm_trail_punc) {
    while (i >= 0) {
      ch = str[i];
      if (ch == '.' || ch == ',' || ch == ':' || ch == ';' || ch == '?' ||
          ch == '!') {
        str[i] = '\0';  // removing clause-terminating punctuation
        flag = 1;
      } else if (ch == '`' || ch == '"' || ch == '\'' || ch == ')' ||
                 ch == ']') {
        str[i] = '\0';  // removing proceeding enclosing punctuation
      } else
        break;
      i--;
    }
    i = 0;
    while (1) {  // removing preceding enclosing punctuation
      ch = str[i];
      if (ch == '`' || ch == '"' || ch == '\'' || ch == '(' || ch == '[')
        i++;
      else
        break;
    }
    if (i != 0)
      for (j = i; str[j - 1] != '\0'; j++) str[j - i] = str[j];
  }
  return flag;
}

struct Vocabulary* TextBuildVocab(char* text_file_path, int if_norm_word,
                                  int cap) {
  char str[TEXT_MAX_WORD_LEN];
  int flag = 0;
  struct Vocabulary* vcb = VocabCreate(TEXT_INIT_VCB_CAP);
  FILE* fin = fopen(text_file_path, "rb");
  TEXT_CORPUS_WORD_CNT = 0;
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", text_file_path);
    exit(1);
  }
  while (1) {
    flag = TextReadWord(fin, str);
    if (if_norm_word) TextNormWord(str, 1, 1);
    if (str[0] != '\0') {  // filter 0-length string
      VocabAdd(vcb, str, 1);
      TEXT_CORPUS_WORD_CNT++;
      if ((TEXT_CORPUS_WORD_CNT & 0xFFFFF) == 0xFFFFF)
        LOG(2, "[TextBuildVocab]: reading %lld [*2^20 | M] word \33[2K\r",
            TEXT_CORPUS_WORD_CNT >> 20);
    }
    if (flag == 2) break;
  }
  LOG(1, "[TextBuildVocab]: reading %lld [*2^20 | M] word\n",
      TEXT_CORPUS_WORD_CNT >> 20);
  TEXT_CORPUS_FILE_SIZE = ftell(fin);
  LOG(1, "[TextBuildVocab]: file size %lld [*2^20 | M]\n",
      TEXT_CORPUS_FILE_SIZE >> 20);
  fclose(fin);
  VocabReduce(vcb, cap);
  return vcb;
}

void TextSaveVocab(char* vcb_fp, struct Vocabulary* vcb) {
  int i;
  FILE* fout = fopen(vcb_fp, "wb");
  fprintf(fout, "%lld %lld\n", TEXT_CORPUS_FILE_SIZE, TEXT_CORPUS_WORD_CNT);
  for (i = 0; i < vcb->size; i++)
    fprintf(fout, "%s %d\n", vcb->id2wd[i], vcb->id2cnt[i]);
  fclose(fout);
  LOG(1, "[saving vocabulary]: %s\n", vcb_fp);
  return;
}

struct Vocabulary* TextLoadVocab(char* vcb_fp, int cap, int high_freq_cutoff) {
  // cap: -1=no limit; high_freq_cutoff: first h_f_c number of words removed;
  int cnt, flag, id = 0;
  FILE* fin = fopen(vcb_fp, "rb");
  char s_word[TEXT_MAX_WORD_LEN], s_cnt[TEXT_MAX_WORD_LEN];
  if (!fin) {
    LOG(0, "[error]: cannot find file %s\n", vcb_fp);
    exit(1);
  }
  fscanf(fin, "%lld %lld\n", &TEXT_CORPUS_FILE_SIZE, &TEXT_CORPUS_WORD_CNT);
  struct Vocabulary* vcb =
      VocabCreate((cap > 0) ? (int)(cap * 1.1) : TEXT_INIT_VCB_CAP);
  while (1) {
    TextReadWord(fin, s_word);
    flag = TextReadWord(fin, s_cnt);
    cnt = atoi(s_cnt);
    if (id++ >= high_freq_cutoff) VocabAdd(vcb, s_word, cnt);
    if (flag == 2) break;
    if (cap > 0 && vcb->size >= cap) break;
  }
  fclose(fin);
  LOG(1,
      "[loading vocabulary]: %s\n"
      "[loading vocabulary]: size %d\n",
      vcb_fp, vcb->size);
  return vcb;
}

int TextReadSent(FILE* fin, struct Vocabulary* vcb, int* word_ids,
                 int if_norm_word, int till_line_end) {
  char str[TEXT_MAX_WORD_LEN];
  int flag1, flag2 = 0, id;
  int word_num = 0;
  while (1) {
    flag1 = TextReadWord(fin, str);
    if (if_norm_word) flag2 = TextNormWord(str, 1, 1);
    if (str[0] == '\0')
      id = -1;  // empty string
    else
      id = VocabGetId(vcb, str);  // non-empty string
    if (id != -1 && word_num < TEXT_MAX_SENT_WCT) word_ids[word_num++] = id;
    if ((till_line_end && flag1 > 0) || (flag1 > 0 || flag2 == 1)) break;
  }
  if (flag1 == 2) word_num = -1;
  return word_num;
}

void TextPrintSent(struct Vocabulary* vcb, int* word_ids, int word_num) {
  int i;
  for (i = 0; i < word_num; i++)
    printf("%s(%d) ", VocabGetWord(vcb, word_ids[i]), word_ids[i]);
  printf("\n");
  return;
}

/*
   long long int neg_unigram_size = 1e8;
   void InitNegUnigram()
   {
   long long int i, j, k, total_word_cnt = 0, cnt = 0;
   real cdf, power = 0.75;
   for (i = 0; i < vocab.size; i++)
   total_word_cnt += pow(vocab.id2cnt[i], power);
   neg_unigram = (int*)malloc(1e8 * sizeof(int));
   for (i = 0, j = 0; i < vocab.size; i++) {
   cnt += pow(vocab.id2cnt[i], power);
   cdf = (real)cnt / total_word_cnt;
   cdf = (cdf > 1.0) ? 1.0 : cdf;
   k = neg_unigram_size * cdf;
   while (j < k)
   neg_unigram[j++] = i;
   }
   return;
   }
   int SampleNegUnigram(unsigned long long int s)
   {
   long long int r = RAND(s) * neg_unigram_size;
   return neg_unigram[r];
   }
   */

#endif /* end of include guard: UTIL_TEXT */
