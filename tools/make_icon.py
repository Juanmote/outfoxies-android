#!/usr/bin/env python3
"""Icono de la app: el logo del titulo con el letrero REMASTERED, sobre negro.
Uso: tools/make_icon.py captura_titulo.ppm  (una captura del titulo con el letrero, p. ej. de build/headless).
Escribe ic_launcher.png (legado, el logo ocupa el 90 %) e ic_launcher_foreground.png (adaptativo: dentro de la zona
segura, el 64 %) en cada densidad."""
import sys, os
from PIL import Image
root = os.path.join(os.path.dirname(__file__), '..')
src = Image.open(sys.argv[1]).convert('RGB')
src.paste((0, 0, 0), (262, 127, 284, 138))   # sin la TM
logo = src.crop((12, 24, 274, 168))
big = logo.resize((logo.width * 4, logo.height * 4), Image.NEAREST)   # pixeles nitidos antes de reducir
dens = {'mdpi': 48, 'hdpi': 72, 'xhdpi': 96, 'xxhdpi': 144, 'xxxhdpi': 192}
for d, s in dens.items():
    for name, size, frac in (('ic_launcher', s, 0.90), ('ic_launcher_foreground', s * 108 // 48, 0.64)):
        w = int(size * frac); h = int(w * big.height / big.width)
        im = Image.new('RGB', (size, size), (0, 0, 0))
        im.paste(big.resize((w, h), Image.LANCZOS), ((size - w) // 2, (size - h) // 2))
        im.save(os.path.join(root, 'android/app/src/main/res/mipmap-%s/%s.png' % (d, name)))
print('iconos hechos')
