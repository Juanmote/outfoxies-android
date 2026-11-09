/* Hardware Namco NB-2 de The Outfoxies: CPU 68EC020 (Musashi) y video.
   El video sigue el comportamiento documentado en el driver namconb1 de MAME (BSD-3-Clause).
   Ningun dato del juego esta en este codigo: todo se lee de la ROM del usuario. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "m68k.h"
#include "miniz.h"
#include "nb2.h"
#include "m37710.h"
#include "c352.h"
#include "m68k_state.h"

typedef unsigned char u8; typedef unsigned short u16; typedef unsigned u32;
#define MEMSZ 0x1000000
#define W NB2_W
#define H NB2_H
static u8 mem[MEMSZ];
static u8 *gobj, *groz, *gscr, *mroz, *mscr;
static u8 p1, p2, misc;
static int human2;     /* el 2P ha pulsado START: tambien juega una persona */
static int vbl_level, pos_level, vbl_pend, pos_pend;
static u32 rng = 0x12345;
/* 16:9: se dibujan EXT columnas mas a cada lado (384x224 con el mismo pixel que el 4:3 de 288x224).
   Los arrays de video siempre tienen el ancho maximo; la columna x de pantalla va en x + EXT. */
#define EXT ((NB2_WIDE_W - NB2_W) / 2)
#define FW (W + 2 * EXT)
#define X(x) ((x) + EXT)
static int ext;                         /* 0 en 4:3, EXT en 16:9 */
static int render_on = 1;
static int side_mode;                   /* 0 auto; 1 laterales siempre a la vista (pruebas) */
static int patches_ready, patches_on;     /* parches de 16:9 en el programa (ver wide_patches) */
static void wide_patches(int on);               /* 0 = no dibuja (avance rapido): el juego sigue igual */
static u8 frame_rgb[FW * H * 3];

/* ---------- sonido: CPU C75 (M37702) y C352 ---------- */
static u8 *c75bios, *c75data, *voice;
static int snd_ok, mcu_run = 1, mcu_spin, coin_frames;
static u8 port6;
static unsigned long long mcu_clock, snd_pos;     /* ciclos del C75 y muestras del C352 */
#define MCU_PER_FRAME 134400                       /* 16,128 MHz / 2 / 60 */
#define CYC_PER_SAMPLE 96                          /* C352 a 24,192 MHz / 288 = 84 kHz */
static short audio_buf[NB2_AUDIO_MAX * 2];
static int audio_n;
static void snd_sync(void) {
    unsigned long long target = mcu_clock / CYC_PER_SAMPLE;
    if (target > snd_pos) {
        int n = (int)(target - snd_pos);
        if (audio_n + n > NB2_AUDIO_MAX) n = NB2_AUDIO_MAX - audio_n;
        if (n > 0) { c352_render(audio_buf + audio_n * 2, n); audio_n += n; }
        snd_pos = target;
    }
}
u8 c75_ext_r8(u32 a) {
    if (a >= 0x2000 && a < 0x3000) { u16 v; snd_sync(); v = c352_read((a - 0x2000) >> 1); return (a & 1) ? v >> 8 : v & 0xff; }
    if (a >= 0x4000 && a < 0xc000) return mem[0x200000 + ((a - 0x4000) ^ 1)];
    if (a >= 0x200000 && a < 0x280000) return c75data[a - 0x200000];
    return 0;
}
u16 c75_ext_r16(u32 a) {
    if (a >= 0x2000 && a < 0x3000) { snd_sync(); return c352_read((a - 0x2000) >> 1); }
    return c75_ext_r8(a) | c75_ext_r8(a + 1) << 8;
}
static void shared_w(u32 a, u8 v) { mem[0x200000 + ((a - 0x4000) ^ 1)] = v; }
static void sync_check(u32 a, u16 data) {
    /* la BIOS del C75 deja muy poco tiempo la senal de sincronia: se cede el paso a la 68020 (como MAME) */
    if (a == 0xa000 && (data & 0x80)) { mcu_spin = 300; m37710_abort_slice(); }
}
void c75_ext_w8(u32 a, u8 v) {
    if (a >= 0x2000 && a < 0x3000) { snd_sync(); c352_write((a - 0x2000) >> 1, (a & 1) ? v << 8 : v, (a & 1) ? 0xff00 : 0x00ff); return; }
    if (a >= 0x4000 && a < 0xc000) { shared_w(a, v); sync_check(a & ~1u, (a & 1) ? v << 8 : v); }
}
void c75_ext_w16(u32 a, u16 v) {
    if (a >= 0x2000 && a < 0x3000) { snd_sync(); c352_write((a - 0x2000) >> 1, v, 0xffff); return; }
    if (a >= 0x4000 && a < 0xc000) { shared_w(a, v & 0xff); shared_w(a + 1, v >> 8); sync_check(a, v); }
}
u8 c75_port_in(int p) {
    if (p == 6) return port6;
    if (p == 7) switch (port6 & 0xf0) {
        case 0x20: return coin_frames > 0 ? 0xdf : 0xff;   /* MISC: bit 5 = moneda 1, activo a 0 */
        case 0x40: return ~p1 & 0xff;
        case 0x60: return ~p2 & 0xff;
        default: return 0xff;
    }
    return 0xff;
}
void c75_port_out(int p, u8 v) { if (p == 6) port6 = v; }
u16 c75_analog(int ch) { (void)ch; return 0x80; }   /* jugador 3 en las entradas analogicas: nada pulsado */
static void mcu_run_for(int cyc) {
    while (cyc > 0) {
        int n;
        if (!mcu_run) n = cyc;
        else if (mcu_spin > 0) { n = mcu_spin < cyc ? mcu_spin : cyc; mcu_spin -= n; }
        else n = m37710_execute(cyc);
        cyc -= n; mcu_clock += n;
        if (mcu_spin > 0) break;     /* deja que la 68020 lea la sincronia */
    }
}

