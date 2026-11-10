/* Launcher: pantalla de inicio con la portada de fondo y una fila de botones (los paneles de cristal del usuario),
   al estilo de Re-Master-ed of Darkness. El arte sale de tools/make_launcher.py (launcher_gfx.h).
   Que boton esta elegido y que abre cada uno lo lleva menu.c (menu_launch_*); aqui solo se dibuja y se mira donde
   cae un toque. Los botones LB_* estan en menu.h. */
#ifndef OFX_LAUNCHER_H
#define OFX_LAUNCHER_H
#include <SDL.h>

int launcher_init(SDL_Renderer *r);     /* carga la portada y los botones; 1 si todo bien */
/* portada + botones. sel = boton elegido (LB_*); dimmed = hay una pagina de opciones abierta encima */
void launcher_draw(SDL_Renderer *r, int sel, int dimmed, Uint32 ticks);
int launcher_hit(SDL_Renderer *r, int x, int y);   /* boton en ese punto de la pantalla, o -1 */
/* la portada sola, recortada para llenar area y oscurecida (dim 0-255): relleno de las bandas */
void launcher_cover(SDL_Renderer *r, SDL_Rect area, int dim);
#endif
