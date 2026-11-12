#!/usr/bin/env python3
"""Hojas de partes de la paleta de cada luchador (para ropas a medida): cada bloque de 8 entradas pintado de verde.
Capturas: OFX_IDX=1 OFX_CHAR=n OFX_TRUCOS=1 OFX_STICK=900 headless outfxies.zip 2400 6 300 /tmp/pal/cn (n = 0..6).
Salida: /home/claude/work/pal_cN.png (cambiar las rutas si hace falta)."""
import sys, glob, os, numpy as np
from PIL import Image, ImageDraw, ImageFont
NAMES=['John Smith','Prof. Ching','Dweeb','Eve','Betty Doe','Danny y Demi','Bernard White']
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',14)
def load(d, n):
    idx=np.fromfile('%s/f%06d.idx'%(d,n),dtype=np.uint16).reshape(224,288)
    im=np.array(Image.open('%s/f%06d.ppm'%(d,n)).convert('RGB'))
    return idx, im
for c in range(7):
    d='/tmp/pal/c%d'%c
    frames=sorted(int(os.path.basename(f)[1:7]) for f in glob.glob(d+'/*.idx'))
    frames=[f for f in frames if f>=800]
    cnt=np.zeros(256,int); col=np.zeros((256,3),int); fsz=[]
    for n in frames:
        idx,im=load(d,n); m=idx!=0xffff
        v=idx[m]; cnt+=np.bincount(v,minlength=256)[:256]
        col[v]=im[m]
        ys,xs=np.nonzero(m)
        if len(ys)>200:
            bw=xs.max()-xs.min(); bh=ys.max()-ys.min()
            if bw<150 and xs.min()>1 and xs.max()<286 and ys.min()>1 and ys.max()<222: fsz.append((len(ys),n))
    fsz.sort(reverse=True)
    # 4 poses distintas: las mas grandes separadas en el tiempo
    pick=[]; psz=[]
    for s,n in fsz:
        if all(abs(n-p)>30 for p in pick) and all(abs(s-q)>s*0.15 for q in psz): pick.append(n); psz.append(s)
        if len(pick)==4: break
    blocks=[b for b in range(0,256,8) if cnt[b:b+8].sum()>0]
    crops=[]
    for n in pick:
        idx,im=load(d,n); m=idx!=0xffff; ys,xs=np.nonzero(m)
        crops.append((idx[ys.min():ys.max()+1, xs.min():xs.max()+1], im[ys.min():ys.max()+1, xs.min():xs.max()+1]))
    S=2; cw=max(cr[0].shape[1] for cr in crops)*S+6; ch=max(cr[0].shape[0] for cr in crops)*S+6
    W=Image.new('RGB',(150+len(crops)*cw+ 8*22, 40+(len(blocks)+1)*(ch+4)),(30,30,40)); dr=ImageDraw.Draw(W)
    dr.text((10,10),'%s (OFX_CHAR %d)'%(NAMES[c],c),font=F,fill=(255,160,40))
    rows=[None]+blocks
    for r,b in enumerate(rows):
        y0=40+r*(ch+4)
        lab='original' if b is None else '0x%02X-0x%02X'%(b,b+7)
        dr.text((10,y0+ch//2-8),lab,font=F,fill=(255,255,255))
        if b is not None:
            for k in range(8):
                if cnt[b+k]: dr.rectangle((150+len(crops)*cw+k*22, y0+ch//2-10, 150+len(crops)*cw+k*22+18, y0+ch//2+8), fill=tuple(int(x) for x in col[b+k]))
        for i,(ix,im) in enumerate(crops):
            out=im.copy()
            if b is not None:
                g=(im.mean(axis=2)*0.35+40).astype(np.uint8); out=np.stack([g,g,g],2)
                sel=(ix>=b)&(ix<b+8); out[sel]=[0,255,0]
                out[ix==0xffff]=[30,30,40]
            else: out[ix==0xffff]=[30,30,40]
            W.paste(Image.fromarray(out).resize((ix.shape[1]*S,ix.shape[0]*S),Image.NEAREST),(150+i*cw,y0))
    W.save('/home/claude/work/pal_c%d.png'%c)
    print(NAMES[c], pick, ' '.join('%02x:%d'%(i,cnt[i]) for i in range(256) if cnt[i]))