static void update_irq(void) {
    int l = 0;
    if (vbl_pend && vbl_level > l) l = vbl_level;
    if (pos_pend && pos_level > l) l = pos_level;
    m68k_set_irq(l);
}
static u32 rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 16; }
static u32 lastkey;
static unsigned frame_events, last_events;   /* NB2_EV_* del fotograma en curso y del anterior */
static u32 rb(u32 a) {
    a &= 0xffffff;
    if (!snd_ok) switch (a) {
    case 0x206001: return (mem[a] & 0x70) | 0x80 | (misc & 0x0f);
    case 0x206002: return p1;
    case 0x206004: return p2;
    }
    switch (a) {
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
        case 0x16:      /* control del C75: bit 0 = reset y marcha, 0 = parado */
            if (v & 1) { mcu_run = 1; mcu_spin = 0; if (snd_ok) m37710_reset(); }
            else mcu_run = 0;
            break;
        }
        update_irq();
        return;
    }
    /* Ordenes de sonido: la rutina 0x1810 escribe una palabra por canal en 0x200000-0x20003F (prioridad en
       el nibble alto, 4 = sonar). El sonido 0x18 es el de las explosiones (comprobado en el avion y en el
       bombardeo de la mansion): sirve para la vibracion. */
    if (a >= 0x200000 && a < 0x200040 && (a & 1) && v == 0x18 && (mem[a - 1] & 0xf0) == 0x40) frame_events |= NB2_EV_EXPLOSION;
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
static u16 fb[H][FW]; static u8 pri[H][FW];
static u16 sprbuf[H][FW];
static u8 sprram_copy[0x20000], sprbank_copy[16];

static int clipx0, clipx1, clipy0, clipy1;
static u16 c116reg(int r) { return R16(0x801800 + r * 2); }

/* Textos de monedas: el juego va todo con START, asi que "INSERT COIN" y "CREDIT n" no se dibujan.
   Son losetas fijas de las capas de texto (4 y 5), siempre seguidas: INSERT COIN pequeno (en combate)
   0x49C7..0x49CF, INSERT COIN grande (presentacion y titulo) 0x4954..0x495F, y CREDIT 0x4941..0x4945
   o CREDITS 0x4946..0x494B seguidos del numero. Se buscan por ese patron en cada fila. */
