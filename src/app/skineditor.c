/* Editor de ropa. Ver skineditor.h.
   Mando: arriba/abajo eligen prenda (y bajan a los botones), izquierda/derecha o L1/R1 cambian de personaje, A abre los
   colores de la prenda (dentro: la cruceta mueve, A pone el color, X vuelve al original, B cancela), X deja la prenda
   como la recreativa, Y cambia el nombre del pack, B sale (lo hecho ya esta guardado).
   Tactil: se toca la prenda, el color, las flechas del nombre del personaje y los botones.
   Lo que se cambia se guarda solo en el archivo del pack (ropas/<nombre>.txt). */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "skineditor.h"
#include "skinpacks.h"
#include "launcher.h"
#include "menu.h"
#include "nb2.h"
#include "skin_poses.h"

#define OW 480
enum { B_NEW, B_OPEN, B_USE, B_IMPORT, B_EXPORT, B_COUNT };
static struct {
    int open, closed_flag;
    int ch, part, focus_btn, bsel;         /* focus_btn: el cursor esta en la fila de botones */
    skin_pack ed;                          /* el pack que se edita */
    int picking, pc;                       /* paleta de colores abierta; color bajo el cursor */
    unsigned pick_old; int pick_old_set;
    int listing, lsel, lfirst;             /* lista de packs (Abrir) */
    int naming; char name_buf[64];          /* escribiendo el nombre */
    unsigned waiting_stamp; int waiting; Uint32 poll;
    char msg[160]; Uint32 msg_until;
    Uint32 pulse_from;                     /* la prenda elegida parpadea un poco al elegirla */
    SDL_Texture *ui; uint32_t *uipx; int uh;
    SDL_Texture *spr; int sw, sh;
    unsigned char sprpx[96 * 96 * 4];
} E;

/* ---------- colores de la paleta: 6 filas de 16 ---------- */
#define PCOLS 16
#define PROWS 6
static unsigned hsv(double h, double s, double v) {
    double c = v * s, x = c * (1 - fabs(fmod(h / 60.0, 2) - 1)), m = v - c, r, g, b;
    if (h < 60) { r = c; g = x; b = 0; } else if (h < 120) { r = x; g = c; b = 0; } else if (h < 180) { r = 0; g = c; b = x; }
    else if (h < 240) { r = 0; g = x; b = c; } else if (h < 300) { r = x; g = 0; b = c; } else { r = c; g = 0; b = x; }
    return (unsigned)((r + m) * 255 + 0.5) << 16 | (unsigned)((g + m) * 255 + 0.5) << 8 | (unsigned)((b + m) * 255 + 0.5);
}
static unsigned swatch(int i) {
    static const unsigned extra[16] = {0xFFE0C8, 0xF2C8A0, 0xE0A878, 0xC68A5E, 0xA0683E, 0x7A4A2A, 0x553220, 0x3A2216,   /* pieles */
                                       0xF0D060, 0xC8A040, 0x8A7040, 0x6B6B3A, 0x3E4A2A, 0x1E2A48, 0x5A1020, 0x2A1A30};  /* rubios, caqui, oliva, marino, granate */
    int r = i / PCOLS, c = i % PCOLS;
    if (r == 0) { int v = 255 - c * 17; return (unsigned)v << 16 | v << 8 | v; }
    if (r == 5) return extra[c];
    {
        static const double S[5] = {0, 0.35, 0.85, 0.85, 0.85}, V[5] = {0, 1.0, 0.95, 0.65, 0.38};
        return hsv(c * 22.5, S[r], V[r]);
    }
}

static const char *lt(const char *es, const char *en) { return cfg.lang == LANG_EN ? en : es; }
static void say(const char *s) { snprintf(E.msg, sizeof E.msg, "%s", s); E.msg_until = SDL_GetTicks() + 2600; }

