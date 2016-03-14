import os
import gigaword

giga_path = os.path.join(os.path.expanduser('~'), 'data')
nyt_path = os.path.join(giga_path, 'gigaword_eng_5_d2/data/nyt_eng/')


def read_files():
    for f in os.listdir(nyt_path):
        fp = os.path.join(nyt_path, f)
        print('[giga_proc]: reading file {0}'.format(fp))
        t = gigaword.read_file(fp)
        for d in t:
            txt = ''
            for ln in d.text:
                txt += ln
