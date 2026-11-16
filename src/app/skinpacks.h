/* Packs de ropa instalados: archivos .txt (skins.h) en la carpeta "ropas" de la app (junto a las partidas). Los crea el
   editor de ropa del launcher (skineditor.c) o se importan con el selector de archivos de Android. */
#ifndef OFX_SKINPACKS_H
#define OFX_SKINPACKS_H
#include <stddef.h>
#include "skins.h"
#define PACKS_MAX 40
void packs_dir(char *out, size_t n);          /* carpeta de los packs (la crea si no existe) */
void packs_path(const char *name, char *out, size_t n);   /* archivo de un pack por su nombre */
void packs_scan(void);                        /* vuelve a leer la carpeta (orden alfabetico) */
int packs_count(void);
const char *packs_name(int i);
const skin_pack *packs_get(int i);
int packs_find(const char *name);             /* indice, o -1 */
unsigned packs_stamp(void);                   /* cambia si cambian los archivos de la carpeta (para ver si llega uno importado) */
#endif
