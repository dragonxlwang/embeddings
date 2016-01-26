# time ./word2vec -train aan_title.txt -output vectors.bin \
#       -cbow 1 -size 100 -window 8 -negative 25 -hs 0 \
#       -sample 1e-4 -threads 20 -binary 1 -iter 1000 -alpha 0.1 \
#       -save-vocab "vocab.txt"

if [[ $1 == ns ]]; then
  prog="word2vec"
elif [[ $1 == me_2 ]]; then
  prog="word2vec_me_2"
elif [[ $1 == me_k ]]; then
  prog="word2vec_me_k"
fi
data="text8"

make $prog distance
  time ./$prog -train $data -output ${data}_${prog}.bin \
      -cbow 1 -size 100 -window 8 -negative 25 -hs 0 \
      -sample 1e-4 -threads 20 -binary 1 -iter 100 -alpha 0.01 \
      -save-vocab "${data}_vocab.txt"
  ./distance ${data}_${prog}.bin
