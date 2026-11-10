/* Menu de opciones: launcher al arrancar y en partida (boton MENU, SELECT por defecto). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "menu.h"
#include "achievements.h"
#include "font.h"

ofx_config cfg;

/* ---------- configuracion ---------- */
/* "moneda" ya no existe: los opciones.txt viejos la llevan y simplemente se ignora */
/* el boton 3 existe en la placa pero el juego no lo usa: no se ensena en el menu */
#define ACT_SHOWN (ACT_COUNT - 1)
static int act_of_row(int k) { return k >= ACT_B3 ? k + 1 : k; }
static const char *act_key[ACT_COUNT] = {"boton1", "boton2", "boton3", "start", "menu", "rebobinar", "avance"};
static const char *act_name[LANG_COUNT][ACT_COUNT] = {
    {"Botón 1 (disparo)", "Botón 2 (salto)", "Botón 3", "Start / pausa", "Menú", "Rebobinar", "Avance rápido"},
    {"Button 1 (attack)", "Button 2 (jump)", "Button 3", "Start / pause", "Menu", "Rewind", "Fast forward"}};

void config_defaults(void) {
    cfg.filter = FILT_SMOOTH; cfg.aspect = ASP_169; cfg.volume = 100; cfg.vibration = 50;
    cfg.colors = COL_ORIGINAL; cfg.bands = BAND_BLACK; cfg.difficulty = DIF_NORMAL;
    cfg.invuln = cfg.ammo = cfg.onehit = 0; cfg.stage = -1;
    cfg.lang = LANG_ES; cfg.touch = TOUCH_M; cfg.game_lang = 1; cfg.outfit = 0; cfg.weapons = 0;
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
int config_load(const char *path) {
    char line[128], key[64]; int v, has_lang = 0;
    FILE *f;
    config_defaults();
    if (!(f = fopen(path, "r"))) return 0;
    while (fgets(line, sizeof line, f)) {
        if (sscanf(line, "%63[^=]=%d", key, &v) != 2) continue;
        if (!strcmp(key, "imagen") && v >= 0 && v < FILT_COUNT) cfg.filter = v;
        else if (!strcmp(key, "formato") && v >= 0 && v < ASP_COUNT) cfg.aspect = v;
        else if (!strcmp(key, "volumen") && v >= 0 && v <= 100) cfg.volume = v;
        else if (!strcmp(key, "vibracion") && v >= 0 && v <= 100) cfg.vibration = v;
        else if (!strcmp(key, "colores") && v >= 0 && v < COL_COUNT) cfg.colors = v;
        else if (!strcmp(key, "bandas") && v >= 0 && v < BAND_COUNT) cfg.bands = v;
        else if (!strcmp(key, "dificultad") && v >= 0 && v < DIF_COUNT) cfg.difficulty = v;
        else if (!strcmp(key, "truco_invulnerable")) cfg.invuln = v != 0;
        else if (!strcmp(key, "truco_municion")) cfg.ammo = v != 0;
        else if (!strcmp(key, "truco_puno")) cfg.onehit = v != 0;
        else if (!strcmp(key, "truco_escenario") && v >= -1 && v <= 7) cfg.stage = v;
        else if (!strcmp(key, "idioma") && v >= 0 && v < LANG_COUNT) { cfg.lang = v; has_lang = 1; }
        else if (!strcmp(key, "tactil") && v >= 0 && v < TOUCH_COUNT) cfg.touch = v;
        else if (!strcmp(key, "ropa") && (v == 0 || v == 1)) cfg.outfit = v;
        else if (!strcmp(key, "textos_juego") && (v == 0 || v == 1)) cfg.game_lang = v;
        else if (!strcmp(key, "armas") && v >= 0 && v < WEAPONS_COUNT) cfg.weapons = v;
        else for (int i = 0; i < ACT_COUNT; i++) if (!strcmp(key, act_key[i]) && v >= 0 && v <= PADBTN_R2) cfg.map[i] = v;
    }
    fclose(f);
    return has_lang;
}
void config_save(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "imagen=%d\nformato=%d\nvolumen=%d\nvibracion=%d\n", cfg.filter, cfg.aspect, cfg.volume, cfg.vibration);
    fprintf(f, "colores=%d\nbandas=%d\ndificultad=%d\n", cfg.colors, cfg.bands, cfg.difficulty);
    fprintf(f, "truco_invulnerable=%d\ntruco_municion=%d\ntruco_puno=%d\ntruco_escenario=%d\n",
            cfg.invuln, cfg.ammo, cfg.onehit, cfg.stage);
    fprintf(f, "idioma=%d\ntactil=%d\ntextos_juego=%d\nropa=%d\narmas=%d\n", cfg.lang, cfg.touch, cfg.game_lang, cfg.outfit, cfg.weapons);
    for (int i = 0; i < ACT_COUNT; i++) fprintf(f, "%s=%d\n", act_key[i], cfg.map[i]);
    fclose(f);
}
const char *padbtn_name(int b) {
    static const char *n[] = {"A", "B", "X", "Y", "SELECT", "HOME", "START", "L3", "R3", "L1", "R1",
                              "Arriba", "Abajo", "Izquierda", "Derecha", "Extra"};
    static const char *ne[] = {"Up", "Down", "Left", "Right"};
    if (cfg.lang == LANG_EN && b >= 11 && b <= 14) return ne[b - 11];
    if (b == PADBTN_L2) return "L2";
    if (b == PADBTN_R2) return "R2";
    if (b >= 0 && b < (int)(sizeof n / sizeof n[0])) return n[b];
    return "?";
}

