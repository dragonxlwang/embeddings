#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../classify/test.c"
#include "../classify/variables.c"
#include "../utils/util_misc.c"
#include "../utils/util_num.c"
#include "../utils/util_text.c"

int completed_iters = 0;

void EvalClassify(char* weight_file_path, char* test_file_path,
                  int* correct_ptr, int* total_ptr, real* accuracy_ptr,
                  real* probability_ptr) {
  int fsv[LUP], fn, label, correct = 0, total = 0, x;
  real p, totalp = 0;
  long int fsz = FileSize(test_file_path);
  FILE* fin = fsopen(test_file_path, "rb");
  int weightc, weightn;
  real* weight = WeightLoad(weight_file_path, &weightc, &weightn);
  int* detail_correct = NumNewHugeIntVec(C);
  int* detail_total = NumNewHugeIntVec(C);
  NumFillZeroIntVec(detail_correct, C);
  NumFillZeroIntVec(detail_total, C);
  while (HelperReadInstance(fin, vcb, classes, fsv, &fn, &label, V_TEXT_LOWER,
                            V_TEXT_RM_TRAIL_PUNC)) {
    if (label < 0) continue;  // filtering classes not shown in training data
    x = TestClassify(fsv, fn, weight, C, N, label, &p);
    detail_total[label]++;
    detail_correct[label] += x;
    correct += x;
    totalp += p;
    total++;
    if (total % 10 == 0) {
      LOGCLR(2);
      LOG(2, "Completing %.2lf%%", (double)ftell(fin) / fsz * 100);
    }
  }
  LOGCR(2);
  *correct_ptr = correct;
  *total_ptr = total;
  *accuracy_ptr = (double)correct / total;
  *probability_ptr = totalp / total;
#ifdef DEBUG
  int i, j;
  pair* detail_tuple = sortedi(detail_total, C, 1);
  for (j = 0; j < C; j++) {
    i = detail_tuple[j].key;
    if (detail_total[i] >= 50)
      printf("%20s: %15d / %-15d = %-15lf\n", DictGetKey(classes, i),
             detail_correct[i], detail_total[i],
             (double)detail_correct[i] / detail_total[i]);
  }
  free(detail_tuple);
#endif
  free(weight);
  free(detail_correct);
  free(detail_total);
  fclose(fin);
  return;
}

void* sid_classify_thread(void* param) {
  void** p = (void**)param;
  int i = 0, j;
  int* assigned_iters = p[i++];
  int assigned_num = *((int*)p[i++]);
  int fitting = *((int*)p[i++]);
  int* correct_ptr = p[i++];
  int* total_ptr = p[i++];
  real* accuracy_ptr = p[i++];
  real* probability_ptr = p[i++];
  for (i = 0; i < assigned_num; i++) {
    j = assigned_iters[i];
    char* wfp = sformat("%s.part%d", V_WEIGHT_SAVE_PATH, j);
    EvalClassify(wfp, fitting ? V_TRAIN_FILE_PATH : V_TEST_FILE_PATH,
                 correct_ptr + j, total_ptr + j, accuracy_ptr + j,
                 probability_ptr + j);
    completed_iters++;
    LOGCLR(0);
    LOG(0,
        "complet %d / %d, %d instances, iter %d, accuracy = %lf, probability = "
        "%lf",
        completed_iters, V_ITER_NUM, total_ptr[j], j, accuracy_ptr[j],
        probability_ptr[j]);
    free(wfp);
  }
  return NULL;
}

void EvalMultiThreadClassify(int fitting) {
  int i, j;
  int** assigned_iters = (int**)malloc(V_THREAD_NUM * sizeof(int*));
  int* assigned_num = (int*)malloc(V_THREAD_NUM * sizeof(int));
  for (i = 0; i < V_THREAD_NUM; i++) {
    assigned_iters[i] =
        (int*)malloc((V_ITER_NUM / V_THREAD_NUM + 2) * sizeof(int));
    assigned_num[i] = 0;
  }
  for (i = 0; i < V_ITER_NUM; i++) {
    j = i % V_THREAD_NUM;
    assigned_iters[j][assigned_num[j]++] = i;
  }
  int stride = 7;
  void** parameters = (void**)malloc(V_THREAD_NUM * stride * sizeof(void*));
  int* correct_ptr = (int*)malloc(V_ITER_NUM * sizeof(int));
  int* total_ptr = (int*)malloc(V_ITER_NUM * sizeof(int));
  real* accuracy_ptr = (real*)malloc(V_ITER_NUM * sizeof(real));
  real* probability_ptr = (real*)malloc(V_ITER_NUM * sizeof(real));
  for (i = 0; i < V_THREAD_NUM; i++) {
    j = 0;
    parameters[i * stride + j++] = assigned_iters[i];
    parameters[i * stride + j++] = assigned_num + i;
    parameters[i * stride + j++] = &fitting;
    parameters[i * stride + j++] = correct_ptr;
    parameters[i * stride + j++] = total_ptr;
    parameters[i * stride + j++] = accuracy_ptr;
    parameters[i * stride + j++] = probability_ptr;
  }
  printf("%s\n", fitting ? "Training" : "Testing");
  pthread_t* pt = (pthread_t*)malloc(V_THREAD_NUM * sizeof(pthread_t));  // >>
  for (i = 0; i < V_THREAD_NUM; i++)
    pthread_create(pt + i, NULL, sid_classify_thread,
                   (void*)(parameters + i * stride));
  for (i = 0; i < V_THREAD_NUM; i++) pthread_join(pt[i], NULL);
  printf("\n");
  for (i = 0; i < V_ITER_NUM; i++) {
    printf("iter %3d: accuracy = %6d / %6d = %.6lf%% probability = %.6lf%%\n",
           i, correct_ptr[i], total_ptr[i], (real)accuracy_ptr[i],
           (real)probability_ptr[i]);
  }
  free(pt);
  for (i = 0; i < V_THREAD_NUM; i++) free(assigned_iters[i]);
  free(assigned_iters);
  free(assigned_num);
  free(parameters);
  free(correct_ptr);
  free(total_ptr);
  free(accuracy_ptr);
  free(probability_ptr);
  return;
}

int main(int argc, char** argv) {
  NumInit();
  VariableInit(argc, argv);
  log_debug_mode = 0;
  EvalMultiThreadClassify(0);
  completed_iters = 0;
  EvalMultiThreadClassify(1);
  return 0;
}
