/* Editor de ropa (boton ROPA del launcher). Se elige el personaje, se ve dibujado (sacado de la ROM, con la postura de
   skin_poses.h) y cada prenda tiene su cajon de color: al cambiarlo se ve al momento. Lo que se hace es un pack de ropa
   (skins.h) que se guarda en la carpeta "ropas" de la app y se elige en el juego (OPCIONES > Pantalla > Ropa). Se puede
   abrir cualquier pack para cambiarlo, importar uno con el selector de archivos y exportarlo (guardar como...) para
   pasarlo a otros. */
#ifndef OFX_SKINEDITOR_H
#define OFX_SKINEDITOR_H
#include <SDL.h>
void skined_open(SDL_Renderer *r);
int skined_active(void);
int skined_event(SDL_Renderer *r, SDL_Event *e);   /* 1 si el evento era del editor */
void skined_draw(SDL_Renderer *r);
void skined_resume(void);                          /* la app vuelve al frente (tras el selector de archivos) */
int skined_closed(void);                           /* 1 una vez tras cerrarse: hay que volver a leer los packs */
#endif
