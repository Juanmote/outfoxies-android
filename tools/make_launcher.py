#!/usr/bin/env python3
"""Arte del launcher (pantalla de inicio con botones, al estilo de Re-Master-ed of Darkness).
Entrada (imagenes del usuario): assets/art/portada.jpg (fondo) y assets/art/paneles.jpg (plantilla con 6 paneles de
cristal agrietado sobre negro: azul, acero, rojo / verde, oro, plata).
Salida: src/app/launcher_gfx.h (lo que usa la app). Con una carpeta como argumento, guarda ahi los botones montados para
verlos. En el .h van:
  - la portada en JPEG a 1600 px de ancho;
  - 7 paneles sin texto (PNG de 256 colores con transparencia): los 6 de la plantilla y uno morado (el azul con el tono
    girado) para Trucos;
  - el rotulo de cada boton en castellano e ingles (PNG transparente), con Inter Display Black Italic (licencia OFL).
La app pone el rotulo encima del panel, asi cambiar un texto o el idioma no duplica los paneles.
No usa nada de la ROM."""
import os, io, sys, colorsys
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops
import numpy as np
root = os.path.join(os.path.dirname(__file__), '..')
PW = 420                                   # ancho de cada panel en la app
FONT = '/usr/share/fonts/opentype/inter/InterDisplay-BlackItalic.otf'
# botones en el orden de la fila (ver launcher.h): panel de la plantilla y rotulos
BUTTONS = [('opciones', 'azul', 'OPCIONES', 'OPTIONS'), ('controles', 'acero', 'CONTROLES', 'CONTROLS'),
           ('partidas', 'verde', 'PARTIDAS', 'SAVES'), ('jugar', 'oro', 'JUGAR', 'PLAY'),
           ('logros', 'plata', 'LOGROS', 'AWARDS'), ('trucos', 'morado', 'TRUCOS', 'CHEATS'), ('salir', 'rojo', 'SALIR', 'QUIT')]

# ---------- paneles ----------
src = Image.open(os.path.join(root, 'assets/art/paneles.jpg')).convert('RGB')
a = np.asarray(src).astype(int)
lum = a.max(axis=2)
def runs(v, thr, minlen):
    r, s = [], None
    for i, x in enumerate(list(v) + [0]):
        if x > thr and s is None: s = i
        if x <= thr and s is not None:
            if i - s >= minlen: r.append((s, i))
            s = None
    return r
rows = runs((lum > 70).sum(1), 60, 100); cols = runs((lum > 70).sum(0), 60, 100)
assert len(rows) == 2 and len(cols) == 3, (rows, cols)
names = [['azul', 'acero', 'rojo'], ['verde', 'oro', 'plata']]
panels = {}
for j, (y0, y1) in enumerate(rows):
    for i, (x0, x1) in enumerate(cols):
        m = 10
        box = (x0 - m, y0 - m, x1 + m, y1 + m)
        im = src.crop(box)
        # fondo: lo oscuro conectado con el borde del recorte -> transparente
        dark = Image.fromarray(((np.asarray(im).max(axis=2) < 46) * 255).astype('uint8'))
        seed = dark.copy()
        ImageDraw.floodfill(seed, (0, 0), 128, thresh=0)
        for p in ((im.width - 1, 0), (0, im.height - 1), (im.width - 1, im.height - 1)):
            if seed.getpixel(p) == 255: ImageDraw.floodfill(seed, p, 128, thresh=0)
        alpha = seed.point(lambda v: 0 if v == 128 else 255)
        alpha = alpha.filter(ImageFilter.MinFilter(5)).filter(ImageFilter.GaussianBlur(1.6))
        im = im.convert('RGBA'); im.putalpha(alpha)
        bb = alpha.point(lambda v: 255 if v > 40 else 0).getbbox()
        im = im.crop(bb)
        panels[names[j][i]] = im.resize((PW, round(PW * im.height / im.width)), Image.LANCZOS)
# panel morado: el azul con el tono girado
def hue_shift(im, deg, sat=1.0):
    arr = np.asarray(im.convert('RGBA')).astype(float) / 255
    out = arr.copy()
    h, w, _ = arr.shape
    flat = arr.reshape(-1, 4)
    res = np.empty_like(flat)
    for k, (r, g, b, al) in enumerate(flat):
        hh, ll, ss = colorsys.rgb_to_hls(r, g, b)
        r2, g2, b2 = colorsys.hls_to_rgb((hh + deg / 360) % 1, ll, min(1, ss * sat))
        res[k] = (r2, g2, b2, al)
    return Image.fromarray((res.reshape(h, w, 4) * 255 + 0.5).astype('uint8'), 'RGBA')
panels['morado'] = hue_shift(panels['azul'], 62, 0.95)
PH = max(p.height for p in panels.values())

