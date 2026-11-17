/* Menu de opciones: launcher al arrancar y en partida (boton MENU, SELECT por defecto). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <string.h>
#include "menu.h"
#include "achievements.h"
#include "font.h"
#include "inet.h"
#include "nb2.h"
#include "miniz.h"
#include "ach_icons.h"

ofx_config cfg;

/* ---------- configuracion ---------- */
/* "moneda" ya no existe: los opciones.txt viejos la llevan y simplemente se ignora */
/* el boton 3 existe en la placa pero el juego no lo usa: no se ensena en el menu */
/* el boton 3 (la placa lo tiene, el juego no lo usa) cambia de arma con "Dos armas" */
#define ACT_SHOWN ACT_COUNT
static int act_of_row(int k) { return k; }
static const char *act_key[ACT_COUNT] = {"boton1", "boton2", "boton3", "start", "menu", "rebobinar", "avance"};
static const char *act_name[2][ACT_COUNT] = {
    {"Botón 1 (disparo)", "Botón 2 (salto)", "Botón 3 (cambiar arma)", "Start / pausa", "Menú", "Rebobinar", "Avance rápido"},
    {"Button 1 (attack)", "Button 2 (jump)", "Button 3 (swap weapon)", "Start / pause", "Menu", "Rewind", "Fast forward"}};

/* Los trucos (armas y escenario incluidos) no duran de una vez para otra: cada arranque empieza sin ninguno */
void config_reset_cheats(void) { cfg.invuln = cfg.ammo = cfg.onehit = cfg.noifr = 0; cfg.stage = -1; cfg.weapons = 0; cfg.two_weapons = 0; }
static void set_difficulty(int d) {
    cfg.dmg_own = d == DIF_EASY ? 50 : d == DIF_HARD ? 200 : 100;
    cfg.dmg_rival = d == DIF_EASY ? 200 : d == DIF_HARD ? 50 : 100;
}
int config_difficulty(void) {
    if (cfg.dmg_own == 50 && cfg.dmg_rival == 200) return DIF_EASY;
    if (cfg.dmg_own == 100 && cfg.dmg_rival == 100) return DIF_NORMAL;
    if (cfg.dmg_own == 200 && cfg.dmg_rival == 50) return DIF_HARD;
    return DIF_COUNT;
}
static int dmg_ok(int v) { return v >= DMG_MIN && v <= DMG_MAX && v % DMG_STEP == 0; }
/* Formato automatico (17-11-2026): mientras el jugador no lo cambie a mano, se elige segun la pantalla de la consola
   (config_auto_aspect). Al cambiarlo en Opciones queda guardado formato_elegido=1 y ya no se toca. */
static int aspect_chosen;
void config_auto_aspect(int w, int h) {
    if (aspect_chosen || w <= 0 || h <= 0) return;
    if ((long)w * 5 <= (long)h * 6) cfg.aspect = ASP_87;            /* hasta 6:5: 8:7, 1:1... */
    else if ((long)w * 100 < (long)h * 154) cfg.aspect = ASP_43;    /* 4:3, 3:2 */
    else cfg.aspect = ASP_169;                                      /* 16:10, 16:9 y mas anchas */
}
void config_defaults(void) {
    cfg.filter = FILT_SMOOTH; cfg.aspect = ASP_169; cfg.volume = 100; cfg.vibration = 50; cfg.music = cfg.sfx = 100;
    cfg.colors = COL_ORIGINAL; cfg.bands = BAND_BLACK; cfg.dmg_own = cfg.dmg_rival = 100;
    config_reset_cheats();
    cfg.net_stage = -1;
    cfg.lang = LANG_ES; cfg.touch = TOUCH_M; cfg.game_lang = 1; cfg.outfit_pack[0][0] = cfg.outfit_pack[1][0] = 0;
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
/* configuraciones de antes de la 0.27.0: los estilos de ropa 1-4 pasan al pack que los sustituye */
static const char *const old_style[5] = {"", "Gótica", "Negativa", "Pinky", "Vampiros"};
int config_load(const char *path) {
    char line[128], key[64]; int v, has_lang = 0;
    FILE *f;
    config_defaults();
    inet_set_relay(NULL);
    if (!(f = fopen(path, "r"))) return 0;
    while (fgets(line, sizeof line, f)) {
        { char c[80]; if (sscanf(line, "red_servidor=%79s", c) == 1) { inet_set_relay(c); continue; } }   /* servidor intermedio */
        { char c[16]; if (sscanf(line, "red_codigo=%15s", c) == 1 && strlen(c) == 8) { memcpy(cfg.net_code, c, 9); continue; } }
        /* packs de ropa: por nombre (el resto de la linea; puede llevar espacios) */
        if (!strncmp(line, "ropa_pack=", 10) || !strncmp(line, "ropa_rival_pack=", 16)) {
            int pl = line[5] == 'r'; char *v0 = strchr(line, '=') + 1; size_t l = strcspn(v0, "\r\n");
            snprintf(cfg.outfit_pack[pl], sizeof cfg.outfit_pack[pl], "%.*s", (int)l, v0);
            continue;
        }
        if (sscanf(line, "%63[^=]=%d", key, &v) != 2) continue;
        if (!strcmp(key, "imagen") && v >= 0 && v < FILT_COUNT) cfg.filter = v;
        else if (!strcmp(key, "formato") && v >= 0 && v < ASP_COUNT) cfg.aspect = v;
        else if (!strcmp(key, "formato_elegido")) aspect_chosen = v != 0;
        else if (!strcmp(key, "volumen") && v >= 0 && v <= 100) cfg.volume = v;
        else if (!strcmp(key, "vibracion") && v >= 0 && v <= 100) cfg.vibration = v;
        else if (!strcmp(key, "colores") && v >= 0 && v < COL_COUNT) cfg.colors = v;
        else if (!strcmp(key, "bandas3") && v >= 0 && v < BAND_COUNT) cfg.bands = v;   /* "bandas"/"bandas2" (de antes de que las negras fueran lo normal) se ignoran */
        else if (!strcmp(key, "musica") && v >= 0 && v <= 100) cfg.music = v;
        else if (!strcmp(key, "efectos") && v >= 0 && v <= 100) cfg.sfx = v;
        else if (!strcmp(key, "truco_sin_invencibilidad") && v >= 0 && v <= 3) cfg.noifr = v;
        else if (!strcmp(key, "ropa_rival") && v > 0 && v < 5 && !cfg.outfit_pack[1][0]) snprintf(cfg.outfit_pack[1], sizeof cfg.outfit_pack[1], "%s", old_style[v]);
        else if (!strcmp(key, "dificultad") && v >= 0 && v < DIF_COUNT) set_difficulty(v);   /* opciones.txt antiguos */
        else if (!strcmp(key, "dano_propio") && dmg_ok(v)) cfg.dmg_own = v;
        else if (!strcmp(key, "dano_rival") && dmg_ok(v)) cfg.dmg_rival = v;
        else if (!strcmp(key, "truco_invulnerable")) cfg.invuln = v != 0;
        else if (!strcmp(key, "truco_municion")) cfg.ammo = v != 0;
        else if (!strcmp(key, "truco_puno")) cfg.onehit = v != 0;
        else if (!strcmp(key, "truco_escenario") && v >= -1 && v <= 7) cfg.stage = v;
        else if (!strcmp(key, "idioma") && v >= 0 && v < LANG_COUNT) { cfg.lang = v; has_lang = 1; }
        else if (!strcmp(key, "tactil") && v >= 0 && v < TOUCH_COUNT) cfg.touch = v;
        else if (!strcmp(key, "ropa") && v > 0 && v < 5 && !cfg.outfit_pack[0][0]) snprintf(cfg.outfit_pack[0], sizeof cfg.outfit_pack[0], "%s", old_style[v]);
        else if (!strcmp(key, "textos_juego") && v >= 0 && v < GLANG_COUNT) cfg.game_lang = v;
        else if (!strcmp(key, "armas") && v >= 0 && v < WEAPONS_COUNT) cfg.weapons = v;
        else if (!strcmp(key, "dos_armas")) cfg.two_weapons = v != 0;
        else if (!strcmp(key, "red_codigo")) {}   /* lo lee el bucle de abajo (no es un numero) */
        else if (!strcmp(key, "red_escenario") && v >= -1 && v <= 7) cfg.net_stage = v == 7 ? -1 : v;   /* la mansion ya no se ofrece */
        else for (int i = 0; i < ACT_COUNT; i++) if (!strcmp(key, act_key[i]) && v >= 0 && v <= PADBTN_R2) cfg.map[i] = v;
    }
    fclose(f);
    config_reset_cheats();
    return has_lang;
}
void config_save(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "imagen=%d\nformato=%d\nformato_elegido=%d\nvolumen=%d\nvibracion=%d\n", cfg.filter, cfg.aspect, aspect_chosen, cfg.volume, cfg.vibration);
    fprintf(f, "colores=%d\nbandas3=%d\ndano_propio=%d\ndano_rival=%d\nmusica=%d\nefectos=%d\n", cfg.colors, cfg.bands, cfg.dmg_own, cfg.dmg_rival, cfg.music, cfg.sfx);
    fprintf(f, "truco_sin_invencibilidad=%d\n", cfg.noifr);
    fprintf(f, "truco_invulnerable=%d\ntruco_municion=%d\ntruco_puno=%d\ntruco_escenario=%d\n",
            cfg.invuln, cfg.ammo, cfg.onehit, cfg.stage);
    fprintf(f, "idioma=%d\ntactil=%d\ntextos_juego=%d\narmas=%d\n", cfg.lang, cfg.touch, cfg.game_lang, cfg.weapons);
    fprintf(f, "dos_armas=%d\nred_escenario=%d\n", cfg.two_weapons, cfg.net_stage);
    if (cfg.outfit_pack[0][0]) fprintf(f, "ropa_pack=%s\n", cfg.outfit_pack[0]);
    if (cfg.outfit_pack[1][0]) fprintf(f, "ropa_rival_pack=%s\n", cfg.outfit_pack[1]);
    if (strlen(cfg.net_code) == 8) fprintf(f, "red_codigo=%s\n", cfg.net_code);
    if (strcmp(inet_relay_host, "bore.pub") || inet_relay_port != 7835) fprintf(f, "red_servidor=%s:%d\n", inet_relay_host, inet_relay_port);
    for (int i = 0; i < ACT_COUNT; i++) fprintf(f, "%s=%d\n", act_key[i], cfg.map[i]);
    fclose(f);
}
const char *padbtn_name(int b) {
    static const char *n[] = {"A", "B", "X", "Y", "SELECT", "HOME", "START", "L3", "R3", "L1", "R1",
                              "Arriba", "Abajo", "Izquierda", "Derecha", "Extra"};
    static const char *ne[] = {"Up", "Down", "Left", "Right"};
    if (b >= 11 && b <= 14) return TR(n[b], ne[b - 11]);
    if (b == PADBTN_L2) return "L2";
    if (b == PADBTN_R2) return "R2";
    if (b >= 0 && b < (int)(sizeof n / sizeof n[0])) return n[b];
    return "?";
}

