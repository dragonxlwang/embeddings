#ifndef VARIABLES
#define VARIABLES

#include "../utils/util_file.c"
#include <string.h>
// The following parameters are set in order to have variables on stack
#define NUP 200      // upper bound for N  (embedding dimension)
#define KUP 200      // upper bound for K  (dual dimension)
#define VUP 0xFFFFF  // upper bound for V (vocabulary)
#define SUP 100      // upper bound for word number in a sentence

char *V_TEXT_FILE_PATH = "~/data/gigaword/giga_nyt.txt";
char *V_TEXT_VOCAB_PATH = NULL;  // don't set it if can be inferred from above
int V_THREAD_NUM = 20;
int V_ITER_NUM = 10;

// always call this function before work
void VariableInit() {
  V_TEXT_FILE_PATH = FilePathExpand(V_TEXT_FILE_PATH);
  if (!V_TEXT_VOCAB_PATH)
    V_TEXT_VOCAB_PATH = FilePathSubExtension(V_TEXT_FILE_PATH, "vcb");
}

#endif /* ifndef CONSTANTS */
