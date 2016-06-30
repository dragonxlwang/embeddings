make vectors/train NOEXEC=1 CFLAGS="-DDEBUG"
make eval/eval_peek NOEXEC=1
make eval/eval_word_distance NOEXEC=1
make eval/eval_question_accuracy NOEXEC=1
train="./bin/vectors/train"
peek="./bin/eval/eval_peek"
wd="./bin/eval/eval_word_distance"
qa="./bin/eval/eval_question_accuracy"
if [[ $1 == "train" ]]; then bin=$train
elif [[ $1 == "peek" ]]; then bin=$peek
elif [[ $1 == "wd" ]]; then bin=$wd
elif [[ $1 == "qa" ]]; then bin=$qa
else bin=$train
fi

dcme=" \
  V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_no-cutoff \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
  # V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  # V_MODEL_PROJ_BALL_NORM 1e2 \
  # V_MICRO_ME 1"

# 28.69 %   Semantic accuracy: 22.35 %   Syntactic accuracy: 30.68 %
# PEEK:8.22e-03
w2v_1=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 0 \
  V_NS_WRH 1 "

# 47.41 %   Semantic accuracy: 32.52 %   Syntactic accuracy: 52.08 %
# PEEK:1.72e-03
w2v_2=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 "

# 29.92 %   Semantic accuracy: 24.66 %   Syntactic accuracy: 31.57 %
# PEEK:8.36e-03
w2v_3=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 1 \
  V_NS_WRH 1 "

# 45.17 %   Semantic accuracy: 27.04 %   Syntactic accuracy: 50.86 %
# PEEK:7.51e-03
w2v_4=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 "

# 27.42 %   Semantic accuracy: 19.83 %   Syntactic accuracy: 29.80 %
# PEEK:8.10e-03
w2v_5=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_ns_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 52.27 %   Semantic accuracy: 33.60 %   Syntactic accuracy: 58.13 %
# PEEK:7.87e-04
w2v_6=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 28.55 %   Semantic accuracy: 22.28 %   Syntactic accuracy: 30.53 %
# PEEK:8.86e-03
w2v_7=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-2_nce_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-2 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# 40.43 %   Semantic accuracy: 24.80 %   Syntactic accuracy: 45.34%
# PEEK:8.04e-03
w2v_8=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_pow-1 \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1"

# no cutoff
# PEEK:1.74e-02
# 28.98 %   Semantic accuracy: 17.00 %   Syntactic accuracy: 32.84 %
w2v_9=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 28.71 %   Semantic accuracy: 15.89 %   Syntactic accuracy: 32.84 %
# PEEK:1.74e-02
w2v_10=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 26.60 %   Semantic accuracy: 15.54 %   Syntactic accuracy: 30.16 %
# PEEK:1.63e-02
w2v_11=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_ns_wrh_pow-1_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 0 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 26.91 %   Semantic accuracy: 16.24 %   Syntactic accuracy: 30.34 %
# PEEK:1.62e-02
w2v_12=" \
  V_MODEL_DECOR_FILE_PATH w2v_gd-1e-3_nce_wrh_pow-1_nc \
  V_TRAIN_METHOD w2v \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  V_NCE 1 \
  V_NS_WRH 1 \
  V_NS_POWER 1 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 13.53 %   Semantic accuracy: 9.88 %   Syntactic accuracy: 14.67 %
# PEEK:2.01e-02
dcme_1=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4 \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4"

# 9.66 %   Semantic accuracy: 11.38 %   Syntactic accuracy: 9.10 %
# PEEK:1.09e-02
dcme_2=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0"

# 9.66 %   Semantic accuracy: 11.38 %   Syntactic accuracy: 9.10 %
# PEEK:2.11e-02
dcme_3=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2"

# 13.34 %   Semantic accuracy: 10.96 %   Syntactic accuracy: 14.10 %
# PEEK:1.64e-02
dcme_4=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2"

# 15.77 %   Semantic accuracy: 11.18 %   Syntactic accuracy: 17.21 %
# PEEK:2.04e-02
dcme_5=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1"

# 5.59 %   Semantic accuracy: 6.87 %   Syntactic accuracy: 5.18 %
# PEEK:6.02e-03
dcme_6=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_mm \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1"

# 17.85 %   Semantic accuracy: 10.09 %   Syntactic accuracy: 20.29 %
# PEEK:1.95e-02
dcme_7=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_ub_mm_mmsu \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_MICRO_ME_SCR_UPDATE 1"

# 5.38 %   Semantic accuracy: 6.38 %   Syntactic accuracy: 5.06 %
# PEEK:6.72e-03
dcme_8=" \
  V_MODEL_DECOR_FILE_PATH dcme_gd-1e-4_nc_ub_mm_mmsu \
  V_TRAIN_METHOD dcme \
  V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  V_VOCAB_HIGH_FREQ_CUTOFF 0 \
  V_MODEL_PROJ_BALL_NORM 1e2 \
  V_MICRO_ME 1 \
  V_MICRO_ME_SCR_UPDATE 1"

eval "model=\$$2"
$bin $model


################################################################

# V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  # V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4

# # P: 8.09%  Q: 13.44%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4 \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4

# # P: 3.77%  Q: 13.89%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 8.13%  Q: 17.73%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1

# # P:6.18%  Q: 18.66%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me-scr \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1

################################################################


################################################################

# # nan
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3 \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3

# # P: 4.47%  Q: 6.89%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2

# # P: 8.36%  Q: 16.30%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2

# # P: 3.63%  Q: 6.31%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1

# # P: 9.23%  Q: 5.95%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me-scr \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1

# # P: 4.46% Q: 9.91%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: %6.53 Q: 11.33%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-3_uniball_micro-me-scr_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-3 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 5.06%  Q: 5.86%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0

# # P: 6.70% Q: 5.06%
# ./bin/vectors/train \
  #   V_MODEL_DECOR_FILE_PATH gd-1e-4_uniball_micro-me-scr_no-cutoff \
  #   V_INIT_GRAD_DESCENT_STEP_SIZE 1e-4 \
  #   V_MODEL_PROJ_BALL_NORM 1e2 \
  #   V_MICRO_ME 1 \
  #   V_MICRO_ME_SCR_UPDATE 1 \
  #   V_VOCAB_HIGH_FREQ_CUTOFF 0
