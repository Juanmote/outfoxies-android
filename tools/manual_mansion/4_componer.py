import numpy as np
from PIL import Image, ImageFilter
b=np.load('/tmp/pan/build.npy')   # -1 = transparente
CH,CW,_=b.shape
def clear(x0,x1,y0,y1): b[y0:y1,x0:x1]=-1
# luchadores amontonados donde la camara toca el borde (fuera del edificio)
clear(0,265,0,762); clear(0,152,762,790); clear(0,182,790,968); clear(0,200,968,1065); clear(0,152,1065,1081); clear(0,118,1081,1100)
clear(1207,CW,600,762); clear(1240,CW,762,1095)
X0,Y0,X1,Y1=40,470,CW,CH
crop=b[Y0:Y1,X0:X1]
h,w=crop.shape[:2]
g=Image.open('/tmp/pan/g_632_-64.ppm')
k=max(w/g.width,h/g.height)*1.02
bg=g.resize((round(g.width*k),round(g.height*k)),Image.LANCZOS).filter(ImageFilter.GaussianBlur(1.2))
bg=bg.crop((0,(bg.height-h)//2,w,(bg.height-h)//2+h))
bg=np.asarray(bg).astype(np.int16)
m=(crop<0).any(2)
out=crop.copy(); out[m]=bg[m]
Image.fromarray(out.astype(np.uint8)).save('/tmp/pan/mansion_final.png')
print(out.shape)
