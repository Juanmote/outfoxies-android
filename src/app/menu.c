/* Menu de opciones: launcher al arrancar y en partida (boton MENU, SELECT por defecto). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "font.h"

ofx_config cfg;

/* ---------- configuracion ---------- */
/* "moneda" ya no existe: los opciones.txt viejos la llevan y simplemente se ignora */
static const char *act_key[ACT_COUNT] = {"boton1", "boton2", "boton3", "start", "menu", "rebobinar", "avance"};
static const char *act_name[ACT_COUNT] = {"Botón 1", "Botón 2", "Botón 3", "Start / pausa", "Menú", "Rebobinar", "Avance rápido"};

void config_defaults(void) {
    cfg.filter = FILT_SMOOTH; cfg.aspect = ASP_169; cfg.volume = 100;
    cfg.invuln = cfg.ammo = cfg.onehit = 0; cfg.stage = -1;
    cfg.map[ACT_B1] = 0;  /* A */
    cfg.map[ACT_B2] = 1;  /* B */
    cfg.map[ACT_B3] = 2;  /* X */
    cfg.map[ACT_START] = 6; /* START */
    cfg.map[ACT_MENU] = 4;  /* SELECT */
    cfg.map[ACT_REWIND] = PADBTN_L2;
    cfg.map[ACT_FF] = PADBTN_R2;
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
        else if (!strcmp(key, "truco_invulnerable")) cfg.invuln = v != 0;
        else if (!strcmp(key, "truco_municion")) cfg.ammo = v != 0;
        else if (!strcmp(key, "truco_puno")) cfg.onehit = v != 0;
        else if (!strcmp(key, "truco_escenario") && v >= -1 && v <= 7) cfg.stage = v;
        else for (int i = 0; i < ACT_COUNT; i++) if (!strcmp(key, act_key[i]) && v >= 0 && v <= PADBTN_R2) cfg.map[i] = v;
    }
    fclose(f);
}
void config_save(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "imagen=%d\nformato=%d\nvolumen=%d\n", cfg.filter, cfg.aspect, cfg.volume);
    fprintf(f, "truco_invulnerable=%d\ntruco_municion=%d\ntruco_puno=%d\ntruco_escenario=%d\n",
            cfg.invuln, cfg.ammo, cfg.onehit, cfg.stage);
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
enum { IT_PLAY, IT_SAVE, IT_LOAD, IT_FILTER, IT_ASPECT, IT_VOLUME, IT_BUTTONS, IT_CHEATS, IT_QUIT };
/* pagina de trucos */
enum { CH_INVULN, CH_AMMO, CH_ONEHIT, CH_STAGE, CH_BACK, CH_COUNT };
/* escenarios en el orden en que suelen salir; el numero es el rival dueno del escenario (nb2.h) */
static const int stage_order[9] = {-1, 5, 2, 3, 4, 6, 1, 7, 0};
static const char *stage_name(int s) {
    static const char *n[8] = {"Rascacielos", "Circo", "Acuario", "Tren", "Fábrica", "Avión", "Barco", "Mansión"};
    return s >= 0 && s < 8 ? n[s] : "Normal";
}
static const char *rival_name(int s) {
    static const char *n[8] = {"John Smith", "Eve", "Betty Joe", "Bernard White", "Prof. Ching", "Dweeb", "Danny y Demi", "Mr. Ache"};
    return s >= 0 && s < 8 ? n[s] : "";
}
static int page, sel, in_game, waiting = -1, wait_ms;
static int items[12], nitems, slot;
static char save_dir[512] = ".";
static const char *filt_name[FILT_COUNT] = {"Nítida", "Suavizada", "Scanlines"};
static const char *asp_name[ASP_COUNT] = {"4:3", "4:3 escala exacta", "4:3 estirado", "16:9 panorámico"};

static void build(void) {
    nitems = 0;
    items[nitems++] = IT_PLAY;
    if (in_game) items[nitems++] = IT_SAVE;
    items[nitems++] = IT_LOAD; items[nitems++] = IT_FILTER; items[nitems++] = IT_ASPECT;
    items[nitems++] = IT_VOLUME; items[nitems++] = IT_BUTTONS; items[nitems++] = IT_CHEATS;
    items[nitems++] = IT_QUIT;
}
void menu_open(int game) { in_game = game; page = 0; sel = 0; waiting = -1; build(); }
int menu_waiting_button(void) { return waiting >= 0; }
void menu_set_save_dir(const char *dir) { snprintf(save_dir, sizeof save_dir, "%s", dir); }
void menu_slot_path(int s, char *buf, int n) { snprintf(buf, n, "%s/partida%d.sav", save_dir, s + 1); }
int menu_slot(void) { return slot; }
static int slot_info(int s, char *buf, int n) {     /* 1 si la ranura tiene partida */
    char p[600]; struct stat st;
    menu_slot_path(s, p, sizeof p);
    if (stat(p, &st) != 0) { snprintf(buf, n, "Ranura %d: vacía", s + 1); return 0; }
    {
        struct tm *t = localtime(&st.st_mtime);
        if (t) snprintf(buf, n, "Ranura %d: guardada el %02d/%02d a las %02d:%02d", s + 1, t->tm_mday, t->tm_mon + 1, t->tm_hour, t->tm_min);
        else snprintf(buf, n, "Ranura %d: guardada", s + 1);
    }
    return 1;
}

