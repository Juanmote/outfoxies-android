#!/usr/bin/env python3
"""Manual de The Outfoxies Remastered: paginas de 1920x1080 (la forma de las consolas) dibujadas con PIL.
Salida: manual.pdf (una pagina por imagen) y manual.zip (el pack del boton MANUAL del launcher: "01 Portada.jpg"...).
Uso: make_manual.py <carpeta_imagenes> <carpeta_salida> [es|en|pt|it|todos]   (textos: tools/manual_textos.py)
La carpeta de imagenes lleva capturas sacadas del juego (no van en el repositorio ni en el APK):
  sel_<personaje>.png (eleccion de personaje), esc0..esc7.ppm (escenarios, 16:9), tv.ppm y dossier.ppm (encargos de
  Mr. Acme), y opcionalmente fin_<personaje>.png / fin_<personaje>_2.png (fotos de los finales; si faltan, hueco).
Textos: biografias del juego (traduccion de tools/make_lang.py) y datos del documento "Personajes" (tablas de la ROM)."""
import os, sys, io, zipfile
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manual_textos

R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC, OUT = sys.argv[1], sys.argv[2]
LANG = sys.argv[3] if len(sys.argv) > 3 else 'es'
if LANG == 'todos':
    import subprocess
    for l in manual_textos.LANGS: subprocess.check_call([sys.executable, __file__, SRC, OUT, l])
    sys.exit(0)
L = manual_textos.T[LANG]
DEC = manual_textos.DEC[LANG]
os.makedirs(OUT, exist_ok=True)
W, H = 1920, 1080
FD = '/usr/share/fonts/opentype/inter/'
def F(name, size): return ImageFont.truetype(FD + name, size)
TITLE = 'InterDisplay-BlackItalic.otf'
RED, GOLD, ICE, LCD, STEEL = (214, 38, 28), (242, 182, 50), (205, 220, 245), (70, 255, 110), (150, 158, 172)
BG0, BG1 = (8, 9, 16), (22, 26, 40)

# ---------------------------------------------------------------- piezas
def background():
    im = Image.new('RGB', (W, H), BG0)
    d = ImageDraw.Draw(im)
    for y in range(H):                                       # degradado
        t = y / H
        d.line([(0, y), (W, y)], fill=tuple(int(BG0[k] + (BG1[k] - BG0[k]) * (1 - abs(0.5 - t) * 2)) for k in range(3)))
    ov = Image.new('L', (W, H), 0); o = ImageDraw.Draw(ov)    # lineas de comic en diagonal, muy suaves
    for x in range(-H, W, 46): o.line([(x, H), (x + H, 0)], fill=14, width=3)
    im.paste((40, 46, 70), (0, 0), ov)
    return im

