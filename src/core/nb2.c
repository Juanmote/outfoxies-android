/* Hardware Namco NB-2 de The Outfoxies: CPU 68EC020 (Musashi) y video.
   El video sigue el comportamiento documentado en el driver namconb1 de MAME (BSD-3-Clause).
   Ningun dato del juego esta en este codigo: todo se lee de la ROM del usuario. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68k.h"
#include "miniz.h"
#include "nb2.h"

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned u32;
#define MEMSZ 0x1000000
#define W NB2_W
#define H NB2_H
static u8 mem[MEMSZ];
static u8 *gobj, *groz, *gscr, *mroz, *mscr;
static u8 p1, p2, misc;
static int vbl_level, pos_level, vbl_pend, pos_pend;
static u32 rng = 0x12345;
static u8 frame_rgb[W * H * 3];

static void update_irq(void) {
    int l = 0;
    if (vbl_pend && vbl_level > l) l = vbl_level;
    if (pos_pend && pos_level > l) l = pos_level;
    m68k_set_irq(l);
}
static u32 rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 16; }
static u32 lastkey;
static u32 rb(u32 a) {
    a &= 0xffffff;
    switch (a) {
    case 0x206001: return (mem[a] & 0x70) | 0x80 | (misc & 0x0f);
    case 0x206002: return p1;
    case 0x206004: return p2;
    case 0xc00002: return 0x01;
    case 0xc00003: return 0x86;
    case 0xc00004: lastkey = rnd(); return lastkey >> 8 & 0xff;
    case 0xc00005: return lastkey & 0xff;
    }
    if (a >= 0xc00000 && a < 0xc00020) return 0;
    if (a >= 0xf00000 && a < 0xf00020) return 0xff;
    return mem[a];
}
static void wb(u32 a, u32 v) {
    a &= 0xffffff; v &= 0xff;
    if (a < 0x100000 || (a >= 0x400000 && a < 0x500000)) return;
    if (a >= 0xf00000 && a < 0xf00020) {
        switch (a - 0xf00000) {
        case 0: vbl_level = v & 15; vbl_pend = 0; break;
        case 2: pos_level = v & 15; pos_pend = 0; break;
        case 4: vbl_pend = 0; break;
        case 6: pos_pend = 0; break;
        }
        update_irq();
        return;
    }
    mem[a] = v;
    /* copia hacia las tablas de sprites de la pagina 0 */
    if (a >= 0x610000 && a < 0x612000) mem[a - 0x10000] = v;
    else if (a >= 0x614000 && a < 0x614400) mem[a - 0x12000] = v;
}
u32 m68k_read_memory_8(u32 a) { return rb(a); }
u32 m68k_read_memory_16(u32 a) { return rb(a) << 8 | rb(a + 1); }
u32 m68k_read_memory_32(u32 a) { return rb(a) << 24 | rb(a + 1) << 16 | rb(a + 2) << 8 | rb(a + 3); }
u32 m68k_read_disassembler_8(u32 a) { return mem[a & 0xffffff]; }
u32 m68k_read_disassembler_16(u32 a) { a &= 0xffffff; return mem[a] << 8 | mem[a + 1]; }
u32 m68k_read_disassembler_32(u32 a) { a &= 0xffffff; return mem[a] << 24 | mem[a + 1] << 16 | mem[a + 2] << 8 | mem[a + 3]; }
void m68k_write_memory_8(u32 a, u32 v) { wb(a, v); }
void m68k_write_memory_16(u32 a, u32 v) { wb(a, v >> 8); wb(a + 1, v); }
void m68k_write_memory_32(u32 a, u32 v) { wb(a, v >> 24); wb(a + 1, v >> 16); wb(a + 2, v >> 8); wb(a + 3, v); }

/* ---------- video ---------- */
static u16 R16(u32 a) { return mem[a] << 8 | mem[a + 1]; }
static u16 fb[H][W]; static u8 pri[H][W];
static u16 sprbuf[H][W];
static u8 sprram_copy[0x20000], sprbank_copy[16];

static int clipx0, clipx1, clipy0, clipy1;
static u16 c116reg(int r) { return R16(0x801800 + r * 2); }

