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
    if (getenv("OFX_ROPA")) nb2_set_outfit(0, atoi(getenv("OFX_ROPA")));   /* OFX_ROPA=n: ropa del 1P (1 gotica, 2 negativa, 3 pinky, 4 vampiro) */
    if (getenv("OFX_ROPA2")) nb2_set_outfit(1, atoi(getenv("OFX_ROPA2")));   /* OFX_ROPA2=n: ropa del rival */   /* OFX_ES=1: textos del juego en castellano */
    /* OFX_ARMAS=n: todas las armas pasan a ser del tipo n; OFX_ARMAS=a,b,c,...: tabla completa (0x15 tipos) */
    if (getenv("OFX_ARMAS")) {
        static unsigned char map[0x15]; const char *q = getenv("OFX_ARMAS");
        if (!strchr(q, ',')) memset(map, atoi(q), sizeof map);
        else for (int i = 0; i < 0x15 && q; i++) { map[i] = atoi(q); q = strchr(q, ','); if (q) q++; }
        nb2_set_weapon_map(map);
    }
    nb2_set_cheats(getenv("OFX_TRUCOS") ? atoi(getenv("OFX_TRUCOS")) : 0, getenv("OFX_ESCENARIO") ? atoi(getenv("OFX_ESCENARIO")) : -1);
    if (getenv("OFX_DOSARMAS")) nb2_set_two_weapons(1);   /* OFX_DOSARMAS=1: dos armas (el boton 3 es el bit 0x40) */
    if (getenv("OFX_VS")) nb2_set_vs_stage(atoi(getenv("OFX_VS")));   /* OFX_VS=rival 0-7: escenario del combate VS */
    nb2_reset();
    if (getenv("OFX_DIF")) nb2_set_difficulty(atoi(getenv("OFX_DIF")));
    /* OFX_DANO=propio,rival: dano a medida en % (50-200, de 25 en 25) */
    if (getenv("OFX_DANO")) { int a = 100, b = 100; sscanf(getenv("OFX_DANO"), "%d,%d", &a, &b); nb2_set_damage(a, b); }
    { extern int dbg_pal; if (getenv("OFX_PAL")) dbg_pal = atoi(getenv("OFX_PAL")); }
    { extern int dbg_idx; if (getenv("OFX_IDX")) dbg_idx = atoi(getenv("OFX_IDX")); }
    { extern int dbg_row; if (getenv("OFX_ROW")) dbg_row = strtol(getenv("OFX_ROW"), 0, 16); }
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
        /* OFX_STICK=n: desde el fotograma n, cada 20 fotogramas el 1P se coloca donde esta el rival y pega (para acabar
           combates con el truco del punio); OFX_STICK_END=m lo deja en el fotograma m */
        if (getenv("OFX_STICK") && fr >= atoi(getenv("OFX_STICK")) && !(getenv("OFX_STICK_END") && fr >= atoi(getenv("OFX_STICK_END")))) {
            unsigned char *m = nb2_mem();
            if (m[0x208d59] && m[0x208e0d] && fr % 20 == 0) memcpy(m + 0x208d06 + 0x14, m + 0x208dba + 0x14, 8);
            p1 = (fr % 6) < 3 ? 0x10 : 0;
        }
        /* OFX_P2=n: en el fotograma n entra el 2P (moneda y START); despues pulsa el boton 1 de vez en cuando */
        int p2 = 0;
        if (getenv("OFX_P2")) {
            int n2 = atoi(getenv("OFX_P2"));
            if (fr == n2) nb2_coin();
            if (fr >= n2 + 30 && fr < n2 + 36) p2 = NB2_START;
            if (fr >= n2 + 200 && (fr % 50) < 4) p2 = 0x10;
        }
        nb2_set_inputs(p1, p2, 0);
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
        /* OFX_ARMLOG=1: cada 30 fotogramas, tipos de arma (objetos de clase 0x14) que hay en la RAM */
        if (getenv("OFX_ARMLOG") && fr % 30 == 0) {
            const unsigned char *m = nb2_mem(); static unsigned seen[64];
            for (unsigned a = 0x208000; a < 0x220000; a += 2)
                if ((m[a + 0x10] << 8 | m[a + 0x11]) == 0xc000 && (m[a + 0x12] << 8 | m[a + 0x13]) == 0x14 && m[a + 0x5e] == 0 && m[a + 0x5f] < 64) {
                    int t = m[a + 0x5f]; if (!seen[t]++) printf("%d arma tipo %d (id %d) escenario %d en %06x pedido %d modo %d\n", fr, t, t + 2, m[0x20807b] >> 4, a, m[a + 0x7f], m[a + 0x9d]);
                }
        }
        if (getenv("OFX_WATCH")) {   /* OFX_WATCH=dir,len: esos bytes en cada fotograma */
            unsigned a = 0, n = 0; sscanf(getenv("OFX_WATCH"), "%x,%x", &a, &n);
            printf("%d:", fr); for (unsigned k = 0; k < n; k++) printf("%02x", nb2_mem()[a + k]); printf("\n");
        }
        if (getenv("OFX_SND")) {   /* OFX_SND=1: voces del C352 que suenan, cuando cambian */
            extern unsigned c352_active(void); static unsigned last = 0xffffffff; unsigned a = c352_active();
            if (a != last) { printf("%d voces %08x\n", fr, a); last = a; }
        }
        if (getenv("OFX_LOG") && every && fr % every == 0) {
            const unsigned char *m = nb2_mem();
            printf("%d E1=%02x E2=%02x arma1=%d mun1=%d rival=%d combate=%d t=%d\n", fr, m[0x208d59], m[0x208e0d], m[0x208d65], m[0x208d67], m[0x20807b] >> 4, m[0x20808f], m[0x208092] << 8 | m[0x208093]);
        }
        /* OFX_STYLES=banco (1 jugador, 2 rival): en cada guardado, el mismo fotograma con las 5 ropas y solo ese banco
           de sprites sobre magenta (f000123_s0.ppm ... _s4.ppm); para las hojas de tools/make_outfit_sheets.py */
        if (getenv("OFX_STYLES") && every && fr % every == 0 && (nb2_mem()[0x208d59] || nb2_mem()[0x208e0d])) {
            extern int dbg_only; int bank = atoi(getenv("OFX_STYLES"));
            dbg_only = bank;
            for (int st = 0; st < NB2_OUTFIT_COUNT; st++) {
                char n[256]; FILE *f; const unsigned char *im;
                nb2_set_outfit(bank - 1, st); im = nb2_redraw();
                sprintf(n, "%s/f%06d_s%d.ppm", dir, fr, st);
                f = fopen(n, "wb"); fprintf(f, "P6 %d %d 255\n", nb2_width(), NB2_H); fwrite(im, 1, nb2_width() * NB2_H * 3, f); fclose(f);
            }
            dbg_only = -1; nb2_set_outfit(bank - 1, 0); nb2_redraw();
        } else
        if (every && fr % every == 0) {
            char n[256];
            sprintf(n, "%s/f%06d.ppm", dir, fr);
            FILE *f = fopen(n, "wb");
            if (getenv("OFX_VB") && nb2_vborder()) {   /* OFX_VB=1: con las franjas de arriba y abajo del 8:7 (teles) */
                const unsigned char *vb = nb2_vborder();
                fprintf(f, "P6 %d %d 255\n", NB2_W, NB2_H + 2 * NB2_VEXT);
                fwrite(vb, 1, NB2_VEXT * NB2_W * 3, f); fwrite(rgb, 1, NB2_W * NB2_H * 3, f); fwrite(vb + NB2_VEXT * NB2_W * 3, 1, NB2_VEXT * NB2_W * 3, f);
            } else {
            fprintf(f, "P6 %d %d 255\n", nb2_width(), NB2_H);
            fwrite(rgb, 1, nb2_width() * NB2_H * 3, f);
            }
            fclose(f);
            /* OFX_IDX=banco: ademas f000123.idx con la entrada de paleta (0-255) de ese banco en cada pixel (u16, 0xffff = otro) */
            { extern unsigned short dbg_idxbuf[]; if (getenv("OFX_IDX")) { sprintf(n, "%s/f%06d.idx", dir, fr); f = fopen(n, "wb"); fwrite(dbg_idxbuf, 2, nb2_width() * NB2_H, f); fclose(f); } }
        }
    }
    if (wav) {
        unsigned hdr[2]; unsigned rate = NB2_AUDIO_RATE, br = NB2_AUDIO_RATE * 4;
        fseek(wav, 24, SEEK_SET); fwrite(&rate, 4, 1, wav); fwrite(&br, 4, 1, wav);
        hdr[0] = 36 + wbytes; fseek(wav, 4, SEEK_SET); fwrite(hdr, 4, 1, wav);
        hdr[0] = wbytes; fseek(wav, 40, SEEK_SET); fwrite(hdr, 4, 1, wav);
        fclose(wav);
    }
    if (getenv("OFX_DECOR")) { extern void nb2_dbg_decor(const char *); nb2_dbg_decor(getenv("OFX_DECOR")); }   /* OFX_DECOR=archivo.ppm: decorado de las teles */
    if (getenv("OFX_DUMP")) { FILE *f = fopen(getenv("OFX_DUMP"), "wb"); fwrite(nb2_mem(), 1, 0x1000000, f); fclose(f); }
    return 0;
}
