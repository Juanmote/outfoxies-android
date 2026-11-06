/* Menu de opciones: launcher al arrancar y pausa en partida (mantener moneda + start). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "font.h"

ofx_config cfg;

/* ---------- configuracion ---------- */
static const char *act_key[ACT_COUNT] = {"boton1", "boton2", "boton3", "moneda", "start"};
static const char *act_name[ACT_COUNT] = {"Botón 1", "Botón 2", "Botón 3", "Moneda", "Start"};

void config_defaults(void) {
    cfg.filter = FILT_SMOOTH; cfg.aspect = ASP_43; cfg.volume = 100;
    cfg.map[ACT_B1] = 0;  /* A */
    cfg.map[ACT_B2] = 1;  /* B */
    cfg.map[ACT_B3] = 2;  /* X */
    cfg.map[ACT_COIN] = 4;  /* SELECT */
    cfg.map[ACT_START] = 6; /* START */
}
static void map_defaults(void) {
    ofx_config keep = cfg; config_defaults();
    memcpy(keep.map, cfg.map, sizeof cfg.map); cfg = keep;
}
void config_load(const char *path) {
    char line[128], key[64]; int v;
    FILE *f;
    config_defaults();
    if (!(f = fopen(path, "r"))) return;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%63[^=]=%d", key, &v) != 2) continue;
        if (!strcmp(key, "imagen") && v >= 0 && v < FILT_COUNT) cfg.filter = v;
        else if (!strcmp(key, "formato") && v >= 0 && v < ASP_COUNT) cfg.aspect = v;
        else if (!strcmp(key, "volumen") && v >= 0 && v <= 100) cfg.volume = v;
        else for (int i = 0; i < ACT_COUNT; i++) if (!strcmp(key, act_key[i]) && v >= 0 && v <= PADBTN_R2) cfg.map[i] = v;
    }
    fclose(f);
}
void config_save(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "imagen=%d\nformato=%d\nvolumen=%d\n", cfg.filter, cfg.aspect, cfg.volume);
    for (int i = 0; i < ACT_COUNT; i++) fprintf(f, "%s=%d\n", act_key[i], cfg.map[i]);
    fclose(f);
}
const char *padbtn_name(int b) {
    static const char *n[] = {"A", "B", "X", "Y", "SELECT", "HOME", "START", "L3", "R3", "L1", "R1",
                              "Arriba", "Abajo", "Izquierda", "Derecha", "Extra"};
    if (b == PADBTN_L2) return "L2";
    if (b == PADBTN_R2) return "R2";
    if (b >= 0 && b < (int)(sizeof n / sizeof n[0])) return n[b];
    return "?";
}

/* ---------- estado ---------- */
enum { IT_PLAY, IT_FILTER, IT_ASPECT, IT_VOLUME, IT_BUTTONS, IT_ROM, IT_QUIT };
static int page, sel, in_game, can_rom, waiting = -1, wait_ms;
static int items[8], nitems;
static const char *filt_name[FILT_COUNT] = {"Nítida", "Suavizada", "Scanlines"};
static const char *asp_name[ASP_COUNT] = {"4:3", "4:3 escala exacta", "Pantalla completa"};

static void build(void) {
    nitems = 0;
    items[nitems++] = IT_PLAY; items[nitems++] = IT_FILTER; items[nitems++] = IT_ASPECT;
    items[nitems++] = IT_VOLUME; items[nitems++] = IT_BUTTONS;
    if (can_rom) items[nitems++] = IT_ROM;
    items[nitems++] = IT_QUIT;
}
void menu_open(int game, int rom) { in_game = game; can_rom = rom; page = 0; sel = 0; waiting = -1; build(); }
int menu_waiting_button(void) { return waiting >= 0; }

