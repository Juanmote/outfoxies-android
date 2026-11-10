/* Controles tactiles: cruceta y botones (disparo, salto, START y MENU) dibujados encima del juego. */
#ifndef OFX_TOUCH_H
#define OFX_TOUCH_H
#include <SDL.h>

#define TOUCH_MENU 0x100    /* ademas de los bits NB2_* (direcciones, B1, B2 y START) */

void touch_init(SDL_Renderer *r);
/* mira los eventos de dedos; devuelve 1 si era de la pantalla tactil */
int touch_event(const SDL_Event *e, SDL_Renderer *r);
unsigned touch_state(void);         /* NB2_* y TOUCH_MENU de lo que se esta pulsando ahora */
void touch_render(SDL_Renderer *r);
void touch_hide(void);              /* se ha usado un mando o el teclado: se esconden hasta el siguiente toque */
void touch_show(void);
void touch_release_all(void);
int touch_on_control(SDL_Renderer *r, float x, float y);
void touch_set_alpha(int a);   /* 255 normal; en el menu, mas transparentes para que se lea lo de detras */
#endif