static void draw_tmap(int i, int prival) {
    u16 ctrl_pal = R16(0x6c0030 + i * 2);
    int palbase = 0x1000 + ((ctrl_pal & 7) << 8);
    int sx = 0, sy = 0, cols = 64, rows = 64; u32 base;
    if (i < 4) {
        static const int adj[4] = {4, 2, 1, 0};
        sx = (R16(0x6c0002 + i * 8) & 0x1ff) + (44 + adj[i]);
        sy = (R16(0x6c0006 + i * 8) & 0x1ff) + 24;
        base = 0x680000 + i * 0x2000;
    } else {
        cols = 36; rows = 28;
        base = 0x680000 + (i == 4 ? 0x8010 : 0x8810);
    }
    int mw = cols * 8, mh = rows * 8;
    for (int y = clipy0; y < clipy1; y++) {
        int ty = ((y + sy) % mh + mh) % mh;
        for (int x = clipx0; x < clipx1; x++) {
            int tx = ((x + sx) % mw + mw) % mw;
            u16 code = R16(base + ((ty >> 3) * cols + (tx >> 3)) * 2);
            u32 m = code;
            u32 t = (code & 0xfe3f) | ((code & 0x40) << 2) | (code & 0x80) | ((code & 0x100) >> 2);
            t &= 0x7fff; m &= 0xffff;
            u8 mb = mscr[(m * 8 + (ty & 7)) % 0x80000];
            if (mb & (0x80 >> (tx & 7))) {
                fb[y][x] = palbase + gscr[t * 64 + (ty & 7) * 8 + (tx & 7)];
                pri[y][x] = prival;
            }
        }
    }
}

