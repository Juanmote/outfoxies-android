/* Packs de ropa a medida (editor de ropa del launcher).
   Cada luchador tiene su paleta de 256 colores (banco de sprites 1 el 1P, 2 el rival) y cada prenda son unas entradas
   de esa paleta (ESTADO.md 0.18.0). Un pack dice, por personaje y prenda, de que color va; lo que no dice se queda como
   en la recreativa. Al pintar se mantiene el sombreado del original: cada entrada de la prenda guarda su luz
   comparada con la media de la prenda y se pinta con el color elegido a esa luz (si se pasa de 255, el brillo va
   hacia el blanco).
   Archivo de texto (se puede tocar a mano):
       # The Outfoxies Remastered: pack de ropa
       john.vaqueros=1E3A8A
       eve.pelo=E01020
   (personaje.prenda=RRGGBB; ver skin_chars en skins.c para los nombres). */
#ifndef OFX_SKINS_H
#define OFX_SKINS_H
#define SKIN_CHARS 7          /* numeros del juego: 0 John, 1 Eve, 2 Betty, 3 Bernard, 4 Ching, 5 Dweeb, 6 Danny y Demi */
#define SKIN_MAXPARTS 12
typedef struct { const char *slug, *es, *en; unsigned char rng[3][2]; } skin_part;   /* hasta 3 tramos [desde, hasta] */
typedef struct { const char *slug, *name; unsigned pal_rom; int nparts; skin_part parts[SKIN_MAXPARTS]; } skin_char;
extern const skin_char skin_chars[SKIN_CHARS];

typedef struct {
    char name[64];                                     /* nombre (el del archivo sin .txt) */
    unsigned char set[SKIN_CHARS][SKIN_MAXPARTS];      /* 1 = esa prenda cambia */
    unsigned color[SKIN_CHARS][SKIN_MAXPARTS];         /* 0xRRGGBB */
    unsigned version;                                  /* sube con cada cambio (para rehacer lo calculado) */
} skin_pack;

void skin_pack_clear(skin_pack *p);
int skin_pack_load(skin_pack *p, const char *path);    /* 1 si se ha podido leer */
int skin_pack_save(const skin_pack *p, const char *path);
int skin_part_of(int ch, int idx);                     /* prenda de esa entrada de la paleta, o -1 */
/* pal: los 256 colores originales del personaje (R,G,B). out: los del pack; changed[i] = 1 si la entrada i cambia */
void skin_apply(const skin_pack *p, int ch, const unsigned char *pal, unsigned char *out, unsigned char *changed);
unsigned skin_part_color(int ch, int part, const unsigned char *pal);   /* color tipico de la prenda en esa paleta */
#endif
