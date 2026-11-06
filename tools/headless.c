/* Prueba sin imagen: headless <rom.zip> <frames> <cada_N> <frame_moneda> [carpeta] [p1_fijo_hex desde_frame] */
#include <stdio.h>
#include <stdlib.h>
#include "nb2.h"
int main(int c, char **v) {
    char err[128];
    int frames = atoi(v[2]), every = atoi(v[3]), play = atoi(v[4]);
    const char *dir = c > 5 ? v[5] : ".";
    int fixed = c > 7 ? (int)strtol(v[6], 0, 16) : -1, from = c > 7 ? atoi(v[7]) : 0;
    if (!nb2_load(v[1], err, sizeof err)) { fprintf(stderr, "ROM: %s\n", err); return 1; }
    nb2_reset();
    for (int fr = 0; fr < frames; fr++) {
        int p1 = 0;
        if (play) {
            if (fr == play || fr == play + 60) nb2_coin();
            if (fr >= play + 120 && fr < play + 126) p1 = NB2_START;
            if (fixed >= 0) { if (fr >= from) p1 = fixed & 0xff; }
            else if (fr >= play + 200 && (fr % 30) < 15) {
                unsigned seed = (fr / 30) * 2654435761u;
                p1 = (seed >> 7) & 0x7f;
                if ((fr / 30) % 3 == 0) p1 |= 0x10;
            }
        }
        nb2_set_inputs(p1, 0, 0);
        const unsigned char *rgb = nb2_frame();
        if (every && fr % every == 0) {
            char n[256];
            sprintf(n, "%s/f%06d.ppm", dir, fr);
            FILE *f = fopen(n, "wb");
            fprintf(f, "P6 %d %d 255\n", NB2_W, NB2_H);
            fwrite(rgb, 1, NB2_W * NB2_H * 3, f);
            fclose(f);
        }
    }
    return 0;
}
