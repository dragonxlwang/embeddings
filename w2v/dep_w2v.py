"""Dependency Word2Vec."""
import nltk
import string
import os
from optparse import OptionParser
from nltk.parse import malt
import numpy as np
import sys
import math

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


def ProjToSphere(arr):
    arr /= np.linalg.norm(arr)
    return arr


class HKcluster(object):
    '''Hierarchical K-means Clustering'''
    bf = 10                     # branching factor
    nlvl = 0                    # total levels
    parent = []                 # parent cluster lookup
    children = []               # children component lookup
    point = []                  # cluster means
    npoint = []                 # point number
    W = None                    # word embeddings

    def Init(self, N, M, bf=None):      # top-down cluster for initialization
        if(bf):
            self.bf = bf
        self.nlvl = 0
        npoint = N
        self.children.append([])
        while(npoint > 1):
            ncluster = math.ceil(float(npoint) / self.bf)
            parent = np.random.random_integers(0, ncluster - 1, npoint)
            children = [set() for i in range(ncluster)]
            for i in range(parent):
                children[parent[i]].add(i)
            self.parent.append(parent)
            self.children.append(children)
            self.npoint.append(npoint)
            self.nlvl += 1
            npoint = ncluster
        self.point = [None for i in range(self.nlvl + 1)]
        self.point[self.nlvl] = np.zeros((1, N))
        q = math.pow(1e4, 1.0 / self.nlvl)
        std = 1.0
        for l in range(self.nlvl - 1, -1, -1):
            parent = self.parent[l]
            parent_point = self.point[l + 1]
            npoint = self.point[l]
            point = np.random.normal(0, std, (npoint, N))
            for i in range(npoint):
                point[i] += parent_point[parent[l]]
                point[i] = ProjToSphere(point[i])
            self.point[l] = point
            std /= q
        self.W = self.point[0]
        return

    def Update(i, w):                   # update i-th word to vector w
        self.W[i] = w
        for l in range(0, self.nlvl):
            point = self.point[l][i]
            parent_point = self.point[l + 1]
            nm1 = np.linalg.norm(point)
            nm2 = np.linalg.norm(parent_point[0])
            c = np.dot(point, parent_point[0], out=None)
        return


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
    return


def LoadSent(f, ftell=0):
    dp = []
    for ln in f:
        ftell += len(ln)
        if(ln.strip()):
            (w, l, p, r) = ln.split()
            p = int(p)
            dp.append((w, l, p, r))
        else:
            if(len(dp)):
                yield (p, ftell)
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


def GetFileStats(fp):
    mods = set()        # modifiers
    fptrs = [0]         # threads file pointers
    sent_num = 0
    print("=> Getting file statistics")
    with open(fp, 'r') as f:
        for (s, ftell) in LoadSent(f):
            sent_num += 1
            for (w, l, p, r) in s:
                mods.add(r)
    with open(fp, 'r') as f:
        i = 0
        for (s, ftell) in LoadSent(f):
            i += 1
            if(i == sent_num / opts.threads):
                i = 0
                fptrs.append(ftell)
        fptrs[opts.threads] = ftell
    return (mods, fptrs)


def ThreadedTrain(tid, fp, fptrs):
    with open(fp, 'r') as f:
        for (s, ftell) in LoadSent(f, fptrs):
            if(ftell > fptrs[tid + 1]):
                break
            for (w, l, p, r) in s:

                pass
    return


def Train(fp):
    if(opts.build_vocab or not os.path.exists(fp.replace('.conll', '.vocab'))):
        wd2cnt = {}
        with open(fp, 'r') as f:
            for (s, ftell) in LoadSent(f):
                for (w, l, p, r) in s:
                    wd2cnt[w] = wd2cnt.get(w, 0) + 1
        id2wd = [w for w in sorted(wd2cnt, key=lambda x:-wd2cnt[x])]
        wd2id = dict(zip(id2wd, range(len(id2wd))))
        SaveVocab(fp, wd2cnt, id2wd, wd2id)
    else:
        (wd2cnt, id2wd, wd2id) = LoadVocab(fp)
    (mods, fptrs) = GetFileStats(fp)
    N = len(wd2cnt)                         # vocabulary size
    M = opts.dim                            # embedding dim
    W = np.random.rand(N, M)                # word vectors
    A = {}                                  # modifier projection
    for mod in mods:
        A[mod] = np.random.rand(M, M)
    return


def ParseArgs():
    global opts
    parser = OptionParser(prog='DepW2V', description='Dependency W2V')
    parser.add_option('--threads', default=10, type="int",
                      help='threads number')
    parser.add_option('--dim', default=100, type="int", help='dimension size')
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
