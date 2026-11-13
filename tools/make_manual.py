#!/usr/bin/env python3
"""Manual de The Outfoxies Remastered: paginas de 1920x1080 (la forma de las consolas) dibujadas con PIL.
Salida: manual.pdf (una pagina por imagen) y manual.zip (el pack del boton MANUAL del launcher: "01 Portada.jpg"...).
Uso: make_manual.py <carpeta_imagenes> <carpeta_salida>
La carpeta de imagenes lleva capturas sacadas del juego (no van en el repositorio ni en el APK):
  sel_<personaje>.png (eleccion de personaje), esc0..esc7.ppm (escenarios, 16:9), tv.ppm y dossier.ppm (encargos de
  Mr. Acme), y opcionalmente fin_<personaje>.png / fin_<personaje>_2.png (fotos de los finales; si faltan, hueco).
Textos: biografias del juego (traduccion de tools/make_lang.py) y datos del documento "Personajes" (tablas de la ROM)."""
import os, sys, io, zipfile
from PIL import Image, ImageDraw, ImageFont, ImageFilter, ImageChops

R = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
SRC, OUT = sys.argv[1], sys.argv[2]
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
    im.paste(p, (px, py))
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
    if num: d.text((W - 40, H - 40), str(num), font=F('Inter-Bold.otf', 26), fill=(110, 118, 140), anchor='rs')

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

def paragraph(im, xy, text, width, size=30, fill=(222, 226, 236), spacing=1.42, font='Inter-Regular.otf', bold_lead=None):
    d = ImageDraw.Draw(im); f = F(font, size); x, y = xy
    for ln in wrap(d, text, f, width):
        d.text((x, y), ln, font=f, fill=fill); y += int(size * spacing)
    return y

def panel(im, box, title=None, accent=RED):
    x0, y0, x1, y1 = box
    ov = Image.new('RGBA', (W, H), (0, 0, 0, 0)); o = ImageDraw.Draw(ov)
    o.rounded_rectangle(box, 14, fill=(6, 8, 14, 205), outline=(90, 98, 120, 255), width=2)
    im.paste(ov, (0, 0), ov)
    d = ImageDraw.Draw(im)
    d.rectangle([x0 + 14, y0, x1 - 14, y0 + 5], fill=accent)
    if title: d.text((x0 + 26, y0 + 18), title.upper(), font=F('Inter-ExtraBoldItalic.otf', 26), fill=accent)
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
    return y

def load(name, crop43=False):
    p = os.path.join(SRC, name)
    if not os.path.exists(p): return None
    im = Image.open(p).convert('RGB')
    if crop43 and im.width == 384: im = im.crop((48, 0, 336, 224))
    return im

pages = []
def save(im, name): pages.append((name, im))