/* ---------- estado ---------- */
enum { IT_PLAY, IT_SAVE, IT_LOAD, IT_SCREEN, IT_DIFFICULTY, IT_WEAPONS, IT_SOUND, IT_BUTTONS, IT_CHEATS, IT_ACH, IT_LANG, IT_TITLE, IT_QUIT };
/* pagina de sonido */
enum { SO_VOLUME, SO_VIBRATION, SO_BACK, SO_COUNT };
/* pagina de pantalla */
enum { SC_ASPECT, SC_FILTER, SC_COLORS, SC_BANDS, SC_OUTFIT, SC_BACK, SC_COUNT };
enum { LG_MENU, LG_GAME, LG_BACK, LG_COUNT };
/* pagina de trucos */
enum { CH_INVULN, CH_AMMO, CH_ONEHIT, CH_STAGE, CH_BACK, CH_COUNT };
/* escenarios en el orden en que suelen salir; el numero es el rival dueno del escenario (nb2.h) */
static const int stage_order[9] = {-1, 5, 2, 3, 4, 6, 1, 7, 0};
static const char *stage_name(int s) {
    static const char *n[LANG_COUNT][8] = {{"Rascacielos", "Circo", "Acuario", "Tren", "Fábrica", "Avión", "Barco", "Mansión"},
                                           {"Skyscraper", "Circus", "Aquarium", "Train", "Factory", "Plane", "Ship", "Mansion"}};
    return s >= 0 && s < 8 ? n[cfg.lang][s] : "Normal";
}
static const char *rival_name(int s) {
    static const char *n[8] = {"John Smith", "Eve", "Betty Doe", "Bernard White", "Prof. Ching", "Dweeb", "Danny y Demi", "Mr. Acme"};
    if (s == 6 && cfg.lang == LANG_EN) return "Danny & Demi";
    return s >= 0 && s < 8 ? n[s] : "";
}
static int page, sel, in_game, waiting = -1, wait_ms;
static int items[12], nitems, slot;
static char save_dir[512] = ".";
static const char *filt_name[LANG_COUNT][FILT_COUNT] = {{"Nítida", "Suavizada", "Scanlines", "Nítida suavizada", "CRT", "CRT curvo"},
                                                        {"Sharp", "Smooth", "Scanlines", "Sharp smoothed", "CRT", "Curved CRT"}};
