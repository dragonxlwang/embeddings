make distance

if [[ $1 == ns ]]; then
  prog="word2vec"
elif [[ $1 == me_2 ]]; then
  prog="word2vec_me_2"
elif [[ $1 == me_k ]]; then
  prog="word2vec_me_k"
fi

if [[ $2 -eq 0 ]]; then
  embeded="syn0"
else
  embeded="syn1_neg"
fi

data="text8"
output=${data}_${prog}_${embeded}.bin
echo $output

./distance ${output}
