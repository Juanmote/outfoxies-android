#!/usr/bin/env python3
"""Genera src/app/touch_gfx.h con los graficos de los controles tactiles (assets/touch/*.png).
Cada imagen va en RGBA comprimida con zlib (se descomprime con miniz al arrancar).
Los PNG salen de la imagen de los controles que paso el usuario (cruceta y cuatro botones sobre negro):
el fondo negro conectado con el borde pasa a transparente. El boton MENU es el de PAUSE con el texto cambiado."""
import zlib, os
from PIL import Image
root = os.path.join(os.path.dirname(__file__), '..')
names = ['cruceta', 'disparo', 'salto', 'start', 'menu']
out = ['/* Generado por tools/make_touch.py: no editar */', '#ifndef OFX_TOUCH_GFX_H', '#define OFX_TOUCH_GFX_H']
for n in names:
    im = Image.open(os.path.join(root, 'assets/touch', n + '.png')).convert('RGBA')
    z = zlib.compress(im.tobytes(), 9)
    out.append('static const unsigned char tg_%s[%d] = {' % (n, len(z)))
    for i in range(0, len(z), 24):
        out.append('  ' + ','.join(str(b) for b in z[i:i + 24]) + ',')
    out.append('};')
    out.append('#define TG_%s_W %d\n#define TG_%s_H %d' % (n.upper(), im.width, n.upper(), im.height))
out.append('#endif')
open(os.path.join(root, 'src/app/touch_gfx.h'), 'w').write('\n'.join(out) + '\n')
