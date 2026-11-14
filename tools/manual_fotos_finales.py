import sys, glob, numpy as np, os
from PIL import Image
d, out = sys.argv[1], sys.argv[2]
fs = sorted(glob.glob(d + '/*.ppm'))
Y0, Y1, X0, X1 = 100, 220, 0, 150
reg = [np.asarray(Image.open(f).convert('RGB'))[Y0:Y1, X0:X1].astype(np.int16) for f in fs]
def sig(a):  # firma robusta: mediana por bloques
    h, w = a.shape[:2]
    return np.array([[np.median(a[y:y+10, x:x+10].max(2)) for x in range(0, w - 9, 10)] for y in range(0, h - 9, 10)])
sg = [sig(a) for a in reg]
present = [ (s > 25).mean() > 0.35 for s in sg ]
segs = []; cur = None
for i in range(len(reg)):
    if not present[i]:
        if cur: segs.append(cur); cur = None
        continue
    if cur and np.median(np.abs(sg[i] - sg[cur[-1]])) < 12: cur.append(i)
    else:
        if cur: segs.append(cur)
        cur = [i]
if cur: segs.append(cur)
segs = [s for s in segs if len(s) >= 8][-3:]   # los 3 cuadros del final (antes va la tele del saldo)
print('cuadros', [(s[0], len(s)) for s in segs])
os.makedirs(out, exist_ok=True)
meds = [np.median(np.stack([reg[i] for i in s]), axis=0).astype(np.uint8) for s in segs]
u = np.zeros(meds[0].shape[:2], bool)
for med in meds: u |= med.max(2) > 20
ys = np.where(u.mean(1) > 0.3)[0]; xs = np.where(u.mean(0) > 0.3)[0]
for k, s in enumerate(segs):
    im = Image.fromarray(meds[k]).crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    im.save('%s/%d.png' % (out, k + 1)); print(k + 1, im.size, len(s))
for k, s in []:
    st = np.stack([reg[i] for i in s])
    med = np.median(st, axis=0).astype(np.uint8)
    m = med.max(2) > 20
    ys = np.where(m.mean(1) > 0.3)[0]; xs = np.where(m.mean(0) > 0.3)[0]
    im = Image.fromarray(med).crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    im.save('%s/%d.png' % (out, k + 1)); print(k + 1, im.size, len(s))