# ---------- rotulos ----------
SS = 3
def fit_size(text, maxw, maxh):
    size = int(maxh * SS * 1.25)
    while True:
        f = ImageFont.truetype(FONT, size)
        w = sum(f.getlength(c) for c in text) + size * 0.03 * (len(text) - 1)
        bb = f.getbbox(text)
        if w <= maxw * SS and (bb[3] - bb[1]) <= maxh * SS: return size
        size -= 2
def label(text, size):
    f = ImageFont.truetype(FONT, size)
    sp = size * 0.03
    w = sum(f.getlength(c) for c in text) + sp * (len(text) - 1)
    bb = f.getbbox(text)
    st = max(3, round(size * 0.085))                              # contorno
    W, H = int(w + st * 4 + size * 0.3), int((bb[3] - bb[1]) + st * 4 + size * 0.25)
    def draw(fill, stroke, sw):
        im = Image.new('L', (W, H), 0); d = ImageDraw.Draw(im)
        x = st * 2
        for c in text:
            d.text((x, st * 2 - bb[1]), c, font=f, fill=fill, stroke_width=sw, stroke_fill=stroke)
            x += f.getlength(c) + sp
        return im
    body = draw(255, 0, 0); outline = draw(255, 255, st)
    # relleno: degradado blanco -> azul hielo, con una linea de brillo arriba
    grad = Image.new('RGB', (W, H))
    gp = grad.load()
    for y in range(H):
        t = min(1, max(0, (y - st * 2) / max(1, H - st * 4)))
        c = (round(255 - 58 * t), round(255 - 40 * t), round(255 - 14 * t))
        for x in range(W): gp[x, y] = c
    res = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    sh = Image.new('RGBA', (W, H), (0, 0, 0, 0))
    sh.paste((4, 6, 14, 200), (0, 0), ImageChops.offset(outline, 0, round(size * 0.07)).filter(ImageFilter.GaussianBlur(size * 0.05)))
    res.alpha_composite(sh)
    res.paste((9, 14, 30, 255), (0, 0), outline)
    res.paste(grad, (0, 0), body)
    return res.resize((round(W / SS), round(H / SS)), Image.LANCZOS)

def png8(im):                                # 256 colores con transparencia
    q = im.quantize(256, method=Image.Quantize.FASTOCTREE, dither=Image.Dither.NONE)
    b = io.BytesIO(); q.save(b, 'PNG', optimize=True); return b.getvalue()
def png32(im):
    b = io.BytesIO(); im.save(b, 'PNG', optimize=True); return b.getvalue()

blobs = []                                   # (nombre, bytes)
cover = Image.open(os.path.join(root, 'assets/art/portada.jpg')).convert('RGB')
cover = cover.resize((1600, round(1600 * cover.height / cover.width)), Image.LANCZOS)
b = io.BytesIO(); cover.save(b, 'JPEG', quality=84, optimize=True); blobs.append(('lg_cover', b.getvalue()))
prev_dir = sys.argv[1] if len(sys.argv) > 1 else None      # make_launcher.py <carpeta>: guarda ahi los botones para verlos
if prev_dir: os.makedirs(prev_dir, exist_ok=True)
# todos los rotulos de un idioma al mismo tamano (el del mas largo); el de Jugar, un 30 % mayor
sizes = {}
for li in (2, 3):
    common = min(fit_size(b[li], PW * 0.80, PH * 0.32) for b in BUTTONS if b[0] != 'jugar')
    for b in BUTTONS: sizes[b[li]] = min(fit_size(b[li], PW * 0.80, PH * 0.42), round(common * 1.3)) if b[0] == 'jugar' else common
for k, (name, pan, es, en) in enumerate(BUTTONS):
    p = panels[pan]
    data = png8(p); blobs.append(('lg_panel_%d' % k, data))
    for lang, t in (('es', es), ('en', en)):
        lab = label(t, sizes[t])
        data = png8(lab); blobs.append(('lg_text_%s_%d' % (lang, k), data))
        if prev_dir:
            prev = p.copy(); prev.alpha_composite(lab, ((p.width - lab.width) // 2, (p.height - lab.height) // 2))
            prev.save(os.path.join(prev_dir, '%s%s.png' % ('' if lang == 'es' else 'en_', name)))
out = ['/* Generado por tools/make_launcher.py: no editar */', '#ifndef OFX_LAUNCHER_GFX_H', '#define OFX_LAUNCHER_GFX_H']
total = 0
for name, data in blobs:
    total += len(data)
    out.append('static const unsigned char %s[%d] = {' % (name, len(data)))
    for i in range(0, len(data), 32): out.append(','.join(str(c) for c in data[i:i + 32]) + ',')
    out.append('};')
out.append('#endif')
open(os.path.join(root, 'src/app/launcher_gfx.h'), 'w').write('\n'.join(out) + '\n')
print('launcher_gfx.h:', total, 'bytes de arte;', ', '.join('%s %d' % (n, len(d)) for n, d in blobs[:3]))
