/* Musica del launcher (pantalla de inicio y sus opciones). La pieza es original: se compone y se sintetiza en
   menumusic.c al arrancar (en otro hilo), sin archivos de audio ni nada de la ROM. Estilo: banda sonora de espias
   de los 80-90 (bajo hipnotico, guitarra con eco de muelle, metales y la linea cromatica de cuerdas). */
#ifndef OFX_MENUMUSIC_H
#define OFX_MENUMUSIC_H
#include <SDL.h>
void menumusic_start(void);      /* empieza a componerla en otro hilo */
/* llamar en cada vuelta del bucle: active = se ve el launcher (o sus paginas) y la musica esta puesta;
   gain 0-1. Al dejar de estar activa se vacia la cola, para que el juego entre limpio. */
void menumusic_update(SDL_AudioDeviceID dev, int active, float gain);
#endif