static void save_pack(void) {
    char p[800];
    if (!E.ed.name[0]) return;
    packs_path(E.ed.name, p, sizeof p);
    skin_pack_save(&E.ed, p);
}
static void new_pack(void) {
    char nm[64];
    for (int i = 1; i < 1000; i++) {
        snprintf(nm, sizeof nm, "%s %d", lt("Mi ropa", "My outfit"), i);
        if (packs_find(nm) < 0) break;
    }
    skin_pack_clear(&E.ed);
    snprintf(E.ed.name, sizeof E.ed.name, "%s", nm);
    save_pack(); packs_scan();
}
static void load_pack(int i) {
    const skin_pack *p = packs_get(i);
    unsigned v = E.ed.version;
    if (!p) return;
    E.ed = *p; E.ed.version = v + 1;
}
void skined_open(SDL_Renderer *r) {
    (void)r;
    memset(&E.msg, 0, sizeof E.msg);
    E.open = 1; E.picking = E.listing = E.naming = E.waiting = 0; E.focus_btn = 0; E.part = 0;
    packs_scan();
    /* se abre el pack que lleva puesto el jugador 1; si no, el ultimo; si no hay ninguno, uno nuevo */
    {
        int i = packs_find(cfg.outfit_pack[0]);
        if (i < 0 && packs_count()) i = packs_count() - 1;
        if (i >= 0) load_pack(i); else new_pack();
    }
    E.pulse_from = SDL_GetTicks();
}
int skined_active(void) { return E.open; }
int skined_closed(void) { int c = E.closed_flag; E.closed_flag = 0; return c; }
static void close_editor(void) {
    save_pack();
    if (E.naming) SDL_StopTextInput();
    E.open = 0; E.closed_flag = 1;
}