# ---------------------------------------------------------------- portada
im = Image.open(os.path.join(R, 'assets/art/portada.jpg')).convert('RGB')
k = max(W / im.width, H / im.height); im = im.resize((round(im.width * k), round(im.height * k)), Image.LANCZOS)
im = im.crop(((im.width - W) // 2, 0, (im.width - W) // 2 + W, H))
ov = Image.new('RGBA', (W, H), (0, 0, 0, 0)); o = ImageDraw.Draw(ov)
for y in range(H - 300, H): o.line([(0, y), (W, y)], fill=(4, 5, 10, int(235 * (y - H + 300) / 300)))
im.paste(ov, (0, 0), ov)
d = ImageDraw.Draw(im)
d.polygon([(0, H - 196), (W, H - 236), (W, H - 228), (0, H - 188)], fill=RED)
title_text(im, (W // 2, H - 172), 'MANUAL DEL JUEGO', 84, anchor='ma', fill_bot=GOLD)
d.text((W // 2, H - 26), 'Historia  ·  Personajes  ·  Escenarios  ·  Finales', font=F('Inter-SemiBoldItalic.otf', 30), fill=(200, 206, 222), anchor='ms')
save(im, 'Portada')

# ---------------------------------------------------------------- historia
im = background(); header(im, 'Historia principal', 'SIETE ASESINOS, UN SOLO SUPERVIVIENTE', 2)
y = panel(im, (60, 210, 1000, 1010), 'El encargo de Mr. Acme')
y = paragraph(im, (90, y), 'Un hombre misterioso llamado Mr. Acme contrató a siete asesinos para acabar con siete famosos '
              'coleccionistas de arte del mundo. Curiosamente, a cada asesino también le encargó eliminar a los otros seis...', 860, 32)
y = paragraph(im, (90, y + 14), '¿Cuál es el móvil de Mr. Acme? ¿Qué le espera al superviviente? ¿Una gran recompensa y la fama '
              'de ser el mejor asesino del mundo?', 860, 32, fill=GOLD, font='Inter-SemiBoldItalic.otf')
y = paragraph(im, (90, y + 22), 'Eliges a uno de los siete. Antes de cada combate, Mr. Acme te da las instrucciones por la tele: '
              'quién es tu próximo objetivo, cuánto te paga y dónde encontrarlo. Cada combate se libra en el terreno del objetivo, '
              'y la única regla es la del cartel que lo abre: ¡acaba con tu enemigo como sea!', 860, 28)
y = paragraph(im, (90, y + 14), 'Cuando hayas eliminado a los otros seis, queda el último encargo: llegar hasta la mansión del '
              'propio Mr. Acme.', 860, 28)
tv = load('tv.ppm', True); dos = load('dossier.ppm', True)
if tv: place(im, tv, (1080, 220, 1860, 600))
if dos: place(im, dos, (1080, 650, 1860, 1010))
save(im, 'Historia')

im = background(); header(im, 'Historia principal', 'CÓMO SE JUEGA', 3)
y = panel(im, (60, 210, 930, 1010), 'Reglas del combate')
y = bullets(im, (90, y), [
    'Todos empiezan con la barra de energía llena. Nadie tiene más vida que otro: lo que cambia es cuánto le quita cada golpe.',
    '200 segundos por combate. Si se acaba el tiempo, pierde el jugador.',
    'Las armas van apareciendo por el escenario: pistola, metralleta, lanzacohetes, granada, katana, látigo y lanzallamas. '
    'También se puede lanzar lo que haya a mano: sopa, tarta, cesta de fruta o carbón.',
    'Tras recibir un golpe, el luchador parpadea un segundo y no le pueden volver a dar.',
    'Con dos toques seguidos hacia un lado se corre; corriendo se salta más lejos, se desliza agachándose y se hace un placaje.',
    'El escenario también ataca: caer al vacío quita energía y hay trampas propias de cada sitio.'], 820, 27)
y = panel(im, (980, 210, 1860, 1010), 'Daño de cada ataque', GOLD)
rows = [('Cohete / explosión de granada', '35', '16 %'), ('Caer al vacío', '25', '11 %'), ('Pistola (por bala)', '20', '9 %'),
        ('Katana (por golpe)', '15', '7 %'), ('Lanzallamas (por toque)', '10', '5 %'), ('Sopa', '10', '5 %'),
        ('Látigo (por golpe)', '5', '2 %'), ('Metralleta (por bala)', '2', '1 %'), ('Puñetazo (Bernard: 3)', '1', '0,5 %'),
        ('Tarta', '0', '0 %')]
d = ImageDraw.Draw(im); fh = F('Inter-Bold.otf', 24); fr_ = F('Inter-Regular.otf', 27)
d.text((1010, y), 'ATAQUE', font=fh, fill=(150, 158, 178)); d.text((1560, y), 'BASE', font=fh, fill=(150, 158, 178), anchor='ra')
d.text((1820, y), 'BARRA DE JOHN', font=fh, fill=(150, 158, 178), anchor='ra'); y += 46
for i, (a, b, c) in enumerate(rows):
    if i % 2 == 0: d.rectangle([1000, y - 8, 1840, y + 40], fill=(18, 22, 34))
    d.text((1010, y), a, font=fr_, fill=(226, 230, 240)); d.text((1560, y), b, font=F('Inter-Bold.otf', 27), fill=GOLD, anchor='ra')
    d.text((1820, y), c, font=fr_, fill=LCD, anchor='ra'); y += 56
paragraph(im, (1010, y + 10), 'Daño base de la ROM. El porcentaje es lo que le quita a John Smith en Normal; a los demás, según '
          'lo que aguante cada uno (ver las fichas).', 800, 22, fill=(150, 158, 178))
save(im, 'Como se juega')

# ---------------------------------------------------------------- fichas
CH = [  # nombre, archivo, bio, escenario, andar, correr, t. correr, salto, recibe %, pistola, cohetes, metralleta, placaje, pros, contras, curiosidades
    ('JOHN SMITH', 'john', 'El hombre capaz de hacer lo que sea, de cuidar niños a incitar una revolución, con tal de ganar dinero.',
     'Rascacielos', 2.19, 4.0, '0,4 s', 59, 109, 10, 20, 14, 5,
     ['El más equilibrado: ni débil ni lento en nada.', 'El segundo que menos daño recibe.', 'Buena puntería con todas las armas.'],
     ['No destaca en nada concreto.', 'Corriendo es de los normales (4,0).'],
     ['El cursor de la elección de personaje empieza sobre él.', 'En su rascacielos, el helicóptero de la azotea acaba '
      'explotando y todos caen al comedor.']),
    ('EVE', 'eve', 'Ex estrella de cine, ahora ladrona para pagarse su vida de lujo. Es capaz de entrar en la fortaleza de Nox '
     'con la ayuda de su lagarto bien entrenado.', 'Circo', 2.38, 5.0, '0,5 s', 75, 113, 14, 24, 16, 4,
     ['La más rápida andando de todo el juego.', 'Salta más alto que nadie: llega a plataformas que otros no alcanzan.',
      'Aguanta bien para ser tan ágil.'],
     ['Puntería algo peor que John.', 'Su placaje es de los flojos (4).'],
     ['Siempre va con su lagarto.', 'En su circo, abajo aparecen tres pistolas de juguete.']),
    ('BETTY DOE', 'betty', 'Una exitosa empresaria internacional que necesita capital con urgencia para financiar sus muchos negocios. '
     'Su variado pasado incluye haber sido una sicaria muy bien pagada.', 'Acuario', 2.13, 3.0, 'al instante', 54, 117, 4, 4, 6, 4,
     ['La que dispara más recto con cualquier arma (pistola 4°).', 'Llega a su velocidad de carrera al instante.'],
     ['Corriendo es la más lenta.', 'Salta poco y recibe un 17 % más que Bernard.'],
     ['Parece la más rápida por cómo arranca a correr, pero andando es la cuarta.', 'Agachada con pistola sigue siendo precisa (7°).']),
    ('BERNARD WHITE', 'bernard', 'Ingeniero biotecnológico con fuerza sobrehumana. Un experimento le hizo perder una mano: '
     'ahora tiene una mano artificial de acero.', 'Tren', 2.25, 6.0, '1,2 s', 55, 100, 5, 15, 10, 12,
     ['El que menos daño recibe de todos.', 'Su puñetazo pega el triple que el de los demás.', 'El placaje más fuerte (12).',
      'El segundo más rápido andando.'],
     ['Salta poco (55).', 'Es el que más tarda en arrancar a correr.', 'Su pistola se desvía hacia un lado.'],
     ['La fuerza de su puño sale de la mano de acero.', 'Su escenario es un tren en marcha: quien cae vuelve en una vagoneta.']),
    ('PROF. CHING', 'ching', 'El misterioso científico oriental que va en una silla de ruedas asesina diseñada por él. '
     'Aunque tiene una discapacidad, es un famoso maestro de artes marciales chinas.', 'Fábrica', 1.75, 8.0, '1,1 s', 69, 130, 24, 0, 24, 6,
     ['Corriendo es el más rápido con diferencia (8,0).', 'Con el lanzacohetes dispara perfectamente recto: el único.',
      'Salta bastante (69).'],
     ['El que más despacio anda.', 'Recibe un 30 % más que Bernard.', 'Mala puntería con pistola y metralleta.'],
     ['De espaldas, el respaldo de la silla le hace a prueba de balas.', 'La silla tiene brazos mecánicos, pilotos rojos y '
      'carrocería propia en su paleta de colores.']),
    ('DWEEB', 'dweeb', 'El único asesino profesional conocido que cobra en plátanos. Es el hijo secuestrado del chimpancé más listo '
     'del mundo, estrella de la tele: "Mr. Happy".', 'Avión', 2.00, 4.5, '0,4 s', 72, 120, 30, 37, 30, 5,
     ['El más pequeño (2 losetas de alto): el más difícil de acertar.', 'Salta mucho (72).', 'Arranca a correr enseguida.'],
     ['El que peor apunta de todo el juego.', 'Recibe un 20 % más que Bernard.', 'Anda despacio.'],
     ['Sus cohetes salen desviados hacia un lado.', 'En su avión, si alguien cae al vacío el juego lo devuelve con menos energía.']),
    ('DANNY Y DEMI', 'gemelos', 'Los ex siameses, separados en un accidente de tren. Además de ser muy inteligentes, '
     'los dos tienen una mente siniestra y criminal.', 'Barco', 2.06, 3.5, '0,3 s', 53, 133, 20, 36, 18, 6,
     ['Pueden llevar dos armas: guardan una de reserva y la sacan solos al gastar la de la mano.',
      'Los que antes arrancan a correr (0,3 s).', 'Placaje fuerte (6).'],
     ['Los más frágiles: reciben un 33 % más que Bernard.', 'Los que menos saltan (53).', 'Mala puntería con cohetes.'],
     ['Con la opción "Dos armas" del port llevan tres (las dos suyas y la de reserva).', 'Danny y Demi tienen cada uno sus colores '
      'de pelo, piel y ropa.']),
]
MIN = {'andar': 1.75, 'correr': 3.0, 'salto': 53, 'aguante': 133, 'punt': 30}
MAX = {'andar': 2.38, 'correr': 8.0, 'salto': 75, 'aguante': 100, 'punt': 4}
def norm(k, v): return 0.15 + 0.85 * (v - MIN[k]) / (MAX[k] - MIN[k])
for i, c in enumerate(CH):
    name, fn, bio, esc, andar, correr, tcor, salto, rec, pis, coh, met, plac, pros, cons, cur = c
    im = background(); header(im, 'Fichas de los personajes', name, 4 + i)
    pic = load('sel_%s.png' % fn)
    if pic: place(im, pic, (70, 220, 560, 1010))
    x = 620
    y = panel(im, (x, 200, 1860, 380), 'Quién es')
    paragraph(im, (x + 30, y - 4), bio, 1180, 29, font='Inter-MediumItalic.otf', fill=(232, 236, 244))
    ImageDraw.Draw(im).text((1836, 218), 'Escenario: ' + esc.upper(), font=F('Inter-Bold.otf', 24), fill=GOLD, anchor='ra')
    y = panel(im, (x, 384, 1290, 650), 'Estadísticas', LCD)
    lcd_bar(im, x + 30, y, 'Andar', norm('andar', andar), note=('%.2f' % andar).replace('.', ','))
    lcd_bar(im, x + 30, y + 40, 'Correr', norm('correr', correr), note=('%.1f' % correr).replace('.', ',') + ' · ' + tcor)
    lcd_bar(im, x + 30, y + 80, 'Salto', norm('salto', salto), note='%d px' % salto)
    lcd_bar(im, x + 30, y + 120, 'Aguante', norm('aguante', rec), note='recibe %d %%' % rec)
    lcd_bar(im, x + 30, y + 160, 'Puntería', norm('punt', pis), note='pistola %d°' % pis)
    y = panel(im, (1310, 384, 1860, 650), 'Armas y golpes', GOLD)
    d = ImageDraw.Draw(im); f = F('Inter-Regular.otf', 26); fb = F('Inter-Bold.otf', 26)
    for j, (a, b) in enumerate([('Abanico con pistola', '%d°' % pis), ('Con lanzacohetes', '%d°' % coh),
                                ('Con metralleta', '%d°' % met), ('Placaje a la carrera', str(plac)),
                                ('Puñetazo', '3' if fn == 'bernard' else '1')]):
        d.text((1338, y + j * 40), a, font=f, fill=(222, 226, 236)); d.text((1832, y + j * 40), b, font=fb, fill=GOLD, anchor='ra')
    y = panel(im, (x, 666, 1230, 868), 'Pros', LCD); bullets(im, (x + 26, y - 10), pros, 590, 22, '+', LCD)
    y = panel(im, (1250, 666, 1860, 868), 'Contras', RED); bullets(im, (1276, y - 10), cons, 570, 22, '−', RED)
    y = panel(im, (x, 882, 1860, 1030), 'Curiosidades', GOLD); bullets(im, (x + 26, y - 12), cur, 1200, 22, '★', GOLD)
    save(im, 'Ficha %s' % name.title())

# comparativa
im = background(); header(im, 'Fichas de los personajes', 'COMPARATIVA', 11)
y = panel(im, (60, 200, 1860, 1040), 'Los siete, frente a frente', LCD)
d = ImageDraw.Draw(im)
cols = ['Personaje', 'Andar', 'Correr', 'Salto', 'Recibe', 'Pistola', 'Cohetes', 'Metralleta', 'Placaje']
xs = [100, 560, 740, 920, 1090, 1270, 1450, 1640, 1820]
for j, c in enumerate(cols): d.text((xs[j], y), c.upper(), font=F('Inter-Bold.otf', 24), fill=(150, 158, 178), anchor='la' if j == 0 else 'ra')
y += 54
for i, c in enumerate(CH):
    name, fn, bio, esc, andar, correr, tcor, salto, rec, pis, coh, met, plac = c[:13]
    if i % 2 == 0: d.rectangle([80, y - 14, 1840, y + 66], fill=(18, 22, 34))
    d.text((xs[0], y), name, font=F(TITLE, 34), fill=(236, 240, 248))
    vals = [('%.2f' % andar).replace('.', ','), ('%.1f' % correr).replace('.', ','), '%d' % salto, '%d %%' % rec, '%d°' % pis, '%d°' % coh, '%d°' % met, str(plac)]
    best = [andar == 2.38, correr == 8.0, salto == 75, rec == 100, pis == 4, coh == 0, met == 6, plac == 12]
    for j, v in enumerate(vals):
        d.text((xs[j + 1], y + 4), v, font=F('Inter-Bold.otf', 30), fill=LCD if best[j] else (226, 230, 240), anchor='ra')
    y += 88
paragraph(im, (100, y + 2), 'En verde, el mejor de cada columna. Andar y correr en píxeles por fotograma; salto, altura en píxeles; '
          'recibe, daño comparado con Bernard (100 %); armas, abanico de desvío de cada disparo (menos = más recto).', 1700, 24, fill=(150, 158, 178))
save(im, 'Comparativa')

# Mr. Acme
im = background(); header(im, 'Fichas de los personajes', 'MR. ACME', 12)
y = panel(im, (60, 210, 960, 1010), 'El que lo empezó todo', GOLD)
y = paragraph(im, (90, y), 'El misterioso cliente que contrató a los siete asesinos y que, por lo que se ve, quería que solo '
              'quedara uno. Habla siempre a través de la tele, con las instrucciones de cada encargo.', 840, 29)
y = paragraph(im, (90, y + 16), 'Al final espera en su mansión junto a Mrs. Acme. Fuera, ataca desde su helicóptero; para '
              'derribarlo hay un lanzamisiles especial (SPECIAL ROCKET) con munición ILIMITADA.', 840, 29)
bullets(im, (90, y + 30), ['Su mansión está hecha para pelear contra la máquina: por eso no sale en el combate entre dos jugadores.',
                           'Mr. y Mrs. Acme usan los colores del escenario, así que la ropa del rival no les afecta.'], 840, 25, '★', GOLD)
e7 = load('esc7.ppm')
if e7: place(im, e7, (1010, 230, 1860, 1000))
save(im, 'Mr. Acme')

# ---------------------------------------------------------------- escenarios
ESC = [
    ('RASCACIELOS', 'John Smith', 'Un edificio de oficinas de arriba abajo. El combate empieza en la azotea, junto a un '
     'helicóptero; hacia la mitad del tiempo el helicóptero explota y todos caen al comedor de la planta de abajo. En el '
     'sótano, a la izquierda del garaje, hay un foso con siete granadas en fila.'),
    ('CIRCO', 'Eve', 'La carpa de un circo, con trapecios y cuerdas por encima de la pista y el público alrededor. Los '
     'trapecios sirven para cruzar de lado a lado. Ya avanzado el combate, abajo aparecen tres pistolas de juguete.'),
    ('ACUARIO', 'Betty Doe', 'Un acuario con una ballena en su tanque, peces y varias plantas unidas por escaleras. Con el '
     'tiempo el agua va subiendo, y todo lo que queda por debajo de su nivel se ve verde.'),
    ('TREN', 'Bernard White', 'Se pelea encima de un tren de vapor en marcha, entre montañas: vagones, ténder con carbón '
     'para lanzar y la locomotora. Quien cae del tren por delante o por detrás vuelve a subir en una vagoneta.'),
    ('FÁBRICA', 'Prof. Ching', 'La fábrica del profesor: un laberinto de pasarelas, escaleras, cintas y ascensores que suben '
     'cohetes. Es de los escenarios más grandes, y la cámara se aleja mucho para enseñarlo entero.'),
    ('AVIÓN', 'Dweeb', 'Un avión en pleno vuelo, por fuera y por dentro, con un helicóptero alrededor. Tiene metralletas '
     'montadas y la cabina del piloto. Si alguien cae al vacío, vuelve con menos energía.'),
    ('BARCO', 'Danny y Demi', 'Un yate en alta mar con varias cubiertas, del puente a las bodegas. Por los lados está el mar: '
     'quien cae al agua acaba volviendo a bordo.'),
    ('MANSIÓN', 'Mr. Acme', 'El último encargo: la mansión de Mr. Acme, por dentro y por fuera. Dentro hay vitrinas, ascensor '
     'y extintores (su espuma es un arma más); fuera espera Mr. Acme con su helicóptero.'),
]
for i, (nm, who, txt) in enumerate(ESC):
    im = background(); header(im, 'Escenarios', nm, 13 + i)
    pic = load('esc%d.ppm' % i)
    if pic: place(im, pic, (70, 215, 1260, 1010))
    y = panel(im, (1300, 215, 1860, 1010), 'Terreno de ' + who, GOLD)
    paragraph(im, (1328, y), txt, 510, 29)
    save(im, 'Escenario %s' % nm.title())

# ---------------------------------------------------------------- finales
im = background(); header(im, 'Finales', 'EL SUPERVIVIENTE', 21)
y = panel(im, (60, 210, 1860, 1010), 'Al vencer a Mr. Acme', GOLD)
y = paragraph(im, (90, y), 'Si acabas con Mr. Acme, el juego cierra la historia de tu asesino: el cartel de quién ha sobrevivido '
              '("JOHN SMITH SOBREVIVE.") y las cuentas del trabajo: la recompensa, la factura del hospital, el beneficio y el saldo '
              'bancario. Después, la lista de LOS 5 MEJORES OUTFOXIES.', 1740, 30)
y = paragraph(im, (90, y + 20), 'Cada personaje tiene su propio final. En las páginas siguientes, un resumen de cada uno con las '
              'imágenes de su final.', 1740, 30, fill=GOLD, font='Inter-SemiBoldItalic.otf')
save(im, 'Finales')
for i, c in enumerate(CH):
    name, fn = c[0], c[1]
    im = background(); header(im, 'Finales', name, 22 + i)
    pics = [p for p in (load('fin_%s.png' % fn), load('fin_%s_2.png' % fn)) if p]
    boxes = [(70, 215, 930, 760), (990, 215, 1850, 760)]
    for j, b in enumerate(boxes):
        if j < len(pics): place(im, pics[j], b)
        else:
            dd = ImageDraw.Draw(im); dd.rectangle(b, outline=(70, 76, 96), width=3)
            for k in range(0, 40, 10): dd.rectangle([b[0] + k, b[1] + k, b[2] - k, b[3] - k], outline=(26, 30, 44))
            dd.text(((b[0] + b[2]) // 2, (b[1] + b[3]) // 2), 'FOTO PENDIENTE', font=F(TITLE, 44), fill=(70, 76, 96), anchor='mm')
    y = panel(im, (70, 800, 1850, 1010), 'Su final', GOLD)
    paragraph(im, (100, y), FIN.get(fn, 'Resumen pendiente: a falta de las fotos del final de %s.' % name.title())
              if 'FIN' in globals() else 'Resumen pendiente: a falta de las fotos del final.', 1720, 28)
    save(im, 'Final %s' % name.title())

# ---------------------------------------------------------------- salida
pdf_imgs = [p.convert('RGB') for _, p in pages]
pdf_imgs[0].save(os.path.join(OUT, 'The Outfoxies Remastered - Manual.pdf'), save_all=True, append_images=pdf_imgs[1:], resolution=144, quality=90)
with zipfile.ZipFile(os.path.join(OUT, 'manual.zip'), 'w', zipfile.ZIP_STORED) as z:
    for n, (name, p) in enumerate(pages, 1):
        b = io.BytesIO(); p.convert('RGB').save(b, 'JPEG', quality=88, optimize=True)
        z.writestr('%02d %s.jpg' % (n, name), b.getvalue())
for n, (name, p) in enumerate(pages, 1): p.save(os.path.join(OUT, 'p%02d.png' % n))
print(len(pages), 'paginas')
