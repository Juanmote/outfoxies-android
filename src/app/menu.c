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
#include "nb2.h"

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
    cfg.filter = FILT_SMOOTH; cfg.aspect = ASP_169; cfg.volume = 100; cfg.vibration = 50; cfg.music = cfg.sfx = 100;
    cfg.colors = COL_ORIGINAL; cfg.bands = BAND_COVER; cfg.difficulty = DIF_NORMAL;
    cfg.invuln = cfg.ammo = cfg.onehit = cfg.noifr = 0; cfg.stage = -1;
    cfg.lang = LANG_ES; cfg.touch = TOUCH_M; cfg.game_lang = 1; cfg.outfit = cfg.outfit_rival = 0; cfg.weapons = 0;
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
        else if (!strcmp(key, "bandas2") && v >= 0 && v < BAND_COUNT) cfg.bands = v;   /* "bandas" (sin portada) se ignora */
        else if (!strcmp(key, "musica") && v >= 0 && v <= 100) cfg.music = v;
        else if (!strcmp(key, "efectos") && v >= 0 && v <= 100) cfg.sfx = v;
        else if (!strcmp(key, "truco_sin_invencibilidad") && v >= 0 && v <= 3) cfg.noifr = v;
        else if (!strcmp(key, "ropa_rival") && v >= 0 && v < NB2_OUTFIT_COUNT) cfg.outfit_rival = v;
        else if (!strcmp(key, "dificultad") && v >= 0 && v < DIF_COUNT) cfg.difficulty = v;
        else if (!strcmp(key, "truco_invulnerable")) cfg.invuln = v != 0;
        else if (!strcmp(key, "truco_municion")) cfg.ammo = v != 0;
        else if (!strcmp(key, "truco_puno")) cfg.onehit = v != 0;
        else if (!strcmp(key, "truco_escenario") && v >= -1 && v <= 7) cfg.stage = v;
        else if (!strcmp(key, "idioma") && v >= 0 && v < LANG_COUNT) { cfg.lang = v; has_lang = 1; }
        else if (!strcmp(key, "tactil") && v >= 0 && v < TOUCH_COUNT) cfg.touch = v;
        else if (!strcmp(key, "ropa") && v >= 0 && v < NB2_OUTFIT_COUNT) cfg.outfit = v;
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
    fprintf(f, "colores=%d\nbandas2=%d\ndificultad=%d\nmusica=%d\nefectos=%d\n", cfg.colors, cfg.bands, cfg.difficulty, cfg.music, cfg.sfx);
    fprintf(f, "truco_sin_invencibilidad=%d\nropa_rival=%d\n", cfg.noifr, cfg.outfit_rival);
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
enum { IT_PLAY, IT_GAMES, IT_SCREEN, IT_DIFFICULTY, IT_WEAPONS, IT_SOUND, IT_BUTTONS, IT_CHEATS, IT_ACH, IT_LANG, IT_TITLE, IT_QUIT, IT_BACK };
/* paginas */
enum { PG_MAIN, PG_BUTTONS, PG_CHEATS, PG_SCREEN, PG_LANG, PG_ACH, PG_SOUND, PG_GAMES };
/* pagina de sonido */
enum { SO_VOLUME, SO_MUSIC, SO_SFX, SO_VIBRATION, SO_BACK, SO_COUNT };
/* pagina de pantalla */
enum { SC_ASPECT, SC_FILTER, SC_COLORS, SC_BANDS, SC_OUTFIT, SC_OUTFIT2, SC_BACK, SC_COUNT };
enum { LG_MENU, LG_GAME, LG_BACK, LG_COUNT };
/* pagina de trucos */
enum { CH_INVULN, CH_AMMO, CH_ONEHIT, CH_NOIFR, CH_STAGE, CH_BACK, CH_COUNT };
/* pagina de partidas (guardar y cargar juntas): la fila de guardar solo esta en partida */
enum { GA_SLOT, GA_SAVE, GA_LOAD, GA_BACK, GA_COUNT };
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
static int items[16], nitems, slot;
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
static const char *band_name[LANG_COUNT][BAND_COUNT] = {{"Negras", "Ambiente", "Portada"}, {"Black", "Ambient", "Cover art"}};
static const char *outfit_name[LANG_COUNT][NB2_OUTFIT_COUNT] = {{"Original", "Gótica", "Negativa", "Pinky", "Vampiro"},
                                                               {"Original", "Gothic", "Negative", "Pinky", "Vampire"}};
static const char *outfit_help[LANG_COUNT][NB2_OUTFIT_COUNT] = {
    {"La ropa de la recreativa", "Negros, grises y rojos; la piel no cambia", "El color contrario de cada prenda",
     "Rosas, fucsias y lavanda", "Negro y rojo, con la piel pálida"},
    {"The arcade's clothes", "Blacks, greys and reds; skin stays", "Each garment in its opposite color",
     "Pinks, fuchsias and lavender", "Black and red, with pale skin"}};
