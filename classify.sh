#!/usr/bin/env bash

function check_cmd_arg  {
  argarr=($(echo $1))
  xarg=$2
  for x in "${argarr[@]}"; do
    if [[ $x == $xarg ]]; then
      echo 1
      return 0
    fi
  done
  echo 0
  return 1
}

train_debug_opt=$(check_cmd_arg "$*" DEBUG)
train_peek_opt=$(check_cmd_arg "$*" PEEK)
CFLAGS=""
[[ $train_debug_opt -eq 1  ]] && { CFLAGS="-DDEBUG "; }
[[ $train_peek_opt -eq 1  ]] && { CFLAGS+="-DDEBUGPEEK "; }

nomake=$(check_cmd_arg "$*" nomake)

if [[ $nomake -eq 0  || $1 == "make" ]]; then
  make classify/train NOEXEC=1 CFLAGS="$CFLAGS"
  make eval/eval_classify NOEXEC=1
  # make eval/eval_word_distance NOEXEC=1
  # make eval/eval_question_accuracy NOEXEC=1 # CFLAGS="-DACCURACY"
  if [[ $1 == "make" ]]; then exit 0; fi
fi

train="./bin/classify/train"
classify="./bin/eval/eval_classify"

if [[ $1 == "train" ]]; then bin=$train
elif [[ $1 == "classify" ]]; then bin=$classify
else bin=$train
fi

# accuracy    = 0.211029
# probability = 0.165033
dcme_1=" \
  V_TRAIN_FILE_PATH ~/data/acm_corpus/proc-perm.train \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10"

# accuracy    = 0.176587
# probability = 0.176389
dcme_2=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10"

# accuracy    = 0.170918
# probability = 0.155897
dcme_3=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-10_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 10 \
  Q 10"

# accuracy    = 0.191559
# probability = 0.191464
dcme_4=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-10_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 10 \
  Q 10"

# accuracy    = 0.203266
# probability = 0.197433
dcme_5=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-10_Q-0 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 10 \
  Q 0"

# accuracy    = 0.191559
# probability = 0.191488
dcme_6=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-10_Q-0 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 10 \
  Q 0"

##################################

dcme_7=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 5 \
  Q 0"

dcme_8=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 5 \
  Q 0"

dcme_9=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 5 \
  Q 0"

dcme_10=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-5_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 5 \
  Q 0"

dcme_11=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 0"

dcme_12=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 0"

dcme_13=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 0"

dcme_14=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-0 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 0"

dcme_15=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 10"

dcme_16=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10"

dcme_17=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10"

dcme_18=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10"

dcme_19=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-1_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

dcme_20=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

dcme_21=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-3_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

dcme_22=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10_mme \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10 \
  V_MICRO_ME 1"

dcme_test=" \
  V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-0_tn-1 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 0 \
  V_THREAD_NUM 20"

# accuracy    = 0.244239
# probability = 0.196357
w2v_1=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-10_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 10 \
  V_NCE 0 \
  V_NS_WRH 1 "

# accuracy    = 0.221072
# probability = 0.172354
w2v_2=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-20_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 20 \
  V_NCE 0 \
  V_NS_WRH 1 "

##################################

w2v_3=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

w2v_4=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

w2v_5=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

w2v_6=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-5_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 0 \
  V_NS_WRH 1 "

w2v_7=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-1_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-1 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

w2v_8=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-2_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

w2v_9=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-3_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

w2v_10=" \
  V_WEIGHT_DECOR_FILE_PATH w2v_gd-1e-4_N-30K_NEG-5_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  V_NS_NEG 5 \
  V_NCE 1 \
  V_NS_WRH 1 "

eval "model=\$$2"
$bin $model
