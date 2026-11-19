"""Genera src/app/font.h: fuente de mapa de bits (alfa de 8 bits) a partir de DejaVu Sans Bold.
DejaVu Sans es libre (licencia Bitstream Vera / DejaVu). Uso: python3 tools/make_font.py <ttf> <salida>"""
import sys
from PIL import Image, ImageDraw, ImageFont

ttf, out = sys.argv[1], sys.argv[2]
allout = []


def make(prefix, size, H, chars):
  font = ImageFont.truetype(ttf, size)
  asc, desc = font.getmetrics()
  glyphs = []
  for ch in chars:
    w = max(1, int(round(font.getlength(ch))))
    im = Image.new("L", (w + 2, H), 0)
    ImageDraw.Draw(im).text((1, H - H // 5 - asc), ch, font=font, fill=255)
    glyphs.append((ch, im))
  P = prefix.upper()
  lines = ["#define %s_H %d" % (P, H), "#define %s_N %d" % (P, len(glyphs))]
  lines.append("static const unsigned short %s_cp[%s_N] = {%s};" % (prefix, P, ",".join(str(ord(g[0])) for g in glyphs)))
  lines.append("static const unsigned char %s_w[%s_N] = {%s};" % (prefix, P, ",".join(str(g[1].size[0]) for g in glyphs)))
  offs, data, o = [], [], 0
  for ch, im in glyphs:
    offs.append(o)
    px = list(im.tobytes())
    data += px
    o += len(px)
  lines.append("static const unsigned int %s_off[%s_N] = {%s};" % (prefix, P, ",".join(map(str, offs))))
  rows = [",".join(str(v) for v in data[i:i + 32]) for i in range(0, len(data), 32)]
  lines.append("static const unsigned char %s_px[%d] = {\n%s};" % (prefix, len(data), ",\n".join(rows)))
  allout.extend(lines)
  print(prefix, len(glyphs), "glifos,", len(data), "bytes")


text = [chr(c) for c in range(32, 127)] + list("áéíóúÁÉÍÓÚñÑüÜ¿¡·«»←→↑↓‹›") + list("àâãçèêìòôõùÀÂÃÇÈÊÌÒÔÕÙ") + list("©")   # portugues e italiano al final; © para los creditos
make("font", 13, 16, text)
make("big", 30, 36, [chr(c) for c in range(32, 91)] + list("ÁÉÍÓÚÑ") + list("ÀÂÃÇÈÊÌÒÔÕÙ"))   # mayusculas con tilde al final (no cambian los indices)
open(out, "w", encoding="utf-8").write("/* Generado por tools/make_font.py desde DejaVu Sans Bold (licencia libre DejaVu/Bitstream Vera). */\n" + "\n".join(allout) + "\n")
