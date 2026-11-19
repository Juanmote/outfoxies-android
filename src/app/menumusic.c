/* Musica del launcher. Ver menumusic.h.
   La pieza: Mi menor, 126 pulsos, 32 compases (~61 s) que se repiten sin corte.
     1-4   entrada: bajo, charles y la linea cromatica de cuerdas (si - do - do# - do)
     5-12  A: la guitarra (twang grave con vibrato y eco de muelle) toca el tema
     13-20 B: Do maj7 - La m - Fa# m7b5 - Si7, golpes de metales y la guitarra mas abajo
     21-28 A': el tema otra vez; la segunda mitad la doblan los metales una octava arriba
     29-32 final: acorde de Mi m(maj9) en los metales, un lick de guitarra y redoble para volver al principio
   Se sintetiza a 28000 Hz en mono (nota a nota sobre un buffer, mas una reverb sencilla) y al sonar se pasa a
   84000 Hz estereo (NB2_AUDIO_RATE = 3 x 28000) interpolando. */
#include "menumusic.h"
#include "../core/nb2.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define SR 28000
#define UP 3
#if NB2_AUDIO_RATE != SR * UP
#error "la musica del menu da por hecho NB2_AUDIO_RATE = 84000"
#endif
#define BPM 126.0
#define BARS 32
#define PI2 6.283185307179586

static short *pcm;            /* la pieza ya hecha, mono a SR */
static int pcm_n;
static SDL_atomic_t ready;
static float *mix, *wet;      /* seco y lo que va a la reverb */
static int buf_n;
static double E8;             /* muestras por corchea */
static unsigned rng = 0x1234567u;

static float noise(void) { rng = rng * 1664525u + 1013904223u; return (float)((rng >> 8) & 0xffff) / 32768.0f - 1.0f; }
static double mfreq(double m) { return 440.0 * pow(2.0, (m - 69.0) / 12.0); }
static void put(long i, float v, float send) { if (i >= 0 && i < buf_n) { mix[i] += v; wet[i] += v * send; } }

