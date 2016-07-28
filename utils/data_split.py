import os
import re
import sys
import math
import random


def split_data(file_path, outfile_path, n, r):
    fout_train = open(outfile_path + ".train", "w")
    fout_test = open(outfile_path + ".test", "w")
    with open(file_path, "r") as fin:
        while(1):
            block = []
            for i in range(n):
                block.append(fin.readline())
            if(block[0] == ""):
                break
            if(random.random() < r):
                fout_train.writelines(block)
            else:
                fout_test.writelines(block)
    fout_train.close()
    fout_test.close()
    return

if __name__ == '__main__':
    split_data = split_data('/home/xwang95/data/acm_corpus/proc.txt',
                            '/home/xwang95/data/acm_corpus/proc', 4, 0.9)
