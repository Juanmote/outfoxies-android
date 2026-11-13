import base64,zlib,sys
from PIL import Image, ImageDraw
P='/home/claude/outfoxies-android/assets/logros/iconos.b64'
raw=bytearray(zlib.decompress(base64.b64decode(open(P).read().replace('\n',''))))[:21*2048]
def px(i,x,y): o=i*2048+(y*32+x)*2; return raw[o]<<8|raw[o+1]
ics=[12,13,14,15,20]
mask=[[len({px(i,x,y)>>1&0x3def for i in ics})>2 for x in range(32)] for y in range(32)]
frame=raw[20*2048:21*2048]
def canvas(bg): im=Image.new('RGB',(32,32),bg); return im, ImageDraw.Draw(im)
# 1 tren: cielo, montañas, locomotora, Bernard cayendo
a,d=canvas((110,160,220))
d.polygon([(2,20),(9,12),(15,19),(21,11),(27,20)],fill=(200,210,225))
d.rectangle((2,24,27,29),fill=(90,70,50))   # terraplen
d.rectangle((3,15,16,22),fill=(40,40,48)); d.rectangle((5,10,8,15),fill=(40,40,48)); d.rectangle((4,9,9,10),fill=(70,70,80))
d.rectangle((13,12,18,22),fill=(55,55,65)); d.ellipse((3,19,9,25),fill=(170,40,40)); d.ellipse((10,20,15,25),fill=(170,40,40))
d.point([(6,22),(12,22)],fill=(30,30,30))
d.ellipse((3,4,9,8),fill=(235,235,235)); d.ellipse((7,2,13,6),fill=(220,220,220))
# Bernard: traje gris, piel oscura, cabeza abajo
d.rectangle((20,15,23,19),fill=(80,80,95)); d.rectangle((21,20,22,22),fill=(110,75,50))
d.line([(19,14),(17,12)],fill=(80,80,95)); d.line([(24,14),(26,12)],fill=(80,80,95))
d.line([(20,14),(19,11)],fill=(60,60,70)); d.line([(23,14),(24,11)],fill=(60,60,70))
for k in range(3): d.point((18+k*3,9),fill=(255,255,255))
# 2 barco: mar, casco, salpicadura con los gemelos
b,d=canvas((150,165,160))
d.rectangle((2,17,27,29),fill=(20,70,70))
d.polygon([(2,12),(13,12),(11,18),(2,18)],fill=(120,90,60)); d.rectangle((3,8,9,12),fill=(230,230,220))
for x,y in [(16,15),(18,13),(21,12),(24,13),(26,15),(15,18),(27,18)]: d.ellipse((x-1,y-1,x+1,y+1),fill=(235,245,250))
d.ellipse((18,15,21,18),fill=(240,215,90)); d.ellipse((22,15,25,18),fill=(240,215,90))
d.point([(19,17),(23,17)],fill=(240,200,170))
d.line([(14,20),(27,20)],fill=(200,230,235)); d.line([(16,22),(25,22)],fill=(120,180,190))
# 3 fabrica: cohete volando hacia Ching
c,d=canvas((60,20,25))
for x in (6,18): d.rectangle((x,2,x+1,29),fill=(90,90,95))
d.rectangle((2,24,27,25),fill=(110,110,110)); d.line([(2,24),(27,24)],fill=(200,180,40))
d.line([(4,18),(14,10)],fill=(235,235,235),width=3); d.polygon([(14,8),(17,8),(16,11)],fill=(210,40,40))
d.point([(3,19),(2,20)],fill=(255,170,40)); d.point([(4,20)],fill=(255,220,80))
d.ellipse((18,15,26,23),fill=(150,150,160)); d.ellipse((20,10,24,14),fill=(225,190,150)); d.ellipse((20,22,25,27),fill=(40,40,40))
d.point([(19,7),(22,6),(25,8)],fill=(255,230,80)); d.point([(20,8),(24,7)],fill=(255,140,40))
out=bytearray(); sheet=Image.new('RGB',(96,32))
for k,im in enumerate((a,b,c)):
  o=bytearray(frame)
  for y in range(32):
    for x in range(32):
      if mask[y][x]:
        r,g,bb=im.getpixel((x,y)); v=(r*31//255)<<10|(g*31//255)<<5|(bb*31//255); o[(y*32+x)*2]=v>>8; o[(y*32+x)*2+1]=v&255
  out+=o
  t=Image.new('RGB',(32,32))
  for p in range(1024):
    v=o[p*2]<<8|o[p*2+1]; t.putpixel((p%32,p//32),((v>>10&31)*8,(v>>5&31)*8,(v&31)*8))
  sheet.paste(t,(32*k,0))
sheet.resize((384,128),0).save('/home/claude/work/nuevos.png')
if len(sys.argv)>1:
  open(P,'w').write(base64.b64encode(zlib.compress(bytes(raw+out),9)).decode()+'\n')