static const char *noifr_name[LANG_COUNT][4] = {{"Normal", "Tú no", "El rival no", "Nadie"},
                                                {"Normal", "Not you", "Not the rival", "Nobody"}};
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

/* Launcher: al arrancar no hay lista, hay botones (launcher.c). launcher = el menu se abrio al arrancar; at_launch = se
   ven los botones (ninguna pagina abierta). Con el launcher, la pagina principal es la de OPCIONES, que agrupa pantalla,
   sonido, idioma, dificultad y armas; el resto de paginas cuelgan directamente de su boton. */
static int launcher, at_launch, launch_sel = LB_PLAY, launch_two;
static void build(void) {
    nitems = 0;
    if (launcher) {
        items[nitems++] = IT_SCREEN; items[nitems++] = IT_SOUND; items[nitems++] = IT_LANG;
        items[nitems++] = IT_DIFFICULTY; items[nitems++] = IT_WEAPONS; items[nitems++] = IT_BACK;
        return;
    }
    items[nitems++] = IT_PLAY;
    items[nitems++] = IT_GAMES; items[nitems++] = IT_SCREEN; items[nitems++] = IT_DIFFICULTY; items[nitems++] = IT_WEAPONS;
    items[nitems++] = IT_SOUND; items[nitems++] = IT_BUTTONS; items[nitems++] = IT_CHEATS;
    items[nitems++] = IT_ACH; items[nitems++] = IT_LANG;
    if (in_game) items[nitems++] = IT_TITLE;
    items[nitems++] = IT_QUIT;
}
void menu_open(int game) {
    in_game = game; page = 0; sel = 0; waiting = -1;
    launcher = !game; at_launch = launcher; launch_sel = LB_PLAY;
    build();
}
int menu_at_launcher(void) { return launcher && at_launch; }
int menu_launch_sel(void) { return launch_sel; }
static int launch_go(void) {
    switch (launch_sel) {
    case LB_PLAY: return MR_PLAY;
    case LB_QUIT: return MR_QUIT;
    case LB_OPTIONS: page = PG_MAIN; break;
    case LB_CONTROLS: page = PG_BUTTONS; break;
    case LB_GAMES: page = PG_GAMES; break;
    case LB_ACH: page = PG_ACH; break;
    case LB_CHEATS: page = PG_CHEATS; break;
    }
    sel = 0; at_launch = 0;
    return MR_NONE;
}
int menu_launch_tap(int b) {
    if (!menu_at_launcher() || b < 0 || b >= LB_COUNT) return MR_NONE;
    launch_sel = b;
    return launch_go();
}
/* moverse por los botones. En una fila: izquierda y derecha. En dos filas (PARTIDAS JUGAR LOGROS / OPCIONES CONTROLES
   TRUCOS SALIR): izquierda y derecha en orden de lectura, arriba y abajo al boton mas cercano de la otra fila. */
