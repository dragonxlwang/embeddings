#!/bin/sh

thread_num=10
dim=100
file="${HOME}/workspace/src/s3e/w2v/aan_title.txt"

# generating options
opts=""
opts+="--threads $thread_num "
opts+="--dim $dim "
opts+="--file $file "

# select python version
python26=/usr/bin/python
python27=/usr/local/bin/python
python=$python26

$python dep_w2v.py ${opts}