/* ---------- importar / exportar (Android: GameActivity, mensaje 0x8007 con 3 = importar, 4 = exportar) ---------- */
static void import_pack(void) {
#ifdef __ANDROID__
    E.waiting_stamp = packs_stamp(); E.waiting = 1; E.poll = SDL_GetTicks();
    SDL_AndroidSendMessage(0x8007, 3);
    say(lt("Elige el archivo .txt del pack de ropa", "Choose the outfit pack .txt file"));
#else
    { char d[600], m[700]; packs_dir(d, sizeof d); snprintf(m, sizeof m, lt("Copia el .txt en %s", "Copy the .txt into %s"), d); say(m); }
#endif
}
static void export_pack(void) {
    char src[800];
    save_pack();
    packs_path(E.ed.name, src, sizeof src);
#ifdef __ANDROID__
    {   /* copia fija para la app de Java y el nombre que propone al guardar */
        char p[700]; FILE *f;
        snprintf(p, sizeof p, "%s/ropa_exportar.txt", menu_save_dir());
        if ((f = fopen(p, "w"))) {
            FILE *in = fopen(src, "r"); char buf[512]; size_t n;
            if (in) { while ((n = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, n, f); fclose(in); }
            fclose(f);
        }
        snprintf(p, sizeof p, "%s/ropa_exportar.nombre", menu_save_dir());
        if ((f = fopen(p, "w"))) { fprintf(f, "%s.txt", E.ed.name); fclose(f); }
        SDL_AndroidSendMessage(0x8007, 4);
        say(lt("Elige dónde guardar el pack", "Choose where to save the pack"));
    }
#else
    { char m[900]; snprintf(m, sizeof m, lt("Guardado en %s", "Saved to %s"), src); say(m); }
#endif
}
static void check_waiting(void) {
    if (!E.waiting || SDL_GetTicks() - E.poll < 500) return;
    E.poll = SDL_GetTicks();
    if (packs_stamp() == E.waiting_stamp) return;
    {   /* ha llegado: se abre el que no estaba antes */
        char before[PACKS_MAX][64]; int nb = packs_count();
        for (int i = 0; i < nb; i++) snprintf(before[i], 64, "%s", packs_name(i));
        save_pack(); packs_scan(); E.waiting = 0;
        for (int i = 0; i < packs_count(); i++) {
            int was = 0;
            for (int j = 0; j < nb; j++) if (!strcmp(before[j], packs_name(i))) was = 1;
            if (!was) { char m[120]; load_pack(i); snprintf(m, sizeof m, lt("Importado: %s", "Imported: %s"), packs_name(i)); say(m); break; }
        }
    }
}
void skined_resume(void) { check_waiting(); }

static void use_pack(void) {
    save_pack(); packs_scan();
    snprintf(cfg.outfit_pack[0], sizeof cfg.outfit_pack[0], "%s", E.ed.name);
    { char m[120]; snprintf(m, sizeof m, lt("\"%s\": ropa del jugador 1", "\"%s\": player 1 outfit"), E.ed.name); say(m); }
}
static void rename_pack(const char *nm) {
    char a[800], b[800], clean[64]; int n = 0;
    for (const char *s = nm; *s && n < 40; s++) if (!strchr("/\\:*?\"<>|", *s) && (unsigned char)*s >= 32) clean[n++] = *s;
    while (n && clean[n - 1] == ' ') n--;
    clean[n] = 0;
    if (!n || !strcmp(clean, E.ed.name)) return;
    if (packs_find(clean) >= 0) { say(lt("Ya hay un pack con ese nombre", "There is already a pack with that name")); return; }
    packs_path(E.ed.name, a, sizeof a); packs_path(clean, b, sizeof b);
    save_pack();
    if (rename(a, b) != 0) { say(lt("No se pudo cambiar el nombre", "Could not rename")); return; }
    if (!strcmp(cfg.outfit_pack[0], E.ed.name)) snprintf(cfg.outfit_pack[0], 64, "%s", clean);
    if (!strcmp(cfg.outfit_pack[1], E.ed.name)) snprintf(cfg.outfit_pack[1], 64, "%s", clean);
    snprintf(E.ed.name, sizeof E.ed.name, "%s", clean);
    packs_scan();
}

/* ---------- personaje dibujado ---------- */
static const unsigned char *rom_pal(int ch) {
    const unsigned char *m = nb2_mem();
    return m ? m + skin_chars[ch].pal_rom : NULL;
}
static void build_sprite(SDL_Renderer *r) {
    const pose_t *P = &poses[E.ch];
    const unsigned char *base = rom_pal(E.ch);
    unsigned char out[768], chg[256];
    skin_pack tmp = E.ed;
    int sel = -1;
    double pulse = 0;
    if (!base || !nb2_obj_tile(0)) return;
    if (E.picking) { tmp.set[E.ch][E.part] = 1; tmp.color[E.ch][E.part] = swatch(E.pc); }
    skin_apply(&tmp, E.ch, base, out, chg);
    if (!E.picking && !E.focus_btn) {
        Uint32 t = SDL_GetTicks() - E.pulse_from;
        if (t < 1400) { sel = E.part; pulse = 0.5 * (1 - cos(t / 1400.0 * 2 * M_PI * 2)) * 0.6; }
    }
    E.sw = P->w; E.sh = P->h;
    memset(E.sprpx, 0, sizeof E.sprpx);
    for (int k = 0; k < P->n; k++) {
        const pose_tile *t = &P->t[k];
        const unsigned char *src = nb2_obj_tile(t->code);
        for (int y = 0; y < 16; y++) for (int x = 0; x < 16; x++) {
            int px = t->x + x, py = t->y + y;
            unsigned char c = src[(t->fy ? 15 - y : y) * 16 + (t->fx ? 15 - x : x)];
            unsigned char *d;
            if (c == 0xff || px < 0 || py < 0 || px >= 96 || py >= 96) continue;
            d = E.sprpx + (py * 96 + px) * 4;
            d[0] = out[c * 3]; d[1] = out[c * 3 + 1]; d[2] = out[c * 3 + 2]; d[3] = 255;
            if (sel >= 0 && skin_part_of(E.ch, c) == sel)
                for (int j = 0; j < 3; j++) d[j] = (unsigned char)(d[j] + (255 - d[j]) * pulse);
        }
    }
    if (!E.spr) {
        E.spr = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 96, 96);
        if (E.spr) { SDL_SetTextureBlendMode(E.spr, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(E.spr, SDL_ScaleModeNearest); }
    }
    if (E.spr) SDL_UpdateTexture(E.spr, NULL, E.sprpx, 96 * 4);
}

/* ---------- colocacion (en el lienzo de OW de ancho) ---------- */
#define PX 6
#define PY 28
#define PW 160
#define RX 172
#define RW 302
static int rows_y(void) { return PY + 5; }
static int row_h(void) { int n = skin_chars[E.ch].nparts, avail = E.uh - 74 - rows_y(); int h = avail / (n ? n : 1); return h > 18 ? 18 : h < 12 ? 12 : h; }
static int btn_y(void) { return E.uh - 26; }
static SDL_Rect btn_rect(int i) { int w = (OW - 16 - 4 * 6) / B_COUNT; return (SDL_Rect){8 + i * (w + 6), btn_y(), w, 20}; }
static const char *btn_label(int i) {
    switch (i) {
    case B_NEW: return lt("Nuevo", "New");
    case B_OPEN: return lt("Abrir", "Open");
    case B_USE: return lt("Ponérmela", "Wear it");
    case B_IMPORT: return lt("Importar", "Import");
    default: return lt("Exportar", "Export");
    }
}
#define CELL 15
static int pick_x0(void) { return RX + (RW - PCOLS * CELL) / 2; }
static int pick_y0(void) { return PY + 24; }

static void ui_begin(SDL_Renderer *r, int ww, int wh) {
    int uh = OW * wh / (ww ? ww : 1);
    if (uh < 250) uh = 250;
    if (uh > 600) uh = 600;
    if (uh != E.uh || !E.ui) {
        if (E.ui) SDL_DestroyTexture(E.ui);
        free(E.uipx);
        E.uh = uh; E.uipx = malloc((size_t)OW * uh * 4);
        E.ui = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, OW, uh);
        if (E.ui) { SDL_SetTextureBlendMode(E.ui, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(E.ui, SDL_ScaleModeNearest); }
    }
    if (E.uipx) memset(E.uipx, 0, (size_t)OW * E.uh * 4);
}
#define BOX(x, y, w, h, c, a) menu_ext_rect(E.uipx, OW, E.uh, x, y, w, h, c, a)
#define TXT(x, y, s, c) menu_ext_text(E.uipx, OW, E.uh, x, y, s, c, 0)
#define TXTB(x, y, s, c) menu_ext_text(E.uipx, OW, E.uh, x, y, s, c, 1)
#define TW(s) menu_ext_text_w(s, 0)
#define TWB(s) menu_ext_text_w(s, 1)
static void frame(int x, int y, int w, int h, unsigned c) { BOX(x, y, w, 1, c, 255); BOX(x, y + h - 1, w, 1, c, 255); BOX(x, y, 1, h, c, 255); BOX(x + w - 1, y, 1, h, c, 255); }
static void swatch_box(int x, int y, int w, int h, unsigned col, int set) {
    BOX(x - 1, y - 1, w + 2, h + 2, 0x000000, 255);
    BOX(x, y, w, h, col, 255);
    if (!set) for (int i = 0; i < w; i += 4) BOX(x + i, y + h - 2, 2, 2, 0x000000, 160);   /* original: rayita */
}
static unsigned part_color(int ch, int k) {
    const unsigned char *b = rom_pal(ch);
    if (E.ed.set[ch][k]) return E.ed.color[ch][k];
    return b ? skin_part_color(ch, k, b) : 0x808080;
}

void skined_draw(SDL_Renderer *r) {
    int ww, wh;
    if (!E.open) return;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    check_waiting();
    launcher_cover(r, (SDL_Rect){0, 0, ww, wh}, 70);
    ui_begin(r, ww, wh);
    if (!E.ui || !E.uipx) return;
    {
        const skin_char *C = &skin_chars[E.ch];
        char t[160];
        int k = (int)(SDL_GetTicks() / 400) & 1;
        /* cabecera */
        BOX(0, 0, OW, 22, 0x0a0812, 220); BOX(0, 22, OW, 1, 0xe03c8c, 255);
        TXT(10, 3, lt("EDITOR DE ROPA", "OUTFIT EDITOR"), 0xffd040);
        snprintf(t, sizeof t, "%s: %s%s", lt("Pack", "Pack"), E.naming ? E.name_buf : E.ed.name, E.naming && k ? "_" : "");
        TXT(OW - 10 - TW(t), 3, t, E.naming ? 0xffd040 : 0xffffff);
        /* personaje */
        BOX(PX, PY, PW, btn_y() - PY - 8, 0x0a0812, 190); frame(PX, PY, PW, btn_y() - PY - 8, 0x5a3a60);
        TXT(PX + 6, PY + 4, "<", 0xffd040); TXT(PX + PW - 13, PY + 4, ">", 0xffd040);
        TXT(PX + (PW - TW(C->name)) / 2, PY + 4, C->name, 0xffffff);
        BOX(PX + 4, PY + 21, PW - 8, 1, 0x5a3a60, 255);
        /* prendas */
        BOX(RX, PY, RW, btn_y() - PY - 8, 0x0a0812, 190); frame(RX, PY, RW, btn_y() - PY - 8, 0x5a3a60);
        if (!E.picking && !E.listing) {
            int y = rows_y(), h = row_h();
            for (int i = 0; i < C->nparts; i++, y += h) {
                int on = !E.focus_btn && i == E.part;
                if (on) BOX(RX + 3, y - 1, RW - 6, h, 0xe03c8c, 110);
                swatch_box(RX + 10, y + (h - 11) / 2, 26, 10, part_color(E.ch, i), E.ed.set[E.ch][i]);
                {   /* "original" a la derecha si cabe sin pisar el nombre */
                    const char *nm = cfg.lang == LANG_EN ? C->parts[i].en : C->parts[i].es, *og = lt("original", "original");
                    int ty = y + (h - 14) / 2;
                    TXT(RX + 42, ty, nm, on ? 0xffffff : 0xd0c8d8);
                    if (!E.ed.set[E.ch][i] && RX + 42 + TW(nm) + 10 < RX + RW - 8 - TW(og)) TXT(RX + RW - 8 - TW(og), ty, og, 0x807888);
                }
            }
            {   /* ayuda: en una linea si cabe, si no en dos */
                const char *h1 = lt("A color   X original", "A color   X original"), *h2 = lt("Y nombre   B salir", "Y name   B exit");
                char hb[96]; snprintf(hb, sizeof hb, "%s   %s", h1, h2);
                if (TW(hb) <= RW - 12) TXT(RX + (RW - TW(hb)) / 2, btn_y() - 28, hb, 0x9890a0);
                else { TXT(RX + (RW - TW(h1)) / 2, btn_y() - 42, h1, 0x9890a0); TXT(RX + (RW - TW(h2)) / 2, btn_y() - 28, h2, 0x9890a0); }
            }
        } else if (E.picking) {
            int x0 = pick_x0(), y0 = pick_y0();
            snprintf(t, sizeof t, "%s: %s", lt("Color", "Color"), cfg.lang == LANG_EN ? C->parts[E.part].en : C->parts[E.part].es);
            TXT(RX + 8, PY + 6, t, 0xffd040);
            for (int i = 0; i < PCOLS * PROWS; i++) {
                int cx = x0 + (i % PCOLS) * CELL, cy = y0 + (i / PCOLS) * CELL;
                BOX(cx + 1, cy + 1, CELL - 2, CELL - 2, swatch(i), 255);
                if (i == E.pc) { frame(cx - 1, cy - 1, CELL + 2, CELL + 2, k ? 0xffffff : 0xffd040); frame(cx, cy, CELL, CELL, 0x000000); }
            }
            {
                int yy = y0 + PROWS * CELL + 8;
                SDL_Rect o = {RX + 10, yy, 110, 18};
                BOX(o.x, o.y, o.w, o.h, 0x2a2430, 255);
                TXT(o.x + (o.w - TW(lt("X: Original", "X: Original"))) / 2, o.y + 4, lt("X: Original", "X: Original"), 0xffffff);
                BOX(RX + RW - 120, yy, 110, 18, 0x8a1a4a, 255);
                TXT(RX + RW - 120 + (110 - TW(lt("A: Poner", "A: Apply"))) / 2, yy + 4, lt("A: Poner", "A: Apply"), 0xffffff);
                TXT(RX + 8, yy + 26, lt("B: dejarla como estaba", "B: leave it as it was"), 0x9890a0);
            }
        } else {   /* lista de packs */
            int n = packs_count(), vis = (btn_y() - PY - 40) / 16, y = PY + 26;
            TXT(RX + 8, PY + 6, lt("Abrir pack (A abrir, B volver)", "Open pack (A open, B back)"), 0xffd040);
            if (E.lsel < E.lfirst) E.lfirst = E.lsel;
            if (E.lsel >= E.lfirst + vis) E.lfirst = E.lsel - vis + 1;
            for (int i = E.lfirst; i < n && i < E.lfirst + vis; i++, y += 16) {
                if (i == E.lsel) BOX(RX + 3, y - 2, RW - 6, 15, 0xe03c8c, 110);
                TXT(RX + 12, y, packs_name(i), !strcmp(packs_name(i), E.ed.name) ? 0xffd040 : 0xffffff);
            }
            if (!n) TXT(RX + 12, y, lt("No hay packs", "No packs"), 0x9890a0);
        }
        /* botones */
        for (int i = 0; i < B_COUNT; i++) {
            SDL_Rect b = btn_rect(i);
            int on = E.focus_btn && E.bsel == i && !E.picking && !E.listing;
            BOX(b.x, b.y, b.w, b.h, on ? 0xe03c8c : 0x2a2430, on ? 255 : 230);
            frame(b.x, b.y, b.w, b.h, on ? 0xffffff : 0x5a3a60);
            TXT(b.x + (b.w - TW(btn_label(i))) / 2, b.y + 2, btn_label(i), 0xffffff);
        }
        if ((Sint32)(SDL_GetTicks() - E.msg_until) < 0 && E.msg[0]) {
            int w = TW(E.msg) + 20; if (w > OW - 10) w = OW - 10;
            BOX((OW - w) / 2, btn_y() - 46, w, 18, 0x000000, 220); frame((OW - w) / 2, btn_y() - 46, w, 18, 0xffd040);
            TXT((OW - w) / 2 + 10, btn_y() - 42, E.msg, 0xffffff);
        }
        if (!nb2_obj_tile(0)) TXT(PX + 10, PY + 60, lt("Falta la ROM", "ROM missing"), 0xff6060);
    }
    SDL_UpdateTexture(E.ui, NULL, E.uipx, OW * 4);
    SDL_RenderCopy(r, E.ui, NULL, NULL);
    /* el personaje, a escala entera, de pie sobre el fondo del panel */
    build_sprite(r);
    if (E.spr && E.sw) {
        double kx = (double)ww / OW, ky = (double)wh / E.uh;
        int ax = (int)((PX + 8) * kx), ay = (int)((PY + 26) * ky), aw = (int)((PW - 16) * kx), ah = (int)((btn_y() - 8 - PY - 34) * ky);
        int s = aw / E.sw < ah / E.sh ? aw / E.sw : ah / E.sh;
        SDL_Rect src = {0, 0, E.sw, E.sh}, dst;
        if (s < 1) s = 1;
        dst = (SDL_Rect){ax + (aw - E.sw * s) / 2, ay + ah - E.sh * s, E.sw * s, E.sh * s};
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 90);
        { SDL_Rect sh = {dst.x - s * 4, ay + ah - s * 3, dst.w + s * 8, s * 4}; SDL_RenderFillRect(r, &sh); }
        SDL_RenderCopy(r, E.spr, &src, &dst);
    }
}