static int launch_input(int w) {
    static const int order2[LB_COUNT] = {LB_GAMES, LB_PLAY, LB_ACH, LB_OPTIONS, LB_CONTROLS, LB_CHEATS, LB_QUIT};
    static const int down2[LB_COUNT] = {-1, -1, LB_OPTIONS, LB_CONTROLS, LB_QUIT, -1, -1};
    static const int up2[LB_COUNT] = {LB_GAMES, LB_PLAY, -1, -1, -1, LB_PLAY, LB_ACH};
    int pos = launch_sel;
    if (launch_two) for (pos = 0; pos < LB_COUNT - 1 && order2[pos] != launch_sel; pos++) {}
    switch (w) {
    case MI_LEFT: case MI_RIGHT:
        pos = (pos + (w == MI_LEFT ? LB_COUNT - 1 : 1)) % LB_COUNT;
        launch_sel = launch_two ? order2[pos] : pos;
        break;
    case MI_UP: if (launch_two && up2[launch_sel] >= 0) launch_sel = up2[launch_sel]; break;
    case MI_DOWN: if (launch_two && down2[launch_sel] >= 0) launch_sel = down2[launch_sel]; break;
    case MI_OK: return launch_go();
    case MI_BACK: launch_sel = LB_QUIT; break;      /* B o Atras: se va al boton de salir (no cierra sin querer) */
    }
    return MR_NONE;
}
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
    case GA_SLOT + 200: slot = (slot + dir + NSLOTS) % NSLOTS; return MR_NONE;
    case IT_DIFFICULTY: {
        int v = cfg.difficulty + dir;
        if (v < 0 || v >= DIF_COUNT) return MR_NONE;
        cfg.difficulty = v; return MR_CHANGED;
    }
    case IT_WEAPONS: {
        cfg.weapons = (cfg.weapons + dir + WEAPONS_COUNT) % WEAPONS_COUNT; return MR_CHANGED;
    }
    case SO_VOLUME + 100: case SO_MUSIC + 100: case SO_SFX + 100: {
        int *t = it == SO_VOLUME + 100 ? &cfg.volume : it == SO_MUSIC + 100 ? &cfg.music : &cfg.sfx, v = *t + dir * 10;
        if (v < 0) v = 0; if (v > 100) v = 100;
        if (v == *t) return MR_NONE;
        *t = v; return MR_CHANGED;
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
/* volver de una pagina: en partida, a la lista principal; con el launcher, pantalla / sonido / idioma vuelven a
   OPCIONES y el resto a los botones */
static void back_to(int it) {
    if (launcher && it != IT_SCREEN && it != IT_SOUND && it != IT_LANG) { at_launch = 1; return; }
    page = PG_MAIN; for (sel = 0; sel < nitems - 1 && items[sel] != it; sel++) {}
}
/* filas de la pagina de partidas que se ven (sin "guardar" fuera de partida) */
static int ga_rows(int *r) { int n = 0; r[n++] = GA_SLOT; if (in_game) r[n++] = GA_SAVE; r[n++] = GA_LOAD; r[n++] = GA_BACK; return n; }
int menu_input(int w) {
    if (waiting >= 0) { if (w == MI_BACK) waiting = -1; return MR_NONE; }
    if (launcher && at_launch) return launch_input(w);
    if (page == 0) {
        int it = items[sel];
        switch (w) {
        case MI_UP: sel = (sel + nitems - 1) % nitems; break;
        case MI_DOWN: sel = (sel + 1) % nitems; break;
        case MI_LEFT: return change(it, -1);
        case MI_RIGHT: return change(it, 1);
        case MI_BACK: if (in_game) return MR_PLAY; at_launch = launcher; break;
        case MI_OK:
            switch (it) {
            case IT_PLAY: return MR_PLAY;
            case IT_GAMES: page = PG_GAMES; sel = 0; break;
            case IT_BUTTONS: page = 1; sel = 0; break;
            case IT_SCREEN: page = 3; sel = 0; break;
            case IT_CHEATS: page = 2; sel = 0; break;
            case IT_LANG: page = 4; sel = 0; break;
            case IT_ACH: page = 5; sel = 0; break;
            case IT_SOUND: page = 6; sel = 0; break;
            case IT_TITLE: return MR_TITLE;
            case IT_QUIT: return MR_QUIT;
            case IT_BACK: at_launch = launcher; break;
            default: return change(it, 1);
            }
        }
        return MR_NONE;
    }
    if (page == PG_GAMES) {   /* partidas: ranura, guardar (en partida), cargar, volver */
        int r[GA_COUNT], n = ga_rows(r), row = r[sel % n];
        char b[96];
        switch (w) {
        case MI_UP: sel = (sel + n - 1) % n; break;
        case MI_DOWN: sel = (sel + 1) % n; break;
        case MI_LEFT: case MI_RIGHT: return change(GA_SLOT + 200, w == MI_LEFT ? -1 : 1);
        case MI_BACK: back_to(IT_GAMES); break;
        case MI_OK:
            if (row == GA_SLOT) return change(GA_SLOT + 200, 1);
            if (row == GA_SAVE) return MR_SAVE;
            if (row == GA_LOAD) { if (slot_info(slot, b, sizeof b)) return MR_LOAD; break; }
            back_to(IT_GAMES);
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
        case MI_BACK: back_to(IT_SOUND); break;
        }
        return MR_NONE;
    }
    if (page == 5) {   /* logros: solo se mira la lista */
        switch (w) {
        case MI_UP: sel = (sel + ACH_COUNT - 1) % ACH_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % ACH_COUNT; break;
        case MI_LEFT: sel = sel >= 8 ? sel - 8 : 0; break;
        case MI_RIGHT: sel = sel + 8 < ACH_COUNT ? sel + 8 : ACH_COUNT - 1; break;
        case MI_BACK: case MI_OK: back_to(IT_ACH); break;
        }
        return MR_NONE;
    }
    if (page == 4) {
        switch (w) {
        case MI_UP: sel = (sel + LG_COUNT - 1) % LG_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % LG_COUNT; break;
        case MI_BACK: back_to(IT_LANG); break;
        case MI_OK: case MI_LEFT: case MI_RIGHT:
            if (sel == LG_MENU) { cfg.lang = !cfg.lang; return MR_CHANGED; }
            if (sel == LG_GAME) { cfg.game_lang = !cfg.game_lang; return MR_CHANGED; }
            if (w == MI_OK) { back_to(IT_LANG); }
        }
        return MR_NONE;
    }
    if (page == 3) {
        int dir = (w == MI_LEFT) ? -1 : 1;
        switch (w) {
        case MI_UP: sel = (sel + SC_COUNT - 1) % SC_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % SC_COUNT; break;
        case MI_BACK: back_to(IT_SCREEN); break;
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
            case SC_OUTFIT: cfg.outfit = (cfg.outfit + dir + NB2_OUTFIT_COUNT) % NB2_OUTFIT_COUNT; return MR_CHANGED;
            case SC_OUTFIT2: cfg.outfit_rival = (cfg.outfit_rival + dir + NB2_OUTFIT_COUNT) % NB2_OUTFIT_COUNT; return MR_CHANGED;
            case SC_BACK: if (w == MI_OK) { back_to(IT_SCREEN); } break;
            }
        }
        return MR_NONE;
    }
    if (page == 2) {
        int dir = (w == MI_LEFT) ? -1 : 1;
        switch (w) {
        case MI_UP: sel = (sel + CH_COUNT - 1) % CH_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % CH_COUNT; break;
        case MI_BACK: back_to(IT_CHEATS); break;
        case MI_OK: case MI_LEFT: case MI_RIGHT:
            switch (sel) {
            case CH_INVULN: cfg.invuln = !cfg.invuln; return MR_CHANGED;
            case CH_AMMO: cfg.ammo = !cfg.ammo; return MR_CHANGED;
            case CH_ONEHIT: cfg.onehit = !cfg.onehit; return MR_CHANGED;
            case CH_NOIFR: cfg.noifr = (cfg.noifr + dir + 4) % 4; return MR_CHANGED;
            case CH_STAGE: {
                int i = 0;
                while (i < 9 && stage_order[i] != cfg.stage) i++;
                cfg.stage = stage_order[((i % 9) + dir + 9) % 9];
                return MR_CHANGED;
            }
            case CH_BACK: if (w == MI_OK) { back_to(IT_CHEATS); } break;
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
        case MI_BACK: back_to(IT_BUTTONS); break;
        case MI_LEFT: case MI_RIGHT:
            if (sel == 0) { cfg.touch = (cfg.touch + (w == MI_LEFT ? -1 : 1) + TOUCH_COUNT) % TOUCH_COUNT; return MR_CHANGED; }
            break;
        case MI_OK:
            if (sel == 0) { cfg.touch = (cfg.touch + 1) % TOUCH_COUNT; return MR_CHANGED; }
            if (sel <= ACT_SHOWN) { waiting = act_of_row(sel - 1); wait_ms = 5000; }
            else if (sel == ACT_SHOWN + 1) { map_defaults(); return MR_CHANGED; }
            else { back_to(IT_BUTTONS); }
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

/* ---------- dibujo ----------
   Se dibuja en coordenadas del diseno (480x270); blend() las pasa al lienzo real sumando (ox, oy). */
int menu_cw = MENU_W, menu_ch = MENU_H;
static int ox, oy;
void menu_layout(int sw, int sh) {
    /* 16:9 o mas ancho: 480x270. Mas cuadrada: 400 de ancho (lo justo para el diseno) y el alto que pida la pantalla */
    launch_two = sw > 0 && sh > 0 && LAUNCH_TWO_ROWS(sw, sh);
    if (sw <= 0 || sh <= 0 || sw * 9 >= sh * 16) { menu_cw = MENU_W; menu_ch = MENU_H; }
    else {
        menu_cw = 400; menu_ch = 400 * sh / sw;
        if (menu_ch < MENU_H) { menu_ch = MENU_H; menu_cw = MENU_H * sw / sh; if (menu_cw > MENU_W) menu_cw = MENU_W; }
        if (menu_ch > MENU_MAXH) menu_ch = MENU_MAXH;
    }
    ox = (menu_cw - MENU_W) / 2; oy = (menu_ch - MENU_H) / 2;
}
static uint32_t *cv;
static void blend(int x, int y, uint32_t rgb, int a) {
    uint32_t d, da, r, g, b;
    x += ox; y += oy;
    if (x < 0 || y < 0 || x >= menu_cw || y >= menu_ch || a <= 0) return;
    if (a > 255) a = 255;
    d = cv[y * menu_cw + x]; da = d >> 24;
    r = (((rgb >> 16) & 255) * a + ((d >> 16) & 255) * (255 - a)) / 255;
    g = (((rgb >> 8) & 255) * a + ((d >> 8) & 255) * (255 - a)) / 255;
    b = ((rgb & 255) * a + (d & 255) * (255 - a)) / 255;
    da = a + da * (255 - a) / 255;
    cv[y * menu_cw + x] = da << 24 | r << 16 | g << 8 | b;
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
    return 31;   /* '?' */
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

/* texto centrado partido en lineas de como mucho maxw pixeles (por palabras), desde y, 15 px por linea.
   Con draw = 0 solo cuenta las lineas. Devuelve cuantas lineas ocupa. */
static int wrap_lines(const char *s, int y, int maxw, int maxlines, uint32_t rgb, int draw) {
    char line[200]; int n = 0;
    while (*s && n < maxlines) {
        int len = 0, best = 0;
        while (s[len]) {
            int e = len; while (s[e] && s[e] != ' ') e++;
            if (e >= (int)sizeof line) break;
            memcpy(line, s, e); line[e] = 0;
            if (best && text_w(0, line) > maxw) break;
            best = e; len = s[e] ? e + 1 : e;
        }
        if (!best) break;
        memcpy(line, s, best); line[best] = 0;
        if (draw) ctext(0, y + n * 15, line, rgb, 255);
        s += best; while (*s == ' ') s++;
        n++;
    }
    return n;
}
static void wrap_text(const char *s, int y, int maxw, int maxlines, uint32_t rgb) { wrap_lines(s, y, maxw, maxlines, rgb, 1); }
/* Texto de ayuda: hasta 3 lineas centradas con margen a los lados. En el lienzo estrecho (8:7, 4:3) las lineas son mas
   cortas para que no lleguen al borde de la pantalla. Si no cabe hasta abajo, sube. Devuelve la y de la linea siguiente. */
static int help_w(void) { return menu_cw < MENU_W ? menu_cw * 82 / 100 : 440; }
static int help_text(int y, const char *s, uint32_t rgb) {
    int n = wrap_lines(s, y, help_w(), 3, rgb, 0), bottom = MENU_H + oy - 4;
    if (y + n * 15 > bottom) y = bottom - n * 15;
    wrap_lines(s, y, help_w(), 3, rgb, 1);
    return y + n * 15;
}
/* pie de pagina (que hace cada boton): baja un poco en el lienzo alto para dejar sitio a la ayuda */
static int foot_y(void) { return 246 + (oy > 24 ? 24 : oy > 0 ? oy : 0); }
#define ORANGE 0xf09628
#define WHITE 0xffffff
#define GREY 0x9a9aa8

static int row_h = 20, row_x0 = 96, row_x1 = 384;   /* barra de la opcion elegida y ancho de las filas */
/* donde se ha dibujado cada fila (para tocarlas con el dedo) */
#define MAXROWS 32
static struct { int i, y0, y1, x0, x1; } rows_at[MAXROWS];
static int nrows_at;
static void row(int i, int y, const char *label, const char *value) {
    int s = (i == sel);
    if (nrows_at < MAXROWS) { rows_at[nrows_at].i = i; rows_at[nrows_at].y0 = y - 2; rows_at[nrows_at].y1 = y - 2 + row_h;
                              rows_at[nrows_at].x0 = row_x0; rows_at[nrows_at].x1 = row_x1; nrows_at++; }
    if (s) { rect(row_x0, y - 2, row_x1 - row_x0, row_h, ORANGE, 70); rect(row_x0, y - 2, 3, row_h, ORANGE, 255); }
    if (value) {
        /* el valor manda: si la etiqueta no cabe a su izquierda se acorta con "..." (nunca se pisan) */
        char buf[64], lab[96]; int vx, room;
        if (s) snprintf(buf, sizeof buf, "‹ %s ›", value); else snprintf(buf, sizeof buf, "%s", value);
        vx = row_x1 - 12 - text_w(0, buf); room = vx - 8 - (row_x0 + 14);
        snprintf(lab, sizeof lab, "%s", label);
        if (text_w(0, lab) > room) {
            int n = (int)strlen(lab);
            while (n > 1) {
                n--; while (n > 1 && ((unsigned char)lab[n] & 0xc0) == 0x80) n--;   /* sin partir una letra con tilde */
                while (n > 1 && lab[n - 1] == ' ') n--;
                strcpy(lab + n, "...");
                if (text_w(0, lab) <= room) break;
            }
        }
        text(0, row_x0 + 14, y, lab, s ? WHITE : GREY, 255);
        text(0, vx, y, buf, s ? ORANGE : GREY, 255);
    } else text(0, row_x0 + 14, y, label, s ? WHITE : GREY, 255);
}
/* vista previa de la pagina Pantalla, en coordenadas del diseno */
#define PV_X 310
#define PV_Y 84
#define PV_W 124
#define PV_H 93
int menu_preview_rect(int *x, int *y, int *w, int *h) {
    if (page != PG_SCREEN) return 0;
    *x = PV_X + ox; *y = PV_Y + oy; *w = PV_W; *h = PV_H;
    return 1;
}

/* toque en (x, y) del lienzo: una fila no elegida se elige; la elegida se acepta (o cambia su valor) */
int menu_tap(int x, int y) {
    x -= ox; y -= oy;
    if (waiting >= 0) return MR_NONE;
    for (int k = 0; k < nrows_at; k++)
        if (y >= rows_at[k].y0 && y < rows_at[k].y1 && x >= rows_at[k].x0 - 20 && x < rows_at[k].x1 + 20) {
            if (rows_at[k].i == sel) return menu_input(MI_OK);
            sel = rows_at[k].i;
            return MR_NONE;
        }
    return MR_NONE;
}
void menu_render(uint32_t *argb) {
    int y;
    cv = argb;
    nrows_at = 0;
    if (launcher && at_launch) { memset(cv, 0, sizeof(uint32_t) * menu_cw * menu_ch); return; }   /* solo los botones (launcher.c) */
    /* fondo: en partida, el juego oscurecido (lo pone main.c); al arrancar, la portada (main.c) bajo un velo oscuro */
    if (in_game) for (int i = 0; i < menu_cw * menu_ch; i++) cv[i] = 0x50000000 | 0x06060a;
    else for (int yy = 0; yy < menu_ch; yy++) {
        int t = yy * 255 / menu_ch;
        uint32_t r = 26 - 21 * t / 255, g = 16 - 11 * t / 255, b = 48 - 40 * t / 255;
        for (int x = 0; x < menu_cw; x++) cv[yy * menu_cw + x] = 0xb4000000 | r << 16 | g << 8 | b;
    }
    row_x0 = 96; row_x1 = 384;
    text(1, (MENU_W - text_w(1, "THE OUTFOXIES")) / 2 + 2, 16, "THE OUTFOXIES", 0x000000, 160);
    ctext(1, 14, "THE OUTFOXIES", ORANGE, 255);
    ctext(0, 52, page == PG_GAMES ? TR("Partidas", "Saved games") : page == 6 ? TR("Sonido y vibración", "Sound and rumble") : page == 5 ? TR("Logros", "Achievements") : page == 4 ? TR("Idioma", "Language") : page == 3 ? TR("Pantalla", "Display") : page == 2 ? TR("Trucos", "Cheats") : page == 1 ? TR("Controles", "Controls")
                 : in_game ? TR("PAUSA", "PAUSED") : TR("Opciones", "Options"), GREY, 255);
    y = 80;
    if (page == 0) {
        int step = nitems > 12 ? 14 : nitems > 11 ? 14 : nitems > 10 ? 15 : nitems > 9 ? 17 : nitems > 7 ? 19 : 22;
        y = nitems > 11 ? 70 : nitems > 10 ? 72 : nitems > 9 ? 72 : nitems > 7 ? 74 : 80;
        row_h = step < 20 ? step + 1 : 20;
        for (int i = 0; i < nitems; i++, y += step) {
            char v[24];
            switch (items[i]) {
            case IT_PLAY: row(i, y, in_game ? TR("Continuar", "Resume") : TR("Jugar", "Play"), NULL); break;
            case IT_GAMES: row(i, y, in_game ? TR("Guardar y cargar partida", "Save and load game") : TR("Cargar partida", "Load game"), NULL); break;
            case IT_SCREEN: row(i, y, TR("Pantalla", "Display"), NULL); break;
            case IT_DIFFICULTY: row(i, y, TR("Dificultad", "Difficulty"), dif_name[cfg.lang][cfg.difficulty]); break;
            case IT_WEAPONS: row(i, y, TR("Armas", "Weapons"), weap_name[cfg.lang][cfg.weapons]); break;
            case IT_SOUND: row(i, y, TR("Sonido y vibración", "Sound and rumble"), NULL); break;
            case IT_TITLE: row(i, y, TR("Volver a la pantalla de inicio", "Back to title screen"), NULL); break;
            case IT_BUTTONS: row(i, y, TR("Controles", "Controls"), NULL); break;
            case IT_CHEATS: row(i, y, TR("Trucos", "Cheats"), NULL); break;
            case IT_ACH: snprintf(v, sizeof v, "%d/%d", ach_total(), ACH_COUNT); row(i, y, TR("Logros", "Achievements"), v); break;
            case IT_LANG: row(i, y, TR("Idioma", "Language"), NULL); break;
            case IT_QUIT: row(i, y, TR("Salir del juego", "Quit game"), NULL); break;
            case IT_BACK: row(i, y, TR("Volver", "Back"), NULL); break;
            }
        }
        if (items[sel] == IT_DIFFICULTY) help_text(252, dif_help[cfg.lang][cfg.difficulty], WHITE);
        else if (items[sel] == IT_WEAPONS) help_text(252, weap_help[cfg.lang][cfg.weapons], WHITE);
        else if (items[sel] == IT_TITLE) help_text(252, TR("Reinicia la recreativa (lo no guardado se pierde)", "Restarts the arcade (unsaved progress is lost)"), WHITE);
        else if (items[sel] == IT_QUIT) help_text(252, TR("Cierra la aplicación", "Closes the app"), WHITE);
        else if (items[sel] == IT_GAMES) help_text(252, TR("5 ranuras, en la carpeta de la app", "5 slots, in the app's folder"), GREY);
        else if (items[sel] == IT_SCREEN) help_text(252, TR("Formato, filtro, colores, bandas y ropa", "Aspect, filter, colors, bands and outfits"), GREY);
        else if (items[sel] == IT_ACH) help_text(252, TR("Los de RetroAchievements, en Normal o Difícil", "RetroAchievements list, Normal or Hard"), GREY);
        else if (items[sel] == IT_LANG) help_text(252, TR("Menú y textos del juego", "Menu and game text"), GREY);
        else if (items[sel] == IT_BUTTONS) help_text(252, TR("Controles táctiles y botones del mando", "Touch controls and gamepad buttons"), GREY);
        else help_text(252, in_game ? TR("↑↓ elegir  ←→ cambiar  A aceptar  B volver al juego", "↑↓ choose  ←→ change  A accept  B back to game")
                                   : TR("↑↓ elegir   ←→ cambiar   A aceptar   B volver", "↑↓ choose   ←→ change   A accept   B back"), GREY);
    } else if (page == PG_GAMES) {
        int r[GA_COUNT], n = ga_rows(r);
        char v[24], b[96];
        row_h = 20;
        for (int i = 0; i < n; i++, y += 22) {
            switch (r[i]) {
            case GA_SLOT: snprintf(v, sizeof v, "%d / %d", slot + 1, NSLOTS); row(i, y, TR("Ranura", "Slot"), v); break;
            case GA_SAVE: row(i, y, TR("Guardar en esta ranura", "Save to this slot"), NULL); break;
            case GA_LOAD: row(i, y, TR("Cargar esta ranura", "Load this slot"), NULL); break;
            case GA_BACK: row(i, y, TR("Volver", "Back"), NULL); break;
            }
        }
        slot_info(slot, b, sizeof b);
        help_text(196, b, WHITE);
        help_text(foot_y(), TR("←→ ranura   A aceptar   B volver", "←→ slot   A accept   B back"), GREY);
    } else if (page == 6) {
        char v[24];
        row_h = 20;
        snprintf(v, sizeof v, "%d%%", cfg.volume); row(SO_VOLUME, y, TR("Volumen general", "Master volume"), v); y += 22;
        snprintf(v, sizeof v, "%d%%", cfg.music); row(SO_MUSIC, y, TR("Música", "Music"), v); y += 22;
        snprintf(v, sizeof v, "%d%%", cfg.sfx); row(SO_SFX, y, TR("Efectos y voces", "Effects and voices"), v); y += 22;
        if (cfg.vibration) snprintf(v, sizeof v, "%d%%", cfg.vibration); else snprintf(v, sizeof v, "%s", TR("No", "Off"));
        row(SO_VIBRATION, y, TR("Vibración", "Rumble"), v); y += 22;
        row(SO_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == SO_VIBRATION) help_text(196, TR("En el mando; si no tiene motor, en la consola", "On the gamepad; if it has no motor, on the console"), WHITE);
        else if (sel == SO_MUSIC || sel == SO_SFX) help_text(196, TR("La placa toca la música y los efectos por separado", "The board plays music and effects separately"), WHITE);
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
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
            wrap_text(d, 200, help_w() < 380 ? help_w() : 380, 3, WHITE);
        }
        {
            char b[64]; int pts = 0, tot = 0;
            for (int i = 0; i < ACH_COUNT; i++) { tot += ach_points(i); if (ach_got(i)) pts += ach_points(i); }
            /* en el lienzo estrecho no cabe en una linea con "B volver": solo la cuenta */
            snprintf(b, sizeof b, menu_cw < MENU_W ? TR("%d de %d logros · %d de %d puntos", "%d of %d · %d of %d points")
                                                   : TR("%d de %d logros · %d de %d puntos   B volver", "%d of %d · %d of %d points   B back"), ach_total(), ACH_COUNT, pts, tot);
            help_text(252, b, GREY);
        }
    } else if (page == 4) {
        row_h = 20;
        row(LG_MENU, y, TR("Menú", "Menu"), cfg.lang == LANG_EN ? "English" : "Español"); y += 22;
        row(LG_GAME, y, TR("Textos del juego", "Game text"), cfg.game_lang ? TR("Castellano", "Spanish") : TR("Inglés (original)", "English (original)")); y += 22;
        row(LG_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == LG_GAME) help_text(196, TR("Historias, encargos y mensajes de la recreativa", "Stories, missions and messages of the arcade"), WHITE);
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else if (page == 3) {
        /* filas a la izquierda y vista previa a la derecha (la dibuja main.c en menu_preview_rect) */
        const char *help = NULL;
        row_h = 18; row_x0 = 42; row_x1 = 304; y = 72;
        row(SC_ASPECT, y, TR("Formato", "Aspect"), asp_name[cfg.lang][cfg.aspect]); y += 19;
        row(SC_FILTER, y, TR("Filtro", "Filter"), filt_name[cfg.lang][cfg.filter]); y += 19;
        row(SC_COLORS, y, TR("Colores", "Colors"), col_name[cfg.lang][cfg.colors]); y += 19;
        row(SC_BANDS, y, TR("Bandas", "Bands"), band_name[cfg.lang][cfg.bands]); y += 19;
        row(SC_OUTFIT, y, TR("Ropa jugador 1", "P1 outfit"), outfit_name[cfg.lang][cfg.outfit]); y += 19;
        row(SC_OUTFIT2, y, TR("Ropa del rival", "Rival outfit"), outfit_name[cfg.lang][cfg.outfit_rival]); y += 19;
        row(SC_BACK, y, TR("Volver", "Back"), NULL);
        rect(PV_X - 2, PV_Y - 2, PV_W + 4, PV_H + 4, ORANGE, 120);    /* marco de la vista previa */
        rect(PV_X, PV_Y, PV_W, PV_H, 0x000000, 255);
        if (sel == SC_FILTER) help = filt_help[cfg.lang][cfg.filter];
        else if (sel == SC_ASPECT && cfg.aspect == ASP_87) help = TR("En combate se recortan 20 píxeles por lado; el resto, entero con relleno", "In fights 20 pixels are cut per side; the rest, whole with filler");
        else if (sel == SC_BANDS) help = cfg.aspect == ASP_169 ? TR("Solo se ven si la imagen no llena la pantalla", "Only visible if the picture doesn't fill the screen")
                                   : cfg.bands == BAND_COVER ? TR("La portada, fija, en lo que no tapa el juego", "The cover art, still, around the game")
                                   : TR("Ambiente: los lados toman los colores de la escena", "Ambient: the sides take the scene's colors");
        else if (sel == SC_OUTFIT) help = outfit_help[cfg.lang][cfg.outfit];
        else if (sel == SC_OUTFIT2) help = outfit_help[cfg.lang][cfg.outfit_rival];
        else if (sel == SC_COLORS) help = TR("Cambia los colores, no el dibujo", "Changes the colors, not the graphics");
        { int hy = 206;
          if (help) hy = help_text(hy, help, WHITE);
          if (sel == SC_OUTFIT || sel == SC_OUTFIT2) help_text(hy, TR("La ropa solo cambia en los combates", "Outfits only change during fights"), GREY); }
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else if (page == 2) {
        const char *yes = TR("Sí", "On"), *no = TR("No", "Off");
        row_h = 18; row_x0 = 62; row_x1 = 418;   /* filas mas anchas: "Inmunidad tras golpe" y su valor no cabian */
        row(CH_INVULN, y, TR("Invulnerable", "Invincible"), cfg.invuln ? yes : no); y += 19;
        row(CH_AMMO, y, TR("Munición infinita", "Infinite ammo"), cfg.ammo ? yes : no); y += 19;
        row(CH_ONEHIT, y, TR("Puño de la muerte", "Death punch"), cfg.onehit ? yes : no); y += 19;
        row(CH_NOIFR, y, TR("Inmunidad tras golpe", "Post-hit immunity"), noifr_name[cfg.lang][cfg.noifr]); y += 19;
        row(CH_STAGE, y, TR("Escenario", "Stage"), stage_name(cfg.stage)); y += 19;
        row(CH_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == CH_NOIFR)
            help_text(196, TR("Tras recibir daño se parpadea un segundo sin que te puedan dar; aquí se quita a quien elijas",
                         "After being hit you blink for a second and can't be hit; here it is removed for whoever you pick"), WHITE);
        else if (cfg.stage >= 0) {
            char buf[96];
            snprintf(buf, sizeof buf, TR("Siguiente combate: %s, contra %s", "Next fight: %s, against %s"), stage_name(cfg.stage), rival_name(cfg.stage));
            help_text(help_text(196, buf, WHITE), TR("No cambia si ya le has ganado o es tu personaje", "No change if already beaten or it's your character"), GREY);
        } else help_text(196, TR("Los combates siguen el orden del juego", "Fights follow the game's order"), GREY);
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else {
        y = 70; row_h = 18;
        row(0, y, TR("Controles táctiles", "Touch controls"), touch_name[cfg.lang][cfg.touch]); y += 17;
        for (int i = 0; i < ACT_SHOWN; i++, y += 18) row(i + 1, y, act_name[cfg.lang][act_of_row(i)], padbtn_name(cfg.map[act_of_row(i)]));
        row(ACT_SHOWN + 1, y, TR("Restaurar botones", "Restore buttons"), NULL); y += 18;
        row(ACT_SHOWN + 2, y, TR("Volver", "Back"), NULL);
        help_text(252, sel == 0 ? TR("Se esconden al usar el mando; vuelven al tocar", "Hidden when a gamepad is used; tap to show")
                               : TR("A cambiar el botón   B volver", "A change button   B back"), GREY);
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

/* Encima del juego: PAUSA y/o un aviso. El aviso va arriba en el centro del lienzo (que cubre toda la pantalla en 8:7 y
   4:3), mas grande y con fondo, para que se vea bien en cualquier formato. */
void menu_render_osd(uint32_t *argb, int paused, const char *msg) {
    cv = argb;
    memset(cv, 0, sizeof(uint32_t) * menu_cw * menu_ch);
    if (paused) {
        const char *p = TR("PAUSA", "PAUSED");
        rect(-ox, 100, menu_cw, 70, 0x06060a, 170);
        text(1, (MENU_W - text_w(1, p)) / 2 + 2, 108, p, 0x000000, 160);
        ctext(1, 106, p, ORANGE, 255);
        ctext(0, 146, TR("START seguir   SELECT / MENU menú", "START resume   SELECT / MENU menu"), GREY, 255);
    }
    if (msg && *msg) {
        int w = text_w(0, msg) + 24, x = (MENU_W - w) / 2, y = 8 - oy + (oy > 0 ? 6 : 0);
        rect(x, y, w, 24, 0x06060a, 210);
        rect(x, y, w, 2, ORANGE, 255);
        text(0, x + 12, y + 6, msg, WHITE, 255);
    }
}