static u8 hide_txt[2][28][36];
static void find_coin_text(int i) {
    static const u16 run[4][2] = {{0x49c7, 9}, {0x4954, 12}, {0x4941, 5}, {0x4946, 6}};
    u32 base = 0x680000 + (i == 4 ? 0x8010 : 0x8810);
    u8 (*h)[36] = hide_txt[i - 4];
    memset(h, 0, sizeof hide_txt[0]);
    for (int r = 0; r < 28; r++) for (int c = 0; c < 36; c++) {
        u16 t = R16(base + (r * 36 + c) * 2);
        for (int q = 0; q < 4; q++) {
            int n = run[q][1], ok = t == run[q][0] && c + n <= 36, k;
            for (k = 1; ok && k < n; k++) if (R16(base + (r * 36 + c + k) * 2) != run[q][0] + k) ok = 0;
            if (!ok) continue;
            for (k = 0; k < n; k++) h[r][c + k] = 1;
            if (q >= 2)   /* el numero de creditos */
                for (; k < n + 3 && c + k < 36 && R16(base + (r * 36 + c + k) * 2) != 0x0020; k++) h[r][c + k] = 1;
        }
    }
}
static void draw_tmap(int i, int prival) {
    if (i >= 4) find_coin_text(i);
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
    int mw = cols * 8, mh = rows * 8, x0 = clipx0, x1 = clipx1;
    /* las capas fijas (marcador, textos) miden justo 288: en 16:9 se quedan en el centro */
    if (i >= 4) { if (x0 < 0) x0 = 0; if (x1 > W) x1 = W; }
    /* Fondos de 288 (el paisaje de la mansion): la capa solo tiene dibujo en las columnas que caben en el
       4:3 y el resto esta vacio. En 16:9 los laterales se rellenan con el propio fondo reflejado en el borde
       (como un espejo), asi no quedan franjas negras. Solo si el dibujo ocupa justo la pantalla del 4:3. */
    int mirror = 0;
    if (ext && i < 4) {
        static u8 colfull[64];
        int first = -1, n = 0, tiles = 0;
        for (int c = 0; c < 64; c++) {
            colfull[c] = 0;
            for (int r = 0; r < 64; r++) {
                u16 code = R16(base + (r * 64 + c) * 2);
                const u8 *mb = mscr + (code * 8) % 0x80000;
                if (mb[0] | mb[1] | mb[2] | mb[3] | mb[4] | mb[5] | mb[6] | mb[7]) { colfull[c] = 1; tiles++; }
            }
        }
        /* columnas con dibujo seguidas (circular) */
        for (int c = 0; c < 64; c++) if (colfull[c] && !colfull[(c + 63) & 63]) { first = c; break; }
        if (first >= 0) {
            while (n < 64 && colfull[(first + n) & 63]) n++;
            int left = ((first * 8 - sx) % 512 + 512) % 512;      /* x de pantalla donde empieza el dibujo */
            if (left > 256) left -= 512;
            int total = 0; for (int c = 0; c < 64; c++) total += colfull[c];
            /* y que sea un fondo de pantalla completa (el marcador tambien mide 288, pero solo ocupa arriba) */
            if (total == n && tiles >= 36 * 24 && n >= 35 && n <= 37 && left >= -8 && left <= 0 && left + n * 8 >= W) mirror = 1;
        }
    }
    for (int y = clipy0; y < clipy1; y++) {
        int ty = ((y + sy) % mh + mh) % mh;
        for (int x = x0; x < x1; x++) {
            int xs = x;
            if (mirror) { if (x < 0) xs = -1 - x; else if (x >= W) xs = 2 * W - 1 - x; }
            int tx = ((xs + sx) % mw + mw) % mw;
            u16 code = R16(base + ((ty >> 3) * cols + (tx >> 3)) * 2);
            if (i >= 4 && hide_txt[i - 4][ty >> 3][tx >> 3]) continue;
            u32 m = code;
            u32 t = (code & 0xfe3f) | ((code & 0x40) << 2) | (code & 0x80) | ((code & 0x100) >> 2);
            t &= 0x7fff; m &= 0xffff;
            u8 mb = mscr[(m * 8 + (ty & 7)) % 0x80000];
            if (mb & (0x80 >> (tx & 7))) {
                fb[y][X(x)] = palbase + gscr[t * 64 + (ty & 7) * 8 + (tx & 7)];
                pri[y][X(x)] = prival;
            }
        }
    }
}

/* color (pen) del plano ROZ en (xpos, ypos), o -1 si ahi no hay dibujo */
static int roz_pen(int which, int color, u32 xpos, u32 ypos) {
    int col = xpos >> 4, row = ypos >> 4;
    u32 idx = ((col & 0x80) << 8) | ((row & 0xff) << 7) | (col & 0x7f);
    u32 code = R16(0x700000 + idx * 2) & 0x3fff;
    u32 bank = mem[0x980000 + which * 8 + ((code >> 11) & 7)];
    u32 mg = (code & 0x7ff) | (bank << 11);
    u32 t = (mg & ~0x50u) | ((mg & 0x10) << 2) | ((mg & 0x40) >> 2);
    t %= 0x6000;
    u32 mo = (mg * 32 + (ypos & 15) * 2 + ((xpos & 15) >> 3)) % 0x100000;
    if (!(mroz[mo] & (0x80 >> (xpos & 7)))) return -1;
    return 0x1800 + color + groz[t * 256 + (ypos & 15) * 16 + (xpos & 15)];
}
static void roz_pixel(int which, int color, u32 xpos, u32 ypos, int y, int x, int prival) {
    int pen = roz_pen(which, color, xpos, ypos);
    if (pen >= 0) { fb[y][X(x)] = pen; pri[y][X(x)] = prival; }
}
/* Presentacion con las teles (16:9). El decorado del plano ROZ mide 864x672 (se ve reducido a 288 de ancho)
   y fuera de el no hay nada. En los laterales se pone pared: a la derecha los ladrillos de arriba a la derecha
   (se repiten cada 96x64, y 864 es multiplo de 96, asi que empalman con el borde) y a la izquierda la pared
   oscura de arriba a la izquierda, reflejada. Las baldas siguen de lado a lado: en las filas en que los dos
   bordes del decorado tienen el mismo color, se prolonga ese color. */
