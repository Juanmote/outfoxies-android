/* Logros (los 20 de RetroAchievements para The Outfoxies, juego 19839), guardados en el movil, sin conexion.
   Se miran al final de cada fotograma con la RAM del juego:
   - energia 1P 0x208D59 y 2P 0x208E0D (0xF0 llena, 0 = fuera de combate o KO);
   - 0x20807B: nibble alto = rival actual, bajo = personaje del 1P (0 John Smith, 1 Eve, 2 Betty Doe,
     3 Bernard White, 4 Prof. Ching, 5 Dweeb, 6 Danny y Demi, 7 Mr. Acme; el rival marca el escenario);
   - 0x20807C: 7 parejas (estado, rival); estado 0x00 = ya vencido.
   - 0x208092 (palabra): tiempo que queda, en fotogramas (12000 = 200,0 s al empezar).
   Un asalto empieza cuando las dos energias pasan de 0 a llenas; lo gana el 1P si la del rival llega a 0 con la
   suya por encima; acaba por tiempo cuando el reloj llega a 0 (ese fotograma el juego pone a 0 al 1P).
   - Arma del 1P: 0x208D65 (0 ninguna; 2 granada, 3 pistola, 4 lanzacohetes, 5 katana, 6 metralleta, 8 latigo,
     9 lanzallamas; 14 carbon, 18 tarta, 19 sopa, 20 cesta de fruta; 13 un objeto de la fabrica).
   Como en RA ("1P Normal+"), no cuentan en dificultad Facil. De momento SI cuentan con trucos (para probarlos).
   Los de sitios concretos del escenario estan en estudio (ACH_TODO): se ensenan, pero aun no se pueden conseguir. */
#include <stdio.h>
#include <string.h>
#include "achievements.h"
#include "menu.h"

#define ACH_TODO 0x80
static const struct { const char *title, *es, *en; int pts, flags; } list[ACH_COUNT] = {
    {"Action Man", "Termina el juego con John Smith", "Clear as John Smith", 10, 0},
    {"High Class", "Termina el juego con Betty Doe", "Clear as Betty Doe", 10, 0},
    {"Family Man", "Termina el juego con Bernard White", "Clear as Bernard White", 10, 0},
    {"Iguana Be Famous", "Termina el juego con Eve", "Clear as Eve", 10, 0},
    {"Calculated", "Termina el juego con el Prof. Ching", "Clear as Professor Ching", 10, 0},
    {"He's Got A Gun!", "Termina el juego con Dweeb", "Clear as Dweeb", 10, 0},
    {"Come Play With Us", "Termina el juego con Danny y Demi", "Clear as Danny & Demi", 10, 0},
    {"Eat Lead!", "Mata a un asesino solo con pistola y metralleta", "Kill an assassin using only Gun and Machine Gun", 5, 0},
    {"Demolitions Expert", "Mata a un asesino solo con granadas y lanzacohetes", "Kill an assassin using only Grenade and Rocket Launcher", 10, 0},
    {"Hobbyist Vampire Hunter", "Con el látigo, daña 10 veces al rival en un asalto", "While holding Whip, damage the enemy 10 times in a round", 5, 0},
    {"By ANY Means", "Mata a un asesino sin armas (solo comida y carbón)", "Kill an assassin without weapons, except food and coal", 10, 0},
    {"Helicopter Insurance", "Mata a John Smith antes de caer al comedor", "Kill John Smith before falling into the dining room", 10, ACH_TODO},
    {"Going Down", "Agota el tiempo en el foso de granadas del rascacielos", "Time-out in the grenade pit of the Skyscraper", 10, ACH_TODO},
    {"Saving The Whale", "Mata a Betty Doe en 90 segundos", "Kill Betty Doe in 90 seconds", 5, 0},
    {"Rising Sea Levels", "Agota el tiempo en el acuario", "Time-out in the Aquarium", 10, 0},
    {"Fly, My Pretty", "Tira a Dweeb del avión", "Throw Dweeb off the Plane", 2, ACH_TODO},
    {"It's Even Funnier The Third Time!", "Dispara 3 pistolas de juguete en un asalto en el circo", "Fire 3 toy guns in one round in the Circus", 5, ACH_TODO},
    {"Officially Outfoxed", "Mata a un asesino sin recibir daño", "Kill an assassin without taking damage", 10, 0},
    {"Acme Outfoxed", "Mata a Mr. Acme sin recibir daño", "Kill Mr. Acme without taking damage", 25, 0},
    {"The Outfoxiest", "Termina el juego en Difícil", "Clear on Hard", 25, 0},
};
/* logro de "terminar con" cada personaje, por numero de personaje del juego */
static const int clear_ach[8] = {0, 3, 1, 2, 4, 5, 6, -1};