/* ---------- estado ---------- */
enum { IT_PLAY, IT_GAMES, IT_SCREEN, IT_DIFFICULTY, IT_DMG_OWN, IT_DMG_RIVAL, IT_WEAPONS, IT_SOUND, IT_BUTTONS, IT_CHEATS, IT_ACH, IT_LANG, IT_TITLE, IT_QUIT, IT_BACK, IT_NET, IT_DIFF_PAGE, IT_TWO_WEAPONS, IT_NET_LEAVE };
/* paginas */
enum { PG_MAIN, PG_BUTTONS, PG_CHEATS, PG_SCREEN, PG_LANG, PG_ACH, PG_SOUND, PG_GAMES, PG_NET, PG_DIFF, PG_CODE };
/* pagina de dificultad */
enum { DF_DIF, DF_OWN, DF_RIVAL, DF_BACK, DF_COUNT };
/* pagina de juego en red */
enum { NT_HOST, NT_JOIN, NT_CODE, NT_STAGE, NT_BACK, NT_COUNT };
static int code_bad;    /* el codigo escrito no es valido */
static int net_mode_m, net_found_m; static char net_status_m[96], net_inet_m[256];
void menu_net_state(int mode, int found, const char *status, const char *inet) { net_mode_m = mode; net_found_m = found; snprintf(net_status_m, sizeof net_status_m, "%s", status ? status : ""); snprintf(net_inet_m, sizeof net_inet_m, "%s", inet ? inet : ""); }
/* pagina de sonido */
enum { SO_VOLUME, SO_MUSIC, SO_SFX, SO_VIBRATION, SO_BACK, SO_COUNT };
/* pagina de pantalla */
enum { SC_ASPECT, SC_FILTER, SC_COLORS, SC_BANDS, SC_OUTFIT, SC_OUTFIT2, SC_BACK, SC_COUNT };
enum { LG_MENU, LG_GAME, LG_BACK, LG_COUNT };
/* pagina de trucos */
enum { CH_INVULN, CH_AMMO, CH_ONEHIT, CH_NOIFR, CH_WEAPONS, CH_TWO_WEAPONS, CH_STAGE, CH_BACK, CH_COUNT };
/* pagina de partidas (guardar y cargar juntas): la fila de guardar solo esta en partida */
enum { GA_SLOT, GA_SAVE, GA_LOAD, GA_BACK, GA_COUNT };
/* escenarios en el orden en que suelen salir; el numero es el rival dueno del escenario (nb2.h) */
static const int stage_order[9] = {-1, 5, 2, 3, 4, 6, 1, 7, 0};
static const char *stage_name(int s) {
    static const char *n[2][8] = {{"Rascacielos", "Circo", "Acuario", "Tren", "Fábrica", "Avión", "Barco", "Mansión"},
                                           {"Skyscraper", "Circus", "Aquarium", "Train", "Factory", "Plane", "Ship", "Mansion"}};
    return s >= 0 && s < 8 ? TR(n[0][s], n[1][s]) : "Normal";
}
static const char *rival_name(int s) {
    static const char *n[8] = {"John Smith", "Eve", "Betty Doe", "Bernard White", "Prof. Ching", "Dweeb", "Danny y Demi", "Mr. Acme"};
    if (s == 6) return TR("Danny y Demi", "Danny & Demi");
    return s >= 0 && s < 8 ? n[s] : "";
}
static int page, sel, in_game, waiting = -1, wait_ms;
static int items[16], nitems, slot;
static char save_dir[512] = ".";
static const char *filt_name[2][FILT_COUNT] = {{"Nítida", "Suavizada", "Scanlines", "Nítida suavizada", "CRT", "CRT curvo"},
                                                        {"Sharp", "Smooth", "Scanlines", "Sharp smoothed", "CRT", "Curved CRT"}};