def steel_frame(im, box, w=12, rivets=True, inner=RED):
    x0, y0, x1, y1 = box
    d = ImageDraw.Draw(im)
    for i in range(w):                                       # marco de acero: degradado de claro a oscuro hacia dentro
        t = i / max(1, w - 1)
        c = tuple(int(v) for v in (200 - 120 * t, 206 - 120 * t, 218 - 115 * t))
        d.rectangle([x0 - w + i, y0 - w + i, x1 + w - i, y1 + w - i], outline=c)
    d.rectangle([x0 - 1, y0 - 1, x1 + 1, y1 + 1], outline=inner, width=2)
    if rivets:
        for cx, cy in ((x0 - w // 2, y0 - w // 2), (x1 + w // 2, y0 - w // 2), (x0 - w // 2, y1 + w // 2), (x1 + w // 2, y1 + w // 2)):
            r = max(3, w // 3)
            d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=(90, 96, 108), outline=(230, 234, 240))
            d.ellipse([cx - r + 2, cy - r + 2, cx, cy], fill=(240, 244, 250))

def place(im, pic, box, mode='fit', frame=True, pixel=True):
    """pic dentro de box (x0,y0,x1,y1); fit = entera, fill = recortada para llenar"""
    x0, y0, x1, y1 = box; bw, bh = x1 - x0, y1 - y0
    k = (min if mode == 'fit' else max)(bw / pic.width, bh / pic.height)
    p = pic.resize((max(1, round(pic.width * k)), max(1, round(pic.height * k))), Image.NEAREST if pixel and k >= 2 else Image.LANCZOS)
    if mode == 'fill':
        cx, cy = (p.width - bw) // 2, (p.height - bh) // 2
        p = p.crop((cx, cy, cx + bw, cy + bh)); px, py = x0, y0
    else: px, py = x0 + (bw - p.width) // 2, y0 + (bh - p.height) // 2
    im.paste(p, (px, py), p if p.mode == 'RGBA' else None)
    if frame: steel_frame(im, (px, py, px + p.width - 1, py + p.height - 1))
    return (px, py, px + p.width, py + p.height)

def title_text(im, xy, text, size, anchor='la', fill_top=(255, 255, 255), fill_bot=ICE, stroke=(9, 14, 30)):
    """rotulo como los del launcher: relleno blanco -> hielo, contorno oscuro y sombra"""
    f = F(TITLE, size); d = ImageDraw.Draw(im)
    bb = d.textbbox(xy, text, font=f, anchor=anchor, stroke_width=max(3, size // 12))
    w, h = bb[2] - bb[0] + 20, bb[3] - bb[1] + 20
    m = Image.new('L', (w, h), 0); md = ImageDraw.Draw(m)
    ox, oy = xy[0] - bb[0] + 10, xy[1] - bb[1] + 10
    st = max(3, size // 12)
    md.text((ox, oy), text, font=f, anchor=anchor, fill=255, stroke_width=st, stroke_fill=255)
    body = Image.new('L', (w, h), 0); ImageDraw.Draw(body).text((ox, oy), text, font=f, anchor=anchor, fill=255)
    grad = Image.new('RGB', (w, h)); g = ImageDraw.Draw(grad)
    for y in range(h):
        t = y / h; g.line([(0, y), (w, y)], fill=tuple(int(fill_top[k] + (fill_bot[k] - fill_top[k]) * t) for k in range(3)))
    sh = m.filter(ImageFilter.GaussianBlur(size // 18 + 1))
    im.paste((0, 0, 0), (bb[0] - 10 + size // 16, bb[1] - 10 + size // 14), sh)
    im.paste(stroke, (bb[0] - 10, bb[1] - 10), m)
    im.paste(grad, (bb[0] - 10, bb[1] - 10), body)

def header(im, section, title, num):
    d = ImageDraw.Draw(im)
    d.polygon([(0, 0), (W, 0), (W, 118), (0, 150)], fill=(14, 16, 26))
    d.polygon([(0, 150), (W, 118), (W, 126), (0, 158)], fill=RED)
    d.polygon([(0, 162), (W, 130), (W, 133), (0, 165)], fill=GOLD)
    d.text((60, 26), section.upper(), font=F('Inter-ExtraBoldItalic.otf', 30), fill=RED)
    title_text(im, (58, 56), title, 64)
    d.text((W - 60, 40), 'THE OUTFOXIES', font=F(TITLE, 34), fill=(80, 88, 110), anchor='ra')
    d.text((W - 60, 80), 'REMASTERED', font=F(TITLE, 22), fill=(64, 72, 92), anchor='ra')

def wrap(d, text, font, width):
    out = []
    for para in text.split('\n'):
        line = ''
        for w_ in para.split(' '):
            t = (line + ' ' + w_).strip()
            if d.textlength(t, font=font) <= width: line = t
            else: out.append(line); line = w_
        out.append(line)
    return out

PB = [H]   # borde de abajo del ultimo panel: aviso si un texto se sale (paginas en otros idiomas)
def check(y, size, spacing, what):
    if y - int(size * (spacing - 1)) > PB[0] - 6: print('AVISO [%s] se sale del panel: %s' % (LANG, what[:50]))
def paragraph(im, xy, text, width, size=30, fill=(222, 226, 236), spacing=1.42, font='Inter-Regular.otf', bold_lead=None):
    d = ImageDraw.Draw(im); f = F(font, size); x, y = xy
    for ln in wrap(d, text, f, width):
        d.text((x, y), ln, font=f, fill=fill); y += int(size * spacing)
    check(y, size, spacing, text)
    return y
def bullets_size(items, width, height, sizes):
    """el tamano mas grande con el que la lista de bullets() cabe en height"""
    d = ImageDraw.Draw(Image.new('L', (1, 1)))
    for sz in sizes:
        h = sum(len(wrap(d, it, F('Inter-Regular.otf', sz), width - 40)) * int(sz * 1.36) + 8 for it in items) - 8
        if h <= height: return sz
    return sizes[-1]
def fit_size(text, width, height, sizes, spacing=1.42, font='Inter-Regular.otf'):
    """el tamano mas grande de sizes con el que text cabe en width x height"""
    d = ImageDraw.Draw(Image.new('L', (1, 1)))
    for sz in sizes:
        if len(wrap(d, text, F(font, sz), width)) * int(sz * spacing) <= height: return sz
    return sizes[-1]

def panel(im, box, title=None, accent=RED):
    x0, y0, x1, y1 = box
    ov = Image.new('RGBA', (W, H), (0, 0, 0, 0)); o = ImageDraw.Draw(ov)
    o.rounded_rectangle(box, 14, fill=(6, 8, 14, 205), outline=(90, 98, 120, 255), width=2)
    im.paste(ov, (0, 0), ov)
    d = ImageDraw.Draw(im)
    d.rectangle([x0 + 14, y0, x1 - 14, y0 + 5], fill=accent)
    if title: d.text((x0 + 26, y0 + 18), title.upper(), font=F('Inter-ExtraBoldItalic.otf', 26), fill=accent)
    PB[0] = y1
    return y0 + (64 if title else 22)

def lcd_bar(im, x, y, label, value, n=8, color=LCD, note=''):
    """barra de segmentos como el reloj del marcador: value de 0 a 1"""
    d = ImageDraw.Draw(im)
    d.text((x, y), label, font=F('Inter-SemiBold.otf', 26), fill=(214, 220, 232))
    bx = x + 160
    d.rectangle([bx - 6, y - 4, bx + n * 30 + 2, y + 34], fill=(0, 0, 0), outline=(70, 74, 86))
    on = max(1, round(value * n))
    for i in range(n):
        c = color if i < on else (28, 36, 30)
        d.rectangle([bx + i * 30, y + 2, bx + i * 30 + 24, y + 28], fill=c)
        if i < on: d.rectangle([bx + i * 30, y + 2, bx + i * 30 + 24, y + 6], fill=tuple(min(255, v + 90) for v in c))
    if note: d.text((bx + n * 30 + 18, y + 2), note, font=F('Inter-Medium.otf', 23), fill=(150, 158, 178))

def bullets(im, xy, items, width, size=27, mark='+', color=LCD):
    d = ImageDraw.Draw(im); f = F('Inter-Regular.otf', size); x, y = xy
    for it in items:
        d.text((x, y), mark, font=F('Inter-Black.otf', size), fill=color)
        for i, ln in enumerate(wrap(d, it, f, width - 40)):
            d.text((x + 40, y), ln, font=f, fill=(222, 226, 236)); y += int(size * 1.36)
        y += 8
    check(y - 8, size, 1.36, items[-1])
    return y

def load(name, crop43=False):
    p = os.path.join(SRC, name)
    b, e = os.path.splitext(name)                   # la del idioma si la hay (tv_en.png: teles con los textos en ingles)
    for q in (os.path.join(SRC, '%s_%s%s' % (b, LANG, e)), os.path.join(SRC, '%s_%s.png' % (b, LANG))):
        if os.path.exists(q): p = q; break
    if not os.path.exists(p) and name.endswith('.ppm'): p = p[:-4] + '.png'   # o recortada de un manual anterior
    if not os.path.exists(p): return None
    im = Image.open(p).convert('RGB')
    if crop43 and im.width == 384: im = im.crop((48, 0, 336, 224))
    if crop43 and im.width == 288 and im.height == 224: pass   # captura del juego en 4:3 tal cual
    return im

pages = []
def save(im, name):
    if pages:   # numero de pagina abajo en el centro (la portada no lleva)
        d = ImageDraw.Draw(im); n = str(len(pages) + 1)
        d.rounded_rectangle([W // 2 - 34, H - 40, W // 2 + 34, H - 6], 8, fill=(10, 12, 20), outline=(70, 76, 96), width=2)
        d.text((W // 2, H - 23), n, font=F('Inter-Bold.otf', 24), fill=(170, 178, 198), anchor='mm')
    pages.append((name, im))
def num(v, fmt): return (fmt % v).replace('.', DEC)

# ---------------------------------------------------------------- portada
P = L['pages']
im = Image.open(os.path.join(R, 'assets/art/portada.jpg')).convert('RGB')
k = max(W / im.width, H / im.height); im = im.resize((round(im.width * k), round(im.height * k)), Image.LANCZOS)
im = im.crop(((im.width - W) // 2, 0, (im.width - W) // 2 + W, H))
ov = Image.new('RGBA', (W, H), (0, 0, 0, 0)); o = ImageDraw.Draw(ov)
for y in range(H - 300, H): o.line([(0, y), (W, y)], fill=(4, 5, 10, int(235 * (y - H + 300) / 300)))
im.paste(ov, (0, 0), ov)
d = ImageDraw.Draw(im)
d.polygon([(0, H - 196), (W, H - 236), (W, H - 228), (0, H - 188)], fill=RED)
title_text(im, (W // 2, H - 172), L['cover_title'], 84, anchor='ma', fill_bot=GOLD)
d.text((W // 2, H - 26), L['cover_sub'], font=F('Inter-SemiBoldItalic.otf', 30), fill=(200, 206, 222), anchor='ms')
save(im, P['cover'])

# ---------------------------------------------------------------- historia
im = background(); header(im, L['sec_story'], L['story_title'], 2)
y = panel(im, (60, 210, 1000, 1010), L['story_panel'])
st = L['story']
sz = fit_size(' '.join(st) + ' x x x x x x', 860, 760, (32, 31, 30, 29, 28))
y = paragraph(im, (90, y), st[0], 860, sz)
y = paragraph(im, (90, y + 14), st[1], 860, sz, fill=GOLD, font='Inter-SemiBoldItalic.otf')
y = paragraph(im, (90, y + 22), st[2], 860, sz - 4)
y = paragraph(im, (90, y + 14), st[3], 860, sz - 4)
tv = load('tv.ppm', True); dos = load('dossier.ppm', True)
if tv: place(im, tv, (1080, 220, 1860, 600))
if dos: place(im, dos, (1080, 650, 1860, 1010))
save(im, P['story'])

im = background(); header(im, L['sec_story'], L['howto_title'], 3)
y = panel(im, (60, 210, 930, 1010), L['rules_panel'])
y = bullets(im, (90, y), L['rules'], 820, bullets_size(L['rules'], 820, 1010 - y - 20, (28, 27, 26, 25, 24, 23)))
y = panel(im, (980, 210, 1860, 1010), L['dmg_panel'], GOLD)
vals = [('35', '16 %'), ('25', '11 %'), ('20', '9 %'), ('15', '7 %'), ('10', '5 %'), ('10', '5 %'), ('5', '2 %'), ('2', '1 %'),
        ('1', num(0.5, '%.1f') + ' %'), ('0', '0 %')]
d = ImageDraw.Draw(im); fh = F('Inter-Bold.otf', 24)
c0, c1, c2 = L['dmg_cols']
d.text((1010, y), c0, font=fh, fill=(150, 158, 178)); d.text((1560, y), c1, font=fh, fill=(150, 158, 178), anchor='ra')
d.text((1820, y), c2, font=fh, fill=(150, 158, 178), anchor='ra'); y += 46
for i, (a, (b, c)) in enumerate(zip(L['dmg_rows'], vals)):
    fr_ = F('Inter-Regular.otf', 27)
    if d.textlength(a, font=fr_) > 500: fr_ = F('Inter-Regular.otf', 23)
    if i % 2 == 0: d.rectangle([1000, y - 8, 1840, y + 40], fill=(18, 22, 34))
    d.text((1010, y), a, font=fr_, fill=(226, 230, 240)); d.text((1560, y), b, font=F('Inter-Bold.otf', 27), fill=GOLD, anchor='ra')
    d.text((1820, y), c, font=F('Inter-Regular.otf', 27), fill=LCD, anchor='ra'); y += 56
paragraph(im, (1010, y + 10), L['dmg_note'], 800, 22, fill=(150, 158, 178))
save(im, P['howto'])

# ---------------------------------------------------------------- fichas
CH = [  # archivo, andar, correr, salto, recibe %, pistola, cohetes, metralleta, placaje (textos en manual_textos.py)
    ('john', 2.19, 4.0, 59, 109, 10, 20, 14, 5),
    ('eve', 2.38, 5.0, 75, 113, 14, 24, 16, 4),
    ('betty', 2.13, 3.0, 54, 117, 4, 4, 6, 4),
    ('bernard', 2.25, 6.0, 55, 100, 5, 15, 10, 12),
    ('ching', 1.75, 8.0, 69, 130, 24, 0, 24, 6),
    ('dweeb', 2.00, 4.5, 72, 120, 30, 37, 30, 5),
    ('gemelos', 2.06, 3.5, 53, 133, 20, 36, 18, 6),
]
MIN = {'andar': 1.75, 'correr': 3.0, 'salto': 53, 'aguante': 133, 'punt': 30}
MAX = {'andar': 2.38, 'correr': 8.0, 'salto': 75, 'aguante': 100, 'punt': 4}
def norm(k, v): return 0.15 + 0.85 * (v - MIN[k]) / (MAX[k] - MIN[k])
for i, c in enumerate(CH):
    fn, andar, correr, salto, rec, pis, coh, met, plac = c
    t = L['ch'][fn]; name = t['name']
    im = background(); header(im, L['sec_chars'], name, 4 + i)
    pic = load('sel_%s.png' % fn)
    if pic: place(im, pic, (70, 220, 560, 690))
    y = panel(im, (40, 712, 600, 1030), L['p_data'], GOLD)
    d = ImageDraw.Draw(im)
    lw = max(d.textlength(a.upper(), font=F('Inter-Bold.otf', 18)) for a, _ in t['wiki'])
    vx = 214 if lw <= 140 else 66 + int(lw) + 14
    for j, (a, b) in enumerate(t['wiki']):          # etiqueta a la izquierda y el dato al lado (el remate, en dos lineas)
        d.text((66, y + 2 + j * 39), a.upper(), font=F('Inter-Bold.otf', 18), fill=(150, 158, 178))
        bs = 22 if d.textlength(b, font=F('Inter-SemiBold.otf', 22)) <= 2 * (580 - vx) - 40 else 19
        PB[0] = H
        paragraph(im, (vx, y - 2 + j * 39), b, 580 - vx, bs, font='Inter-SemiBold.otf', fill=(236, 240, 248), spacing=1.25)
    PB[0] = 1030
    x = 620
    y = panel(im, (x, 200, 1860, 380), L['p_who'])
    paragraph(im, (x + 30, y - 4), t['bio'], 1180, fit_size(t['bio'], 1180, 120, (29, 28, 27, 26, 25)), font='Inter-MediumItalic.otf', fill=(232, 236, 244))
    ImageDraw.Draw(im).text((1836, 218), L['stage_lbl'] + t['esc'].upper(), font=F('Inter-Bold.otf', 24), fill=GOLD, anchor='ra')
    y = panel(im, (x, 384, 1290, 650), L['p_stats'], LCD)
    sa, sc, ss, sg, sp = L['st']
    lcd_bar(im, x + 30, y, sa, norm('andar', andar), note=num(andar, '%.2f'))
    lcd_bar(im, x + 30, y + 40, sc, norm('correr', correr), note=num(correr, '%.1f') + ' · ' + t['tcor'])
    lcd_bar(im, x + 30, y + 80, ss, norm('salto', salto), note='%d px' % salto)
    lcd_bar(im, x + 30, y + 120, sg, norm('aguante', rec), note=L['st_recv'] % rec)
    lcd_bar(im, x + 30, y + 160, sp, norm('punt', pis), note=L['st_gun'] % pis)
    y = panel(im, (1310, 384, 1860, 650), L['p_arms'], GOLD)
    d = ImageDraw.Draw(im); f = F('Inter-Regular.otf', 26); fb = F('Inter-Bold.otf', 26)
    for j, (a, b) in enumerate(zip(L['arms'], ['%d°' % pis, '%d°' % coh, '%d°' % met, str(plac), '3' if fn == 'bernard' else '1'])):
        d.text((1338, y + j * 40), a, font=f, fill=(222, 226, 236)); d.text((1832, y + j * 40), b, font=fb, fill=GOLD, anchor='ra')
    bs = min(bullets_size(t['pros'], 590, 126, (24, 23, 22, 21, 20, 19, 18)), bullets_size(t['cons'], 570, 126, (24, 23, 22, 21, 20, 19, 18)))
    y = panel(im, (x, 666, 1230, 868), L['p_pros'], LCD); bullets(im, (x + 26, y - 10), t['pros'], 590, bs, '+', LCD)
    y = panel(im, (1250, 666, 1860, 868), L['p_cons'], RED); bullets(im, (1276, y - 10), t['cons'], 570, bs, '−', RED)
    y = panel(im, (x, 882, 1860, 1030), L['p_cur'], GOLD); bullets(im, (x + 26, y - 12), t['cur'], 1200, 22, '★', GOLD)
    save(im, P['card'] % name.title())

# comparativa
im = background(); header(im, L['sec_chars'], L['cmp_title'], 11)
y = panel(im, (60, 200, 1860, 1040), L['cmp_panel'], LCD)
d = ImageDraw.Draw(im)
xs = [100, 560, 740, 920, 1090, 1270, 1450, 1640, 1820]
for j, c in enumerate(L['cmp_cols']): d.text((xs[j], y), c.upper(), font=F('Inter-Bold.otf', 24), fill=(150, 158, 178), anchor='la' if j == 0 else 'ra')
y += 54
for i, c in enumerate(CH):
    fn, andar, correr, salto, rec, pis, coh, met, plac = c
    if i % 2 == 0: d.rectangle([80, y - 14, 1840, y + 66], fill=(18, 22, 34))
    d.text((xs[0], y), L['ch'][fn]['name'], font=F(TITLE, 34), fill=(236, 240, 248))
    vals = [num(andar, '%.2f'), num(correr, '%.1f'), '%d' % salto, '%d %%' % rec, '%d°' % pis, '%d°' % coh, '%d°' % met, str(plac)]
    best = [andar == 2.38, correr == 8.0, salto == 75, rec == 100, pis == 4, coh == 0, met == 6, plac == 12]
    for j, v in enumerate(vals):
        d.text((xs[j + 1], y + 4), v, font=F('Inter-Bold.otf', 30), fill=LCD if best[j] else (226, 230, 240), anchor='ra')
    y += 88
paragraph(im, (100, y + 2), L['cmp_note'], 1700, 24, fill=(150, 158, 178))
save(im, P['cmp'])

# Mr. Acme (17-11-2026): foto apaisada de cuerpo entero junto a Mrs. Acme en su despacho (acmes.png: el despacho entero,
# sacado del juego en la mansion con el 1P escondido: OFX_MASK 0x2000 y sin el marcador) arriba, y el texto debajo
im = background(); header(im, L['sec_chars'], 'MR. ACME', 12)
pic = load('acmes.png')
if pic: place(im, pic, (70, 215, 1850, 700))
y = panel(im, (60, 730, 1860, 1030), L['acme_panel'], GOLD)
asz = fit_size(' '.join(L['acme']) + ' x x x x x x x x x x x x x x', 1740, 1030 - y - 44, (28, 27, 26, 25, 24, 23, 22), 1.32)
for k_, para in enumerate(L['acme']):
    y = paragraph(im, (90, y - 4 if k_ == 0 else y + 12), para, 1740, asz, spacing=1.32)
save(im, P['acme'])

# ---------------------------------------------------------------- armas
ARMF = ['pistola', 'metralleta', 'lanzacohetes', 'lanzallamas', 'granada', 'katana', 'latigo']
im = background(); header(im, L['sec_weap'], L['weap_title'], 0)
for k, (fn, (nm, mun, dmg, txt)) in enumerate(zip(ARMF, L['weap'])):
    col, row = k % 4, k // 4
    x0 = 60 + col * 455; y0 = 210 + row * 410; x1 = x0 + 430; y1 = y0 + 390
    panel(im, (x0, y0, x1, y1), None, LCD)
    raw = load('arma_raw_%s.png' % fn)                   # el recuadro del icono sacado del manual anterior (mismo sitio)
    ic = None if raw else load('arma_%s.png' % fn)
    if ic: ic.putalpha(ic.convert('L').point(lambda v: min(255, v * 4)))
    d = ImageDraw.Draw(im)
    if raw: im.paste(raw, (x0 + 30, y0 + 24))
    elif ic: place(im, ic, (x0 + 30, y0 + 24, x1 - 30, y0 + 150), frame=False, pixel=False)
    ns = 40
    while ns > 26 and d.textlength(nm, font=F(TITLE, ns)) > 380: ns -= 2
    d.text((x0 + 26, y0 + 162 + (40 - ns) // 2), nm, font=F(TITLE, ns), fill=RED)
    d.text((x0 + 26, y0 + 210), L['ammo_lbl'] + mun, font=F('Inter-SemiBold.otf', 23), fill=(200, 206, 222))
    d.text((x0 + 26, y0 + 238), L['dmg_lbl'] + dmg, font=F('Inter-SemiBold.otf', 23), fill=GOLD)
    sz = fit_size(txt, 380, y1 - (y0 + 272) - 10, (22, 21, 20, 19, 18, 17), 1.34)
    PB[0] = y1
    paragraph(im, (x0 + 26, y0 + 272), txt, 380, sz, fill=(206, 212, 226), spacing=1.34)
x0, y0 = 60 + 3 * 455, 210 + 410
panel(im, (x0, y0, x0 + 430, y0 + 390), L['use_panel'], GOLD)
bullets(im, (x0 + 26, y0 + 56), L['use'], 390, bullets_size(L['use'], 390, 320, (22, 21, 20, 19, 18)), '•', GOLD)
save(im, P['weap'])

im = background(); header(im, L['sec_weap'], L['obj_title'], 0)
y = panel(im, (60, 210, 930, 760), L['throw_panel'], GOLD)
y = paragraph(im, (90, y), L['throw'], 820, 26)
d = ImageDraw.Draw(im); y += 20
for k, (a_, b_) in enumerate(L['throw_rows']):
    if k % 2 == 0: d.rectangle([80, y - 8, 910, y + 42], fill=(18, 22, 34))
    d.text((100, y), a_, font=F('Inter-Regular.otf', 27), fill=(226, 230, 240)); d.text((890, y), b_, font=F('Inter-Bold.otf', 27), fill=GOLD, anchor='ra')
    y += 58
y = panel(im, (980, 210, 1860, 760), L['traps_panel'], RED)
bullets(im, (1006, y), L['traps'], 830, bullets_size(L['traps'], 830, 760 - y - 16, (26, 25, 24, 23, 22, 21)), '!', RED)
save(im, P['obj'])

# ---------------------------------------------------------------- escenarios
for i, (nm, who, txt) in enumerate(L['stages']):
    im = background(); header(im, L['sec_stages'], nm, 13 + i)
    pic = load('esc%d.ppm' % i)
    pano = load('mansion_panoramica.png') if i == 7 else None   # la mansion entera (tools/manual_mansion.py)
    if pano: place(im, pano, (70, 215, 1260, 1010), pixel=False)
    elif pic: place(im, pic, (70, 215, 1260, 1010))
    y = panel(im, (1300, 215, 1860, 1010), L['turf'] + who, GOLD)
    paragraph(im, (1328, y), txt, 510, fit_size(txt, 510, 700, (29, 28, 27, 26)))
    save(im, P['stage'] % nm.title())

# ---------------------------------------------------------------- finales
im = background(); header(im, L['sec_end'], L['end_title'], 0)
y = panel(im, (60, 210, 900, 1010), L['end_panel'], GOLD)
y = paragraph(im, (90, y + 4), L['end'], 790, fit_size(L['end'], 790, 250, (34, 32, 30)), font='Inter-SemiBold.otf', fill=(240, 242, 248))
d = ImageDraw.Draw(im); y += 24
for k, (t, dsc) in enumerate(L['end_rows']):
    d.rounded_rectangle([90, y, 870, y + 92], 10, fill=(14, 18, 28), outline=(60, 66, 86), width=2)
    d.text((112, y + 12), t, font=F(TITLE, 36), fill=GOLD)
    d.text((112, y + 56), dsc, font=F('Inter-Medium.otf', 24), fill=(170, 178, 198)); y += 108
paragraph(im, (90, y + 6), L['end_tag'], 790, 28, fill=LCD, font='Inter-BoldItalic.otf')
sal = load('saldo.ppm', True)
if sal: place(im, sal, (950, 215, 1860, 900))
d.text((1405, 950), L['end_cap'], font=F('Inter-MediumItalic.otf', 24), fill=(150, 158, 178), anchor='ma')
save(im, P['end'])
# Fotos de los finales, limpias (sin pies: quedaban peor), lo mas grandes que caben tres en fila
for i, c in enumerate(CH):
    fn = c[0]; name = L['ch'][fn]['name']
    im = background(); header(im, L['sec_end'], name, 0)
    for j in range(3):
        x0 = 60 + j * 610; b = (x0, 390, x0 + 580, 390 + 435)
        p_ = load('fin_%s_%d.png' % (fn, j + 1))
        if p_: place(im, p_, b)
        else:
            dd = ImageDraw.Draw(im); dd.rectangle(b, outline=(70, 76, 96), width=3)
            dd.text(((b[0] + b[2]) // 2, (b[1] + b[3]) // 2), L['pending'], font=F(TITLE, 40), fill=(70, 76, 96), anchor='mm')
    save(im, P['fin'] % name.title())

# ---------------------------------------------------------------- salida
# PDF y pack del visor (Outfoxies_Manual_<Idioma>.zip: en la app se elige con el selector y se copia como manual.zip)
pdf_imgs = [p.convert('RGB') for _, p in pages]
pdf_imgs[0].save(os.path.join(OUT, L['pdf']), save_all=True, append_images=pdf_imgs[1:], resolution=144, quality=90)
zname = 'Outfoxies_Manual_%s.zip' % manual_textos.SUFIJO[LANG]
with zipfile.ZipFile(os.path.join(OUT, zname), 'w', zipfile.ZIP_STORED) as z:
    for n, (name, p) in enumerate(pages, 1):
        b = io.BytesIO(); p.convert('RGB').save(b, 'JPEG', quality=88, optimize=True)
        z.writestr('%02d %s.jpg' % (n, name), b.getvalue())
if os.environ.get('MANUAL_PNG'):
    os.makedirs(os.path.join(OUT, LANG), exist_ok=True)
    for n, (name, p) in enumerate(pages, 1): p.save(os.path.join(OUT, LANG, 'p%02d.png' % n))
print(LANG, len(pages), 'paginas ->', zname)