static int change(int it, int dir) {
    switch (it) {
    case IT_FILTER: cfg.filter = (cfg.filter + dir + FILT_COUNT) % FILT_COUNT; return MR_CHANGED;
    case IT_ASPECT: cfg.aspect = (cfg.aspect + dir + ASP_COUNT) % ASP_COUNT; return MR_CHANGED;
    case IT_VOLUME: {
        int v = cfg.volume + dir * 10;
        if (v < 0) v = 0; if (v > 100) v = 100;
        if (v == cfg.volume) return MR_NONE;
        cfg.volume = v; return MR_CHANGED;
    }
    }
    return MR_NONE;
}
int menu_input(int w) {
    if (waiting >= 0) { if (w == MI_BACK) waiting = -1; return MR_NONE; }
    if (page == 0) {
        int it = items[sel];
        switch (w) {
        case MI_UP: sel = (sel + nitems - 1) % nitems; break;
        case MI_DOWN: sel = (sel + 1) % nitems; break;
        case MI_LEFT: return change(it, -1);
        case MI_RIGHT: return change(it, 1);
        case MI_BACK: if (in_game) return MR_PLAY; break;
        case MI_OK:
            switch (it) {
            case IT_PLAY: return MR_PLAY;
            case IT_BUTTONS: page = 1; sel = 0; break;
            case IT_ROM: return MR_CHANGE_ROM;
            case IT_QUIT: return MR_QUIT;
            default: return change(it, 1);
            }
        }
        return MR_NONE;
    }
    /* pagina de botones: acciones, restaurar, volver */
    {
        int n = ACT_COUNT + 2;
        switch (w) {
        case MI_UP: sel = (sel + n - 1) % n; break;
        case MI_DOWN: sel = (sel + 1) % n; break;
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_BUTTONS; sel++) {} break;
        case MI_OK:
            if (sel < ACT_COUNT) { waiting = sel; wait_ms = 5000; }
            else if (sel == ACT_COUNT) { map_defaults(); return MR_CHANGED; }
            else { page = 0; for (sel = 0; items[sel] != IT_BUTTONS; sel++) {} }
            break;
        }
    }
    return MR_NONE;
}
int menu_bind(int b) {
    if (waiting < 0) return MR_NONE;
    for (int i = 0; i < ACT_COUNT; i++) if (i != waiting && cfg.map[i] == b) cfg.map[i] = cfg.map[waiting];
    cfg.map[waiting] = b;
    waiting = -1;
    return MR_CHANGED;
}
void menu_tick(int ms) { if (waiting >= 0 && (wait_ms -= ms) <= 0) waiting = -1; }

/* ---------- dibujo ---------- */
static uint32_t *cv;
static void blend(int x, int y, uint32_t rgb, int a) {
    uint32_t d, da, r, g, b;
    if (x < 0 || y < 0 || x >= MENU_W || y >= MENU_H || a <= 0) return;
    if (a > 255) a = 255;
    d = cv[y * MENU_W + x]; da = d >> 24;
    r = (((rgb >> 16) & 255) * a + ((d >> 16) & 255) * (255 - a)) / 255;
    g = (((rgb >> 8) & 255) * a + ((d >> 8) & 255) * (255 - a)) / 255;
    b = ((rgb & 255) * a + (d & 255) * (255 - a)) / 255;
    da = a + da * (255 - a) / 255;
    cv[y * MENU_W + x] = da << 24 | r << 16 | g << 8 | b;
}
static void rect(int x0, int y0, int w, int h, uint32_t rgb, int a) {
    for (int y = y0; y < y0 + h; y++) for (int x = x0; x < x0 + w; x++) blend(x, y, rgb, a);
}
static unsigned next_cp(const char **s) {
    const unsigned char *p = (const unsigned char *)*s; unsigned c = *p;
    if (c < 0x80) { *s += 1; return c; }
    if ((c & 0xe0) == 0xc0) { *s += 2; return (c & 0x1f) << 6 | (p[1] & 0x3f); }
    if ((c & 0xf0) == 0xe0) { *s += 3; return (c & 0x0f) << 12 | (p[1] & 0x3f) << 6 | (p[2] & 0x3f); }
    *s += 1; return '?';
}
static int glyph(int big, unsigned cp) {
    int n = big ? BIG_N : FONT_N; const unsigned short *t = big ? big_cp : font_cp;
    for (int i = 0; i < n; i++) if (t[i] == cp) return i;
    return big ? 31 : 31;   /* '?' */
}
static int text_w(int big, const char *s) {
    int w = 0;
    while (*s) w += (big ? big_w : font_w)[glyph(big, next_cp(&s))] - 1;
    return w;
}
static int text(int big, int x, int y, const char *s, uint32_t rgb, int a) {
    while (*s) {
        int g = glyph(big, next_cp(&s)), gw = (big ? big_w : font_w)[g], gh = big ? BIG_H : FONT_H;
        const unsigned char *px = (big ? big_px : font_px) + (big ? big_off : font_off)[g];
        for (int yy = 0; yy < gh; yy++) for (int xx = 0; xx < gw; xx++) {
            int v = px[yy * gw + xx];
            if (v) blend(x + xx, y + yy, rgb, v * a / 255);
        }
        x += gw - 1;
    }
    return x;
}
static void ctext(int big, int y, const char *s, uint32_t rgb, int a) { text(big, (MENU_W - text_w(big, s)) / 2, y, s, rgb, a); }