static const int filt_order[FILT_COUNT] = {FILT_SHARP, FILT_SHARP_SMOOTH, FILT_SMOOTH, FILT_SCANLINES, FILT_CRT, FILT_CRT_CURVED};
static const char *filt_help[2][FILT_COUNT] = {{
    "Píxeles tal cual, sin suavizar", "Algo borrosa, como cualquier pantalla moderna", "Líneas oscuras entre las del juego",
    "Píxeles nítidos con los bordes suavizados", "Tele de tubo: máscara de fósforo y resplandor", "Tele de tubo con la pantalla abombada"}, {
    "Raw pixels, no smoothing", "A bit blurry, like any modern screen", "Dark lines between the game's lines",
    "Sharp pixels with smoothed edges", "Tube TV: phosphor mask and glow", "Tube TV with a curved screen"}};
static const char *col_name[2][COL_COUNT] = {{"Original", "Tele antigua", "Vivos", "Nocturna", "Sepia", "Pantalla verde"},
                                                      {"Original", "Old TV", "Vivid", "Night", "Sepia", "Green screen"}};
static const char *band_name[2][BAND_COUNT] = {{"Negras", "Ambiente", "Portada"}, {"Black", "Ambient", "Cover art"}};
/* Ropa: la de la recreativa y detras los packs instalados (carpeta ropas: los que trae la app y los del editor). Los
   estilos calculados de nb2.c (Gotica, Negativa...) ya no salen: los sustituyen los packs del usuario. Si el pack
   elegido ya no esta, la recreativa. */
/* "Aleatoria" (17-11-2026): va la ultima (con la izquierda desde Recreativa); en la configuracion es OUTFIT_RANDOM.
   Cada pantalla (outfit_reroll, desde main.c al empezar cada combate; antes solo el primero) toca una al azar entre la
   recreativa y los packs, sin repetir la de la pantalla anterior si hay donde elegir. */
#define OUTFIT_RANDOM "*aleatoria*"
static int rnd_pick[2] = {-2, -2};   /* -2 sin sortear, -1 recreativa, >= 0 pack */
static int outfit_random(int pl) { return !strcmp(cfg.outfit_pack[pl], OUTFIT_RANDOM); }
void outfit_reroll(void) {
    for (int pl = 0; pl < 2; pl++) {
        int n = 1 + packs_count(), r;
        if (!outfit_random(pl)) continue;
        do r = rand() % n - 1; while (n > 1 && r == rnd_pick[pl]);
        rnd_pick[pl] = r;
    }
}
int outfit_pos(int pl) {
    int i;
    if (outfit_random(pl)) return 1 + packs_count();
    i = packs_find(cfg.outfit_pack[pl]);
    return i >= 0 ? 1 + i : 0;
}
const skin_pack *outfit_pack(int pl) {
    if (outfit_random(pl)) {
        if (rnd_pick[pl] == -2 || rnd_pick[pl] >= packs_count()) outfit_reroll();
        return rnd_pick[pl] >= 0 ? packs_get(rnd_pick[pl]) : NULL;
    }
    return packs_get(packs_find(cfg.outfit_pack[pl]));
}
static void outfit_step(int pl, int dir) {
    int n = 2 + packs_count(), p = (outfit_pos(pl) + dir + n) % n;
    if (p == n - 1) { snprintf(cfg.outfit_pack[pl], sizeof cfg.outfit_pack[pl], "%s", OUTFIT_RANDOM); rnd_pick[pl] = -2; }
    else if (p >= 1) snprintf(cfg.outfit_pack[pl], sizeof cfg.outfit_pack[pl], "%s", packs_name(p - 1));
    else cfg.outfit_pack[pl][0] = 0;
}
static const char *outfit_label(int pl) {
    int p = outfit_pos(pl);
    if (outfit_random(pl)) return TR("Aleatoria", "Random");
    return p ? packs_name(p - 1) : TR("Recreativa", "Arcade");
}
static const char *outfit_helptext(int pl) {
    if (outfit_random(pl)) return TR("Una ropa al azar en cada pantalla", "A random outfit on every stage");
    return outfit_pos(pl) ? TR("Pack de ropa (se cambia en el EDITOR DE ROPA del inicio)", "Outfit pack (edit it in the OUTFIT EDITOR on the start screen)")
                          : TR("La ropa de la recreativa", "The arcade's clothes");
}
static const char *noifr_name[2][4] = {{"Normal", "Tú no", "El rival no", "Nadie"},
                                                {"Normal", "Not you", "Not the rival", "Nobody"}};
static const char *dif_name[2][DIF_COUNT + 1] = {{"Fácil", "Normal", "Difícil", "A medida"}, {"Easy", "Normal", "Hard", "Custom"}};
static const char *dif_help[2][DIF_COUNT + 1] = {
    {"Recibes la mitad de daño y haces el doble", "Como la recreativa", "Recibes el doble de daño y haces la mitad",
     "El daño que has elegido en las dos filas de abajo"},
    {"You take half damage and deal double", "Like the arcade", "You take double damage and deal half",
     "The damage you picked in the two rows below"}};
static const char *asp_name[2][ASP_COUNT] = {{"4:3", "4:3 escala exacta", "4:3 estirado", "16:9 panorámico", "8:7 recortado"},
                                                      {"4:3", "4:3 integer scale", "4:3 stretched", "16:9 widescreen", "8:7 cropped"}};
/* mismo orden que NB2_WEAPONS_* (nb2.h) */
static const char *weap_name[2][WEAPONS_COUNT] = {{"Todas", "De fuego", "Cuerpo a cuerpo", "Explosivas", "Sin armas"},
                                                           {"All", "Firearms", "Melee", "Explosives", "No weapons"}};
static const char *weap_help[2][WEAPONS_COUNT] = {
    {"Como la recreativa", "Solo pistola y metralleta", "Solo katana y látigo", "Granada, lanzacohetes y lanzallamas",
     "A puñetazos (quedan la comida, el carbón y los juguetes)"},
    {"Like the arcade", "Gun and machine gun only", "Katana and whip only", "Grenade, rocket launcher and flamethrower",
     "Fists only (food, coal and toys stay)"}};
static const char *touch_name[2][TOUCH_COUNT] = {{"No", "Pequeños", "Medianos", "Grandes"}, {"Off", "Small", "Medium", "Large"}};

/* Launcher: al arrancar no hay lista, hay botones (launcher.c). launcher = el menu se abrio al arrancar; at_launch = se
   ven los botones (ninguna pagina abierta). Con el launcher, la pagina principal es la de OPCIONES, que agrupa partidas,
   pantalla, dificultad, sonido, controles, trucos, idioma y juego en red; LOGROS cuelga directamente de su boton. */