/* ---------- instrumentos ---------- */
enum { BASS, GTR, BRASS, PAD };
static void play(int inst, int midi, double t0, double dur, float vel) {
    double f = mfreq(midi), ph = 0, ph2 = 0, ph3 = 0;
    long start = (long)t0, rel = inst == PAD ? SR / 2 : inst == GTR ? SR / 12 : inst == BRASS ? SR / 11 : SR / 40;
    long n = (long)dur + rel;
    float lp = 0, lp2 = 0, low = 0, band = 0;
    for (long i = 0; i < n; i++) {
        double t = (double)i / SR, g = i < dur ? 1.0 : exp(-(i - dur) / (rel / 4.0));
        float x = 0, out;
        switch (inst) {
        case BASS: {        /* bajo pulsado: triangulo y sierra con un filtro que se cierra */
            double a = 1 - exp(-PI2 * (260 + 1500 * exp(-t * 14)) / SR);
            ph += f / SR; ph -= floor(ph);
            x = (float)(0.65 * (1 - 4 * fabs(ph - 0.5)) + 0.35 * (2 * ph - 1));
            lp += (float)a * (x - lp); lp2 += (float)a * (lp - lp2);
            out = lp2 * (float)(exp(-t * 2.6) * g * (i < 60 ? i / 60.0 : 1));
            put(start + i, out * vel, 0.1f);
            break;
        }
        case GTR: {         /* guitarra twang: sierra + pulso, filtro con ataque brillante, vibrato que entra tarde */
            double vib = t < 0.22 ? 0 : (t < 0.6 ? (t - 0.22) / 0.38 : 1) * 0.011 * sin(PI2 * 5.6 * t);
            double fc = 700 + 3000 * exp(-t * 7), ff = 2 * sin(3.14159265 * (fc > 4000 ? 4000 : fc) / SR);
            ph += f * (1 + vib) / SR; ph -= floor(ph);
            x = (float)((2 * ph - 1) * 0.6 + (ph < 0.3 ? 0.5 : -0.5) * 0.5);
            if (i < SR / 150) x += noise() * 0.5f;                 /* la pua */
            low += (float)ff * band; band += (float)ff * (x - low - 0.9f * band);
            out = low * (float)((0.5 + 0.5 * exp(-t * 5)) * exp(-t * 0.45) * g * (i < 40 ? i / 40.0 : 1));
            put(start + i, out * vel, 0.32f);
            put(start + i + (long)(0.115 * SR), out * vel * 0.28f, 0.2f);   /* eco corto (slapback) */
            break;
        }
        case BRASS: {       /* metales: dos sierras desafinadas y una octava abajo, el filtro se abre al atacar */
            double scoop = i < SR / 25 ? -0.018 * (1 - (double)i / (SR / 25)) : 0;
            double open = (1 - exp(-t * 35)) * (0.7 + 0.3 * exp(-t * 4)), fc = 400 + 3200 * open;
            double ff = 2 * sin(3.14159265 * (fc > 3800 ? 3800 : fc) / SR);
            ph += f * (1.0035 + scoop) / SR; ph -= floor(ph);
            ph2 += f * (0.9965 + scoop) / SR; ph2 -= floor(ph2);
            ph3 += f * 0.5 / SR; ph3 -= floor(ph3);
            x = (float)((2 * ph - 1) + (2 * ph2 - 1) + 0.4 * (2 * ph3 - 1)) * 0.4f;
            low += (float)ff * band; band += (float)ff * (x - low - 1.1f * band);
            out = low * (float)((0.8 + 0.2 * exp(-t * 7)) * g * (i < SR / 70 ? (double)i / (SR / 70) : 1));
            put(start + i, out * vel, 0.25f);
            break;
        }
        case PAD: {         /* cuerdas: tres sierras desafinadas, suaves, entrada lenta */
            double v = 1 + 0.003 * sin(PI2 * 4.7 * t);
            ph += f * 1.004 * v / SR; ph -= floor(ph);
            ph2 += f * 0.996 * v / SR; ph2 -= floor(ph2);
            ph3 += f * 1.0 * v / SR; ph3 -= floor(ph3);
            x = (float)((2 * ph - 1) + (2 * ph2 - 1) + (2 * ph3 - 1)) / 3;
            lp += 0.22f * (x - lp); lp2 += 0.22f * (lp - lp2);
            out = lp2 * (float)(g * (t < 0.35 ? t / 0.35 : 1));
            put(start + i, out * vel, 0.45f);
            break;
        }
        }
    }
}
enum { KICK, SNARE, HAT, OHAT, RIDE, CRASH };
static void drum(int kind, double t0, float vel) {
    long start = (long)t0, n = kind == CRASH ? SR * 2 : kind == RIDE ? SR : SR / 3;
    double ph = 0;
    float lp = 0;
    for (long i = 0; i < n; i++) {
        double t = (double)i / SR;
        float x = 0, nz;
        switch (kind) {
        case KICK:
            ph += (45 + 95 * exp(-t * 32)) / SR;
            x = (float)(sin(PI2 * ph) * exp(-t * 9)) + (i < 50 ? noise() * 0.3f : 0);
            put(start + i, x * vel, 0.03f); break;
        case SNARE:
            nz = noise(); lp += 0.35f * (nz - lp);
            ph += 185.0 / SR;
            x = (float)(sin(PI2 * ph) * exp(-t * 28) * 0.5 + (nz - lp) * exp(-t * 15) * 0.9);
            put(start + i, x * vel, 0.25f); break;
        case HAT: case OHAT: case RIDE: case CRASH: {
            double d = kind == HAT ? 70 : kind == OHAT ? 9 : kind == RIDE ? 4.5 : 2.0;
            nz = noise(); lp += 0.55f * (nz - lp);
            x = (nz - lp) * (float)exp(-t * d);
            if (kind == RIDE) x = x * 0.5f + (float)((sin(PI2 * 3150 * t) + sin(PI2 * 4270 * t) * 0.7 + sin(PI2 * 5310 * t) * 0.5) * exp(-t * 3.5) * 0.12);
            put(start + i, x * vel, kind == CRASH ? 0.3f : 0.12f); break;
        }
        }
    }
}