static const int filt_order[FILT_COUNT] = {FILT_SHARP, FILT_SHARP_SMOOTH, FILT_SMOOTH, FILT_SCANLINES, FILT_CRT, FILT_CRT_CURVED};
static const char *filt_help[LANG_COUNT][FILT_COUNT] = {{
    "Píxeles tal cual, sin suavizar", "Algo borrosa, como cualquier pantalla moderna", "Líneas oscuras entre las del juego",
    "Píxeles nítidos con los bordes suavizados", "Tele de tubo: máscara de fósforo y resplandor", "Tele de tubo con la pantalla abombada"}, {
    "Raw pixels, no smoothing", "A bit blurry, like any modern screen", "Dark lines between the game's lines",
    "Sharp pixels with smoothed edges", "Tube TV: phosphor mask and glow", "Tube TV with a curved screen"}};
static const char *col_name[LANG_COUNT][COL_COUNT] = {{"Original", "Tele antigua", "Vivos", "Nocturna", "Sepia", "Pantalla verde"},
                                                      {"Original", "Old TV", "Vivid", "Night", "Sepia", "Green screen"}};
static const char *band_name[LANG_COUNT][BAND_COUNT] = {{"Negras", "Ambiente"}, {"Black", "Ambient"}};
static const char *dif_name[LANG_COUNT][DIF_COUNT] = {{"Fácil", "Normal", "Difícil"}, {"Easy", "Normal", "Hard"}};
static const char *dif_help[LANG_COUNT][DIF_COUNT] = {
    {"Recibes la mitad de daño y haces el doble", "Como la recreativa", "Recibes el doble de daño y haces la mitad"},
    {"You take half damage and deal double", "Like the arcade", "You take double damage and deal half"}};
static const char *asp_name[LANG_COUNT][ASP_COUNT] = {{"4:3", "4:3 escala exacta", "4:3 estirado", "16:9 panorámico", "8:7 recortado"},
                                                      {"4:3", "4:3 integer scale", "4:3 stretched", "16:9 widescreen", "8:7 cropped"}};
/* mismo orden que NB2_WEAPONS_* (nb2.h) */
static const char *weap_name[LANG_COUNT][WEAPONS_COUNT] = {{"Todas", "De fuego", "Cuerpo a cuerpo", "Explosivas", "Sin armas"},
                                                           {"All", "Firearms", "Melee", "Explosives", "No weapons"}};
static const char *weap_help[LANG_COUNT][WEAPONS_COUNT] = {
    {"Como la recreativa", "Solo pistola y metralleta", "Solo katana y látigo", "Granada, lanzacohetes y lanzallamas",
     "A puñetazos (quedan la comida, el carbón y los juguetes)"},
    {"Like the arcade", "Gun and machine gun only", "Katana and whip only", "Grenade, rocket launcher and flamethrower",
     "Fists only (food, coal and toys stay)"}};
static const char *touch_name[LANG_COUNT][TOUCH_COUNT] = {{"No", "Pequeños", "Medianos", "Grandes"}, {"Off", "Small", "Medium", "Large"}};

static void build(void) {
    nitems = 0;
    items[nitems++] = IT_PLAY;
    if (in_game) items[nitems++] = IT_SAVE;
    items[nitems++] = IT_LOAD; items[nitems++] = IT_SCREEN; items[nitems++] = IT_DIFFICULTY; items[nitems++] = IT_WEAPONS;
    items[nitems++] = IT_SOUND; items[nitems++] = IT_BUTTONS; items[nitems++] = IT_CHEATS;
    items[nitems++] = IT_ACH; items[nitems++] = IT_LANG;
    if (in_game) items[nitems++] = IT_TITLE;
    items[nitems++] = IT_QUIT;
}
void menu_open(int game) { in_game = game; page = 0; sel = 0; waiting = -1; build(); }
int menu_waiting_button(void) { return waiting >= 0; }
void menu_set_save_dir(const char *dir) { snprintf(save_dir, sizeof save_dir, "%s", dir); }
const char *menu_save_dir(void) { return save_dir; }
void menu_slot_path(int s, char *buf, int n) { snprintf(buf, n, "%s/partida%d.sav", save_dir, s + 1); }
int menu_slot(void) { return slot; }
static int slot_info(int s, char *buf, int n) {     /* 1 si la ranura tiene partida */
    char p[600]; struct stat st;
    menu_slot_path(s, p, sizeof p);
    if (stat(p, &st) != 0) { snprintf(buf, n, TR("Ranura %d: vacía", "Slot %d: empty"), s + 1); return 0; }
    {
        struct tm *t = localtime(&st.st_mtime);
        if (t && cfg.lang == LANG_EN) snprintf(buf, n, "Slot %d: saved %02d/%02d at %02d:%02d", s + 1, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min);
        else if (t) snprintf(buf, n, "Ranura %d: guardada el %02d/%02d a las %02d:%02d", s + 1, t->tm_mday, t->tm_mon + 1, t->tm_hour, t->tm_min);
        else snprintf(buf, n, TR("Ranura %d: guardada", "Slot %d: saved"), s + 1);
    }
    return 1;
}

