import numpy as np
from PIL import Image
H,W=224,384
shots=[]
for ln in open('/tmp/pan/cam.txt'):
    y,x,cx,cy=map(int,ln.split())
    if 700<=cy<=2000: shots.append((y,x,cx,cy))
mnx=min(s[2] for s in shots); mny=min(s[3] for s in shots)
CW=max(s[2] for s in shots)-mnx+W; CH=max(s[3] for s in shots)-mny+H
def get(y,x):
    f=np.asarray(Image.open('/tmp/pan/f_%d_%d.ppm'%(y,x))).astype(np.int16)
    c=np.asarray(Image.open('/tmp/pan/c_%d_%d.ppm'%(y,x))).astype(np.int16)
    f=f.copy(); f[(f!=c).any(2)]=-1    # backdrop
    return f
def compose(shots):
    out=np.zeros((CH,CW,3),np.int16); S=48
    for y0 in range(0,CH,S):
        y1=min(CH,y0+S)
        sel=[s for s in shots if s[3]-mny<y1 and s[3]-mny+H>y0]
        arr=np.full((max(1,len(sel)),y1-y0,CW,3),-1,np.int16)
        for n,(y,x,cx,cy) in enumerate(sel):
            im=CACHE[(y,x)]; px=cx-mnx; py=cy-mny; a0=max(y0,py); a1=min(y1,py+H)
            arr[n,a0-y0:a1-y0,px:px+W]=im[a0-py:a1-py]
        m=np.ma.masked_less(arr,0)
        out[y0:y1]=np.ma.median(m,axis=0).filled(-1)
    return out
CACHE={(s[0],s[1]):get(s[0],s[1]) for s in shots}
out=compose(shots)
for it in range(2):
    keep=[]
    for s in shots:
        im=CACHE[(s[0],s[1])]; px=s[2]-mnx; py=s[3]-mny; ref=out[py:py+H,px:px+W]
        v=(im>=0)&(ref>=0); e=np.abs(im-ref)[v].mean() if v.sum()>1000 else 0
        if e<12: keep.append(s)
    print('quedan',len(keep),'de',len(shots)); shots=keep; out=compose(shots)
np.save('/tmp/pan/build.npy',out)
vis=out.copy(); vis[vis<0]=0
Image.fromarray(vis.astype(np.uint8)).save('/tmp/pan/build.png')
print(CW,CH)