static int change(int it, int dir) {
    switch (it) {
    case IT_SAVE: case IT_LOAD: slot = (slot + dir + NSLOTS) % NSLOTS; return MR_NONE;
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
            case IT_SAVE: return MR_SAVE;
            case IT_LOAD: { char b[96]; if (slot_info(slot, b, sizeof b)) return MR_LOAD; break; }
            case IT_BUTTONS: page = 1; sel = 0; break;
            case IT_CHEATS: page = 2; sel = 0; break;
            case IT_QUIT: return MR_QUIT;
            default: return change(it, 1);
            }
        }
        return MR_NONE;
    }
    if (page == 2) {
        int dir = (w == MI_LEFT) ? -1 : 1;
        switch (w) {
        case MI_UP: sel = (sel + CH_COUNT - 1) % CH_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % CH_COUNT; break;
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_CHEATS; sel++) {} break;
        case MI_OK: case MI_LEFT: case MI_RIGHT:
            switch (sel) {
            case CH_INVULN: cfg.invuln = !cfg.invuln; return MR_CHANGED;
            case CH_AMMO: cfg.ammo = !cfg.ammo; return MR_CHANGED;
            case CH_ONEHIT: cfg.onehit = !cfg.onehit; return MR_CHANGED;
            case CH_STAGE: {
                int i = 0;
                while (i < 9 && stage_order[i] != cfg.stage) i++;
                cfg.stage = stage_order[((i % 9) + dir + 9) % 9];
                return MR_CHANGED;
            }
            case CH_BACK: if (w == MI_OK) { page = 0; for (sel = 0; items[sel] != IT_CHEATS; sel++) {} } break;
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
        int step = nitems > 7 ? 19 : 22;
        y = nitems > 7 ? 74 : 80;
        for (int i = 0; i < nitems; i++, y += step) {
            char v[16];
            switch (items[i]) {
            case IT_PLAY: row(i, y, in_game ? "Continuar" : "Jugar", NULL); break;
            case IT_SAVE: snprintf(v, sizeof v, "Ranura %d", slot + 1); row(i, y, "Guardar partida", v); break;
            case IT_LOAD: snprintf(v, sizeof v, "Ranura %d", slot + 1); row(i, y, "Cargar partida", v); break;
            case IT_FILTER: row(i, y, "Imagen", filt_name[cfg.filter]); break;
            case IT_ASPECT: row(i, y, "Formato", asp_name[cfg.aspect]); break;
            case IT_VOLUME: snprintf(v, sizeof v, "%d%%", cfg.volume); row(i, y, "Volumen", v); break;
            case IT_BUTTONS: row(i, y, "Botones", NULL); break;
            case IT_CHEATS: row(i, y, "Trucos", NULL); break;
            case IT_QUIT: row(i, y, "Salir", NULL); break;
            }
        }
        if (items[sel] == IT_SAVE || items[sel] == IT_LOAD) {
            char b[96]; slot_info(slot, b, sizeof b);
            ctext(0, 250, b, WHITE, 255);
        } else ctext(0, 250, in_game ? "↑↓ elegir   ←→ cambiar   A aceptar   B volver al juego" : "↑↓ elegir   ←→ cambiar   A aceptar", GREY, 255);
    } else if (page == 2) {
        row(CH_INVULN, y, "Invulnerable", cfg.invuln ? "Sí" : "No"); y += 22;
        row(CH_AMMO, y, "Munición infinita", cfg.ammo ? "Sí" : "No"); y += 22;
        row(CH_ONEHIT, y, "Puño de la muerte", cfg.onehit ? "Sí" : "No"); y += 22;
        row(CH_STAGE, y, "Escenario", stage_name(cfg.stage)); y += 22;
        row(CH_BACK, y, "Volver", NULL);
        if (cfg.stage >= 0) {
            char buf[96];
            snprintf(buf, sizeof buf, "Siguiente combate: %s, contra %s", stage_name(cfg.stage), rival_name(cfg.stage));
            ctext(0, 196, buf, WHITE, 255);
            ctext(0, 214, "No cambia si ya le has ganado o es tu personaje", GREY, 255);
        } else ctext(0, 196, "Los combates siguen el orden del juego", GREY, 255);
        ctext(0, 246, "←→ cambiar   B volver", GREY, 255);
    } else {
        y = 74;
        for (int i = 0; i < ACT_COUNT; i++, y += 19) row(i, y, act_name[i], padbtn_name(cfg.map[i]));
        row(ACT_COUNT, y, "Restaurar botones", NULL); y += 19;
        row(ACT_COUNT + 1, y, "Volver", NULL);
        ctext(0, 250, "A cambiar el botón   B volver", GREY, 255);
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

void menu_render_osd(uint32_t *argb, int paused, const char *msg) {
    cv = argb;
    memset(cv, 0, sizeof(uint32_t) * MENU_W * MENU_H);
    if (paused) {
        rect(0, 100, MENU_W, 70, 0x06060a, 170);
        text(1, (MENU_W - text_w(1, "PAUSA")) / 2 + 2, 108, "PAUSA", 0x000000, 160);
        ctext(1, 106, "PAUSA", ORANGE, 255);
        ctext(0, 146, "START seguir   SELECT menú", GREY, 255);
    }
    if (msg && *msg) {
        int w = text_w(0, msg) + 16;
        rect(8, 8, w, 22, 0x06060a, 190);
        rect(8, 8, 3, 22, ORANGE, 255);
        text(0, 16, 11, msg, WHITE, 255);
    }
}
