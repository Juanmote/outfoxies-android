/* Packs de ropa (skins.h) sacados de los estilos de ropa del juego (Original, Gotica, Negativa, Pinky, Vampiro), para
   revisarlos y retocarlos en el editor de ropa. Cada prenda se pone del color que le da el estilo a su color tipico
   (el de luz media); el sombreado lo pone el pack, asi que se parecen al estilo sin ser identicos (la Negativa, por
   ejemplo, no invierte las luces).
   Compilar como el headless (mismos .o) y: packs_estilos <rom.zip> <carpeta de salida> */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "skins.h"
int main(int argc, char **argv) {
    static const char *nm[NB2_OUTFIT_COUNT] = {"Pack Original", "Pack Gótica", "Pack Negativa", "Pack Pinky", "Pack Vampiro"};
    char err[128], path[1024];
    if (argc < 3 || !nb2_load(argv[1], err, sizeof err)) { fprintf(stderr, "uso: packs_estilos rom.zip carpeta\n"); return 1; }
    for (int st = 0; st < NB2_OUTFIT_COUNT; st++) {
        skin_pack p; skin_pack_clear(&p);
        snprintf(p.name, sizeof p.name, "%s", nm[st]);
        for (int ch = 0; ch < SKIN_CHARS; ch++) {
            const unsigned char *pal = nb2_mem() + skin_chars[ch].pal_rom;
            for (int k = 0; k < skin_chars[ch].nparts; k++) {
                unsigned c = skin_part_color(ch, k, pal);
                unsigned char rgb[3] = {c >> 16, c >> 8, c};
                nb2_style_color(st, ch, skin_chars[ch].parts[k].rng[0][0], rgb);   /* el numero de entrada: piel y pelo van aparte */
                p.set[ch][k] = 1; p.color[ch][k] = (unsigned)rgb[0] << 16 | rgb[1] << 8 | rgb[2];
            }
        }
        snprintf(path, sizeof path, "%s/%s.txt", argv[2], nm[st]);
        if (!skin_pack_save(&p, path)) { fprintf(stderr, "no se pudo escribir %s\n", path); return 1; }
        printf("%s\n", path);
    }
    return 0;
}
