/* Menu de opciones (launcher y pausa). No depende de SDL: dibuja en un lienzo ARGB de MENU_W x MENU_H. */
#ifndef OFX_MENU_H
#define OFX_MENU_H
#include <stdint.h>

#define MENU_W 480
#define MENU_H 270

enum { FILT_SHARP, FILT_SMOOTH, FILT_SCANLINES, FILT_COUNT };
/* ASP_169 va al final para no cambiar el significado de los opciones.txt ya guardados */
enum { ASP_43, ASP_43_INT, ASP_FULL, ASP_169, ASP_COUNT };
/* START mete credito y empieza (o pausa en combate); MENU abre este menu */
enum { ACT_B1, ACT_B2, ACT_B3, ACT_START, ACT_MENU, ACT_REWIND, ACT_FF, ACT_COUNT };
#define NSLOTS 5            /* ranuras de partida guardada */

/* Botones del mando: los numeros son los de SDL_GameControllerButton; 100 y 101 son los gatillos L2/R2 */
#define PADBTN_L2 100
#define PADBTN_R2 101

typedef struct {
    int filter, aspect, volume;     /* volume 0-100 */
    int vibration;                  /* fuerza de la vibracion 0-100 (0 = sin vibracion) */
    int map[ACT_COUNT];             /* boton del mando para cada accion */
    int invuln, ammo, onehit;       /* trucos: 0/1 */
    int stage;                      /* truco de escenario: -1 normal o rival 0-7 (ver nb2.h) */
} ofx_config;

extern ofx_config cfg;
void config_defaults(void);
void config_load(const char *path);
void config_save(const char *path);
const char *padbtn_name(int b);

enum { MI_UP, MI_DOWN, MI_LEFT, MI_RIGHT, MI_OK, MI_BACK };
enum { MR_NONE, MR_PLAY, MR_QUIT, MR_CHANGED, MR_SAVE, MR_LOAD };

void menu_open(int in_game);
int menu_input(int what);           /* devuelve MR_* */
int menu_waiting_button(void);      /* esperando a que se pulse un boton para asignarlo */
int menu_bind(int padbtn);          /* asigna el boton pulsado; devuelve MR_CHANGED */
void menu_tick(int ms);             /* cuenta atras de la asignacion */
void menu_render(uint32_t *argb);   /* MENU_W * MENU_H pixeles; en partida el fondo es translucido */
void menu_set_save_dir(const char *dir);
void menu_slot_path(int slot, char *buf, int n);   /* archivo de la ranura 0..NSLOTS-1 */
int menu_slot(void);                                /* ranura elegida en el menu */
/* encima del juego: PAUSA y/o un aviso corto; el resto del lienzo queda transparente */
void menu_render_osd(uint32_t *argb, int paused, const char *msg);
#endif
