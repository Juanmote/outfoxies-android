from PIL import Image
import glob, numpy as np
from scipy import ndimage
names=['john','ching','dweeb','eve','betty','gemelos','bernard']
fr=[np.asarray(Image.open(sorted(glob.glob('selm/%d/*.ppm'%n))[-1]).convert('RGB')).astype(int) for n in range(7)]
for n,nm in enumerate(names):
    a=fr[n]; o1=fr[(n+3)%7]; o2=fr[(n+4)%7]
    mk=((np.abs(a-o1).sum(2)>30)&(np.abs(a-o2).sum(2)>30)); mk[:20]=False
    mk=ndimage.binary_closing(mk,iterations=3)
    lab,k=ndimage.label(mk); sizes=ndimage.sum(mk,lab,range(1,k+1)); big=np.argmax(sizes)+1
    keep=np.isin(lab,[i+1 for i,s in enumerate(sizes) if s>sizes.max()*0.08])
    ys,xs=np.where(lab==big)
    x1,x2=int(xs.min())-10,int(xs.max())+10; y1=int(ys.min())-8; y2=int(ys.max())+6
    b=a.copy(); b[~keep]=(b[~keep]*0.38).astype(int)
    w=x2-x1; need=int((y2-y1)*0.6)
    if w<need: c=(x1+x2)//2; x1=c-need//2; x2=x1+need
    Image.fromarray(b.astype('uint8')).crop((max(0,x1),max(0,y1),min(288,x2),min(224,y2))).save('man/sel_%s.png'%nm)
