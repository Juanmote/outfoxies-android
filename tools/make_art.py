#!/usr/bin/env python3
"""Genera src/app/art_gfx.h con la portada (assets/art/portada.jpg, imagen del usuario) para el fondo del menu y el
relleno de las bandas. Va reducida a 800 px de ancho y a 256 colores (paleta + indices comprimidos con zlib); se
descomprime con miniz al arrancar."""
import zlib, os
from PIL import Image
root = os.path.join(os.path.dirname(__file__), '..')
im = Image.open(os.path.join(root, 'assets/art/portada.jpg')).convert('RGB')
im = im.resize((800, round(800 * im.height / im.width)), Image.LANCZOS)
q = im.quantize(256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG)
pal = bytes(q.getpalette()[:768]).ljust(768, b'\0')
z = zlib.compress(pal + q.tobytes(), 9)
out = ['/* Generado por tools/make_art.py: no editar */', '#ifndef OFX_ART_GFX_H', '#define OFX_ART_GFX_H',
       '#define ART_COVER_W %d' % im.width, '#define ART_COVER_H %d' % im.height,
       '/* 768 bytes de paleta RGB y luego un indice por pixel */', 'static const unsigned char art_cover[%d] = {' % len(z)]
for i in range(0, len(z), 32): out.append('  ' + ','.join(str(b) for b in z[i:i + 32]) + ',')
out += ['};', '#endif']
open(os.path.join(root, 'src/app/art_gfx.h'), 'w').write('\n'.join(out) + '\n')
print('portada', im.size, len(z), 'bytes')
