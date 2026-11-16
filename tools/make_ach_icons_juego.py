#!/usr/bin/env python3
"""Iconos de los logros propios 21-23 (tren, fabrica, barco) con imagenes del juego, como los de RetroAchievements:
un recorte de la pantalla dentro del marco de tele del icono 20.
Uso: make_ach_icons_juego.py tren.png ching.png barco.png  (recortes de capturas del juego, unos 115x125; no van al
repositorio). Sustituye los iconos 21-23 de assets/logros/iconos.b64; luego tools/make_ach_icons.py genera el .h."""
import base64, zlib, sys, os
from PIL import Image
P = os.path.join(os.path.dirname(os.path.abspath(__file__)), '../assets/logros/iconos.b64')
raw = bytearray(zlib.decompress(base64.b64decode(open(P).read().replace('\n', ''))))
def px(i, x, y): o = i * 2048 + (y * 32 + x) * 2; return raw[o] << 8 | raw[o + 1]
ics = [12, 13, 14, 15, 20]          # iconos de RA con la misma tele: lo que cambia entre ellos es la pantalla
mask = [[len({px(i, x, y) >> 1 & 0x3def for i in ics}) > 2 for x in range(32)] for y in range(32)]
xs = [x for y in range(32) for x in range(32) if mask[y][x]]; ys = [y for y in range(32) for x in range(32) if mask[y][x]]
X0, X1, Y0, Y1 = min(xs), max(xs), min(ys), max(ys)
frame = raw[20 * 2048:21 * 2048]
out = bytearray()
for f in sys.argv[1:4]:
    im = Image.open(f).convert('RGB').resize((X1 - X0 + 1, Y1 - Y0 + 1), Image.LANCZOS)
    o = bytearray(frame)
    for y in range(32):
        for x in range(32):
            if mask[y][x]:
                r, g, b = im.getpixel((x - X0, y - Y0)); v = (r * 31 // 255) << 10 | (g * 31 // 255) << 5 | (b * 31 // 255)
                o[(y * 32 + x) * 2] = v >> 8; o[(y * 32 + x) * 2 + 1] = v & 255
    out += o
raw[21 * 2048:24 * 2048] = out
open(P, 'w').write(base64.b64encode(zlib.compress(bytes(raw), 9)).decode() + '\n')
print('iconos 21-23 cambiados')