static int launcher, at_launch, launch_sel = LB_PLAY, launch_two, guest;
static void build(void) {
    nitems = 0;
    if (guest) {      /* invitado del juego en red: lo que se puede tocar aqui (la partida es del anfitrion) */
        items[nitems++] = IT_PLAY; items[nitems++] = IT_SCREEN; items[nitems++] = IT_SOUND; items[nitems++] = IT_BUTTONS;
        items[nitems++] = IT_LANG; items[nitems++] = IT_NET_LEAVE; items[nitems++] = IT_QUIT;
        return;
    }
    if (launcher) {
        items[nitems++] = IT_GAMES; items[nitems++] = IT_SCREEN; items[nitems++] = IT_DIFF_PAGE; items[nitems++] = IT_SOUND;
        items[nitems++] = IT_BUTTONS; items[nitems++] = IT_CHEATS; items[nitems++] = IT_LANG; items[nitems++] = IT_NET;
        items[nitems++] = IT_BACK;
        return;
    }
    items[nitems++] = IT_PLAY;
    items[nitems++] = IT_GAMES; items[nitems++] = IT_SCREEN; items[nitems++] = IT_DIFF_PAGE;
    items[nitems++] = IT_SOUND; items[nitems++] = IT_BUTTONS; items[nitems++] = IT_CHEATS;
    items[nitems++] = IT_ACH; items[nitems++] = IT_LANG;
    if (in_game) items[nitems++] = IT_TITLE;
    items[nitems++] = IT_QUIT;
}
void menu_open(int game) {
    guest = game == 2; in_game = game != 0; page = 0; sel = 0; waiting = -1;
    launcher = !game; at_launch = launcher; launch_sel = LB_PLAY;
    build();
}
int menu_at_launcher(void) { return launcher && at_launch; }
int menu_launch_sel(void) { return launch_sel; }
static int launch_go(void) {
    switch (launch_sel) {
    case LB_PLAY: return MR_PLAY;
    case LB_QUIT: return MR_QUIT;
    case LB_GALLERY: return MR_GALLERY;      /* el visor lo abre main.c (viewer.c) */
    case LB_MANUAL: return MR_MANUAL;
    case LB_SKINS: return MR_SKINS;          /* el editor de ropa lo abre main.c (skineditor.c) */
    case LB_OPTIONS: page = PG_MAIN; break;
    case LB_ACH: page = PG_ACH; break;
    }
    sel = 0; at_launch = 0;
    return MR_NONE;
}
int menu_launch_tap(int b) {
    if (!menu_at_launcher() || b < 0 || b >= LB_COUNT) return MR_NONE;
    launch_sel = b;
    return launch_go();
}
/* moverse por los botones. En una fila: izquierda y derecha. En dos filas (GALERIA JUGAR MANUAL / OPCIONES LOGROS ROPA
   SALIR): izquierda y derecha en orden de lectura, arriba y abajo al boton mas cercano de la otra fila. */
static int launch_input(int w) {
    /* dos filas (indices = LB_*) */
    static const int order2[LB_COUNT] = {LB_GALLERY, LB_PLAY, LB_MANUAL, LB_OPTIONS, LB_ACH, LB_SKINS, LB_QUIT};
    /*                                    OPTIONS     ACH         GALLERY  PLAY    MANUAL    SKINS      QUIT */
    static const int down2[LB_COUNT] = {-1,         -1,         LB_ACH,  LB_ACH, LB_SKINS, -1,        -1};
    static const int up2[LB_COUNT] =   {LB_GALLERY, LB_GALLERY, -1,      -1,     -1,       LB_MANUAL, LB_MANUAL};
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
        else if (t) snprintf(buf, n, TR("Ranura %d: guardada el %02d/%02d a las %02d:%02d", "Ranura %d: guardada el %02d/%02d a las %02d:%02d"), s + 1, t->tm_mday, t->tm_mon + 1, t->tm_hour, t->tm_min);
        else snprintf(buf, n, TR("Ranura %d: guardada", "Slot %d: saved"), s + 1);
    }
    return 1;
}

