#!/usr/bin/env python3
"""Saca los textos del menu que pasan por tr() (castellano, ingles): TR("..", ".."), las tablas [2][N] de menu.c,
los logros (achievements.c) y las prendas (skins.c). Lo usa tools/make_lang_ui.py para ver que falta por traducir."""
import re, os, glob
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
S = r'"(?:[^"\\]|\\.)*"'
CAT = r'((?:%s\s*)+)' % S
def cs(x):   # literal(es) C -> texto
    return ''.join(bytes(t[1:-1], 'utf-8').decode('unicode_escape').encode('latin1').decode('utf-8') for t in re.findall(S, x))
def extract():
    pairs = []
    for f in sorted(glob.glob(os.path.join(root, 'src/app/*.c'))):
        t = open(f, encoding='utf-8').read()
        for m in re.finditer(r'\b(?:TR|lt)\(\s*' + CAT + r'\s*,\s*' + CAT + r'\s*\)', t): pairs.append((cs(m.group(1)), cs(m.group(2))))
        # tablas const char *x[2][N] = {{...}, {...}};
        for m in re.finditer(r'\*(?:const\s+)?\w+\[2\]\[[^\]]*\]\s*=\s*\{(.*?)\};', t, re.S):
            body = m.group(1)
            # dos grupos {...}
            groups = re.findall(r'\{((?:\s*' + CAT + r'\s*,?)+)\s*\}', body)
            if len(groups) >= 2:
                a = [cs(x) for x in re.findall(r'(?:%s\s*)+' % S, groups[0][0])]
                b = [cs(x) for x in re.findall(r'(?:%s\s*)+' % S, groups[1][0])]
                pairs += list(zip(a, b))
    t = open(os.path.join(root, 'src/app/achievements.c'), encoding='utf-8').read()
    for m in re.finditer(r'\{\s*(%s)\s*,\s*(%s)\s*,\s*(%s)\s*,\s*(%s)' % (S, S, S, S), t):
        a = [cs(m.group(i)) for i in range(1, 5)]
        pairs.append((a[0], a[1])); pairs.append((a[2], a[3]))
    t = open(os.path.join(root, 'src/core/skins.c'), encoding='utf-8').read()
    for m in re.finditer(r'\{\s*(%s)\s*,\s*(%s)\s*,\s*(%s)\s*,' % (S, S, S), t):
        pairs.append((cs(m.group(2)), cs(m.group(3))))
    seen, out = set(), []
    for a, b in pairs:
        if a not in seen and a.strip(): seen.add(a); out.append((a, b))
    return out
if __name__ == '__main__':
    for a, b in extract(): print(repr(a), '|', repr(b))
