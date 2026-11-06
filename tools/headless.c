/* Prueba sin imagen: headless <rom.zip> <frames> <cada_N> <frame_moneda> [carpeta] [p1_fijo_hex desde_frame]
   Variables: OFX_WIDE=1 (16:9), OFX_TRUCOS, OFX_ESCENARIO, OFX_LOG=1 (energia, arma y rival en cada guardado) */
#include <stdio.h>
#include <stdlib.h>
#include "nb2.h"
int main(int c, char **v) {
    char err[128];
    int frames = atoi(v[2]), every = atoi(v[3]), play = atoi(v[4]);
    const char *dir = c > 5 ? v[5] : ".";
    int fixed = c > 7 ? (int)strtol(v[6], 0, 16) : -1, from = c > 7 ? atoi(v[7]) : 0;
    if (!nb2_load(v[1], err, sizeof err)) { fprintf(stderr, "ROM: %s\n", err); return 1; }
    if (getenv("OFX_WIDE")) nb2_set_wide(1);
    /* OFX_TRUCOS=bits (1 invulnerable, 2 municion, 4 punio) y OFX_ESCENARIO=rival 0-7 */
    nb2_set_cheats(getenv("OFX_TRUCOS") ? atoi(getenv("OFX_TRUCOS")) : 0, getenv("OFX_ESCENARIO") ? atoi(getenv("OFX_ESCENARIO")) : -1);
    nb2_reset();
    char wn[256]; sprintf(wn, "%s/audio.wav", dir);
    FILE *wav = nb2_has_sound() ? fopen(wn, "wb") : NULL;
    long wbytes = 0;
    if (wav) fwrite("RIFF\0\0\0\0WAVEfmt \20\0\0\0\1\0\2\0\0\0\0\0\0\0\0\0\4\0\20\0data\0\0\0\0", 1, 44, wav);
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
        if (wav) { int n; const short *a = nb2_audio(&n); fwrite(a, 4, n, wav); wbytes += n * 4; }
        if (getenv("OFX_LOG") && every && fr % every == 0) {
            const unsigned char *m = nb2_mem();
            printf("%d E1=%02x E2=%02x arma1=%d mun1=%d rival=%d combate=%d\n", fr, m[0x208d59], m[0x208e0d], m[0x208d65], m[0x208d67], m[0x20807b] >> 4, m[0x20808f]);
        }
        if (every && fr % every == 0) {
            char n[256];
            sprintf(n, "%s/f%06d.ppm", dir, fr);
            FILE *f = fopen(n, "wb");
            fprintf(f, "P6 %d %d 255\n", nb2_width(), NB2_H);
            fwrite(rgb, 1, nb2_width() * NB2_H * 3, f);
            fclose(f);
        }
    }
    if (wav) {
        unsigned hdr[2]; unsigned rate = NB2_AUDIO_RATE, br = NB2_AUDIO_RATE * 4;
        fseek(wav, 24, SEEK_SET); fwrite(&rate, 4, 1, wav); fwrite(&br, 4, 1, wav);
        hdr[0] = 36 + wbytes; fseek(wav, 4, SEEK_SET); fwrite(hdr, 4, 1, wav);
        hdr[0] = wbytes; fseek(wav, 40, SEEK_SET); fwrite(hdr, 4, 1, wav);
        fclose(wav);
    }
    return 0;
}
