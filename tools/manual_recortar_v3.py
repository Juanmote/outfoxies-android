#!/usr/bin/env python3
"""Saca del manual anterior (las .jpg del pack manual.zip de make_manual.py) todas las imagenes del juego que lleva, para
volver a generarlo sin tener que capturarlas otra vez (17-11-2026; amplia manual_recortar_v2.py).
Uso: manual_recortar_v3.py <carpeta con las jpg> <salida>
Saca: sel_*.png, esc0-6.png, tv/dossier/saldo.png (v2), mansion_panoramica.png, fin_<personaje>_1-3.png (reducidas a
128x96, su tamano real) y arma_raw_<arma>.png (el recuadro del icono tal cual, para pegarlo en el mismo sitio)."""
import os, sys, subprocess
from PIL import Image
SRC, OUT = sys.argv[1], sys.argv[2]
os.makedirs(OUT, exist_ok=True)
here = os.path.dirname(os.path.abspath(__file__))
subprocess.check_call([sys.executable, os.path.join(here, 'manual_recortar_v2.py'), SRC, OUT])
sys.argv = [sys.argv[0], SRC, OUT]
import importlib.util
spec = importlib.util.spec_from_file_location('v2', os.path.join(here, 'manual_recortar_v2.py'))
src = open(os.path.join(here, 'manual_recortar_v2.py')).read()
ns = {}; exec(src.split('names = ')[0], ns)       # solo la funcion red_box
red_box = ns['red_box']
# retratos: en el manual de la 0.27.0 van en (70, 220, 560, 690), con el panel Datos debajo (v2 los cogia hasta 1010)
for n, f in zip(['john', 'eve', 'betty', 'bernard', 'ching', 'dweeb', 'gemelos'],
                ['04 Ficha John Smith', '05 Ficha Eve', '06 Ficha Betty Doe', '07 Ficha Bernard White', '08 Ficha Prof. Ching',
                 '09 Ficha Dweeb', '10 Ficha Danny Y Demi']):
    red_box(f + '.jpg', (70, 220, 560, 690)).save(OUT + '/sel_%s.png' % n)
red_box('22 Escenario Mansión.jpg', (70, 215, 1260, 1010)).save(OUT + '/mansion_panoramica.png')
names = ['john', 'eve', 'betty', 'bernard', 'ching', 'dweeb', 'gemelos']
files = ['John Smith', 'Eve', 'Betty Doe', 'Bernard White', 'Prof. Ching', 'Dweeb', 'Danny Y Demi']
for i, (n, f) in enumerate(zip(names, files)):
    for j in range(3):
        x0 = 60 + j * 610
        # 128x96 en 580x435 llena el recuadro justo (place fit): recorte fijo (las fotos tienen mucho rojo)
        p = Image.open(SRC + '/%02d Final %s.jpg' % (24 + i, f)).convert('RGB').crop((x0, 390, x0 + 580, 825))
        p.resize((128, 96), Image.BOX).save(OUT + '/fin_%s_%d.png' % (n, j + 1))
arm = ['pistola', 'metralleta', 'lanzacohetes', 'lanzallamas', 'granada', 'katana', 'latigo']
page = Image.open(SRC + '/13 Armas.jpg').convert('RGB')
for k, fn in enumerate(arm):
    col, row = k % 4, k // 4
    x0 = 60 + col * 455; y0 = 210 + row * 410; x1 = x0 + 430
    page.crop((x0 + 30, y0 + 24, x1 - 30, y0 + 150)).save(OUT + '/arma_raw_%s.png' % fn)
print('hecho')