/* ---------- partitura ---------- */
/* "e3:2 a#3:1 r:2 e3+g3+b3:8": nota (o acorde con +), duracion en corcheas; r = silencio. bar = compas (desde 0) */
static int parse_note(const char **p) {
    static const int base[7] = {9, 11, 0, 2, 4, 5, 7};   /* a b c d e f g */
    int s = base[**p - 'a'], oct; (*p)++;
    if (**p == '#') { s++; (*p)++; } else if (**p == 'b') { s--; (*p)++; }
    oct = **p - '0'; (*p)++;
    return 12 * (oct + 1) + s;
}
static void seq(int inst, int bar, const char *s, float vel, int transpose) {
    double t = bar * 8.0;
    while (*s) {
        int notes[8], nn = 0; double len;
        while (*s == ' ') s++;
        if (!*s) break;
        if (*s == 'r') s++;
        else for (;;) { if (nn < 8) notes[nn++] = parse_note(&s); else parse_note(&s); if (*s == '+') s++; else break; }
        len = strtod(s + 1, (char **)&s);
        for (int k = 0; k < nn; k++) play(inst, notes[k] + transpose, t * E8, len * E8 * 0.92, vel);
        t += len;
    }
}
/* bateria de un compas: una letra por corchea. k bombo, s caja, h charles, o charles abierto, r ride, x crash,
   K bombo y charles, S caja y charles, R bombo y ride, T caja y ride, f redoble (dos semicorcheas de caja), . nada */
static void beat(int bar, const char *p, float vel) {
    vel *= 0.75f;
    for (int i = 0; i < 8 && p[i]; i++) {
        double t = (bar * 8 + i) * E8;
        float acc = i % 2 ? 0.7f : 1.0f;
        switch (p[i]) {
        case 'k': drum(KICK, t, vel); break;
        case 's': drum(SNARE, t, vel * 0.8f); break;
        case 'h': drum(HAT, t, vel * 0.28f * acc); break;
        case 'o': drum(OHAT, t, vel * 0.3f); break;
        case 'r': drum(RIDE, t, vel * 0.4f * acc); break;
        case 'x': drum(CRASH, t, vel * 0.45f); drum(KICK, t, vel); break;
        case 'K': drum(KICK, t, vel); drum(HAT, t, vel * 0.28f * acc); break;
        case 'S': drum(SNARE, t, vel * 0.8f); drum(HAT, t, vel * 0.28f * acc); break;
        case 'R': drum(KICK, t, vel); drum(RIDE, t, vel * 0.4f * acc); break;
        case 'T': drum(SNARE, t, vel * 0.8f); drum(RIDE, t, vel * 0.4f * acc); break;
        case 'f': drum(SNARE, t, vel * 0.55f * (0.6f + i / 14.0f)); drum(SNARE, t + E8 / 2, vel * 0.6f * (0.6f + i / 12.0f)); break;
        }
    }
}

#define VAMP "e2:1 e2:1 f#2:1 f#2:1 g2:1 g2:1 f#2:1 f#2:1"
static const char *const THEME_A =                       /* 4 compases (y THEME_A2 otros 4) */
    "r:2 e3:1 g3:1 a3:2 a#3:1 b3:1 b3:6 r:2 "
    "r:2 b3:1 d4:1 e4:2 d4:1 b3:1 c4:3 b3:1 a3:4 ";
static const char *const THEME_A2 =
    "r:2 e3:1 g3:1 a3:2 a#3:1 b3:1 e4:4 d#4:2 d4:2 "
    "c#4:2 c4:2 b3:2 a3:1 g3:1 e3:6 r:2";
