import sys, numpy as np
from PIL import Image
SRC, OUT = sys.argv[1], sys.argv[2]
def red_box(page, box, fixed=None):
    """foto colocada con place(fit) y centrada en box (la de make_manual.py): se busca su esquina de arriba a la
    izquierda (la linea roja del marco) y el tamano sale de que esta centrada"""
    im = Image.open(SRC + '/' + page).convert('RGB'); a = np.asarray(im).astype(int)
    x0, y0, x1, y1 = box
    r = a[y0 - 30:y1 + 30, x0 - 30:x1 + 30]
    red = (r[:, :, 0] > 150) & (r[:, :, 1] < 90) & (r[:, :, 2] < 90)
    c = red.sum(0); rw = red.sum(1)
    L = np.where(c > 0.5 * c.max())[0].min() + x0 - 30
    T = np.where(rw > 0.5 * rw.max())[0].min() + y0 - 30
    if fixed: L, T = fixed
    px, py = L + 1, T + 1                       # la linea roja ocupa px-1 y px
    w = (x0 + x1) - 2 * px; h = (y0 + y1) - 2 * py
    return im.crop((px + 1, py + 1, px + w - 1, py + h - 1))
names = ['john', 'eve', 'betty', 'bernard', 'ching', 'dweeb', 'gemelos']
files = ['04 Ficha John Smith', '05 Ficha Eve', '06 Ficha Betty Doe', '07 Ficha Bernard White', '08 Ficha Prof. Ching', '09 Ficha Dweeb', '10 Ficha Danny Y Demi']
for n, f in zip(names, files): red_box(f + '.jpg', (70, 220, 560, 1010)).save(OUT + '/sel_%s.png' % n)
esc = ['Rascacielos', 'Circo', 'Acuario', 'Tren', 'Fábrica', 'Avión', 'Barco']
for i, e in enumerate(esc): red_box('%02d Escenario %s.jpg' % (15 + i, e), (70, 215, 1260, 1010), (69, 264)).save(OUT + '/esc%d.png' % i)
red_box('02 Historia.jpg', (1080, 220, 1860, 600)).save(OUT + '/tv.png')
red_box('02 Historia.jpg', (1080, 650, 1860, 1010)).save(OUT + '/dossier.png')
red_box('23 Finales.jpg', (950, 215, 1860, 900)).save(OUT + '/saldo.png')