static int change(int it, int dir) {
    switch (it) {
    case GA_SLOT + 200: slot = (slot + dir + NSLOTS) % NSLOTS; return MR_NONE;
    case IT_DIFFICULTY: {
        int d = config_difficulty(), v = d == DIF_COUNT ? DIF_NORMAL : d + dir;    /* desde "A medida" se vuelve a Normal */
        if (v < 0 || v >= DIF_COUNT) return MR_NONE;
        set_difficulty(v); return MR_CHANGED;
    }
    case IT_DMG_OWN: case IT_DMG_RIVAL: {
        int *t = it == IT_DMG_OWN ? &cfg.dmg_own : &cfg.dmg_rival, v = *t + dir * DMG_STEP;
        if (v < DMG_MIN || v > DMG_MAX) return MR_NONE;
        *t = v; return MR_CHANGED;
    }
    case IT_TWO_WEAPONS: cfg.two_weapons = !cfg.two_weapons; return MR_CHANGED;
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
    if (launcher && it == IT_ACH) { at_launch = 1; return; }
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
            case IT_NET: page = PG_NET; sel = 0; break;
            case IT_DIFF_PAGE: page = PG_DIFF; sel = 0; break;
            case IT_TITLE: return MR_TITLE;
            case IT_NET_LEAVE: return MR_NET_LEAVE;
            case IT_QUIT: return MR_QUIT;
            case IT_BACK: at_launch = launcher; break;
            default: return change(it, 1);
            }
        }
        return MR_NONE;
    }
    if (page == PG_DIFF) {    /* dificultad y los dos porcentajes de dano */
        static const int it_of[DF_COUNT] = {IT_DIFFICULTY, IT_DMG_OWN, IT_DMG_RIVAL, IT_BACK};
        switch (w) {
        case MI_UP: sel = (sel + DF_COUNT - 1) % DF_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % DF_COUNT; break;
        case MI_LEFT: case MI_RIGHT: if (sel != DF_BACK) return change(it_of[sel], w == MI_LEFT ? -1 : 1); break;
        case MI_OK: if (sel != DF_BACK) return change(it_of[sel], 1);   /* sin break: Volver */
        case MI_BACK: back_to(IT_DIFF_PAGE); break;
        }
        return MR_NONE;
    }
    if (page == PG_NET) {     /* juego en red: crear / cerrar, buscar / unirse, escenario, volver (1 = NET_HOST, 2 = NET_SEARCH; net.h) */
        switch (w) {
        case MI_UP: sel = (sel + NT_COUNT - 1) % NT_COUNT; break;
        case MI_DOWN: sel = (sel + 1) % NT_COUNT; break;
        case MI_BACK: back_to(IT_NET); break;
        case MI_LEFT: case MI_RIGHT:
            /* sin la mansion (7): falla entre dos jugadores (nb2.c, vs_patch) */
            if (sel == NT_STAGE) { cfg.net_stage = (cfg.net_stage + 1 + (w == MI_LEFT ? 7 : 1)) % 8 - 1; return MR_CHANGED; }
            break;
        case MI_OK:
            if (sel == NT_STAGE) { cfg.net_stage = (cfg.net_stage + 2) % 8 - 1; return MR_CHANGED; }
            if (sel == NT_HOST) return net_mode_m == 1 ? MR_NET_STOP : MR_NET_HOST;
            if (sel == NT_JOIN) return net_mode_m == 2 ? (net_found_m ? MR_NET_JOIN : MR_NET_STOP) : MR_NET_SEARCH;
            if (sel == NT_CODE) {
                if (strlen(cfg.net_code) != 8) memcpy(cfg.net_code, "AAAAAAAA", 9);
                page = PG_CODE; sel = 0; break;
            }
            back_to(IT_NET);
        }
        return MR_NONE;
    }
    if (page == PG_CODE) {    /* codigo de partida por internet: 8 letras (sel 0-7) y "Conectar" (sel 8) */
        switch (w) {
        case MI_LEFT: sel = (sel + 8) % 9; break;
        case MI_RIGHT: sel = (sel + 1) % 9; break;
        case MI_UP: case MI_DOWN:
            if (sel < 8) {
                int i = inet_code_char(cfg.net_code[sel]); if (i < 0) i = 0;
                cfg.net_code[sel] = inet_code_letter((i + (w == MI_UP ? 1 : 31)) & 31);
            } else sel = w == MI_UP ? 7 : 0;
            break;
        case MI_OK:
            if (sel < 8) { sel++; break; }
            { unsigned ip; int port;
              if (!inet_code_decode(cfg.net_code, &ip, &port)) { code_bad = 1; break; } }
            code_bad = 0;
            return MR_NET_CODE;
        case MI_BACK: page = PG_NET; sel = NT_CODE; code_bad = 0; return MR_CHANGED;   /* MR_CHANGED: guarda el codigo */
        }
        if (w != MI_OK) code_bad = 0;
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
            if (sel == LG_MENU) { cfg.lang = (cfg.lang + (w == MI_LEFT ? LANG_COUNT - 1 : 1)) % LANG_COUNT; return MR_CHANGED; }
            if (sel == LG_GAME) { cfg.game_lang = (cfg.game_lang + (w == MI_LEFT ? GLANG_COUNT - 1 : 1)) % GLANG_COUNT; return MR_CHANGED; }
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
            case SC_ASPECT: cfg.aspect = (cfg.aspect + dir + ASP_COUNT) % ASP_COUNT; aspect_chosen = 1; return MR_CHANGED;
            case SC_FILTER: {
                int i = 0;
                while (i < FILT_COUNT && filt_order[i] != cfg.filter) i++;
                cfg.filter = filt_order[((i % FILT_COUNT) + dir + FILT_COUNT) % FILT_COUNT];
                return MR_CHANGED;
            }
            case SC_COLORS: cfg.colors = (cfg.colors + dir + COL_COUNT) % COL_COUNT; return MR_CHANGED;
            case SC_BANDS: cfg.bands = (cfg.bands + dir + BAND_COUNT) % BAND_COUNT; return MR_CHANGED;
            case SC_OUTFIT: outfit_step(0, dir); return MR_CHANGED;
            case SC_OUTFIT2: outfit_step(1, dir); return MR_CHANGED;
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
            case CH_WEAPONS: cfg.weapons = (cfg.weapons + dir + WEAPONS_COUNT) % WEAPONS_COUNT; return MR_CHANGED;
            case CH_TWO_WEAPONS: cfg.two_weapons = !cfg.two_weapons; return MR_CHANGED;
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
/* textos y cajas sobre un lienzo ajeno (el visor de la galeria y el manual): se cambia el lienzo un momento */
#define EXT_SWAP(px, w, h) uint32_t *scv = cv; int sw = menu_cw, sh = menu_ch, sox = ox, soy = oy; cv = px; menu_cw = w; menu_ch = h; ox = oy = 0
#define EXT_BACK() cv = scv; menu_cw = sw; menu_ch = sh; ox = sox; oy = soy
void menu_ext_rect(uint32_t *px, int w, int h, int x, int y, int rw, int rh, uint32_t rgb, int a) { EXT_SWAP(px, w, h); rect(x, y, rw, rh, rgb, a); EXT_BACK(); }
void menu_ext_text(uint32_t *px, int w, int h, int x, int y, const char *s, uint32_t rgb, int big) { EXT_SWAP(px, w, h); text(big, x, y, s, rgb, 255); EXT_BACK(); }
int menu_ext_text_w(const char *s, int big) { return text_w(big, s); }
/* icono del logro i (32x32 de RetroAchievements) a 16x16 (big = 0) o a 32x32 (big = 1); sin conseguir, en gris oscuro */
static void ach_icon(int i, int x0, int y0, int got, int big) {
    static unsigned char *raw; static int tried;
    if (!tried) {
        mz_ulong n = ACH_ICON_RAW; tried = 1;
        raw = malloc(ACH_ICON_RAW);
        if (raw && mz_uncompress(raw, &n, ach_icons_z, sizeof ach_icons_z) != MZ_OK) { free(raw); raw = NULL; }
    }
    if (!raw || i < 0 || i >= ACH_ICON_N) return;
    { int sz = big ? 32 : 16; rect(x0 - 1, y0 - 1, sz + 2, sz + 2, 0x000000, 220); }   /* marco oscuro: se ve igual sobre la portada */
    for (int y = 0; y < (big ? 32 : 16); y++) for (int x = 0; x < (big ? 32 : 16); x++) {
        int r = 0, g = 0, b = 0, n = big ? 1 : 4;
        for (int k = 0; k < n; k++) {
            const unsigned char *p = raw + i * 2048 + (big ? (y * 32 + x) * 2 : ((y * 2 + (k >> 1)) * 32 + x * 2 + (k & 1)) * 2);
            int v = p[0] << 8 | p[1];
            r += (v >> 10 & 31) * 255 / 31; g += (v >> 5 & 31) * 255 / 31; b += (v & 31) * 255 / 31;
        }
        r /= n; g /= n; b /= n;
        if (!got) { int l = (r * 30 + g * 59 + b * 11) / 100 * 3 / 5; r = g = b = l; }   /* sin conseguir: gris oscuro */
        blend(x0 + x, y0 + y, (uint32_t)(r << 16 | g << 8 | b), 255);
    }
}
static void ctext(int big, int y, const char *s, uint32_t rgb, int a) { text(big, (MENU_W - text_w(big, s)) / 2, y, s, rgb, a); }

/* texto centrado partido en lineas de como mucho maxw pixeles (por palabras), desde y, 15 px por linea.
   Con draw = 0 solo cuenta las lineas. Devuelve cuantas lineas ocupa. */
static int wrap_dx;   /* desplaza el centro de las lineas (descripcion del logro, con el icono a la izquierda) */
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
        if (draw) text(0, (MENU_W - text_w(0, line)) / 2 + wrap_dx, y + n * 15, line, rgb, 255);
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
    ctext(0, 52, page == PG_DIFF ? TR("Dificultad", "Difficulty") : page == PG_NET ? TR("Juego en red", "Network play") : page == PG_CODE ? TR("Código de la partida", "Game code") : page == PG_GAMES ? TR("Partidas", "Saved games") : page == 6 ? TR("Sonido y vibración", "Sound and rumble") : page == 5 ? TR("Logros", "Achievements") : page == 4 ? TR("Idioma", "Language") : page == 3 ? TR("Pantalla", "Display") : page == 2 ? TR("Trucos", "Cheats") : page == 1 ? TR("Controles", "Controls")
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
            case IT_DIFFICULTY: row(i, y, TR("Dificultad", "Difficulty"), TR(dif_name[0][config_difficulty()], dif_name[1][config_difficulty()])); break;
            case IT_DIFF_PAGE: row(i, y, TR("Dificultad", "Difficulty"), NULL); break;   /* solo entra: no se cambia desde aqui */
            case IT_NET_LEAVE: row(i, y, TR("Salir de la partida en red", "Leave the network game"), NULL); break;
            case IT_TWO_WEAPONS: row(i, y, TR("Dos armas", "Two weapons"), cfg.two_weapons ? TR("Sí", "On") : TR("No", "Off")); break;
            case IT_DMG_OWN: snprintf(v, sizeof v, "%d%%", cfg.dmg_own); row(i, y, TR("Daño que recibes", "Damage you take"), v); break;
            case IT_DMG_RIVAL: snprintf(v, sizeof v, "%d%%", cfg.dmg_rival); row(i, y, TR("Daño que haces", "Damage you deal"), v); break;
            case IT_WEAPONS: row(i, y, TR("Armas", "Weapons"), TR(weap_name[0][cfg.weapons], weap_name[1][cfg.weapons])); break;
            case IT_SOUND: row(i, y, TR("Sonido y vibración", "Sound and rumble"), NULL); break;
            case IT_TITLE: row(i, y, TR("Volver a la pantalla de inicio", "Back to title screen"), NULL); break;
            case IT_BUTTONS: row(i, y, TR("Controles", "Controls"), NULL); break;
            case IT_CHEATS: row(i, y, TR("Trucos", "Cheats"), NULL); break;
            case IT_ACH: snprintf(v, sizeof v, "%d/%d", ach_total(), ACH_COUNT); row(i, y, TR("Logros", "Achievements"), v); break;
            case IT_LANG: row(i, y, TR("Idioma", "Language"), NULL); break;
            case IT_QUIT: row(i, y, TR("Salir del juego", "Quit game"), NULL); break;
            case IT_BACK: row(i, y, TR("Volver", "Back"), NULL); break;
            case IT_NET: row(i, y, TR("Juego en red", "Network play"), net_mode_m == 1 ? TR("Abierta", "Open") : NULL); break;
            }
        }
        if (items[sel] == IT_NET) help_text(252, TR("Dos consolas en la misma wifi: una crea la partida y la otra se une como jugador 2", "Two consoles on the same wifi: one hosts and the other joins as player 2"), GREY);
        else if (items[sel] == IT_DIFF_PAGE) {
            char buf[96];
            snprintf(buf, sizeof buf, TR("Ahora: %s (recibes %d %%, haces %d %%). A para cambiarla", "Now: %s (you take %d%%, you deal %d%%). A to change it"),
                     TR(dif_name[0][config_difficulty()], dif_name[1][config_difficulty()]), cfg.dmg_own, cfg.dmg_rival);
            help_text(252, buf, GREY);
        }
        else if (items[sel] == IT_DMG_OWN) help_text(252, TR("Del 50 % al 200 %: lo que te quita cada golpe (100 % = recreativa)", "50% to 200%: how much each hit takes from you (100% = arcade)"), WHITE);
        else if (items[sel] == IT_DMG_RIVAL) help_text(252, TR("Del 50 % al 200 %: lo que quitan tus golpes al rival (100 % = recreativa)", "50% to 200%: how much your hits take from the rival (100% = arcade)"), WHITE);
        else if (items[sel] == IT_WEAPONS) help_text(252, TR(weap_help[0][cfg.weapons], weap_help[1][cfg.weapons]), WHITE);
        else if (items[sel] == IT_NET_LEAVE) help_text(252, TR("Vuelve a la pantalla de inicio; la partida sigue en la otra consola", "Back to the title screen; the game goes on in the other console"), WHITE);
        else if (items[sel] == IT_PLAY && guest) help_text(252, TR("Sigues conectado: la partida no se para mientras miras el menú", "You stay connected: the game doesn't stop while you look at the menu"), GREY);
        else if (items[sel] == IT_TITLE) help_text(252, TR("Reinicia la recreativa (lo no guardado se pierde)", "Restarts the arcade (unsaved progress is lost)"), WHITE);
        else if (items[sel] == IT_QUIT) help_text(252, TR("Cierra la aplicación", "Closes the app"), WHITE);
        else if (items[sel] == IT_GAMES) help_text(252, TR("5 ranuras, en la carpeta de la app", "5 slots, in the app's folder"), GREY);
        else if (items[sel] == IT_SCREEN) help_text(252, TR("Formato, filtro, colores, bandas y ropa", "Aspect, filter, colors, bands and outfits"), GREY);
        else if (items[sel] == IT_ACH) help_text(252, TR("Los de RetroAchievements, en Normal o Difícil", "RetroAchievements list, Normal or Hard"), GREY);
        else if (items[sel] == IT_CHEATS) help_text(252, TR("Con algún truco puesto no se consiguen logros", "No achievements while any cheat is on"), GREY);
        else if (items[sel] == IT_LANG) help_text(252, TR("Menú y textos del juego", "Menu and game text"), GREY);
        else if (items[sel] == IT_BUTTONS) help_text(252, TR("Controles táctiles y botones del mando", "Touch controls and gamepad buttons"), GREY);
        else help_text(252, in_game ? TR("↑↓ elegir  ←→ cambiar  A aceptar  B volver al juego", "↑↓ choose  ←→ change  A accept  B back to game")
                                   : TR("↑↓ elegir   ←→ cambiar   A aceptar   B volver", "↑↓ choose   ←→ change   A accept   B back"), GREY);
    } else if (page == PG_NET) {
        row_h = 20; y = 82;
        row(NT_HOST, y, net_mode_m == 1 ? TR("Cerrar la partida", "Close the game") : TR("Crear partida", "Host a game"), NULL); y += 22;
        row(NT_JOIN, y, net_mode_m == 2 ? (net_found_m ? TR("Entrar en la partida encontrada", "Join the game found") : TR("Dejar de buscar", "Stop searching"))
                                        : TR("Unirse en la wifi", "Join on this wifi"), NULL); y += 22;
        row(NT_CODE, y, TR("Unirse por internet (código)", "Join over the internet (code)"), NULL); y += 22;
        row(NT_STAGE, y, TR("Escenario", "Stage"), cfg.net_stage < 0 ? TR("Al azar", "Random") : stage_name(cfg.net_stage)); y += 22;
        row(NT_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == NT_STAGE) { help_text(help_text(198, TR("Dónde se pelea cuando entra el jugador 2 (lo elige quien crea la partida; vale también con dos mandos)", "Where you fight when player 2 joins (the host picks; also with two gamepads)"), WHITE), TR("La mansión no sale: es para jugar contra la máquina", "No mansion: it is made for playing against the CPU"), GREY); help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY); return; }
        if (sel == NT_CODE) {
            help_text(help_text(198, TR("Escribe el código que ve quien crea la partida (se lo da su consola al crearla)", "Type the code shown to whoever hosts (their console shows it when hosting)"), WHITE),
                      TR("Las dos con la misma versión de la app", "Both with the same app version"), GREY);
            help_text(foot_y(), TR("A aceptar   B volver", "A accept   B back"), GREY); return;
        }
        y = help_text(198, net_status_m, WHITE);
        if (net_mode_m == 1 && net_inet_m[0]) y = help_text(y, net_inet_m, ORANGE);
        if (net_mode_m == 1) {          /* la pista solo si cabe debajo (si no, se pisaba con el aviso de internet) */
            if (y + 30 <= MENU_H + oy - 4) help_text(y, TR("Vuelve atrás y pulsa JUGAR; el jugador 2 entra con START", "Go back and press PLAY; player 2 joins with START"), GREY);
            return;
        }
        else if (net_mode_m != 2) help_text(y, TR("Quien se une ve y oye la partida del otro y juega como jugador 2", "Whoever joins sees and hears the host's game and plays as player 2"), GREY);
        help_text(foot_y(), TR("A aceptar   B volver", "A accept   B back"), GREY);
    } else if (page == PG_CODE) {
        /* 8 casillas grandes ABCD-EFGH; arriba/abajo cambian la letra de la elegida */
        int cw = 32, gap = 18, total = 8 * cw + gap, x0 = (MENU_W - total) / 2, cy = 100;
        for (int i = 0; i < 8; i++) {
            int x = x0 + i * cw + (i >= 4 ? gap : 0), s2 = sel == i; char ch[2] = {cfg.net_code[i], 0};
            if (nrows_at < MAXROWS) { rows_at[nrows_at].i = i; rows_at[nrows_at].y0 = cy - 6; rows_at[nrows_at].y1 = cy + 34; rows_at[nrows_at].x0 = x; rows_at[nrows_at].x1 = x + cw - 4; nrows_at++; }
            rect(x, cy - 4, cw - 4, 34, s2 ? ORANGE : 0x303040, s2 ? 110 : 200);
            if (s2) { rect(x, cy + 28, cw - 4, 2, ORANGE, 255); text(0, x + (cw - 4 - text_w(0, "↑")) / 2, cy - 20, "↑", ORANGE, 255); text(0, x + (cw - 4 - text_w(0, "↓")) / 2, cy + 34, "↓", ORANGE, 255); }
            text(1, x + (cw - 4 - text_w(1, ch)) / 2, cy, ch, s2 ? WHITE : GREY, 255);
        }
        text(1, x0 + 4 * cw + 1, cy, "-", GREY, 255);
        row_h = 20; row_x0 = 150; row_x1 = 330;
        row(8, 168, TR("Conectar", "Connect"), NULL);
        if (code_bad) help_text(198, TR("Ese código no es válido: revisa las letras", "That code is not valid: check the letters"), ORANGE);
        else help_text(198, TR("Sin 0, 1, I ni O: solo letras y números que no se confunden", "No 0, 1, I or O: only letters and digits that can't be mixed up"), GREY);
        help_text(foot_y(), TR("←→ casilla   ↑↓ letra   A siguiente / conectar   B volver", "←→ box   ↑↓ letter   A next / connect   B back"), GREY);
    } else if (page == PG_DIFF) {
        char v[24];
        row_h = 20;
        row(DF_DIF, y, TR("Dificultad", "Difficulty"), TR(dif_name[0][config_difficulty()], dif_name[1][config_difficulty()])); y += 22;
        snprintf(v, sizeof v, "%d%%", cfg.dmg_own); row(DF_OWN, y, TR("Daño que recibes", "Damage you take"), v); y += 22;
        snprintf(v, sizeof v, "%d%%", cfg.dmg_rival); row(DF_RIVAL, y, TR("Daño que haces", "Damage you deal"), v); y += 22;
        row(DF_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == DF_DIF) help_text(196, TR(dif_help[0][config_difficulty()], dif_help[1][config_difficulty()]), WHITE);
        else if (sel == DF_OWN) help_text(196, TR("Del 50 % al 200 %: lo que te quita cada golpe (100 % = recreativa)", "50% to 200%: how much each hit takes from you (100% = arcade)"), WHITE);
        else if (sel == DF_RIVAL) help_text(196, TR("Del 50 % al 200 %: lo que quitan tus golpes al rival (100 % = recreativa)", "50% to 200%: how much your hits take from the rival (100% = arcade)"), WHITE);
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
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
            int k = ach_order(i);   /* i = puesto en la lista (por tipos), k = numero del logro */
            snprintf(v, sizeof v, "%s", ach_got(k) ? TR("Sí", "Yes") : ach_todo(k) ? TR("Pronto", "Soon") : "-");
            row(i, y, ach_title(k), v);   /* row acorta el nombre por lo que mide, si no cabe */
            ach_icon(k, row_x0 - 20, y - 2, ach_got(k), 0);
        }
        {
            char d[200];
            int W = help_w() < 380 ? help_w() : 380, n;
            int k = ach_order(sel);
            snprintf(d, sizeof d, "%s", ach_desc(k));
            /* icono grande a la izquierda y el texto centrado en lo que queda; el bloque entero, centrado */
            n = wrap_lines(d, 0, W - 42, 3, WHITE, 0);
            ach_icon(k, (MENU_W - W) / 2 + 1, 200 + (n * 15 - 32) / 2 - 1, ach_got(k), 1);
            wrap_dx = 21; wrap_text(d, 200, W - 42, 3, WHITE); wrap_dx = 0;
        }
        {
            char b[64];
            snprintf(b, sizeof b, TR("%d de %d logros   B volver", "%d of %d achievements   B back"), ach_total(), ACH_COUNT);
            help_text(252, b, GREY);
        }
    } else if (page == 4) {
        row_h = 20;
        static const char *const lname[LANG_COUNT] = {"Español", "English", "Português", "Italiano"};
        static const char *const gname[2][GLANG_COUNT] = {{"Inglés (original)", "Castellano", "Portugués", "Italiano"},
                                                          {"English (original)", "Spanish", "Portuguese", "Italian"}};
        row(LG_MENU, y, TR("Menú", "Menu"), lname[cfg.lang]); y += 22;
        row(LG_GAME, y, TR("Textos del juego", "Game text"), TR(gname[0][cfg.game_lang], gname[1][cfg.game_lang])); y += 22;
        row(LG_BACK, y, TR("Volver", "Back"), NULL);
        if (sel == LG_GAME) help_text(196, TR("Historias, encargos y mensajes de la recreativa", "Stories, missions and messages of the arcade"), WHITE);
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else if (page == 3) {
        /* filas a la izquierda y vista previa a la derecha (la dibuja main.c en menu_preview_rect) */
        const char *help = NULL;
        row_h = 18; row_x0 = 42; row_x1 = 304; y = 72;
        row(SC_ASPECT, y, TR("Formato", "Aspect"), TR(asp_name[0][cfg.aspect], asp_name[1][cfg.aspect])); y += 19;
        row(SC_FILTER, y, TR("Filtro", "Filter"), TR(filt_name[0][cfg.filter], filt_name[1][cfg.filter])); y += 19;
        row(SC_COLORS, y, TR("Colores", "Colors"), TR(col_name[0][cfg.colors], col_name[1][cfg.colors])); y += 19;
        row(SC_BANDS, y, TR("Bandas", "Bands"), TR(band_name[0][cfg.bands], band_name[1][cfg.bands])); y += 19;
        row(SC_OUTFIT, y, TR("Ropa jugador 1", "P1 outfit"), outfit_label(0)); y += 19;
        row(SC_OUTFIT2, y, TR("Ropa del rival", "Rival outfit"), outfit_label(1)); y += 19;
        row(SC_BACK, y, TR("Volver", "Back"), NULL);
        rect(PV_X - 2, PV_Y - 2, PV_W + 4, PV_H + 4, ORANGE, 120);    /* marco de la vista previa */
        rect(PV_X, PV_Y, PV_W, PV_H, 0x000000, 255);
        if (sel == SC_FILTER) help = TR(filt_help[0][cfg.filter], filt_help[1][cfg.filter]);
        else if (sel == SC_ASPECT && cfg.aspect == ASP_87) help = TR("En combate se recortan 20 píxeles por lado; el resto, entero con relleno", "In fights 20 pixels are cut per side; the rest, whole with filler");
        else if (sel == SC_ASPECT && !aspect_chosen) help = TR("Elegido según tu pantalla; si lo cambias, se queda el tuyo", "Picked for your screen; if you change it, yours stays");
        else if (sel == SC_BANDS) help = cfg.aspect == ASP_169 ? TR("Solo se ven si la imagen no llena la pantalla", "Only visible if the picture doesn't fill the screen")
                                   : cfg.bands == BAND_COVER ? TR("La portada, fija, en lo que no tapa el juego", "The cover art, still, around the game")
                                   : TR("Ambiente: los lados toman los colores de la escena", "Ambient: the sides take the scene's colors");
        else if (sel == SC_OUTFIT) help = outfit_helptext(0);
        else if (sel == SC_OUTFIT2) help = outfit_helptext(1);
        else if (sel == SC_COLORS) help = TR("Cambia los colores, no el dibujo", "Changes the colors, not the graphics");
        { int hy = 206;
          if (help) hy = help_text(hy, help, WHITE);
          if (sel == SC_OUTFIT || sel == SC_OUTFIT2) help_text(hy, TR("La ropa solo cambia en los combates", "Outfits only change during fights"), GREY); }
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else if (page == 2) {
        const char *yes = TR("Sí", "On"), *no = TR("No", "Off");
        row_h = 18; row_x0 = 62; row_x1 = 418; y = 70;   /* filas mas anchas: "Inmunidad tras golpe" y su valor no cabian */
        row(CH_INVULN, y, TR("Invulnerable", "Invincible"), cfg.invuln ? yes : no); y += 18;
        row(CH_AMMO, y, TR("Munición infinita", "Infinite ammo"), cfg.ammo ? yes : no); y += 18;
        row(CH_ONEHIT, y, TR("Puño de la muerte", "Death punch"), cfg.onehit ? yes : no); y += 18;
        row(CH_NOIFR, y, TR("Inmunidad tras golpe", "Post-hit immunity"), TR(noifr_name[0][cfg.noifr], noifr_name[1][cfg.noifr])); y += 18;
        row(CH_WEAPONS, y, TR("Armas", "Weapons"), TR(weap_name[0][cfg.weapons], weap_name[1][cfg.weapons])); y += 18;
        row(CH_TWO_WEAPONS, y, TR("Dos armas", "Two weapons"), cfg.two_weapons ? yes : no); y += 18;
        row(CH_STAGE, y, TR("Escenario", "Stage"), stage_name(cfg.stage)); y += 18;
        row(CH_BACK, y, TR("Volver", "Back"), NULL);
        int hy = 220;
        if (sel == CH_NOIFR)
            hy = help_text(220, TR("Tras recibir daño se parpadea un segundo sin que te puedan dar; aquí se quita a quien elijas",
                         "After being hit you blink for a second and can't be hit; here it is removed for whoever you pick"), WHITE);
        else if (sel == CH_WEAPONS) hy = help_text(220, TR(weap_help[0][cfg.weapons], weap_help[1][cfg.weapons]), WHITE);
        else if (sel == CH_TWO_WEAPONS) hy = help_text(220, TR("Llevas un arma de reserva y la cambias con el botón 3", "Carry a spare weapon and swap it with button 3"), WHITE);
        else if (cfg.stage >= 0) {
            char buf[96];
            snprintf(buf, sizeof buf, TR("Siguiente combate: %s, contra %s", "Next fight: %s, against %s"), stage_name(cfg.stage), rival_name(cfg.stage));
            hy = help_text(help_text(220, buf, WHITE), TR("No cambia si ya le has ganado o es tu personaje", "No change if already beaten or it's your character"), GREY);
        } else hy = help_text(220, TR("Los combates siguen el orden del juego", "Fights follow the game's order"), GREY);
        (void)hy;
        help_text(foot_y(), TR("←→ cambiar   B volver", "←→ change   B back"), GREY);
    } else {
        y = 70; row_h = 18;
        row(0, y, TR("Controles táctiles", "Touch controls"), TR(touch_name[0][cfg.touch], touch_name[1][cfg.touch])); y += 17;
        for (int i = 0; i < ACT_SHOWN; i++, y += 18) row(i + 1, y, TR(act_name[0][act_of_row(i)], act_name[1][act_of_row(i)]), padbtn_name(cfg.map[act_of_row(i)]));
        row(ACT_SHOWN + 1, y, TR("Restaurar botones", "Restore buttons"), NULL); y += 18;
        row(ACT_SHOWN + 2, y, TR("Volver", "Back"), NULL);
        help_text(252, sel == 0 ? TR("Se esconden al usar el mando; vuelven al tocar", "Hidden when a gamepad is used; tap to show")
                               : TR("A cambiar el botón   B volver", "A change button   B back"), GREY);
        if (waiting >= 0) {
            char buf[80];
            rect(70, 96, 340, 78, 0x101018, 255);
            rect(70, 96, 340, 3, ORANGE, 255);
            snprintf(buf, sizeof buf, TR("Pulsa el botón para «%s»", "Press the button for «%s»"), TR(act_name[0][waiting], act_name[1][waiting]));
            ctext(0, 114, buf, WHITE, 255);
            snprintf(buf, sizeof buf, TR("Se cancela en %d s", "Cancels in %d s"), (wait_ms + 999) / 1000);
            ctext(0, 142, buf, GREY, 255);
        }
    }
}

/* Encima del juego: PAUSA y/o un aviso. El aviso va arriba en el centro del lienzo (que cubre toda la pantalla en 8:7 y
   4:3), mas grande y con fondo, para que se vea bien en cualquier formato. */
static char osd_icon_msg[96]; static int osd_icon = -1;
void menu_set_osd_icon(int i, const char *msg) { osd_icon = i; snprintf(osd_icon_msg, sizeof osd_icon_msg, "%s", msg ? msg : ""); }
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
    if (msg && *msg && osd_icon >= 0 && !strcmp(msg, osd_icon_msg)) {   /* aviso de logro: con su icono */
        int w = text_w(0, msg) + 24 + 40, x = (MENU_W - w) / 2, y = 8 - oy + (oy > 0 ? 6 : 0);
        rect(x, y, w, 44, 0x06060a, 210);
        rect(x, y, w, 2, ORANGE, 255);
        ach_icon(osd_icon, x + 10, y + 7, 1, 1);
        text(0, x + 12 + 40, y + 16, msg, WHITE, 255);
    } else if (msg && *msg) {
        int w = text_w(0, msg) + 24, x = (MENU_W - w) / 2, y = 8 - oy + (oy > 0 ? 6 : 0);
        rect(x, y, w, 24, 0x06060a, 210);
        rect(x, y, w, 2, ORANGE, 255);
        text(0, x + 12, y + 6, msg, WHITE, 255);
    }
}