static void roz_pixel(int which, int color, u32 xpos, u32 ypos, int y, int x, int prival) {
    int col = xpos >> 4, row = ypos >> 4;
    u32 idx = ((col & 0x80) << 8) | ((row & 0xff) << 7) | (col & 0x7f);
    u32 code = R16(0x700000 + idx * 2) & 0x3fff;
    u32 bank = mem[0x980000 + which * 8 + ((code >> 11) & 7)];
    u32 mg = (code & 0x7ff) | (bank << 11);
    u32 t = (mg & ~0x50u) | ((mg & 0x10) << 2) | ((mg & 0x40) >> 2);
    t %= 0x6000;
    u32 mo = (mg * 32 + (ypos & 15) * 2 + ((xpos & 15) >> 3)) % 0x100000;
    if (mroz[mo] & (0x80 >> (xpos & 7))) {
        fb[y][x] = 0x1800 + color + groz[t * 256 + (ypos & 15) * 16 + (xpos & 15)];
        pri[y][x] = prival;
    }
}
static int sx12(u16 t) { if (t & 0x8000) t |= 0xf000; else t &= 0x0fff; return (short)t; }
static void draw_roz_params(int which, const u16 *s, int yfrom, int yto, int scan, int prival) {
    u16 a = s[1];
    int wrap = !(a & 0x0800), size = 512 << ((a & 0x300) >> 8), color = (a & 15) * 256;
    u32 left = (s[2] & 0x7000) >> 3, top = (s[3] & 0x7000) >> 3;
    int ixx = sx12(s[2]), ixy = sx12(s[3]), iyx = sx12(s[4]), iyy = sx12(s[5]);
    int sxv = (short)s[6], syv = (short)s[7];
    sxv <<= 4; syv <<= 4;
    sxv += 36 * ixx + 3 * iyx; syv += 36 * ixy + 3 * iyy;
    int startx = sxv << 8, starty = syv << 8;
    ixx <<= 8; ixy <<= 8; iyx <<= 8; iyy <<= 8;
    startx += clipx0 * ixx; starty += clipx0 * ixy;
    if (!scan) { startx += yfrom * iyx; starty += yfrom * iyy; }
    u32 mask = size - 1;
    for (int y = yfrom; y < yto; y++) {
        int cx = startx, cy = starty;
        for (int x = clipx0; x < clipx1; x++) {
            u32 xp, yp;
            if (wrap) {
                if (scan) { xp = (cx >> 16) & 0xfff; yp = (cy >> 16) & 0xfff; }
                else { xp = (((cx >> 16) & mask) + left) & 0xfff; yp = (((cy >> 16) & mask) + top) & 0xfff; }
                roz_pixel(which, color, xp, yp, y, x, prival);
            } else {
                u32 wx = (cx >> 16) & 0xfff, wy = (cy >> 16) & 0xfff, x1 = left + size, y1 = top + size;
                int inx = x1 <= 0x1000 ? (wx >= left && wx < x1) : (wx >= left || wx < x1 - 0x1000);
                int iny = y1 <= 0x1000 ? (wy >= top && wy < y1) : (wy >= top || wy < y1 - 0x1000);
                if (inx && iny) roz_pixel(which, color, wx, wy, y, x, prival);
            }
            cx += ixx; cy += ixy;
        }
        startx += iyx; starty += iyy;
    }
}
static void draw_roz(int p) {
    int scanmode = R16(0x740000) == 0x8000;
    for (int which = 1; which >= 0; which--) {
        u16 s[8]; for (int i = 0; i < 8; i++) s[i] = R16(0x740000 + which * 16 + i * 2);
        if (s[1] & 0x8000) continue;
        if (which == 1 && scanmode) {
            for (int line = clipy0; line < clipy1; line++) {
                u32 o = 0x700000 + (line >> 3) * 0x100 + (line & 7) * 0x10 + 0xe080;
                u16 q[8]; for (int i = 0; i < 8; i++) q[i] = R16(o + i * 2);
                if (q[1] & 0x8000) continue;
                if (((q[1] >> 4) & 15) == p) draw_roz_params(which, q, line, line + 1, 1, p);
            }
        } else if (((s[1] >> 4) & 15) == p) draw_roz_params(which, s, clipy0, clipy1, 0, p);
    }
}
static u16 SP(u32 o) { return sprram_copy[o] << 8 | sprram_copy[o + 1]; }
static int sext(int v, int b) { v &= (1 << b) - 1; return (v ^ (1 << (b - 1))) - (1 << (b - 1)); }
static unsigned long sprites_drawn;
static void draw_tile(int code, int color, int fx, int fy, int hpos, int vpos, int zx, int zy, int prio, int sw, int shh, int cx0, int cx1, int cy0, int cy1) {
    if (!zx || !zy || !sw || !shh) return;
    u8 *src = gobj + ((u32)code % 0x20000) * 256;
    int dx = (16 << 16) / sw, dy = (16 << 16) / shh, ex = hpos + sw, ey = vpos + shh, xb = 0, yi = 0;
    if (fx) { xb = (sw - 1) * dx; dx = -dx; }
    if (fy) { yi = (shh - 1) * dy; dy = -dy; }
    if (hpos < cx0) { int n = cx0 - hpos; hpos += n; xb += n * dx; }
    if (vpos < cy0) { int n = cy0 - vpos; vpos += n; yi += n * dy; }
    if (ex > cx1 + 1) ex = cx1 + 1;
    if (ey > cy1 + 1) ey = cy1 + 1;
    if (ex <= hpos) return;
    for (int y = vpos; y < ey; y++) {
        u8 *s = src + (yi >> 16) * 16; int xi = xb;
        for (int x = hpos; x < ex; x++) {
            u8 c = s[xi >> 16];
            if (c != 0xff) sprbuf[y][x] = (prio & 15) << 12 | ((color * 256 + c) & 0xfff);
            xi += dx;
        }
        yi += dy;
    }
}
static void render_sprites(void) {
    memset(sprbuf, 0xff, sizeof sprbuf);
    int xs = sext(R16(0x620002), 9) + 0x26, ys = sext(R16(0x620000), 9) + 0x19;
    for (int i = 0; i < 256; i++) {
        u16 which = SP(0x2000 + i * 2);
        u32 e = (which & 0xff) * 16;
        u16 pal = SP(e + 12);
        int prio = (pal >> 4) & 15;
        int fmt = SP(e) & 0x7ff, ofs = SP(e + 2), hpos = SP(e + 4), vpos = SP(e + 6), hs = SP(e + 8), vs = SP(e + 10);
        hpos -= xs; vpos -= ys;
        int ce = (pal >> 8) & 15;
        int cx0 = (short)SP(0x2400 + ce * 8) - xs, cx1 = (short)SP(0x2402 + ce * 8) - xs, cy0 = (short)SP(0x2404 + ce * 8) - ys, cy1 = (short)SP(0x2406 + ce * 8) - ys;
        if (cx0 < 0) cx0 = 0; if (cy0 < 0) cy0 = 0; if (cx1 > W - 1) cx1 = W - 1; if (cy1 > H - 1) cy1 = H - 1;
        hpos = sext(hpos, 11); vpos = sext(vpos, 11);
        int ti = SP(0x4000 + fmt * 8); u16 f = SP(0x4002 + fmt * 8);
        int ddx = SP(0x4004 + fmt * 8) & 0x1ff, ddy = SP(0x4006 + fmt * 8) & 0x1ff;
        int nc = (f >> 4) & 15, nr = f & 15; if (!nc) nc = 16; if (!nr) nr = 16;
        int fx = hs & 0x8000, fy = vs & 0x8000; hs &= 0x3ff; vs &= 0x3ff;
        if (hs && vs) {
            unsigned long zx = ((unsigned long)hs << 16) / (nc * 16), zy = ((unsigned long)vs << 16) / (nr * 16);
            int dz = (int)(((ddx & 0xff) * zx + 0x8000) >> 16); if (ddx & 0x100) dz = -dz;
            hpos += fx ? dz : -dz;
            dz = (int)(((ddy & 0xff) * zy + 0x8000) >> 16); if (ddy & 0x100) dz = -dz;
            vpos += fy ? dz : -dz;
            u32 shr = nr * 16, scr = vs; int yy = vpos;
            for (int r = 0; r < nr; r++) {
                int th = 16 * scr / shr; int zyy = (int)(((unsigned long)scr << 16) / shr);
                if (fy) yy -= th;
                u32 swr = nc * 16, sc2 = hs; int xx = hpos;
                for (int c = 0; c < nc; c++) {
                    int tw = 16 * sc2 / swr; int zxx = (int)(((unsigned long)sc2 << 16) / swr);
                    if (fx) xx -= tw;
                    u16 tile = SP(0x8000 + ((ti++) & 0x3fff) * 2);
                    if (!(tile & 0x8000)) {
                        int b = sprbank_copy[(tile >> 11) & 15] & 0x5f;
                        int bs = ((b >> 6) & 1) << 5 | ((b >> 4) & 1) << 4 | ((b >> 3) & 1) << 3 | ((b >> 1) & 1) << 2 | ((b >> 2) & 1) << 1 | (b & 1);
                        int code = (tile & 0x7ff) | (bs << 11);
                        int sw = (zxx * 16 + 0x8000) >> 16, sh = (zyy * 16 + 0x8000) >> 16;
                        draw_tile(code + ofs, pal & 15, fx, fy, xx, yy, zxx, zyy, prio, sw, sh, cx0, cx1, cy0, cy1);
                        sprites_drawn++;
                    }
                    if (!fx) xx += tw;
                    sc2 -= tw; swr -= 16;
                }
                if (!fy) yy += th;
                scr -= th; shr -= 16;
            }
        }
        if (which & 0x100) break;
    }
}
static void render_frame(u8 *rgb) {
    clipx0 = (short)c116reg(0) - 0x4b; clipx1 = (short)c116reg(1) - 0x4b;
    clipy0 = (short)c116reg(2) - 0x21; clipy1 = (short)c116reg(3) - 0x21;
    if (clipx0 < 0) clipx0 = 0; if (clipy0 < 0) clipy0 = 0; if (clipx1 > W) clipx1 = W; if (clipy1 > H) clipy1 = H;
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { fb[y][x] = 0xffff; pri[y][x] = 0; }
    for (int p = 0; p < 16; p++) {
        draw_roz(p);
        if (!(p & 1))
            for (int i = 0; i < 6; i++) {
                u16 c = R16(0x6c0020 + i * 2);
                if ((c & 8)) continue;
                if ((c & 7) == (p >> 1)) draw_tmap(i, p);
            }
    }
    u8 sh[H][W]; memset(sh, 0, sizeof sh);
    for (int y = clipy0; y < clipy1; y++) for (int x = clipx0; x < clipx1; x++) {
        u16 s = sprbuf[y][x];
        if (s == 0xffff) continue;
        int sp = s >> 12; u16 c = s & 0xfff;
        if (sp >= pri[y][x] && (c & 0xff) != 0xff) {
            if (c == 0xffe) sh[y][x] = 1; else fb[y][x] = c;
            pri[y][x] = sp;
        }
    }
    for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
        u16 pen = fb[y][x]; u8 r = 0, g = 0, b = 0;
        if (pen != 0xffff) {
            pen &= 0x1fff;
            u32 o = ((pen & 0x1800) << 2) | (pen & 0x7ff);
            r = mem[0x800000 + o]; g = mem[0x800800 + o]; b = mem[0x801000 + o];
        }
        if (sh[y][x]) { r /= 2; g /= 2; b /= 2; }
        rgb[(y * W + x) * 3] = r; rgb[(y * W + x) * 3 + 1] = g; rgb[(y * W + x) * 3 + 2] = b;
    }
}
static void vblank_copy(void) {
    /* buffer de sprites: renderiza lo copiado en el vblank anterior y luego copia */
    render_sprites();
    memcpy(sprbank_copy, mem + 0x900008, 8);
    memcpy(sprram_copy, mem + 0x600000, 0x20000);
}

