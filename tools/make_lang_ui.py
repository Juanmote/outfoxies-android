#!/usr/bin/env python3
"""Traducciones del menu al portugues y al italiano (tools/lang_ui_pt_it.py) -> src/app/lang_ui.h.
Comprueba que estan todos los textos que pasan por tr() (tools/lang_extract.py) y que los %d/%s coinciden."""
import os, re, sys
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lang_ui_pt_it import T
from lang_extract import extract
EXTRA = ['Arriba', 'Abajo', 'Izquierda', 'Derecha']       # nombres de botones del mando (padbtn_name)
fmt = lambda s: re.findall(r'%[-0-9.]*[a-z%]', s)
need = [a for a, _ in extract()] + EXTRA
missing = [a for a in need if a not in T]
for a in missing: print('FALTA:', repr(a))
bad = 0
for k, (pt, it) in T.items():
    for t in (pt, it):
        if fmt(t) != fmt(k): print('FORMATO:', repr(k), repr(t)); bad = 1
if missing or bad: sys.exit(1)
def c(s): return '"' + s.replace('’', "'").replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'
keys = sorted(T, key=lambda k: k.encode('utf-8'))   # orden de strcmp
o = ['/* Generado por tools/make_lang_ui.py desde tools/lang_ui_pt_it.py: no editar */', '#ifndef OFX_LANG_UI_H', '#define OFX_LANG_UI_H',
     '/* castellano (ordenado como strcmp), portugues, italiano */',
     'static const char *const lang_ui[][3] = {']
for k in keys: o.append('    {%s, %s, %s},' % (c(k), c(T[k][0]), c(T[k][1])))
o += ['};', '#endif']
open(os.path.join(root, 'src/app/lang_ui.h'), 'w', encoding='utf-8').write('\n'.join(o) + '\n')
print(len(keys), 'textos;', len(need), 'usados')
