/* Menu de opciones (launcher y pausa). No depende de SDL: dibuja en un lienzo ARGB de MENU_W x MENU_H. */
#ifndef OFX_MENU_H
#define OFX_MENU_H
#include <stdint.h>

/* El menu se disena en un espacio de 480x270 (MENU_W x MENU_H). El lienzo real (menu_cw x menu_ch, como mucho
   MENU_MAXW x MENU_MAXH) toma la forma de la pantalla: en 16:9 es 480x270; en pantallas mas cuadradas (8:7, 4:3) es mas
   estrecho y mas alto, con el diseno centrado, para que el menu llene la pantalla y se vea mas grande. */
#define MENU_W 480
#define MENU_H 270
#define MENU_MAXW 480
#define MENU_MAXH 440
extern int menu_cw, menu_ch;
void menu_layout(int screen_w, int screen_h);   /* calcula menu_cw x menu_ch para esa pantalla */
/* Pagina Pantalla: rectangulo (en pixeles del lienzo) donde main.c dibuja la vista previa. 0 = no hay. */
int menu_preview_rect(int *x, int *y, int *w, int *h);

/* Filtros. Los tres primeros conservan su numero de siempre (opciones.txt); los nuevos van detras.
   En el menu salen en el orden de filt_order (menu.c). */
enum { FILT_SHARP, FILT_SMOOTH, FILT_SCANLINES, FILT_SHARP_SMOOTH, FILT_CRT, FILT_CRT_CURVED, FILT_COUNT };
/* Colores (como en Master of Darkness): solo cambian el RGB final, el dibujo es el mismo */
enum { COL_ORIGINAL, COL_TV, COL_VIVID, COL_NIGHT, COL_SEPIA, COL_GREEN, COL_COUNT };
/* Bandas a los lados cuando la imagen no llena la pantalla (4:3): negras o con los colores de la escena */
enum { BAND_BLACK, BAND_AMBIENT, BAND_COVER, BAND_COUNT };   /* COVER: la portada (fija) */
/* Dificultad: los mismos valores que NB2_DIFF_* */
enum { DIF_EASY, DIF_NORMAL, DIF_HARD, DIF_COUNT };
/* ASP_169 y ASP_87 van al final para no cambiar el significado de los opciones.txt ya guardados */
enum { ASP_43, ASP_43_INT, ASP_FULL, ASP_169, ASP_87, ASP_COUNT };
/* packs de armas: los mismos valores que NB2_WEAPONS_* (nb2.h) */
#define WEAPONS_COUNT 5
/* idioma del menu */
enum { LANG_ES, LANG_EN, LANG_COUNT };
/* controles tactiles: no, pequenos, medianos, grandes */
enum { TOUCH_OFF, TOUCH_S, TOUCH_M, TOUCH_L, TOUCH_COUNT };
/* START mete credito y empieza (o pausa en combate); MENU abre este menu */
enum { ACT_B1, ACT_B2, ACT_B3, ACT_START, ACT_MENU, ACT_REWIND, ACT_FF, ACT_COUNT };
#define NSLOTS 5            /* ranuras de partida guardada */

/* Botones del mando: los numeros son los de SDL_GameControllerButton; 100 y 101 son los gatillos L2/R2 */
#define PADBTN_L2 100
#define PADBTN_R2 101

typedef struct {
    int filter, aspect, volume;     /* volume 0-100 (general) */
    int music, sfx;                 /* musica y efectos 0-100 */
    int noifr;                      /* truco: sin invencibilidad tras recibir dano. 0 no, 1 jugador, 2 rival, 3 todos */
    int colors, bands, difficulty;
    int vibration;                  /* fuerza de la vibracion 0-100 (0 = sin vibracion) */
    int map[ACT_COUNT];             /* boton del mando para cada accion */
    int invuln, ammo, onehit;       /* trucos: 0/1 */
    int stage;                      /* truco de escenario: -1 normal o rival 0-7 (ver nb2.h) */
    int lang;                       /* LANG_* */
    int touch;                      /* TOUCH_* */
    int outfit, outfit_rival;       /* ropa del jugador 1 y del rival: NB2_OUTFIT_* */
    int game_lang;                  /* textos del juego: 0 ingles (original), 1 castellano */
    int weapons;                    /* pack de armas: NB2_WEAPONS_* (0 todas) */
} ofx_config;
#define TR(es, en) (cfg.lang == LANG_EN ? (en) : (es))

extern ofx_config cfg;
void config_defaults(void);
int config_load(const char *path);   /* devuelve 1 si el archivo traia el idioma */
void config_save(const char *path);
const char *padbtn_name(int b);

enum { MI_UP, MI_DOWN, MI_LEFT, MI_RIGHT, MI_OK, MI_BACK };
enum { MR_NONE, MR_PLAY, MR_QUIT, MR_CHANGED, MR_SAVE, MR_LOAD, MR_TITLE };   /* MR_TITLE: reiniciar la recreativa */

/* Launcher (pantalla de inicio): botones en el orden de la fila; menu.c lleva cual esta elegido y que abre cada uno,
   launcher.c los dibuja. En pantallas casi cuadradas van en dos filas: PARTIDAS JUGAR LOGROS / OPCIONES CONTROLES TRUCOS SALIR. */
enum { LB_OPTIONS, LB_CONTROLS, LB_GAMES, LB_PLAY, LB_ACH, LB_CHEATS, LB_QUIT, LB_COUNT };
#define LAUNCH_TWO_ROWS(w, h) ((long)(w) * 10 < (long)(h) * 15)
int menu_at_launcher(void);         /* se ve el launcher (no hay ninguna pagina abierta encima) */
int menu_launch_sel(void);          /* boton elegido */
int menu_launch_tap(int button);    /* toque en un boton: lo elige y lo abre; devuelve MR_* */

void menu_open(int in_game);        /* 0: launcher (al arrancar); 1: menu de partida */
int menu_input(int what);           /* devuelve MR_* */
int menu_waiting_button(void);      /* esperando a que se pulse un boton para asignarlo */
int menu_bind(int padbtn);          /* asigna el boton pulsado; devuelve MR_CHANGED */
void menu_tick(int ms);             /* cuenta atras de la asignacion */
void menu_render(uint32_t *argb);
int menu_tap(int canvas_x, int canvas_y);   /* toque con el dedo en el lienzo; devuelve MR_* */   /* MENU_W * MENU_H pixeles; en partida el fondo es translucido */
void menu_set_save_dir(const char *dir);
const char *menu_save_dir(void);
void menu_slot_path(int slot, char *buf, int n);   /* archivo de la ranura 0..NSLOTS-1 */
int menu_slot(void);                                /* ranura elegida en el menu */
/* encima del juego: PAUSA y/o un aviso corto; el resto del lienzo queda transparente */
void menu_render_osd(uint32_t *argb, int paused, const char *msg);
#endif
