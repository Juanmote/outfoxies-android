#!/usr/bin/env python3
"""Packs de ropa que trae la app (assets/ropas/*.txt, hechos por el usuario con el editor) -> src/app/packs_incluidos.h.
La app los copia a su carpeta "ropas" la primera vez (skinpacks.c), y desde ahi se pueden retocar o borrar."""
import os, glob
root = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
fs = sorted(glob.glob(os.path.join(root, 'assets/ropas/*.txt')))
def cstr(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n"\n    "') + '"'
o = ['/* Generado por tools/make_packs_incluidos.py: no editar */', '#ifndef OFX_PACKS_INCLUIDOS_H', '#define OFX_PACKS_INCLUIDOS_H',
     'static const struct { const char *name, *text; } packs_incluidos[] = {']
for f in fs:
    o.append('  {%s,\n    %s},' % (cstr(os.path.basename(f)[:-4]), cstr(open(f, encoding='utf-8').read())))
o += ['};', '#endif']
open(os.path.join(root, 'src/app/packs_incluidos.h'), 'w', encoding='utf-8').write('\n'.join(o) + '\n')
print(len(fs), 'packs')