static unsigned char got[ACH_COUNT];
static char path[600];
static int in_round, hurt, prev_e1, prev_e2, prev_t, cleared_game;
static unsigned held;      /* armas principales que ha llevado el 1P en el asalto (bit = numero de arma) */
static int whip_hits;      /* veces que el rival pierde energia mientras el 1P lleva el latigo */
#define W(n) (1u << (n))
/* comida y carbon no cuentan como armas principales (RA) */
#define NOT_PRIMARY (W(0) | W(1) | W(14) | W(18) | W(19) | W(20))

void ach_init(const char *dir) {
    char line[128];
    FILE *f;
    snprintf(path, sizeof path, "%s/logros.txt", dir);
    memset(got, 0, sizeof got);
    if (!(f = fopen(path, "r"))) return;
    while (fgets(line, sizeof line, f)) {
        int i;
        if (sscanf(line, "%d", &i) == 1 && i >= 0 && i < ACH_COUNT) got[i] = 1;
    }
    fclose(f);
}
static void save(void) {
    FILE *f = fopen(path, "w");
    if (!f) return;
    for (int i = 0; i < ACH_COUNT; i++) if (got[i]) fprintf(f, "%d %s\n", i, list[i].title);
    fclose(f);
}
static int queue[ACH_COUNT], nq;
static int unlock(int i) {
    if (i < 0 || got[i] || (list[i].flags & ACH_TODO)) return -1;
    got[i] = 1; save();
    if (nq < ACH_COUNT) queue[nq++] = i;
    return i;
}
int ach_pop(void) {
    int r;
    if (!nq) return -1;
    r = queue[0]; memmove(queue, queue + 1, --nq * sizeof queue[0]);
    return r;
}
void ach_reset(void) { in_round = 0; prev_e1 = prev_e2 = -1; }

void ach_frame(const unsigned char *m, int allowed) {
    int e1 = m[0x208d59], e2 = m[0x208e0d], rival = m[0x20807b] >> 4, me = m[0x20807b] & 15, t = m[0x208092] << 8 | m[0x208093];
    if (prev_e1 < 0) { prev_e1 = e1; prev_e2 = e2; return; }
    if (!in_round && e1 == 0xf0 && e2 == 0xf0 && (prev_e1 == 0 || prev_e2 == 0)) {
        in_round = 1; hurt = 0; cleared_game = 0; held = 0; whip_hits = 0;
    }
    if (in_round) {
        int wpn = m[0x208d65];
        if (e1 < prev_e1) hurt = 1;
        if (wpn < 32 && !(NOT_PRIMARY & W(wpn))) held |= W(wpn);
        if (wpn == 8 && e2 < prev_e2 && e2 > 0 && allowed && ++whip_hits == 10) unlock(9);
        if (t == 0 && prev_t > 0) {                         /* se acaba el tiempo (el juego deja al 1P a 0 a la vez) */
            in_round = 0;
            if (allowed && rival == 2) unlock(14);
        } else if (e2 == 0 && prev_e2 > 0 && e1 > 0) {     /* el 1P gana el asalto */
            in_round = 0;
            if (allowed) {
                if (!hurt) unlock(17);
                if (held && !(held & ~(W(3) | W(6)))) unlock(7);     /* Eat Lead!: solo pistola y metralleta */
                if (held && !(held & ~(W(2) | W(4)))) unlock(8);     /* Demolitions Expert: granada y lanzacohetes */
                if (wpn == 8 && whip_hits >= 9) unlock(9);           /* el golpe final tambien cuenta */
                if (!held) unlock(10);                               /* By ANY Means */
                if (!hurt && rival == 7) unlock(18);
                if (rival == 2 && t >= 12000 - 90 * 60) unlock(13);
                if (rival == 7) {
                    int all = 1;
                    for (int k = 0; k < 7; k++) if (m[0x20807c + k * 2] != 0x00 && m[0x20807d + k * 2] != 7) all = 0;
                    if (all && !cleared_game) {
                        cleared_game = 1;
                        if (me < 8 && clear_ach[me] >= 0) unlock(clear_ach[me]);
                        if (cfg.difficulty == DIF_HARD) unlock(19);
                    }
                }
            }
        } else if (e1 == 0 || e2 == 0) in_round = 0;
    }
    prev_e1 = e1; prev_e2 = e2; prev_t = t;
}
int ach_got(int i) { return got[i]; }
int ach_todo(int i) { return (list[i].flags & ACH_TODO) != 0; }
int ach_points(int i) { return list[i].pts; }
const char *ach_title(int i) { return list[i].title; }
const char *ach_desc(int i) { return cfg.lang == LANG_EN ? list[i].en : list[i].es; }
int ach_total(void) { int n = 0; for (int i = 0; i < ACH_COUNT; i++) n += got[i]; return n; }
