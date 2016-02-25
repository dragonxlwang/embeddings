"""word count for phrase detection and embedding"""
import nltk
import string
import os
from optparse import OptionParser
from nltk.parse import malt
import numpy as np
import sys
import math


def IsKeyHitInLn(klst, wlst, i):
    for j in range(len(klst)):
        if(wlst[i + j] == klst[j]):
            continue
        else:
            return False
    return True


def GenKeyDist(key, file_name, neighbors):
    key = key.strip()
    klst = [x.lower() for x in key.split()]
    ptable = {}
    wtables = {}
    pcnt = 0
    wcnts = {}
    table = {}
    s = 0
    til = 0
    for w in klst:
        wtables[w] = {}
        wcnts[w] = 0
    print('key: {0}'.format(klst))
    with open(file_name, 'r') as f:
        for ln in f:
            wlst = [x.lower() for x in ln.split()]
            for i in range(len(wlst)):
                s += 1
                if(s % 1e6 == 0):
                    sys.stdout.write('\r{0}m words processed'.format(s / 1e6))
                    sys.stdout.flush()
                table[wlst[i]] = table.get(wlst[i], 0) + 1.0
                if(i < til):
                    continue
                if(IsKeyHitInLn(klst, wlst, i)):
                    b = max(0, i - neighbors)
                    e = min(len(wlst), i + len(klst) + neighbors)
                    for j in range(b, e):
                        if(j < i or j >= i + len(klst)):
                            ptable[wlst[j]] = ptable.get(wlst[j], 0) + 1.0
                            pcnt += 1.0
                    til = i + len(klst)
                if(wlst[i] in klst):
                    b = max(0, i - neighbors)
                    e = min(len(wlst), i + 1 + neighbors)
                    for j in range(b, e):
                        if(j != i):
                            wtables[wlst[i]][wlst[j]] = \
                                wtables[wlst[i]].get(wlst[j], 0) + 1.0
                            wcnts[wlst[i]] += 1.0
    print('\nall {0} words processed'.format(s))
    print("Distribution for '{0}'".format(key))
    top = 40
    ptops = []
    wtops = [[] for i in range(len(klst))]
    btops = set()
    n = 100
    for w in sorted(table, key=lambda x: table[x], reverse=True):
        btops.add(w)
        n -= 1
        if(n == 0):
            break
    n = top
    for w in sorted(ptable, key=lambda x: ptable[x], reverse=True):
        if(w in btops):
            continue
        ptops.append('{0:>20}:{1:<10.6f}'.format(w, ptable[w] / pcnt))
        n -= 1
        if(n == 0):
            break
    for k in range(len(klst)):
        n = top
        kw = klst[k]
        for w in sorted(wtables[kw], key=lambda x: wtables[kw][x],
                        reverse=True):
            if(w in btops):
                continue
            wtops[k].append('{0:>20}:{1:<10.6f}'.format(
                w, wtables[kw][w] / wcnts[kw]))
            n -= 1
            if(n == 0):
                break
    ln = '{0:^30}'.format(key)
    ln += (5 * ' ') + \
        (5 * ' ').join(['{0:^30}'.format(klst[k]) for k in range(len(klst))])
    print(ln)
    for i in range(len(ptops)):
        ln = ptops[i]
        ln += (5 * ' ') + (5 * ' ').join([wtops[k][i]
                                          for k in range(len(klst))])
        ln += (5 * ' ') + '[{0}]'.format(i)
        print(ln)

if(__name__ == '__main__'):
    while(True):
        sys.stdout.write('input key: ')
        sys.stdout.flush()
        key = sys.stdin.readline()
        GenKeyDist(key, 'text8', 5)
