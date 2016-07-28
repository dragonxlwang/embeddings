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

dcme1="V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-4_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  N 30000 \
  K 20 \
  Q 10"

dcme2="V_WEIGHT_DECOR_FILE_PATH dcme_gd-1e-2_N-30K_K-20_Q-10 \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  N 30000 \
  K 20 \
  Q 10"

eval "model=\$$2"
$bin $model
