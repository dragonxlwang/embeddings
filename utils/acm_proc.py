import os
import re
import math
from nltk.tokenize import sent_tokenize, word_tokenize
from multiprocessing import Pool

# this code prepares training/test data for dcme
# collect abstract as features, and predict corresponding journal/year
proc_path = os.path.join(os.path.expanduser('~'),
                         'data/acm_corpus/acmdl/proceeding')
perd_path = os.path.join(os.path.expanduser('~'),
                         'data/acm_corpus/acmdl/periodical')



if __name__ == '__main__':
    pass
