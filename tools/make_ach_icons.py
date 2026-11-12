#!/usr/bin/env python3
"""Iconos de los logros: assets/logros/iconos.b64 -> src/app/ach_icons.h

iconos.b64 son los 21 iconos de RetroAchievements (juego 19839, en el orden de achievements.c; el 21 es
"High Roller", que no usamos) reducidos a 32x32 en el navegador del usuario, en RGB555 (2 bytes, alto primero),
seguidos y comprimidos con deflate (zlib), en base64. En el .h va lo comprimido; menu.c lo descomprime con miniz."""
import base64, zlib, sys, os
R = os.path.dirname(os.path.abspath(__file__)) + '/..'
z = base64.b64decode(open(R + '/assets/logros/iconos.b64').read().replace('\n', ''))
raw = zlib.decompress(z)
n = len(raw) // 2048
with open(R + '/src/app/ach_icons.h', 'w') as f:
    f.write('/* generado por tools/make_ach_icons.py: %d iconos 32x32 RGB555 (deflate) */\n' % n)
    f.write('#define ACH_ICON_N %d\n#define ACH_ICON_RAW %d\n' % (n, len(raw)))
    f.write('static const unsigned char ach_icons_z[%d] = {\n' % len(z))
    for i in range(0, len(z), 24):
        f.write(''.join('%d,' % b for b in z[i:i + 24]) + '\n')
    f.write('};\n')
print(n, 'iconos,', len(z), 'bytes')
