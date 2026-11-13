/* Visor de imagenes del launcher: GALERIA y MANUAL (como en Re-Master-ed of Darkness).
   Cada uno es un pack opcional (.zip de .jpg/.png) que NO va en el APK: galeria.zip y manual.zip en la carpeta de la
   app (la de las partidas). La primera vez se elige el archivo con el selector de Android y se copia ahi.
   El nombre de cada imagen, sin el numero del principio, es su pie; el numero solo marca el orden. */
#ifndef OFX_VIEWER_H
#define OFX_VIEWER_H
#include <SDL.h>
enum { VIEW_GALLERY = 1, VIEW_MANUAL = 2 };
void viewer_open(SDL_Renderer *r, int kind);
int viewer_active(void);
int viewer_event(SDL_Renderer *r, SDL_Event *e);   /* 1 si el evento era del visor */
void viewer_draw(SDL_Renderer *r);
void viewer_resume(SDL_Renderer *r);               /* la app vuelve al frente (tras el selector de archivos) */
#endif
