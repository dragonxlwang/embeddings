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
#include "../vectors/peek.c"
#include "../vectors/variables.c"

int main(int argc, char** argv) {
  V_MODEL_LOAD = 1;
  NumInit();
  VariableInit(argc, argv);
  real sample_rate = 1e-4;
  int top_k = 2e5;
  int overwrite = 0;
  char* file_path =
      FilePathSubExtension(V_TEXT_FILE_PATH, sformat("%.2e.lok", sample_rate));
  PeekSet* p;
  if (!fexists(file_path) || overwrite) {
    p = PeekBuild(V_TEXT_FILE_PATH, sample_rate, top_k, vcb, V_THREAD_NUM);
    PeekSave(file_path, vcb, p);
    PeekSetFree(p);
  }
  p = PeekLoad(file_path, vcb);
  real avgp = PeekEval(model, p, C, V_THREAD_NUM);
  LOGC(0, 'c', 'r', "\nPEEK:%.2e\n", avgp);
  VariableFree();
}
