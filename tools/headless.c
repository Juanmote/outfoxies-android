/* Prueba sin imagen: headless <rom.zip> <frames> <cada_N> <frame_moneda> [carpeta] [p1_fijo_hex desde_frame]
   Variables: OFX_WIDE=1 (16:9), OFX_DIF=0/1/2 (facil/normal/dificil), OFX_LOAD=partida.sav, OFX_TRUCOS, OFX_ESCENARIO, OFX_LOG=1 (energia, arma y rival en cada guardado)
   Depuracion: OFX_SPR=n (lista de sprites del fotograma n), OFX_MASK=hex (quita capas: bit i = capa de losetas i,
   0x100/0x200 = ROZ 0/1, 0x1000 = sprites), OFX_TR=n (desde el fotograma n, escrituras en las capas de losetas con
   su PC; con OFX_LO/OFX_HI=hex, las de ese rango), OFX_SCR=1 (scroll de las capas), OFX_DUMP=mem.bin (memoria al final) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "achievements.h"
#include "menu.h"
int main(int c, char **v) {
    char err[128];
    int frames = atoi(v[2]), every = atoi(v[3]), play = atoi(v[4]);
    const char *dir = c > 5 ? v[5] : ".";
    int fixed = c > 7 ? (int)strtol(v[6], 0, 16) : -1, from = c > 7 ? atoi(v[7]) : 0;
    if (!nb2_load(v[1], err, sizeof err)) { fprintf(stderr, "ROM: %s\n", err); return 1; }
    if (getenv("OFX_WIDE")) nb2_set_wide(atoi(getenv("OFX_WIDE")) == 2 ? 2 : 1);   /* 2: laterales siempre a la vista */
    /* OFX_TRUCOS=bits (1 invulnerable, 2 municion, 4 punio) y OFX_ESCENARIO=rival 0-7 */
    if (getenv("OFX_ES")) nb2_set_game_lang(1);
    if (getenv("OFX_HUD87")) nb2_set_hud87(1);   /* OFX_HUD87=1: marcador del 8:7 */
    if (getenv("OFX_ROPA")) nb2_set_outfit(atoi(getenv("OFX_ROPA")));   /* OFX_ROPA=1: ropa gotica */   /* OFX_ES=1: textos del juego en castellano */
    nb2_set_cheats(getenv("OFX_TRUCOS") ? atoi(getenv("OFX_TRUCOS")) : 0, getenv("OFX_ESCENARIO") ? atoi(getenv("OFX_ESCENARIO")) : -1);
    nb2_reset();
    if (getenv("OFX_DIF")) nb2_set_difficulty(atoi(getenv("OFX_DIF")));
    { extern int dbg_pal; if (getenv("OFX_PAL")) dbg_pal = atoi(getenv("OFX_PAL")); }
    { extern int dbg_mask; if (getenv("OFX_MASK")) dbg_mask = strtol(getenv("OFX_MASK"), 0, 16); }
    /* OFX_LOAD=partida.sav: arranca desde una partida guardada */
    if (getenv("OFX_LOAD")) {
        FILE *f = fopen(getenv("OFX_LOAD"), "rb"); long n; unsigned char *b;
        if (!f) { fprintf(stderr, "no se pudo abrir %s\n", getenv("OFX_LOAD")); return 1; }
        fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
        b = malloc(n); fread(b, 1, n, f); fclose(f);
        if (!nb2_state_load(b, n)) { fprintf(stderr, "partida no valida\n"); return 1; }
        free(b);
    }
    char wn[256]; sprintf(wn, "%s/audio.wav", dir);
    FILE *wav = nb2_has_sound() ? fopen(wn, "wb") : NULL;
    long wbytes = 0;
    if (wav) fwrite("RIFF\0\0\0\0WAVEfmt \20\0\0\0\1\0\2\0\0\0\0\0\0\0\0\0\4\0\20\0data\0\0\0\0", 1, 44, wav);
    for (int fr = 0; fr < frames; fr++) {
        int p1 = 0;
        if (play) {
            if (fr == play || fr == play + 60) nb2_coin();
            if (fr >= play + 120 && fr < play + 126) p1 = NB2_START;
            /* OFX_CHAR=n: en la eleccion de personaje, n pulsaciones a la derecha antes de que juegue el bot */
            int nch = getenv("OFX_CHAR") ? atoi(getenv("OFX_CHAR")) : -1, ch0 = play + 240;
            if (nch >= 0 && fr >= play + 200 && fr < ch0 + 12 * nch + 30) {
                if (fr >= ch0 && fr < ch0 + 12 * nch && (fr - ch0) % 12 < 4) p1 = 1;
                if (fr >= ch0 + 12 * nch + 10 && fr < ch0 + 12 * nch + 16) p1 = 0x10;
            } else if (fixed >= 0) { if (fr >= from) p1 = fixed & 0xff; }
            else if (fr >= play + 200 && (fr % 30) < 15) {
                unsigned seed = (fr / 30) * 2654435761u;
                p1 = (seed >> 7) & 0x7f;
                if ((fr / 30) % 3 == 0) p1 |= 0x10;
            }
        }
        nb2_set_inputs(p1, 0, 0);
        /* OFX_POKE=fotograma:dir=valor,dir=valor (hex): escribe en memoria antes de ese fotograma */
        /* OFX_ACH=carpeta: prueba de logros (sin mirar trucos ni dificultad) */
        if (getenv("OFX_ACH")) { static int ini; int r; if (!ini) { ini = 1; config_defaults(); ach_init(getenv("OFX_ACH")); } ach_frame(nb2_mem(), 1); while ((r = ach_pop()) >= 0) printf("%d logro %d %s\n", fr, r, ach_title(r)); }
        /* varias separadas por ';' */
        for (const char *pk = getenv("OFX_POKE"); pk && *pk; pk = strchr(pk, ';') ? strchr(pk, ';') + 1 : "") {
            if (atoi(pk) != fr) continue;
            const char *q = strchr(pk, ':'), *end = strchr(pk, ';');
            while (q && (!end || q < end)) { unsigned a, b; if (sscanf(q + 1, "%x=%x", &a, &b) == 2) ((unsigned char *)nb2_mem())[a] = b; q = strchr(q + 1, ','); }
        }
        { extern int dbg_spr; dbg_spr = getenv("OFX_SPR") && fr == atoi(getenv("OFX_SPR")); extern int dbg_trace; extern unsigned dbg_lo, dbg_hi; dbg_trace = getenv("OFX_TR") && fr >= atoi(getenv("OFX_TR")); if (dbg_trace && getenv("OFX_LO")) { dbg_trace = 2; dbg_lo = strtoul(getenv("OFX_LO"), 0, 16); dbg_hi = strtoul(getenv("OFX_HI"), 0, 16); } }
        const unsigned char *rgb = nb2_frame();
        if (wav) { int n; const short *a = nb2_audio(&n); fwrite(a, 4, n, wav); wbytes += n * 4; }
        if (getenv("OFX_SCR")) { const unsigned char *m = nb2_mem(); fprintf(stderr, "F %d", fr); for (int i = 0; i < 4; i++) fprintf(stderr, " L%d=%d", i, ((m[0x6c0002 + i * 8] << 8 | m[0x6c0003 + i * 8]) & 0x1ff)); fprintf(stderr, "\n"); }
        if (getenv("OFX_LOG") && every && fr % every == 0) {
            const unsigned char *m = nb2_mem();
            printf("%d E1=%02x E2=%02x arma1=%d mun1=%d rival=%d combate=%d t=%d\n", fr, m[0x208d59], m[0x208e0d], m[0x208d65], m[0x208d67], m[0x20807b] >> 4, m[0x20808f], m[0x208092] << 8 | m[0x208093]);
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
    if (getenv("OFX_DUMP")) { FILE *f = fopen(getenv("OFX_DUMP"), "wb"); fwrite(nb2_mem(), 1, 0x1000000, f); fclose(f); }
    return 0;
}