static void compose(void) {
    int b;
    /* bajo */
    for (b = 0; b < 12; b++) seq(BASS, b, VAMP, 0.4f, 0);
    {
        static const char *const broot[4] = {"c2", "a1", "f#2", "b1"};
        for (int c = 0; c < 4; c++) {
            char s[96]; const char *r = broot[c]; char hi[4]; snprintf(hi, sizeof hi, "%c%s%c", r[0], r[1] == '#' ? "#" : "", r[strlen(r) - 1] + 1);
            snprintf(s, sizeof s, "%s:1 %s:1 %s:1 %s:1 %s:1 %s:1 %s:1 %s:1", r, r, hi, r, r, r, hi, r);
            seq(BASS, 12 + c * 2, s, 0.45f, 0); seq(BASS, 13 + c * 2, s, 0.45f, 0);
        }
    }
    for (b = 20; b < 28; b++) seq(BASS, b, VAMP, 0.4f, 0);
    seq(BASS, 28, "e2:8", 0.45f, 0); seq(BASS, 29, "r:4 e2:1 e2:1 g2:1 a#2:1", 0.4f, 0);
    seq(BASS, 30, VAMP, 0.4f, 0); seq(BASS, 31, VAMP, 0.4f, 0);
    /* cuerdas: mi y sol fijos y la linea cromatica */
    for (b = 0; b < 12; b += 2) { seq(PAD, b, "e3+g3:16", 0.12f, 0); seq(PAD, b, "b3:4 c4:4 c#4:4 c4:4", 0.14f, 0); }
    seq(PAD, 12, "c3+e3+g3+b3:16", 0.1f, 0); seq(PAD, 14, "a2+c3+e3+a3:16", 0.1f, 0);
    seq(PAD, 16, "f#3+a3+c4+e4:16", 0.1f, 0); seq(PAD, 18, "b2+d#3+f#3+a3:16", 0.1f, 0);
    for (b = 20; b < 28; b += 2) { seq(PAD, b, "e3+g3:16", 0.12f, 0); seq(PAD, b, "b3:4 c4:4 c#4:4 c4:4", 0.14f, 0); }
    seq(PAD, 28, "e3+g3+b3+d#4+f#4:16", 0.13f, 0);
    seq(PAD, 30, "e3+g3:16", 0.12f, 0); seq(PAD, 30, "b3:4 c4:4 c#4:4 c4:4", 0.14f, 0);
    /* guitarra */
    seq(GTR, 4, THEME_A, 0.7f, 0); seq(GTR, 8, THEME_A2, 0.7f, 0);
    seq(GTR, 12, "g4:4 e4:2 b3:2 c4:6 r:2 a3:2 c4:2 e4:2 g4:2 e4:6 r:2 "
                 "f#4:2 a4:2 c5:2 a4:2 e4:6 r:2 d#4:2 f#4:2 a4:2 b4:2 a4:2 f#4:2 d#4:4", 0.62f, -12);
    seq(GTR, 20, THEME_A, 0.7f, 0); seq(GTR, 24, THEME_A2, 0.7f, 0);
    seq(GTR, 29, "e4:1 d4:1 b3:1 a3:1 g3:1 e3:3", 0.7f, 0);
    /* metales: golpes en B, el tema doblado en A' y el acorde final */
    {
        static const char *const ch[4] = {"e4+g4+b4", "e4+a4+c5", "f#4+a4+c5", "d#4+f#4+a4"};
        for (int c = 0; c < 8; c++) {
            char s[128]; const char *k = ch[c / 2];
            if (c % 2 == 0) snprintf(s, sizeof s, "%s:1 r:2 %s:1 r:2 %s:2", k, k, k);
            else snprintf(s, sizeof s, "r:3 %s:1 r:2 %s:1 %s:1", k, k, k);
            seq(BRASS, 12 + c, s, 0.26f, 0);
        }
    }
    seq(BRASS, 24, THEME_A2, 0.36f, 12);
    seq(BRASS, 28, "e3+g3+b3+d#4+f#4:7 r:1", 0.3f, 0);
    seq(BRASS, 31, "r:6 b3+d#4+f#4:2", 0.22f, 0);
    /* bateria */
    beat(0, "khhhhhhh", 0.8f); beat(1, "hhhhhhho", 0.8f);
    beat(2, "KhShKKSh", 0.85f); beat(3, "KhShKKSo", 0.85f);
    for (b = 4; b < 12; b++) beat(b, b % 2 ? "KhShKKSo" : (b == 4 ? "xhShKKSh" : "KhShKKSh"), 0.9f);
    for (b = 12; b < 20; b++) beat(b, b == 12 ? "xrTrRRTr" : (b == 19 ? "RrTrffff" : "RrTrRRTr"), 0.9f);
    for (b = 20; b < 28; b++) beat(b, b % 2 ? "KhShKKSo" : (b == 20 ? "xhShKKSh" : "KhShKKSh"), 0.9f);
    beat(28, "x.......", 0.95f); beat(29, "hhhhhhho", 0.75f);
    beat(30, "KhShKKSh", 0.9f); beat(31, "KhShffff", 0.95f);
}

