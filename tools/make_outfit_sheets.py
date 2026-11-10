#!/usr/bin/env python3
"""Hojas de revision de ropas: un PNG por estilo con todos los personajes.
Uso: make_outfit_sheets.py <carpeta_capturas> <carpeta_salida>
La carpeta de capturas tiene una subcarpeta por personaje (o_0 ... o_7) hecha con headless:
  OFX_CHAR=n OFX_TRUCOS=1 OFX_STYLES=1 OFX_STICK=900 headless outfxies.zip 1700 10 300 o_n      (jugador, banco 1)
  OFX_TRUCOS=1 OFX_ESCENARIO=7 OFX_STYLES=0 OFX_STICK=1000 headless outfxies.zip 3200 10 300 o_7 (los Acme, banco 0)
Cada captura trae solo los sprites de ese banco sobre magenta, con las 5 ropas (_s0 ... _s4)."""
import sys, glob, os, re
from PIL import Image, ImageDraw, ImageFont
src, out = sys.argv[1], sys.argv[2]
# orden de OFX_CHAR (pulsaciones a la derecha en la eleccion de personaje); los Acme van en el banco 0 y hoy no cambian
NAMES = ['John Smith', 'Prof. Ching', 'Dweeb', 'Eve', 'Betty Doe', 'Danny y Demi', 'Bernard White', 'Mr. y Mrs. Acme\n(hoy no cambian)']
STYLES = ['Original', 'Gótica', 'Negativa', 'Pinky', 'Vampiro']
NPOSE, CELL_W, CELL_H, LABEL_W, TOP = 5, 250, 250, 230, 70
BG, PANEL = (24, 25, 33), (44, 46, 60)
def font(sz):
    for p in ('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',):
        if os.path.exists(p): return ImageFont.truetype(p, sz)
    return ImageFont.load_default()
def info(path):
    im = Image.open(path).convert('RGB'); px = im.load(); w, h = im.size
    xs = [x for x in range(w) for y in range(h) if px[x, y] != (255, 0, 255)]
    if not xs: return None
    ys = [y for y in range(h) for x in range(w) if px[x, y] != (255, 0, 255)]
    bb = (min(xs), min(ys), max(xs) + 1, max(ys) + 1)
    return len(xs), bb, (w, h)
poses = {}
for c in range(8):
    d = os.path.join(src, 'o_%d' % c)
    fr = sorted(int(re.search(r'f(\d+)_s0', f).group(1)) for f in glob.glob(d + '/f*_s0.ppm'))
    cand = []
    for n in fr:
        r = info('%s/f%06d_s0.ppm' % (d, n))
        if not r: continue
        cnt, bb, (w, h) = r
        bw, bh = bb[2] - bb[0], bb[3] - bb[1]
        # el personaje entero y solo: sin tocar los bordes y con forma de persona (no dos cosas separadas)
        if bb[0] <= 1 or bb[1] <= 1 or bb[2] >= w - 1 or bb[3] >= h - 1 or bh < 30 or bw > 150 or cnt < bw * bh * 0.22: continue
        cand.append((n, cnt, bb))
    # NPOSE tramos de tiempo; en cada uno la captura con el personaje mas grande
    sel = []
    if cand:
        t0, t1 = cand[0][0], cand[-1][0] + 1
        for k in range(NPOSE):
            part = [x for x in cand if t0 + (t1 - t0) * k // NPOSE <= x[0] < t0 + (t1 - t0) * (k + 1) // NPOSE]
            if part: sel.append(max(part, key=lambda x: x[1]))
    poses[c] = sel
for s, sname in enumerate(STYLES):
    W = Image.new('RGB', (LABEL_W + NPOSE * CELL_W + 20, TOP + 8 * CELL_H + 20), BG)
    dr = ImageDraw.Draw(W)
    dr.text((20, 16), 'The Outfoxies Remastered · Ropa: %s' % sname, font=font(34), fill=(240, 150, 40))
    for c in range(8):
        y0 = TOP + c * CELL_H
        dr.rectangle((10, y0 + 4, W.width - 10, y0 + CELL_H - 4), fill=PANEL)
        dr.multiline_text((24, y0 + CELL_H // 2 - 14 * (1 + NAMES[c].count('\n'))), NAMES[c], font=font(21), fill=(255, 255, 255), spacing=6)
        for k, (n, cnt, bb) in enumerate(poses[c]):
            im = Image.open('%s/o_%d/f%06d_s%d.ppm' % (src, c, n, s)).convert('RGB').crop(bb)
            px = im.load()
            for yy in range(im.height):
                for xx in range(im.width):
                    if px[xx, yy] == (255, 0, 255): px[xx, yy] = PANEL
            z = max(1, min((CELL_W - 16) // im.width, (CELL_H - 20) // im.height, 4))
            im = im.resize((im.width * z, im.height * z), Image.NEAREST)
            W.paste(im, (LABEL_W + k * CELL_W + (CELL_W - im.width) // 2, y0 + (CELL_H - im.height) // 2))
    os.makedirs(out, exist_ok=True)
    fn = os.path.join(out, 'ropa_%d_%s.png' % (s, sname.lower().replace('ó', 'o')))
    W.save(fn); print(fn)
