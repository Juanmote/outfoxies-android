#!/usr/bin/env python3
"""Letrero REMASTERED de la pantalla de titulo, dibujado a la resolucion del juego (pixel art) con el estilo del logo:
letras estrechas y cuadradas, borde de cristal azul claro, interior azul marino con grietas (la textura sale del panel
azul agrietado de assets/art/paneles.jpg, imagen del usuario) y contorno negro.
Salida: assets/art/remastered.png (RGBA) y, con --preview titulo.ppm, una prueba encima de una captura del titulo.
No usa nada de la ROM."""
import sys, os, math
from PIL import Image, ImageDraw, ImageFilter
root = os.path.join(os.path.dirname(__file__), '..')
SS = 8                      # supersampling
H = 28                      # alto de las letras (pixeles del juego)
GAP = 2
# letras en una rejilla de ancho w x 28: lista de poligonos (el primero relleno, los demas huecos)
L = {
 'R': (17, [[(0,0),(14,0),(17,3),(17,12),(14.5,14.5),(17,17),(17,28),(12,28),(12,17.5),(5,17.5),(5,28),(0,28)],
            [(5,5),(12,5),(12,12.5),(5,12.5)]]),
 'E': (16, [[(0,0),(16,0),(16,5),(5,5),(5,11.5),(13,11.5),(13,16.5),(5,16.5),(5,23),(16,23),(16,28),(0,28)]]),
 'M': (22, [[(0,0),(6,0),(11,9),(16,0),(22,0),(22,28),(17,28),(17,10.5),(13,17),(9,17),(5,10.5),(5,28),(0,28)]]),
 # A de verdad: vertice estrecho arriba y patas abiertas (la anterior parecia una O cortada)
 'A': (19, [[(7,0),(12,0),(19,28),(14,28),(12.6,22),(6.4,22),(5,28),(0,28)],
            [(8.6,8),(10.4,8),(11.6,17),(7.4,17)]]),
 'S': (17, [[(3,0),(17,0),(17,5),(5,5),(5,11.5),(14,11.5),(17,14.5),(17,25),(14,28),(0,28),(0,23),(12,23),(12,16.5),(3,16.5),(0,13.5),(0,3)]]),
 'T': (17, [[(0,0),(17,0),(17,5),(11,5),(11,28),(6,28),(6,5),(0,5)]]),
 'D': (17, [[(0,0),(13,0),(17,4),(17,24),(13,28),(0,28)],[(5,5),(11,5),(12,6),(12,22),(11,23),(5,23)]]),
}
word = 'REMASTERED'
W = sum(L[c][0] for c in word) + GAP * (len(word) - 1)
PAD = 2
cw, ch = W + PAD * 2, H + PAD * 2
mask = Image.new('L', (cw * SS, ch * SS), 0)
d = ImageDraw.Draw(mask)
x = PAD
for c in word:
    w, polys = L[c]
    for k, p in enumerate(polys):
        d.polygon([((x + px) * SS, (PAD + py) * SS) for px, py in p], fill=0 if k else 255)
    x += w + GAP
cov = mask.resize((cw, ch), Image.BOX)                     # cobertura por pixel del juego
m = cov.point(lambda v: 255 if v >= 110 else 0)            # pixel dentro / fuera (sin medias tintas: pixel art)
mp = m.load()
inside = lambda x, y: 0 <= x < cw and 0 <= y < ch and mp[x, y]
# textura: interior del panel azul agrietado, reducida a la escala del juego
pan = Image.open(os.path.join(root, 'assets/art/paneles.jpg')).convert('RGB').crop((770, 205, 1235, 395))
tex = pan.resize((cw, int(cw * pan.height / pan.width)), Image.LANCZOS).filter(ImageFilter.SHARPEN).load()
out = Image.new('RGBA', (cw, ch), (0, 0, 0, 0))
op = out.load()
def mix(a, b, t): return tuple(int(a[i] + (b[i] - a[i]) * t) for i in range(3))
for y in range(ch):
    for x in range(cw):
        if inside(x, y):
            up = not inside(x, y - 1); left = not inside(x - 1, y)
            dn = not inside(x, y + 1); rt = not inside(x + 1, y)
            # segundo anillo (para el bisel de 2 pixeles arriba a la izquierda)
            up2 = not inside(x, y - 2); left2 = not inside(x - 2, y)
            t = tex[x % cw, (y * 2 + 7) % 60]
            lum = (t[0] * 77 + t[1] * 151 + t[2] * 28) >> 8
            # interior: azul marino/verdoso del logo, mas claro donde la textura tiene grieta
            base = mix((6, 16, 40), (14, 46, 64), (x * 7 % 13) / 26 + 0.3 * (y / ch))
            crack = mix(base, (110, 175, 230), max(0, min(1, (lum - 105) / 70)))
            col = crack
            if up or left: col = (206, 247, 255) if (up and left) or up else (173, 231, 255)
            elif dn or rt: col = (33, 90, 150) if dn else (40, 107, 181)
            elif up2 or left2: col = mix(crack, (123, 189, 247), 0.55)
            # brillo general de arriba (cristal)
            if y < PAD + 6 and not (dn or rt): col = mix(col, (220, 245, 255), 0.12)
            op[x, y] = col + (255,)
        else:
            near = any(inside(x + dx, y + dy) for dx in (-1, 0, 1) for dy in (-1, 0, 1))
            if near: op[x, y] = (0, 4, 12, 255)              # contorno
out.save(os.path.join(root, 'assets/art/remastered.png'))
print('REMASTERED %dx%d' % (cw, ch))
if len(sys.argv) > 2 and sys.argv[1] == '--preview':
    t = Image.open(sys.argv[2]).convert('RGB')
    t.paste(out, ((288 - cw) // 2, 133), out)
    t.resize((t.width * 4, t.height * 4), Image.NEAREST).save(sys.argv[3] if len(sys.argv) > 3 else 'preview.png')
# cabecera para el nucleo: RGBA en bruto (es pequena)
im = out
hdr = ['/* Generado por tools/make_title.py: letrero REMASTERED (RGBA). No editar. */',
       '#define TITLE_W %d' % im.width, '#define TITLE_H %d' % im.height, 'static const unsigned char title_rgba[] = {']
raw = im.tobytes()
for i in range(0, len(raw), 32): hdr.append('  ' + ','.join(str(b) for b in raw[i:i + 32]) + ',')
hdr.append('};')
open(os.path.join(root, 'src/core/title_gfx.h'), 'w').write('\n'.join(hdr) + '\n')
