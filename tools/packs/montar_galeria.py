#!/usr/bin/env python3
"""Pack opcional de la GALERIA (galeria.zip) a partir de las imagenes del usuario, en uno de sus tres idiomas.
El nombre de cada imagen dentro del zip es el pie que enseña la app (el numero del principio solo marca el orden).
uso: montar_galeria.py <es|en|jp> <carpeta_imagenes> <salida.zip>
Los pies del pack japones van en castellano: la letra del menu no tiene caracteres japoneses."""
import os, sys, tempfile, zipfile
from PIL import Image

lang, src, out = sys.argv[1:4]
TAG = {'es': 'Castellano', 'en': 'English', 'jp': 'Japan'}[lang]
CAP = {  # archivo -> pie (es, en, jp)
    'The Outfoxies - Advertisement Flyer - Front (%s).jpg' % TAG: ('Folleto - Portada', 'Flyer - Front', 'Folleto japonés - Portada'),
    'The Outfoxies - Advertisement Flyer - Back (%s).jpg' % TAG: ('Folleto - Reverso', 'Flyer - Back', 'Folleto japonés - Reverso'),
    'The Outfoxies - Arcade - Marquee (%s).jpg' % TAG: ('Cómo jugar (panel de la recreativa)', 'How to play (cabinet card)', 'Cómo jugar (panel japonés)'),
    'The Outfoxies - Arcade - Marquee.jpg': ('Marquesina de la recreativa', 'Arcade marquee', 'Marquesina de la recreativa'),
    'The Outfoxies - Box - Front.png': ('Caja - Portada', 'Box - Front', 'Caja - Portada'),
    'The Outfoxies - Box.png': ('Caja - Personajes', 'Box - Characters', 'Caja - Personajes'),
    'The Outfoxies - Clear Logo.png': ('Logotipo', 'Logo', 'Logotipo'),
}
li = {'es': 0, 'en': 1, 'jp': 2}[lang]
MAX_SIDE = 2400
with zipfile.ZipFile(out, 'w', zipfile.ZIP_STORED) as z, tempfile.TemporaryDirectory() as t:
    for n, (f, caps) in enumerate(CAP.items(), 1):
        im = Image.open(os.path.join(src, f))
        alpha = im.mode in ('RGBA', 'LA', 'P') and 'Logo' in f
        if max(im.size) > MAX_SIDE:
            k = MAX_SIDE / max(im.size)
            im = im.resize((round(im.width * k), round(im.height * k)), Image.LANCZOS)
        name = '%02d %s.%s' % (n, caps[li], 'png' if alpha else 'jpg')
        p = os.path.join(t, 'x')
        if alpha: im.convert('RGBA').save(p, 'PNG', optimize=True)
        else: im.convert('RGB').save(p, 'JPEG', quality=90, optimize=True)
        z.write(p, name)
        print(name, im.size, os.path.getsize(p) // 1024, 'KB')
print(out, os.path.getsize(out) // 1024, 'KB')