/* reverb sencilla (4 peines y 2 pasatodo, al estilo Freeverb) sobre wet, sumada a mix */
static void reverb(void) {
    static const int cl[4] = {709, 757, 811, 863}, al[2] = {353, 281};
    float *cb[4], *ab[2], cf[4] = {0, 0, 0, 0};
    int ci[4] = {0, 0, 0, 0}, ai[2] = {0, 0};
    for (int k = 0; k < 4; k++) cb[k] = calloc(cl[k], sizeof(float));
    for (int k = 0; k < 2; k++) ab[k] = calloc(al[k], sizeof(float));
    for (int k = 0; k < 4; k++) if (!cb[k]) goto out;
    for (int k = 0; k < 2; k++) if (!ab[k]) goto out;
    for (int i = 0; i < buf_n; i++) {
        float in = wet[i] * 0.3f, o = 0;
        for (int k = 0; k < 4; k++) {
            float y = cb[k][ci[k]];
            cf[k] = y * 0.7f + cf[k] * 0.3f;
            cb[k][ci[k]] = in + cf[k] * 0.82f;
            if (++ci[k] >= cl[k]) ci[k] = 0;
            o += y;
        }
        for (int k = 0; k < 2; k++) {
            float b = ab[k][ai[k]];
            ab[k][ai[k]] = o + b * 0.5f;
            o = b - o * 0.5f;
            if (++ai[k] >= al[k]) ai[k] = 0;
        }
        mix[i] += o * 0.55f;
    }
out:
    for (int k = 0; k < 4; k++) free(cb[k]);
    for (int k = 0; k < 2; k++) free(ab[k]);
}

static int build(void *unused) {
    double peak = 0;
    int len;
    (void)unused;
    E8 = SR * 60.0 / BPM / 2;
    len = (int)(BARS * 8 * E8);
    buf_n = len + SR * 3;                       /* con la cola de la reverb */
    mix = calloc(buf_n, sizeof(float)); wet = calloc(buf_n, sizeof(float));
    pcm = malloc(sizeof(short) * len);
    if (!mix || !wet || !pcm) { free(mix); free(wet); free(pcm); pcm = NULL; return 0; }
    compose();
    reverb();
    for (int i = len; i < buf_n; i++) mix[i - len] += mix[i];   /* la cola del final suena al volver a empezar */
    for (int i = 0; i < len; i++) { float v = mix[i] / (1 + fabsf(mix[i]) * 0.25f); mix[i] = v; if (fabs(v) > peak) peak = fabs(v); }
    if (peak < 1e-6) peak = 1;
    for (int i = 0; i < len; i++) pcm[i] = (short)(mix[i] / peak * 24000);
    free(mix); free(wet); mix = wet = NULL;
    pcm_n = len;
    SDL_AtomicSet(&ready, 1);
    return 0;
}
void menumusic_start(void) {
    SDL_Thread *t = SDL_CreateThread(build, "menumusic", NULL);
    if (t) SDL_DetachThread(t); else build(NULL);
}

/* ---------- reproduccion ---------- */
static long pos;          /* muestra de salida (a 84000 Hz) dentro de la pieza */
static int playing;
static float fade;        /* entrada suave */
void menumusic_update(SDL_AudioDeviceID dev, int active, float gain) {
    static short out[1024 * 2];
    if (!dev) return;
    if (!active || gain <= 0) {
        if (playing) { SDL_ClearQueuedAudio(dev); playing = 0; }
        return;
    }
    if (!SDL_AtomicGet(&ready) || !pcm) return;
    if (!playing) { SDL_ClearQueuedAudio(dev); playing = 1; fade = 0; }
    if (SDL_GetAudioDeviceStatus(dev) != SDL_AUDIO_PLAYING) SDL_PauseAudioDevice(dev, 0);
    while (SDL_GetQueuedAudioSize(dev) < (Uint32)(SR * UP / 8 * 4)) {     /* ~1/8 s en cola */
        for (int k = 0; k < 1024; k++) {
            long s = pos / UP; int fr = (int)(pos % UP);
            int a = pcm[s], b = pcm[s + 1 < pcm_n ? s + 1 : 0];
            float v = (a + (b - a) * fr / (float)UP) * gain * fade;
            if (fade < 1) { fade += 1.0f / (SR * UP / 2); if (fade > 1) fade = 1; }
            out[2 * k] = out[2 * k + 1] = (short)v;
            if (++pos >= (long)pcm_n * UP) pos = 0;
        }
        SDL_QueueAudio(dev, out, sizeof out);
    }
}