#define ORANGE 0xf09628
#define WHITE 0xffffff
#define GREY 0x9a9aa8

static void row(int i, int y, const char *label, const char *value) {
    int s = (i == sel);
    if (s) { rect(96, y - 2, 288, 20, ORANGE, 70); rect(96, y - 2, 3, 20, ORANGE, 255); }
    text(0, 110, y, label, s ? WHITE : GREY, 255);
    if (value) {
        char buf[64];
        if (s) snprintf(buf, sizeof buf, "‹ %s ›", value); else snprintf(buf, sizeof buf, "%s", value);
        text(0, 372 - text_w(0, buf), y, buf, s ? ORANGE : GREY, 255);
    }
}

void menu_render(uint32_t *argb) {
    int y;
    cv = argb;
    if (in_game) for (int i = 0; i < MENU_W * MENU_H; i++) cv[i] = 0x50000000 | 0x06060a;
    else for (y = 0; y < MENU_H; y++) {
        int t = y * 255 / MENU_H;
        uint32_t r = 26 - 21 * t / 255, g = 16 - 11 * t / 255, b = 48 - 40 * t / 255;
        for (int x = 0; x < MENU_W; x++) cv[y * MENU_W + x] = 0xff000000 | r << 16 | g << 8 | b;
    }
    text(1, (MENU_W - text_w(1, "THE OUTFOXIES")) / 2 + 2, 16, "THE OUTFOXIES", 0x000000, 160);
    ctext(1, 14, "THE OUTFOXIES", ORANGE, 255);
    ctext(0, 52, in_game ? "PAUSA" : "Namco · 1994", GREY, 255);
    y = 80;
    if (page == 0) {
        for (int i = 0; i < nitems; i++, y += 22) {
            char v[16];
            switch (items[i]) {
            case IT_PLAY: row(i, y, in_game ? "Continuar" : "Jugar", NULL); break;
            case IT_FILTER: row(i, y, "Imagen", filt_name[cfg.filter]); break;
            case IT_ASPECT: row(i, y, "Formato", asp_name[cfg.aspect]); break;
            case IT_VOLUME: snprintf(v, sizeof v, "%d%%", cfg.volume); row(i, y, "Volumen", v); break;
            case IT_BUTTONS: row(i, y, "Botones", NULL); break;
            case IT_ROM: row(i, y, "Cambiar ROM", NULL); break;
            case IT_QUIT: row(i, y, "Salir", NULL); break;
            }
        }
        ctext(0, 246, in_game ? "↑↓ elegir   ←→ cambiar   A aceptar   B volver al juego" : "↑↓ elegir   ←→ cambiar   A aceptar", GREY, 255);
    } else {
        for (int i = 0; i < ACT_COUNT; i++, y += 22) row(i, y, act_name[i], padbtn_name(cfg.map[i]));
        row(ACT_COUNT, y, "Restaurar botones", NULL); y += 22;
        row(ACT_COUNT + 1, y, "Volver", NULL);
        ctext(0, 246, "A cambiar el botón   B volver", GREY, 255);
        if (waiting >= 0) {
            char buf[80];
            rect(70, 96, 340, 78, 0x101018, 255);
            rect(70, 96, 340, 3, ORANGE, 255);
            snprintf(buf, sizeof buf, "Pulsa el botón para «%s»", act_name[waiting]);
            ctext(0, 114, buf, WHITE, 255);
            snprintf(buf, sizeof buf, "Se cancela en %d s", (wait_ms + 999) / 1000);
            ctext(0, 142, buf, GREY, 255);
        }
    }
}