/* ---------- carga de la ROM ---------- */
static u8 *zget(mz_zip_archive *z, const char *name, size_t need, char *err, int errlen) {
    int i = mz_zip_reader_locate_file(z, name, NULL, 0);
    size_t n = 0;
    u8 *b, *out;
    if (i < 0) { snprintf(err, errlen, "falta %s", name); return NULL; }
    b = mz_zip_reader_extract_to_heap(z, i, &n, 0);
    if (!b || n < need) { snprintf(err, errlen, "%s no es valido", name); free(b); return NULL; }
    /* algunos romsets traen volcados mas grandes con el contenido repetido */
    out = malloc(need); memcpy(out, b, need); free(b);
    return out;
}
int nb2_load(const char *zip, char *err, int errlen) {
    static const char *nl[5] = {"ou1obj0l.4c", "ou1obj1l.4b", "ou1obj2l.4a", "ou1obj3l.6c", "ou1obj4l.6b"};
    static const char *nu[5] = {"ou1obj0u.8c", "ou1obj1u.8b", "ou1obj2u.8a", "ou1obj3u.9c", "ou1obj4u.9b"};
    static const char *nr[3] = {"ou1-rot0.3d", "ou1-rot1.3c", "ou1-rot2.3b"};
    mz_zip_archive z; u8 *a, *b; int ok = 0;
    memset(&z, 0, sizeof z);
    if (!mz_zip_reader_init_file(&z, zip, 0)) { snprintf(err, errlen, "no se pudo abrir el zip"); return 0; }
    a = zget(&z, "ou2_mpru.11d", 0x80000, err, errlen); if (!a) goto fin;
    b = zget(&z, "ou2_mprl.11c", 0x80000, err, errlen); if (!b) { free(a); goto fin; }
    for (int i = 0; i < 0x80000; i += 2) {
        mem[i * 2] = a[i]; mem[i * 2 + 1] = a[i + 1]; mem[i * 2 + 2] = b[i]; mem[i * 2 + 3] = b[i + 1];
    }
    free(a); free(b);
    a = zget(&z, "ou1dat0.20a", 0x80000, err, errlen); if (!a) goto fin;
    b = zget(&z, "ou1dat1.20b", 0x80000, err, errlen); if (!b) { free(a); goto fin; }
    for (int i = 0; i < 0x80000; i++) { mem[0x400000 + (i ^ 1)] = a[i]; mem[0x480000 + (i ^ 1)] = b[i]; }
    free(a); free(b);
    gobj = calloc(1, 0x2000000);
    for (int k = 0; k < 5; k++) {
        a = zget(&z, nl[k], 0x200000, err, errlen); if (!a) goto fin;
        b = zget(&z, nu[k], 0x200000, err, errlen); if (!b) { free(a); goto fin; }
        for (int i = 0; i < 0x200000; i += 2) {
            u32 o = k * 0x400000 + i * 2;
            gobj[o] = a[i]; gobj[o + 1] = a[i + 1]; gobj[o + 2] = b[i]; gobj[o + 3] = b[i + 1];
        }
        free(a); free(b);
    }
    groz = malloc(0x600000);
    for (int k = 0; k < 3; k++) {
        a = zget(&z, nr[k], 0x200000, err, errlen); if (!a) goto fin;
        memcpy(groz + k * 0x200000, a, 0x200000); free(a);
    }
    gscr = zget(&z, "ou1-scr0.1d", 0x200000, err, errlen); if (!gscr) goto fin;
    mroz = zget(&z, "ou1shar.18s", 0x100000, err, errlen); if (!mroz) goto fin;
    mscr = zget(&z, "ou1shas.12s", 0x80000, err, errlen); if (!mscr) goto fin;
    ok = 1;
fin:
    mz_zip_reader_end(&z);
    return ok;
}

/* ---------- interfaz ---------- */
void nb2_reset(void) {
    m68k_init(); m68k_set_cpu_type(M68K_CPU_TYPE_68EC020); m68k_pulse_reset();
}
void nb2_set_inputs(unsigned char a, unsigned char b, unsigned char c) { p1 = a; p2 = b; misc = c; }
void nb2_coin(void) {
    u16 v = R16(0x20601e) + 1;
    mem[0x20601e] = v >> 8; mem[0x20601f] = v & 0xff;
}
const unsigned char *nb2_frame(void) {
    enum { LINES = 264, CPL = 403200 / 264 };
    int posline = (int)c116reg(5) - 32;
    for (int ln = 0; ln < LINES; ln++) {
        m68k_execute(CPL);
        if (ln == 224) { vblank_copy(); vbl_pend = 1; update_irq(); render_frame(frame_rgb); }
        if (ln == posline) { pos_pend = 1; update_irq(); }
    }
    return frame_rgb;
}
unsigned char *nb2_eeprom(void) { return mem + 0xa00000; }