static int tv_scene;
#define TV_W 864
static int tv_side_pen(int color, u32 xp, u32 yp) {
    if (xp < TV_W) return -2;                           /* dentro del decorado: lo normal */
    int right = xp < (TV_W + 2048) / 2, d = right ? (int)xp - (TV_W - 1) : 2048 - (int)xp;
    int pl = roz_pen(0, color, 2, yp), pr = roz_pen(0, color, TV_W - 3, yp);
    if (pl >= 0 && pl == pr) return pl;
    if (right) return roz_pen(0, color, 768 + (d - 1) % 96, yp % 64);
    int u = (d - 1) % 176, v = yp % 336;
    if (u >= 88) u = 175 - u;
    if (v >= 168) v = 335 - v;
    return roz_pen(0, color, u, v);
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
                if (tv_scene && which == 0 && (x < 0 || x >= W)) {
                    int pen = tv_side_pen(color, xp & 2047, yp & 2047);
                    if (pen >= 0) { fb[y][X(x)] = pen; pri[y][X(x)] = prival; }
                    if (pen != -2) { cx += ixx; cy += ixy; continue; }
                }
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
    /* la escena de las teles se reconoce por su plano ROZ: atributos 0x0211 y losetas fijas en el mapa */
    tv_scene = ext && R16(0x740002) == 0x0211 && R16(0x700000) == 0x09b3 && R16(0x700000 + (2 * 128 + 50) * 2) == 0x0afc;
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
            if (c != 0xff) sprbuf[y][X(x)] = (prio & 15) << 12 | ((color * 256 + c) & 0xfff);
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
        /* una ventana que ocupa toda la pantalla se abre tambien a los laterales del 16:9
           (algunos escenarios, como el acuario, la ponen de 1 a 288 en vez de 0 a 287) */
        if (cx0 <= (ext ? 2 : 0)) cx0 = -ext; if (cx1 >= W - (ext ? 3 : 1)) cx1 = W - 1 + ext;
        if (cy0 < 0) cy0 = 0; if (cy1 > H - 1) cy1 = H - 1;
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
/* color que mas se repite en una tira de EDGE pixeles junto al borde del 4:3 (devuelve cuantos lo tienen) */
#define EDGE 16
static int edge_mode(const u8 *p, u32 *col) {
    int best = 0; *col = 0;
    for (int i = 0; i < EDGE; i++) {
        int n = 0;
        for (int k = 0; k < EDGE; k++)
            n += abs(p[i * 3] - p[k * 3]) + abs(p[i * 3 + 1] - p[k * 3 + 1]) + abs(p[i * 3 + 2] - p[k * 3 + 2]) < 24;
        if (n > best) { best = n; *col = (u32)p[i * 3] << 16 | p[i * 3 + 1] << 8 | p[i * 3 + 2]; }
    }
    return best;
}
static void render_frame(u8 *rgb) {
    clipx0 = (short)c116reg(0) - 0x4b; clipx1 = (short)c116reg(1) - 0x4b;
    clipy0 = (short)c116reg(2) - 0x21; clipy1 = (short)c116reg(3) - 0x21;
    /* en 16:9 la ventana se estira desde el centro: la pantalla completa ocupa los 384 y las cortinillas
       (un rectangulo que crece desde el centro) se abren tambien hasta los laterales */
    if (ext) {
        clipx0 = W / 2 - (W / 2 - clipx0) * (W / 2 + ext) / (W / 2);
        clipx1 = W / 2 + (clipx1 - W / 2) * (W / 2 + ext) / (W / 2);
        if (clipx0 < -ext) clipx0 = -ext; if (clipx1 > W + ext) clipx1 = W + ext;
    } else { if (clipx0 < 0) clipx0 = 0; if (clipx1 > W) clipx1 = W; }
    if (clipy0 < 0) clipy0 = 0; if (clipy1 > H) clipy1 = H;
    for (int y = 0; y < H; y++) for (int x = 0; x < FW; x++) { fb[y][x] = 0xffff; pri[y][x] = 0; }
    for (int p = 0; p < 16; p++) {
        draw_roz(p);
        if (!(p & 1))
            for (int i = 0; i < 6; i++) {
                u16 c = R16(0x6c0020 + i * 2);
                if ((c & 8)) continue;
                if ((c & 7) == (p >> 1)) draw_tmap(i, p);
            }
    }
    static u8 sh[H][FW]; memset(sh, 0, sizeof sh);
    for (int y = clipy0; y < clipy1; y++) for (int x = X(clipx0); x < X(clipx1); x++) {
        u16 s = sprbuf[y][x];
        if (s == 0xffff) continue;
        int sp = s >> 12; u16 c = s & 0xfff;
        if (sp >= pri[y][x] && (c & 0xff) != 0xff) {
            if (c == 0xffe) sh[y][x] = 1; else fb[y][x] = c;
            pri[y][x] = sp;
        }
    }
    int ow = W + 2 * ext;
    for (int y = 0; y < H; y++) for (int x = 0; x < ow; x++) {
        int fx = x + EXT - ext;
        u16 pen = fb[y][fx]; u8 r = 0, g = 0, b = 0;
        if (pen != 0xffff) {
            pen &= 0x1fff;
            u32 o = ((pen & 0x1800) << 2) | (pen & 0x7ff);
            r = mem[0x800000 + o]; g = mem[0x800800 + o]; b = mem[0x801000 + o];
        }
        if (sh[y][fx]) { r /= 2; g /= 2; b /= 2; }
        rgb[(y * ow + x) * 3] = r; rgb[(y * ow + x) * 3 + 1] = g; rgb[(y * ow + x) * 3 + 2] = b;
    }
    /* En 16:9 los laterales se ensenan durante los combates (hay energia en algun marcador): ahi el
       escenario los rellena de verdad, y la camara y los objetos estan parcheados para la vista ancha.
       En el resto de escenas (presentacion, titulo, eleccion de personaje, las teles) el juego solo dibuja
       288 pixeles: los laterales van en negro, salvo las filas en las que los dos bordes del 4:3 tienen el
       mismo color liso (la mesa o las baldas de las teles), que se prolongan con ese color para que la
       mesa siga de lado a lado y las teles queden en el centro. */
    if (ext && side_mode == 0 && !tv_scene && !(mem[0x208d59] || mem[0x208e0d])) {
        for (int y = 0; y < H; y++) {
            u8 *row = rgb + y * ow * 3;
            u32 cl, cr;
            int nl = edge_mode(row + EXT * 3, &cl), nr = edge_mode(row + (EXT + W - EDGE) * 3, &cr);
            int same = abs((int)(cl >> 16) - (int)(cr >> 16)) + abs((int)(cl >> 8 & 255) - (int)(cr >> 8 & 255)) + abs((int)(cl & 255) - (int)(cr & 255)) < 40;
            int lit = (cl >> 16) + (cl >> 8 & 255) + (cl & 255) > 60;
            if (nl * 10 >= EDGE * 6 && nr * 10 >= EDGE * 6 && same && lit) {
                for (int k = 0; k < EXT; k++) {
                    u8 *a = row + k * 3, *b = row + (EXT + W + k) * 3;
                    a[0] = cl >> 16; a[1] = cl >> 8; a[2] = cl; b[0] = cr >> 16; b[1] = cr >> 8; b[2] = cr;
                }
            } else {
                memset(row, 0, EXT * 3);
                memset(row + (EXT + W) * 3, 0, EXT * 3);
            }
        }
    }
}
static void vblank_copy(void) {
    /* buffer de sprites: renderiza lo copiado en el vblank anterior y luego copia */
    if (render_on) render_sprites();
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
    patches_ready = 0; patches_on = 0;
    if (ext) wide_patches(1);
    /* sonido: si falta algo, el juego va igual pero mudo */
    {
        char e2[64];
        c75bios = zget(&z, "c75.bin", 0x4000, e2, sizeof e2);
        c75data = zget(&z, "ou1spr0.5b", 0x80000, e2, sizeof e2);
        voice = zget(&z, "ou1voi0.6n", 0x200000, e2, sizeof e2);
        snd_ok = c75bios && c75data && voice;
    }
fin:
    mz_zip_reader_end(&z);
    return ok;
}

/* ---------- interfaz ---------- */
void nb2_reset(void) {
    m68k_init(); m68k_set_cpu_type(M68K_CPU_TYPE_68EC020); m68k_pulse_reset();
    mcu_run = 1; mcu_spin = 0; mcu_clock = snd_pos = 0;
    if (snd_ok) { m37710_set_internal_rom(c75bios); m37710_reset(); c352_init(voice, 0x200000); }
}
int nb2_has_sound(void) { return snd_ok; }
const short *nb2_audio(int *n) { *n = audio_n; return audio_buf; }
void nb2_set_inputs(unsigned char a, unsigned char b, unsigned char c) {
    p1 = a; p2 = b; misc = c;
    if (b & NB2_START) human2 = 1;   /* quien pulsa START juega; el 1P se da por humano */
}
void nb2_coin(void) {
    u16 v;
    if (snd_ok) { coin_frames = 4; return; }
    v = R16(0x20601e) + 1;
    mem[0x20601e] = v >> 8; mem[0x20601f] = v & 0xff;
}
/* ---------- trucos ----------
   Direcciones halladas con pruebas en el servidor (la energia, de la base de trucos de MAME 2003-Plus):
   - Estructura de cada luchador, contada desde su byte de energia: 1P 0x208D59, 2P 0x208E0D (0xF0 = llena).
     +0x0C arma que lleva (0 = ninguna), +0x0E municion que le queda.
   - Orden de combates en 0x20807C: 7 parejas (estado, rival), estado 0xFF pendiente y 0x00 vencido.
     0x20808F = numero de combate. 0x20807B: rival actual en el nibble alto (el bajo es el personaje del 1P);
     cambia en el mismo fotograma en que se elige el combate y el escenario se carga 25-70 fotogramas despues. */
static int cheat_flags, cheat_stage = -1;
static u8 ammo_max[2], ammo_weapon[2], energy_prev[2], ammo_still[2], ammo_last[2];
static int last_7b = -1;
static void apply_cheats(void) {
    static const u32 en[2] = {0x208d59, 0x208e0d};
    int human[2] = {1, human2};
    for (int i = 0; i < 2; i++) {
        u8 *e = &mem[en[i]], *wpn = e + 0x0c, *ammo = e + 0x0e;
        if (human[i]) {
            if ((cheat_flags & NB2_CHEAT_INVULN) && *e > 0 && *e < 0xf0) *e = 0xf0;
            if (cheat_flags & NB2_CHEAT_AMMO) {
                /* se recuerda la municion con la que se cogio el arma y se repone, pero solo cuando lleva
                   medio segundo sin cambiar: asi no se toca el contador en mitad de una rafaga o un disparo
                   (con algunas armas el juego espera a que baje para terminar la accion). Las armas de pocas
                   balas (pistola 7, lanzacohetes 3...) se reponen ya al llegar a la ultima. */
                if (*wpn != ammo_weapon[i] || *ammo > ammo_max[i]) { ammo_weapon[i] = *wpn; ammo_max[i] = *ammo; }
                if (*ammo != ammo_last[i]) { ammo_last[i] = *ammo; ammo_still[i] = 0; }
                else if (ammo_still[i] < 255) ammo_still[i]++;
                if (*wpn && *ammo && *ammo < ammo_max[i] && (ammo_still[i] >= 30 || (*ammo == 1 && ammo_max[i] <= 10))) { *ammo = ammo_max[i]; ammo_last[i] = *ammo; }
            }
        } else if ((cheat_flags & NB2_CHEAT_ONEHIT) && energy_prev[i] && *e && *e < energy_prev[i]) *e = 0;
        energy_prev[i] = *e;
    }
    /* escenario: cuando el juego elige rival, se pone en su lugar el pedido si sigue pendiente */
    if (mem[0x20807b] != last_7b && cheat_stage >= 0 && mem[0x20807d] != 0) {
        int cur = mem[0x20807b] >> 4, idx = mem[0x20808f], pos = -1;
        for (int k = 0; k < 7; k++) if (mem[0x20807d + k * 2] == cheat_stage && mem[0x20807c + k * 2] == 0xff) pos = k;
        if (cur != cheat_stage && pos >= 0 && idx < 7 && mem[0x20807d + idx * 2] == cur && mem[0x20807c + idx * 2] == 0xff) {
            mem[0x20807d + pos * 2] = cur; mem[0x20807d + idx * 2] = cheat_stage;
            mem[0x20807b] = (cheat_stage << 4) | (mem[0x20807b] & 15);
        }
    }
    last_7b = mem[0x20807b];
}
void nb2_set_cheats(int flags, int stage) { cheat_flags = flags; cheat_stage = stage; }

const unsigned char *nb2_frame(void) {
    enum { LINES = 264, CPL = 403200 / 264 };
    int posline = (int)c116reg(5) - 32;
    unsigned long long f0 = mcu_clock;
    audio_n = 0;
    frame_events = 0;
    if (snd_ok) { m37710_assert_irq(M37710_LINE_IRQ0); m37710_assert_irq(M37710_LINE_IRQ2); }
    for (int ln = 0; ln < LINES; ln++) {
        if (snd_ok) {
            /* 68020 y C75 intercalados en trozos cortos: comparten RAM */
            enum { SL = 8 };
            for (int k = 0; k < SL; k++) {
                long long end = (long long)(ln * SL + k + 1) * MCU_PER_FRAME / (LINES * SL);
                m68k_execute(CPL / SL);
                mcu_run_for((int)(end - (long long)(mcu_clock - f0)));
            }
        } else m68k_execute(CPL);
        if (ln == 224) { vblank_copy(); vbl_pend = 1; update_irq(); if (render_on) render_frame(frame_rgb); }
        if (ln == posline) { pos_pend = 1; update_irq(); }
    }
    if (coin_frames > 0) coin_frames--;
    last_events = frame_events;
    apply_cheats();
    if (snd_ok) snd_sync();
    return frame_rgb;
}
unsigned char *nb2_eeprom(void) { return mem + 0xa00000; }
/* ---------- parches del programa para 16:9 ----------
   El juego decide que objetos manda a la pantalla proyectandolos en 3D con un semiancho de 144 pixeles
   (0x90, la mitad de 288). En 16:9 se usa 192 (0xC0) para que no aparezcan y desaparezcan en los laterales.
   Rutina de visibilidad en 0xB390 (ver ESTADO.md). Los parches se ponen y se quitan al cambiar de formato;
   la ROM original se guarda para poder volver al 4:3 exacto. */
#define PATCH_AREA 0x0ff000      /* hueco libre (0xFF) al final del programa */
typedef struct { u32 addr; u8 len; u8 bytes[24]; u8 orig[24]; } patch_t;
static patch_t patches[] = {
    /* objetos en el plano principal: |dx| - ancho/2 <= dz  ->  <= dz*4/3. Se salta a una rutina nueva. */
    {0x00b418, 10, {0x4e, 0xb9, 0x00, 0x0f, 0xf0, 0x00, 0x4e, 0x71, 0x4e, 0x71}},
    {PATCH_AREA, 22, {0x32, 0x28, 0x00, 0x86,    /* move.w ($86,A0),D1 */
                      0xe2, 0x49,                /* lsr.w #1,D1 */
                      0x90, 0x41,                /* sub.w D1,D0 */
                      0x32, 0x02,                /* move.w D2,D1 */
                      0x48, 0xc1,                /* ext.l D1 */
                      0xe5, 0x89,                /* lsl.l #2,D1 */
                      0x83, 0xfc, 0x00, 0x03,    /* divs.w #3,D1 */
                      0xb0, 0x41,                /* cmp.w D1,D0 */
                      0x4e, 0x75}},              /* rts */
    /* objetos en otros planos: |x pantalla| <= ancho/2 + 0x90  ->  + 0xC0 */
    {0x00b50e, 4, {0x06, 0x42, 0x00, 0xc0}},     /* addi.w #$c0,D2 */
    /* camara (rutina 0x2786): el semiancho visible que usa para no salirse del escenario es 0x90 - z;
       en 16:9 es 4/3 de eso, asi la vista ancha nunca ensena lo que hay fuera del escenario */
    {0x00278e, 8, {0x4e, 0xb9, 0x00, 0x0f, 0xf0, 0x20, 0x4e, 0x71}},
    {PATCH_AREA + 0x20, 18, {0x30, 0x3c, 0x00, 0x90,    /* move.w #$90,D0 */
                             0x90, 0x6d, 0x00, 0x1c,    /* sub.w ($1c,A5),D0 */
                             0x48, 0xc0,                /* ext.l D0 */
                             0xe5, 0x88,                /* lsl.l #2,D0 */
                             0x81, 0xfc, 0x00, 0x03,    /* divs.w #3,D0 */
                             0x4e, 0x75}},              /* rts */
};
static void wide_patches(int on) {
    if (!patches_ready) {
        for (unsigned i = 0; i < sizeof patches / sizeof patches[0]; i++)
            memcpy(patches[i].orig, mem + patches[i].addr, patches[i].len);
        patches_ready = 1;
    }
    if (on == patches_on) return;
    for (unsigned i = 0; i < sizeof patches / sizeof patches[0]; i++)
        memcpy(mem + patches[i].addr, on ? patches[i].bytes : patches[i].orig, patches[i].len);
    patches_on = on;
}
void nb2_set_wide(int on) { side_mode = on == 2; ext = on ? EXT : 0; if (gobj) wide_patches(on); }
int nb2_width(void) { return W + 2 * ext; }
unsigned char *nb2_mem(void) { return mem; }
void nb2_set_render(int on) { render_on = on; }
unsigned nb2_events(void) { return last_events; }

/* ---------- guardar / cargar estado ----------
   Todo lo que cambia mientras se juega: RAM del juego (memoria compartida con el C75, video, paleta,
   EEPROM), las tres CPU/chips y las variables de este archivo. La ROM no se guarda.
   Zonas de RAM sacadas de lo que escribe el juego en una partida larga, redondeadas al bloque de hardware. */
static const u32 ram_zone[][2] = {
    {0x200000, 0x80000}, {0x600000, 0x21000}, {0x640000, 0x1000}, {0x680000, 0x10000}, {0x6c0000, 0x1000},
    {0x700000, 0x20000}, {0x740000, 0x1000}, {0x800000, 0x8000}, {0x900000, 0x1000}, {0x940000, 0x1000},
    {0x980000, 0x1000}, {0xa00000, 0x1000}, {0xc00000, 0x1000},
};
#define NB2_STATE_VARS(X) \
    X(human2) X(vbl_level) X(pos_level) X(vbl_pend) X(pos_pend) X(rng) X(lastkey) X(mcu_run) X(mcu_spin) \
    X(coin_frames) X(port6) X(mcu_clock) X(snd_pos) X(sprbank_copy) X(sprram_copy) X(ammo_still) \
    X(ammo_max) X(ammo_weapon) X(energy_prev) X(ammo_last) X(last_7b)
#define ST_SZ(v) + sizeof(v)
#define ST_PUT(v) memcpy(p, &v, sizeof(v)); p += sizeof(v);
#define ST_GET(v) memcpy(&v, p, sizeof(v)); p += sizeof(v);
static size_t raw_size(void) {
    size_t n = 0;
    for (unsigned i = 0; i < sizeof ram_zone / sizeof ram_zone[0]; i++) n += ram_zone[i][1];
    return n + m68k_state_size() + m37710_state_size() + c352_state_size() + 0 NB2_STATE_VARS(ST_SZ);
}
static u8 *raw_buf;
/* cabecera: "OFX1", tamano sin comprimir y los tamanos de cada chip (si no coinciden, es de otra version) */
typedef struct { char magic[4]; u32 raw, m68k, mcu, snd; } st_head;
unsigned char *nb2_state_save(size_t *len, int level) {
    size_t rs = raw_size();
    mz_ulong clen;
    u8 *p, *out;
    st_head h = {{'O', 'F', 'X', '1'}, 0, 0, 0, 0};
    if (!raw_buf && !(raw_buf = malloc(rs))) return NULL;
    p = raw_buf;
    for (unsigned i = 0; i < sizeof ram_zone / sizeof ram_zone[0]; i++) {
        memcpy(p, mem + ram_zone[i][0], ram_zone[i][1]); p += ram_zone[i][1];
    }
    m68k_state_save(p); p += m68k_state_size();
    m37710_state_save(p); p += m37710_state_size();
    c352_state_save(p); p += c352_state_size();
    NB2_STATE_VARS(ST_PUT)
    clen = mz_compressBound((mz_ulong)rs);
    out = malloc(sizeof h + clen);
    if (!out) return NULL;
    if (mz_compress2(out + sizeof h, &clen, raw_buf, (mz_ulong)rs, level) != MZ_OK) { free(out); return NULL; }
    h.raw = (u32)rs; h.m68k = (u32)m68k_state_size(); h.mcu = (u32)m37710_state_size(); h.snd = (u32)c352_state_size();
    memcpy(out, &h, sizeof h);
    *len = sizeof h + clen;
    return realloc(out, *len);
}
int nb2_state_load(const unsigned char *buf, size_t len) {
    size_t rs = raw_size();
    mz_ulong ulen = (mz_ulong)rs;
    const u8 *p;
    st_head h;
    if (len < sizeof h) return 0;
    memcpy(&h, buf, sizeof h);
    if (memcmp(h.magic, "OFX1", 4) || h.raw != rs || h.m68k != m68k_state_size() ||
        h.mcu != m37710_state_size() || h.snd != c352_state_size()) return 0;
    if (!raw_buf && !(raw_buf = malloc(rs))) return 0;
    if (mz_uncompress(raw_buf, &ulen, buf + sizeof h, (mz_ulong)(len - sizeof h)) != MZ_OK || ulen != rs) return 0;
    p = raw_buf;
    for (unsigned i = 0; i < sizeof ram_zone / sizeof ram_zone[0]; i++) {
        memcpy(mem + ram_zone[i][0], p, ram_zone[i][1]); p += ram_zone[i][1];
    }
    m68k_state_load(p); p += m68k_state_size();
    m37710_state_load(p); p += m37710_state_size();
    c352_state_load(p); p += c352_state_size();
    NB2_STATE_VARS(ST_GET)
    audio_n = 0;
    return 1;
}
