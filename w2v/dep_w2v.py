import nltk
import string
import os
from optparse import OptionParser
from nltk.parse import malt
import numpy as np

ANSI_COLOR_RED = "\x1b[31m"
ANSI_COLOR_GREEN = "\x1b[32m"
ANSI_COLOR_YELLOW = "\x1b[33m"
ANSI_COLOR_BLUE = "\x1b[34m"
ANSI_COLOR_MAGENTA = "\x1b[35m"
ANSI_COLOR_CYAN = "\x1b[36m"
ANSI_COLOR_RESET = "\x1b[0m"

# const or globals
mp = malt.MaltParser('/home/xwang95/resource/maltparser-1.7.2',
                     '/home/xwang95/resource/engmalt.linear-1.7.mco')
aan_title_file = "/home/xwang95/workspace/src/s3e/w2v/aan_title.txt"

def Parse(fp):
    print('=> Parse file using Malt Dependency Parser: {0}'.format(fp))
    with open(fp, 'r') as f:
        lines = f.readlines()
        lines = [filter(lambda x: x in string.printable, s) for s in lines]
        lines = [str.lower(ln).split() for ln in lines]
    with open(fp + ".conll", 'w') as f:
        for dgs in mp.parse_sents(lines, verbose=True):
            for dg in dgs:
                f.write(dg.to_conll(4))
                f.write('\n')

def LoadSent(f):
    dp = []
    for ln in f:
        if(ln.strip()):
            (w, e, p, n) = ln.split()
            p = int(p)
            dp.append((w, e, p, n))
        else:
            if(len(dp)):
                yield dp
            dp = []
    return

def SaveVocab(fp, wd2cnt, id2wd, wd2id):
    fp = fp.replace('.conll', '.vocab')
    print("=> Save vocabulary file to {0}".format(fp))
    with open(fp, 'w') as f:
        for w in id2wd:
            f.write(w + '\t' + str(wd2cnt[w]) + '\n')
    return

def LoadVocab(fp):
    wd2cnt = {}
    id2wd = []
    wd2id = {}
    fp = fp.replace('.conll', '.vocab')
    print("=> Read vocabulary file from {0}".format(fp))
    with open(fp, 'r') as f:
        for ln in f:
            (w, c) = ln.split()
            wd2cnt[w] = c
            wd2id[w] = len(id2wd)
            id2wd.append(w)
    return (wd2cnt, id2wd, wd2id)

def Train(fp):
    if(opts.build_vocab or not os.path.exists(fp.replace('.conll', '.vocab'))):
        wd2cnt = {}
        with open(fp, 'r') as f:
            for s in LoadSent(f):
                for (w, e, p, n) in s:
                    wd2cnt[w] = wd2cnt.get(w, 0) + 1
        id2wd = [w for w in sorted(wd2cnt, key=lambda x:-wd2cnt[x])]
        wd2id = dict(zip(id2wd, range(len(id2wd))))
        SaveVocab(fp, wd2cnt, id2wd, wd2id)
    else:
        (wd2cnt, id2wd, wd2id) = LoadVocab(fp)
    m = len(wd2cnt)
    n = opts.dim
    c = np.random.rand(m, n)
    w = np.random.rand(m, n)
    input(prompt=None)
    input(prompt=None)
    input(prompt=None)
    input(prompt=None)
    return

def ParseArgs():
    global opts
    parser = OptionParser(prog='DepW2V', description='Dependency W2V')
    parser.add_option('--threads', default=10, help='threads number')
    parser.add_option('--dim', default=100, help='dimension size')
    parser.add_option('--file', default=aan_title_file, help='dimension size')
    parser.add_option('--build-vocab', default=False, action="store_true",
                      help='force building vocabulary')
    for option in parser.option_list:
        if option.default != ("NO", "DEFAULT"):
            option.help += (" " if option.help else "") + "[default: %default]"
    (opts, args) = parser.parse_args()
    print('Options:')
    for key, val in vars(opts).items():
        print('\t{0:<30} : {1:<}'.format(key, str(val)))
    return

if __name__ == '__main__':
    ParseArgs()
    dp_file = opts.file + ".conll"
    if(not os.path.exists(dp_file)):
        Parse(opts.file)
    Train(dp_file)
