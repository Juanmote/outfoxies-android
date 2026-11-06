/* Menu de opciones (launcher y pausa). No depende de SDL: dibuja en un lienzo ARGB de MENU_W x MENU_H. */
#ifndef OFX_MENU_H
#define OFX_MENU_H
#include <stdint.h>

#define MENU_W 480
#define MENU_H 270

enum { FILT_SHARP, FILT_SMOOTH, FILT_SCANLINES, FILT_COUNT };
/* ASP_169 va al final para no cambiar el significado de los opciones.txt ya guardados */
enum { ASP_43, ASP_43_INT, ASP_FULL, ASP_169, ASP_COUNT };
enum { ACT_B1, ACT_B2, ACT_B3, ACT_COIN, ACT_START, ACT_COUNT };

/* Botones del mando: los numeros son los de SDL_GameControllerButton; 100 y 101 son los gatillos L2/R2 */
#define PADBTN_L2 100
#define PADBTN_R2 101

typedef struct {
    int filter, aspect, volume;     /* volume 0-100 */
    int map[ACT_COUNT];             /* boton del mando para cada accion */
} ofx_config;

extern ofx_config cfg;
void config_defaults(void);
void config_load(const char *path);
void config_save(const char *path);
const char *padbtn_name(int b);

enum { MI_UP, MI_DOWN, MI_LEFT, MI_RIGHT, MI_OK, MI_BACK };
enum { MR_NONE, MR_PLAY, MR_QUIT, MR_CHANGED };

void menu_open(int in_game);
int menu_input(int what);           /* devuelve MR_* */
int menu_waiting_button(void);      /* esperando a que se pulse un boton para asignarlo */
int menu_bind(int padbtn);          /* asigna el boton pulsado; devuelve MR_CHANGED */
void menu_tick(int ms);             /* cuenta atras de la asignacion */
void menu_render(uint32_t *argb);   /* MENU_W * MENU_H pixeles; en partida el fondo es translucido */
#endif