static int change(int it, int dir) {
    switch (it) {
    case IT_SAVE: case IT_LOAD: slot = (slot + dir + NSLOTS) % NSLOTS; return MR_NONE;
    case IT_DIFFICULTY: {
        int v = cfg.difficulty + dir;
        if (v < 0 || v >= DIF_COUNT) return MR_NONE;
        cfg.difficulty = v; return MR_CHANGED;
    }
    case IT_WEAPONS: {
        cfg.weapons = (cfg.weapons + dir + WEAPONS_COUNT) % WEAPONS_COUNT; return MR_CHANGED;
    }
    case SO_VOLUME + 100: {
        int v = cfg.volume + dir * 10;
        if (v < 0) v = 0; if (v > 100) v = 100;
        if (v == cfg.volume) return MR_NONE;
        cfg.volume = v; return MR_CHANGED;
    }
    case SO_VIBRATION + 100: {
        int v = cfg.vibration + dir * 25;
        if (v < 0) v = 0; if (v > 100) v = 100;
        if (v == cfg.vibration) return MR_NONE;
        cfg.vibration = v; return MR_CHANGED;
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
            case IT_SCREEN: page = 3; sel = 0; break;
            case IT_CHEATS: page = 2; sel = 0; break;
            case IT_LANG: page = 4; sel = 0; break;
            case IT_ACH: page = 5; sel = 0; break;
            case IT_SOUND: page = 6; sel = 0; break;
            case IT_TITLE: return MR_TITLE;
            case IT_QUIT: return MR_QUIT;
            default: return change(it, 1);
            }
        }
        return MR_NONE;
    }
    if (page == 6) {   /* sonido y vibracion (change() los lleva con +100) */
        switch (w) {
        case MI_UP: sel = (sel + SO_COUNT - 1) % SO_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % SO_COUNT; break;
        case MI_LEFT: if (sel != SO_BACK) return change(sel + 100, -1); break;
        case MI_RIGHT: if (sel != SO_BACK) return change(sel + 100, 1); break;
        case MI_OK: if (sel != SO_BACK) return change(sel + 100, 1);   /* sin break: Volver */
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_SOUND; sel++) {} break;
        }
        return MR_NONE;
    }
    if (page == 5) {   /* logros: solo se mira la lista */
        switch (w) {
        case MI_UP: sel = (sel + ACH_COUNT - 1) % ACH_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % ACH_COUNT; break;
        case MI_LEFT: sel = sel >= 8 ? sel - 8 : 0; break;
        case MI_RIGHT: sel = sel + 8 < ACH_COUNT ? sel + 8 : ACH_COUNT - 1; break;
        case MI_BACK: case MI_OK: page = 0; for (sel = 0; items[sel] != IT_ACH; sel++) {} break;
        }
        return MR_NONE;
    }
    if (page == 4) {
        switch (w) {
        case MI_UP: sel = (sel + LG_COUNT - 1) % LG_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % LG_COUNT; break;
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_LANG; sel++) {} break;
        case MI_OK: case MI_LEFT: case MI_RIGHT:
            if (sel == LG_MENU) { cfg.lang = !cfg.lang; return MR_CHANGED; }
            if (sel == LG_GAME) { cfg.game_lang = !cfg.game_lang; return MR_CHANGED; }
            if (w == MI_OK) { page = 0; for (sel = 0; items[sel] != IT_LANG; sel++) {} }
        }
        return MR_NONE;
    }
    if (page == 3) {
        int dir = (w == MI_LEFT) ? -1 : 1;
        switch (w) {
        case MI_UP: sel = (sel + SC_COUNT - 1) % SC_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % SC_COUNT; break;
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_SCREEN; sel++) {} break;
        case MI_OK: case MI_LEFT: case MI_RIGHT:
            switch (sel) {
            case SC_ASPECT: cfg.aspect = (cfg.aspect + dir + ASP_COUNT) % ASP_COUNT; return MR_CHANGED;
            case SC_FILTER: {
                int i = 0;
                while (i < FILT_COUNT && filt_order[i] != cfg.filter) i++;
                cfg.filter = filt_order[((i % FILT_COUNT) + dir + FILT_COUNT) % FILT_COUNT];
                return MR_CHANGED;
            }
            case SC_COLORS: cfg.colors = (cfg.colors + dir + COL_COUNT) % COL_COUNT; return MR_CHANGED;
            case SC_BANDS: cfg.bands = (cfg.bands + dir + BAND_COUNT) % BAND_COUNT; return MR_CHANGED;
            case SC_OUTFIT: cfg.outfit = !cfg.outfit; return MR_CHANGED;
            case SC_BACK: if (w == MI_OK) { page = 0; for (sel = 0; items[sel] != IT_SCREEN; sel++) {} } break;
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
    /* pagina de controles: tactiles, acciones del mando, restaurar, volver (sel 0 = tactiles, 1.. = acciones) */
    {
        int n = ACT_SHOWN + 3;
        switch (w) {
        case MI_UP: sel = (sel + n - 1) % n; break;
        case MI_DOWN: sel = (sel + 1) % n; break;
        case MI_BACK: page = 0; for (sel = 0; items[sel] != IT_BUTTONS; sel++) {} break;
        case MI_LEFT: case MI_RIGHT:
            if (sel == 0) { cfg.touch = (cfg.touch + (w == MI_LEFT ? -1 : 1) + TOUCH_COUNT) % TOUCH_COUNT; return MR_CHANGED; }
            break;
        case MI_OK:
            if (sel == 0) { cfg.touch = (cfg.touch + 1) % TOUCH_COUNT; return MR_CHANGED; }
            if (sel <= ACT_SHOWN) { waiting = act_of_row(sel - 1); wait_ms = 5000; }
            else if (sel == ACT_SHOWN + 1) { map_defaults(); return MR_CHANGED; }
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

/* texto centrado partido en lineas de como mucho maxw pixeles (por palabras), desde y, 15 px por linea */
static void wrap_text(const char *s, int y, int maxw, int maxlines, uint32_t rgb) {
    char line[200]; int n = 0;
    while (*s && n < maxlines) {
        int len = 0, best = 0;
        while (s[len]) {
            int e = len; while (s[e] && s[e] != ' ') e++;
            memcpy(line, s, e); line[e] = 0;
            if (best && text_w(0, line) > maxw) break;
            best = e; len = s[e] ? e + 1 : e;
        }
        memcpy(line, s, best); line[best] = 0;
        ctext(0, y + n * 15, line, rgb, 255);
        s += best; while (*s == ' ') s++;
        n++;
    }
}
#define ORANGE 0xf09628
#define WHITE 0xffffff
#define GREY 0x9a9aa8

static int row_h = 20;   /* alto de la barra de la opcion elegida */
static void row(int i, int y, const char *label, const char *value) {
    int s = (i == sel);
    if (s) { rect(96, y - 2, 288, row_h, ORANGE, 70); rect(96, y - 2, 3, row_h, ORANGE, 255); }
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
    ctext(0, 52, page == 6 ? TR("Sonido y vibración", "Sound and rumble") : page == 5 ? TR("Logros", "Achievements") : page == 4 ? TR("Idioma", "Language") : page == 3 ? TR("Pantalla", "Display") : page == 2 ? TR("Trucos", "Cheats") : page == 1 ? TR("Controles", "Controls")
                 : in_game ? TR("PAUSA", "PAUSED") : "Remastered · Namco 1994", GREY, 255);
    y = 80;
    if (page == 0) {
        int step = nitems > 12 ? 14 : nitems > 11 ? 15 : nitems > 10 ? 16 : nitems > 9 ? 17 : nitems > 7 ? 19 : 22;
        y = nitems > 12 ? 70 : nitems > 11 ? 72 : nitems > 10 ? 70 : nitems > 9 ? 72 : nitems > 7 ? 74 : 80;
        row_h = step < 20 ? step + 1 : 20;
        for (int i = 0; i < nitems; i++, y += step) {
            char v[24];
            switch (items[i]) {
            case IT_PLAY: row(i, y, in_game ? TR("Continuar", "Resume") : TR("Jugar", "Play"), NULL); break;
            case IT_SAVE: snprintf(v, sizeof v, TR("Ranura %d", "Slot %d"), slot + 1); row(i, y, TR("Guardar partida", "Save game"), v); break;
            case IT_LOAD: snprintf(v, sizeof v, TR("Ranura %d", "Slot %d"), slot + 1); row(i, y, TR("Cargar partida", "Load game"), v); break;
            case IT_SCREEN: row(i, y, TR("Pantalla", "Display"), NULL); break;
            case IT_DIFFICULTY: row(i, y, TR("Dificultad", "Difficulty"), dif_name[cfg.lang][cfg.difficulty]); break;
            case IT_WEAPONS: row(i, y, TR("Armas", "Weapons"), weap_name[cfg.lang][cfg.weapons]); break;
            case IT_SOUND: row(i, y, TR("Sonido y vibración", "Sound and rumble"), NULL); break;
            case IT_TITLE: row(i, y, TR("Volver a la pantalla de inicio", "Back to title screen"), NULL); break;
            case IT_BUTTONS: row(i, y, TR("Controles", "Controls"), NULL); break;
            case IT_CHEATS: row(i, y, TR("Trucos", "Cheats"), NULL); break;
            case IT_ACH: snprintf(v, sizeof v, "%d/%d", ach_total(), ACH_COUNT); row(i, y, TR("Logros", "Achievements"), v); break;
            case IT_LANG: row(i, y, TR("Idioma", "Language"), NULL); break;
            case IT_QUIT: row(i, y, TR("Salir", "Quit"), NULL); break;
            }
        }
        if (items[sel] == IT_SAVE || items[sel] == IT_LOAD) {
            char b[96]; slot_info(slot, b, sizeof b);
            ctext(0, 252, b, WHITE, 255);
        } else if (items[sel] == IT_DIFFICULTY) ctext(0, 252, dif_help[cfg.lang][cfg.difficulty], WHITE, 255);
        else if (items[sel] == IT_WEAPONS) ctext(0, 252, weap_help[cfg.lang][cfg.weapons], WHITE, 255);
        else if (items[sel] == IT_TITLE) ctext(0, 252, TR("Reinicia la recreativa (lo no guardado se pierde)", "Restarts the arcade (unsaved progress is lost)"), WHITE, 255);
        else if (items[sel] == IT_SCREEN) ctext(0, 252, TR("Formato, filtro, colores y bandas", "Aspect, filter, colors and side bands"), GREY, 255);
        else if (items[sel] == IT_ACH) ctext(0, 252, TR("Los de RetroAchievements, en Normal o Difícil", "RetroAchievements list, Normal or Hard"), GREY, 255);
        else if (items[sel] == IT_LANG) ctext(0, 252, TR("Menú y textos del juego", "Menu and game text"), GREY, 255);
        else if (items[sel] == IT_BUTTONS) ctext(0, 252, TR("Controles táctiles y botones del mando", "Touch controls and gamepad buttons"), GREY, 255);
        else ctext(0, 252, in_game ? TR("↑↓ elegir   ←→ cambiar   A aceptar   B volver al juego", "↑↓ choose   ←→ change   A accept   B back to game")
                                   : TR("↑↓ elegir   ←→ cambiar   A aceptar", "↑↓ choose   ←→ change   A accept"), GREY, 255);
    } else if (page == 6) {
        char v[24];
        row_h = 20;
        snprintf(v, sizeof v, "%d%%", cfg.volume); row(SO_VOLUME, y, TR("Volumen", "Volume"), v); y += 22;
        if (cfg.vibration) snprintf(v, sizeof v, "%d%%", cfg.vibration); else snprintf(v, sizeof v, "%s", TR("No", "Off"));
        row(SO_VIBRATION, y, TR("Vibración", "Rumble"), v); y += 22;
        row(SO_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == SO_VIBRATION) ctext(0, 206, TR("En el mando; si no tiene motor, en la consola", "On the gamepad; if it has no motor, on the console"), WHITE, 255);
        ctext(0, 246, TR("←→ cambiar   B volver", "←→ change   B back"), GREY, 255);
    } else if (page == 5) {
        /* 8 filas y debajo la descripcion en hasta 3 lineas (en una sola se cortaba) */
        int first = sel < 4 ? 0 : sel > ACH_COUNT - 5 ? ACH_COUNT - 8 : sel - 4;
        char v[32];
        y = 68; row_h = 17;
        for (int i = first; i < first + 8 && i < ACH_COUNT; i++, y += 16) {
            char t[40];
            snprintf(v, sizeof v, "%s", ach_got(i) ? TR("Sí", "Yes") : ach_todo(i) ? TR("Pronto", "Soon") : "-");
            snprintf(t, sizeof t, "%s", ach_title(i));
            if (strlen(t) > 21) strcpy(t + 19, "...");
            row(i, y, t, v);
        }
        {
            char d[200]; 
            snprintf(d, sizeof d, TR("%s (%d puntos)", "%s (%d points)"), ach_desc(sel), ach_points(sel));
            wrap_text(d, 200, 420, 3, WHITE);
        }
        {
            char b[64]; int pts = 0, tot = 0;
            for (int i = 0; i < ACH_COUNT; i++) { tot += ach_points(i); if (ach_got(i)) pts += ach_points(i); }
            snprintf(b, sizeof b, TR("%d de %d logros · %d de %d puntos   B volver", "%d of %d · %d of %d points   B back"), ach_total(), ACH_COUNT, pts, tot);
            ctext(0, 252, b, GREY, 255);
        }
    } else if (page == 4) {
        row_h = 20;
        row(LG_MENU, y, TR("Menú", "Menu"), cfg.lang == LANG_EN ? "English" : "Español"); y += 22;
        row(LG_GAME, y, TR("Textos del juego", "Game text"), cfg.game_lang ? TR("Castellano", "Spanish") : TR("Inglés (original)", "English (original)")); y += 22;
        row(LG_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == LG_GAME) ctext(0, 206, TR("Historias, encargos y mensajes de la recreativa", "Stories, missions and messages of the arcade"), WHITE, 255);
        ctext(0, 246, TR("←→ cambiar   B volver", "←→ change   B back"), GREY, 255);
    } else if (page == 3) {
        row_h = 20;
        row(SC_ASPECT, y, TR("Formato", "Aspect ratio"), asp_name[cfg.lang][cfg.aspect]); y += 22;
        row(SC_FILTER, y, TR("Filtro", "Filter"), filt_name[cfg.lang][cfg.filter]); y += 22;
        row(SC_COLORS, y, TR("Colores", "Colors"), col_name[cfg.lang][cfg.colors]); y += 22;
        row(SC_BANDS, y, TR("Bandas laterales", "Side bands"), band_name[cfg.lang][cfg.bands]); y += 22;
        row(SC_OUTFIT, y, TR("Ropa del jugador 1", "Player 1 outfit"), cfg.outfit ? TR("Gótica", "Gothic") : "Original"); y += 22;
        row(SC_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == SC_FILTER) ctext(0, 206, filt_help[cfg.lang][cfg.filter], WHITE, 255);
        else if (sel == SC_ASPECT && cfg.aspect == ASP_87)
            ctext(0, 206, TR("Sin deformar: se recortan 20 píxeles por lado", "Not distorted: 20 pixels are cut on each side"), WHITE, 255);
        else if (sel == SC_BANDS) ctext(0, 206, cfg.aspect == ASP_169 ? TR("Solo se ven en 4:3 (en 16:9 el juego llena la pantalla)", "Only visible in 4:3 (16:9 fills the screen)")
                                                                      : TR("Ambiente: los lados toman los colores de la escena", "Ambient: the sides take the scene's colors"), WHITE, 255);
        else if (sel == SC_OUTFIT) ctext(0, 206, TR("Negros, grises y rojos; la piel no cambia", "Blacks, greys and reds; skin stays"), WHITE, 255);
        else if (sel == SC_COLORS) ctext(0, 206, TR("Cambia los colores, no el dibujo", "Changes the colors, not the graphics"), WHITE, 255);
        ctext(0, 246, TR("←→ cambiar   B volver", "←→ change   B back"), GREY, 255);
    } else if (page == 2) {
        const char *yes = TR("Sí", "On"), *no = TR("No", "Off");
        row_h = 20;
        row(CH_INVULN, y, TR("Invulnerable", "Invincible"), cfg.invuln ? yes : no); y += 22;
        row(CH_AMMO, y, TR("Munición infinita", "Infinite ammo"), cfg.ammo ? yes : no); y += 22;
        row(CH_ONEHIT, y, TR("Puño de la muerte", "Death punch"), cfg.onehit ? yes : no); y += 22;
        row(CH_STAGE, y, TR("Escenario", "Stage"), stage_name(cfg.stage)); y += 22;
        row(CH_BACK, y, TR("Volver", "Back"), NULL);
        if (cfg.stage >= 0) {
            char buf[96];
            snprintf(buf, sizeof buf, TR("Siguiente combate: %s, contra %s", "Next fight: %s, against %s"), stage_name(cfg.stage), rival_name(cfg.stage));
            ctext(0, 196, buf, WHITE, 255);
            ctext(0, 214, TR("No cambia si ya le has ganado o es tu personaje", "No change if already beaten or it's your character"), GREY, 255);
        } else ctext(0, 196, TR("Los combates siguen el orden del juego", "Fights follow the game's order"), GREY, 255);
        ctext(0, 246, TR("←→ cambiar   B volver", "←→ change   B back"), GREY, 255);
    } else {
        y = 70; row_h = 18;
        row(0, y, TR("Controles táctiles", "Touch controls"), touch_name[cfg.lang][cfg.touch]); y += 17;
        for (int i = 0; i < ACT_SHOWN; i++, y += 18) row(i + 1, y, act_name[cfg.lang][act_of_row(i)], padbtn_name(cfg.map[act_of_row(i)]));
        row(ACT_SHOWN + 1, y, TR("Restaurar botones", "Restore buttons"), NULL); y += 18;
        row(ACT_SHOWN + 2, y, TR("Volver", "Back"), NULL);
        ctext(0, 252, sel == 0 ? TR("Se esconden al usar el mando; vuelven al tocar", "Hidden when a gamepad is used; tap to show")
                               : TR("A cambiar el botón   B volver", "A change button   B back"), GREY, 255);
        if (waiting >= 0) {
            char buf[80];
            rect(70, 96, 340, 78, 0x101018, 255);
            rect(70, 96, 340, 3, ORANGE, 255);
            snprintf(buf, sizeof buf, TR("Pulsa el botón para «%s»", "Press the button for «%s»"), act_name[cfg.lang][waiting]);
            ctext(0, 114, buf, WHITE, 255);
            snprintf(buf, sizeof buf, TR("Se cancela en %d s", "Cancels in %d s"), (wait_ms + 999) / 1000);
            ctext(0, 142, buf, GREY, 255);
        }
    }
}

void menu_render_osd(uint32_t *argb, int paused, const char *msg) {
    cv = argb;
    memset(cv, 0, sizeof(uint32_t) * MENU_W * MENU_H);
    if (paused) {
        const char *p = TR("PAUSA", "PAUSED");
        rect(0, 100, MENU_W, 70, 0x06060a, 170);
        text(1, (MENU_W - text_w(1, p)) / 2 + 2, 108, p, 0x000000, 160);
        ctext(1, 106, p, ORANGE, 255);
        ctext(0, 146, TR("START seguir   SELECT / MENU menú", "START resume   SELECT / MENU menu"), GREY, 255);
    }
    if (msg && *msg) {
        int w = text_w(0, msg) + 16;
        rect(8, 8, w, 22, 0x06060a, 190);
        rect(8, 8, 3, 22, ORANGE, 255);
        text(0, 16, 11, msg, WHITE, 255);
    }
}