/* ---------- entrada ---------- */
static void set_char(int d) { E.ch = (E.ch + d + SKIN_CHARS) % SKIN_CHARS; if (E.part >= skin_chars[E.ch].nparts) E.part = skin_chars[E.ch].nparts - 1; E.pulse_from = SDL_GetTicks(); }
static void open_picker(void) {
    unsigned cur = part_color(E.ch, E.part); int best = 0; long bd = 1L << 30;
    E.picking = 1; E.pick_old = E.ed.color[E.ch][E.part]; E.pick_old_set = E.ed.set[E.ch][E.part];
    for (int i = 0; i < PCOLS * PROWS; i++) {   /* el cursor empieza en el color mas parecido al actual */
        unsigned c = swatch(i); long d = 0;
        for (int j = 0; j < 24; j += 8) { long v = (long)((c >> j) & 255) - (long)((cur >> j) & 255); d += v * v; }
        if (d < bd) { bd = d; best = i; }
    }
    E.pc = best;
}
static void pick_apply(void) { E.ed.set[E.ch][E.part] = 1; E.ed.color[E.ch][E.part] = swatch(E.pc); E.ed.version++; E.picking = 0; save_pack(); }
static void part_original(void) { E.ed.set[E.ch][E.part] = 0; E.ed.version++; E.picking = 0; save_pack(); }
static void do_button(int b) {
    switch (b) {
    case B_NEW: save_pack(); new_pack(); say(lt("Pack nuevo: todo como la recreativa", "New pack: everything as in the arcade")); break;
    case B_OPEN: save_pack(); packs_scan(); E.listing = 1; E.lsel = packs_find(E.ed.name); if (E.lsel < 0) E.lsel = 0; break;
    case B_USE: use_pack(); break;
    case B_IMPORT: save_pack(); import_pack(); break;
    case B_EXPORT: export_pack(); break;
    }
}
static void start_naming(void) {
    E.naming = 1; snprintf(E.name_buf, sizeof E.name_buf, "%s", E.ed.name);
    SDL_StartTextInput();
    say(lt("Escribe el nombre y pulsa Intro", "Type the name and press Enter"));
}
static void stop_naming(int ok) {
    E.naming = 0; SDL_StopTextInput();
    if (ok) rename_pack(E.name_buf);
}
/* acciones: 0 nada, 1 arriba, 2 abajo, 3 izq, 4 der, 5 A, 6 B, 7 X, 8 Y, 9 L1, 10 R1 */
static void action(int a) {
    if (!a) return;
    if (E.naming) { if (a == 5) stop_naming(1); else if (a == 6) stop_naming(0); return; }
    if (E.picking) {
        int c = E.pc % PCOLS, r = E.pc / PCOLS;
        switch (a) {
        case 1: r = (r + PROWS - 1) % PROWS; break;
        case 2: r = (r + 1) % PROWS; break;
        case 3: c = (c + PCOLS - 1) % PCOLS; break;
        case 4: c = (c + 1) % PCOLS; break;
        case 5: pick_apply(); return;
        case 6: E.picking = 0; return;
        case 7: part_original(); return;
        }
        E.pc = r * PCOLS + c;
        return;
    }
    if (E.listing) {
        int n = packs_count();
        if (a == 1 && n) E.lsel = (E.lsel + n - 1) % n;
        else if (a == 2 && n) E.lsel = (E.lsel + 1) % n;
        else if (a == 5 && n) { load_pack(E.lsel); E.listing = 0; }
        else if (a == 6) E.listing = 0;
        return;
    }
    if (a == 9 || a == 10) { set_char(a == 9 ? -1 : 1); return; }
    if (a == 8) { start_naming(); return; }
    if (a == 6) { close_editor(); return; }
    if (E.focus_btn) {
        if (a == 3) E.bsel = (E.bsel + B_COUNT - 1) % B_COUNT;
        else if (a == 4) E.bsel = (E.bsel + 1) % B_COUNT;
        else if (a == 1) { E.focus_btn = 0; E.part = skin_chars[E.ch].nparts - 1; E.pulse_from = SDL_GetTicks(); }
        else if (a == 2) { E.focus_btn = 0; E.part = 0; E.pulse_from = SDL_GetTicks(); }
        else if (a == 5) do_button(E.bsel);
        return;
    }
    switch (a) {
    case 1: if (E.part == 0) E.focus_btn = 1; else { E.part--; E.pulse_from = SDL_GetTicks(); } break;
    case 2: if (E.part == skin_chars[E.ch].nparts - 1) E.focus_btn = 1; else { E.part++; E.pulse_from = SDL_GetTicks(); } break;
    case 3: set_char(-1); break;
    case 4: set_char(1); break;
    case 5: open_picker(); break;
    case 7: part_original(); break;
    }
}
static void tap(int ww, int wh, float fx, float fy) {
    int x = (int)(fx * OW), y = (int)(fy * E.uh);
    (void)ww; (void)wh;
    if (E.naming) { stop_naming(1); return; }
    if (y < 22 && x > OW / 2) { start_naming(); return; }
    for (int i = 0; i < B_COUNT && !E.picking && !E.listing; i++) {
        SDL_Rect b = btn_rect(i);
        if (x >= b.x && x < b.x + b.w && y >= b.y && y < b.y + b.h) { E.focus_btn = 1; E.bsel = i; do_button(i); return; }
    }
    if (x >= PX && x < PX + PW && y >= PY && y < PY + 22) { set_char(x < PX + PW / 2 ? -1 : 1); return; }
    if (x >= PX && x < PX + PW && y >= PY + 22 && y < btn_y() - 8 && !E.picking && !E.listing) { set_char(1); return; }
    if (E.picking) {
        int x0 = pick_x0(), y0 = pick_y0(), yy = y0 + PROWS * CELL + 8;
        if (x >= x0 && x < x0 + PCOLS * CELL && y >= y0 && y < y0 + PROWS * CELL) { E.pc = (y - y0) / CELL * PCOLS + (x - x0) / CELL; pick_apply(); return; }
        if (y >= yy && y < yy + 18 && x >= RX + 10 && x < RX + 120) { part_original(); return; }
        if (y >= yy && y < yy + 18 && x >= RX + RW - 120 && x < RX + RW - 10) { pick_apply(); return; }
        E.picking = 0;
        return;
    }
    if (E.listing) {
        int vis = (btn_y() - PY - 40) / 16, i = E.lfirst + (y - (PY + 24)) / 16;
        if (x >= RX && y >= PY + 24 && i >= E.lfirst && i < E.lfirst + vis && i < packs_count()) { load_pack(i); E.listing = 0; }
        else E.listing = 0;
        return;
    }
    if (x >= RX && x < RX + RW) {
        int i = (y - rows_y()) / row_h();
        if (y >= rows_y() && i >= 0 && i < skin_chars[E.ch].nparts) {
            E.focus_btn = 0;
            if (E.part == i) open_picker(); else { E.part = i; E.pulse_from = SDL_GetTicks(); }
        }
    }
}
int skined_event(SDL_Renderer *r, SDL_Event *e) {
    int ww, wh, a = 0;
    if (!E.open) return 0;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    switch (e->type) {
    case SDL_TEXTINPUT:
        if (E.naming && strlen(E.name_buf) + strlen(e->text.text) < 40) strcat(E.name_buf, e->text.text);
        return 1;
    case SDL_FINGERDOWN: tap(ww, wh, e->tfinger.x, e->tfinger.y); return 1;
    case SDL_FINGERUP: case SDL_FINGERMOTION: case SDL_MULTIGESTURE: return 1;
    case SDL_KEYDOWN: {
        SDL_Keycode k = e->key.keysym.sym;
        if (E.naming) {
            if (k == SDLK_BACKSPACE) { size_t l = strlen(E.name_buf); while (l && ((unsigned char)E.name_buf[l - 1] & 0xC0) == 0x80) l--; if (l) E.name_buf[l - 1] = 0; }
            else if (k == SDLK_RETURN || k == SDLK_KP_ENTER) stop_naming(1);
            else if (k == SDLK_ESCAPE || k == SDLK_AC_BACK) stop_naming(0);
            return 1;
        }
        a = k == SDLK_UP ? 1 : k == SDLK_DOWN ? 2 : k == SDLK_LEFT ? 3 : k == SDLK_RIGHT ? 4 :
            (k == SDLK_RETURN || k == SDLK_z || k == SDLK_SPACE) ? 5 : (k == SDLK_ESCAPE || k == SDLK_AC_BACK || k == SDLK_x || k == SDLK_BACKSPACE) ? 6 :
            k == SDLK_c ? 7 : k == SDLK_v ? 8 : k == SDLK_q ? 9 : k == SDLK_w ? 10 : 0;
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN: {
        int b = e->cbutton.button;
        a = b == SDL_CONTROLLER_BUTTON_DPAD_UP ? 1 : b == SDL_CONTROLLER_BUTTON_DPAD_DOWN ? 2 : b == SDL_CONTROLLER_BUTTON_DPAD_LEFT ? 3 :
            b == SDL_CONTROLLER_BUTTON_DPAD_RIGHT ? 4 : (b == SDL_CONTROLLER_BUTTON_A || b == SDL_CONTROLLER_BUTTON_START) ? 5 :
            (b == SDL_CONTROLLER_BUTTON_B || b == SDL_CONTROLLER_BUTTON_BACK) ? 6 : b == SDL_CONTROLLER_BUTTON_X ? 7 : b == SDL_CONTROLLER_BUTTON_Y ? 8 :
            b == SDL_CONTROLLER_BUTTON_LEFTSHOULDER ? 9 : b == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER ? 10 : 0;
        break;
    }
    case SDL_CONTROLLERAXISMOTION: {
        static int dx, dy;
        int v = e->caxis.value, d = v > 20000 ? 1 : v < -20000 ? -1 : 0;
        if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) { if (d && d != dx) a = d > 0 ? 4 : 3; dx = d; }
        else if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY) { if (d && d != dy) a = d > 0 ? 2 : 1; dy = d; }
        break;
    }
    default:
        return e->type == SDL_KEYUP || e->type == SDL_CONTROLLERBUTTONUP || e->type == SDL_TEXTEDITING;
    }
    action(a);
    return 1;
}
