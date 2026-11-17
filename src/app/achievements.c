/* Logros (los 20 de RetroAchievements para The Outfoxies, juego 19839, algunos cambiados, y uno propio: "La siguiente
   en Normal"), guardados en el movil, sin conexion.
   Se miran al final de cada fotograma con la RAM del juego:
   - energia 1P 0x208D59 y 2P 0x208E0D (0xF0 llena, 0 = fuera de combate o KO);
   - 0x20807B: nibble alto = rival actual, bajo = personaje del 1P (0 John Smith, 1 Eve, 2 Betty Doe,
     3 Bernard White, 4 Prof. Ching, 5 Dweeb, 6 Danny y Demi, 7 Mr. Acme; el rival marca el escenario);
   - 0x20807C: 7 parejas (estado, rival); estado 0xFF = pendiente; al vencerlo pasa a ser el numero del personaje
     que lo ha vencido (por eso con John Smith, que es el 0, parecia que "vencido" era 0x00).
   - 0x208092 (palabra): tiempo que queda, en fotogramas (12000 = 200,0 s al empezar).
   Un asalto empieza cuando las dos energias pasan de 0 a llenas; lo gana el 1P si la del rival llega a 0 con la
   suya por encima; acaba por tiempo cuando el reloj llega a 0 (ese fotograma el juego pone a 0 al 1P).
   - Arma del 1P: 0x208D65 (0 ninguna; 2 granada, 3 pistola, 4 lanzacohetes, 5 katana, 6 metralleta, 8 latigo,
     9 lanzallamas; 14 carbon, 18 tarta, 19 sopa, 20 cesta de fruta; 13 un objeto de la fabrica).
   Como en RA ("1P Normal+"), no cuentan en dificultad Facil. Con trucos (cualquier opcion de TRUCOS) no cuenta ninguno:
   ni el asalto en el que se han usado ni terminar el juego si se han usado en algun momento de la partida (17-11-2026).
   Partidas guardadas y Facil si (Facil solo para "La siguiente en Normal").
   Luchadores: 1P 0x208D06 y 2P 0x208DBA (en una partida de un jugador, el 2P es el rival). Posicion en el mundo:
   x en la palabra +0x14 e y en la +0x18 (y crece hacia arriba). Arma en la mano: puntero en +0x82 (-1 ninguna).
   Los de sitios (11.11.2026, banco de pruebas en el servidor):
   - Rascacielos (rival 0): la azotea esta a y 0x0777-0x0990; al caer el helicoptero todos bajan al comedor
     (y 0x0640-0x06C7). "Antes de caer al comedor" = el 1P sigue por encima de y 0x0740.
   - Foso de granadas: el sotano del rascacielos, a la izquierda del garaje (7 granadas en fila en y 0x67,
     x 0x150-0x1B0, entre escombros y tuberias). "En el foso" = y < 0x0280 y x < 0x01C0.
   - Avion (rival 5): si un luchador se cae, su y baja de 0 (cae al vacio, estado 42) y luego el juego lo devuelve
     con algo menos de energia (estado 38). No es un KO.
   - Pistolas de juguete del circo: el objeto arma lleva en su palabra +0x26 cual es (1-3; rutina 0x198A8). Basta
     con disparar una (antes se pedian las 3 y no saltaba). */
#include <stdio.h>
#include <string.h>
#include "achievements.h"
#include "menu.h"

#define ACH_TODO 0x80
/* titulo en castellano y en ingles (el de RA, salvo los que se han cambiado), descripcion en los dos.
   El indice es el que se guarda en logros.txt: no cambiar el orden. */
static const struct { const char *t_es, *t_en, *es, *en; int flags; } list[ACH_COUNT] = {
    {"Hombre de acción", "Action Man", "Termina el juego con John Smith", "Clear as John Smith", 0},
    {"Alta sociedad", "High Class", "Termina el juego con Betty Doe", "Clear as Betty Doe", 0},
    {"Padre de familia", "Family Man", "Termina el juego con Bernard White", "Clear as Bernard White", 0},
    {"Iguana ser famosa", "Iguana Be Famous", "Termina el juego con Eve", "Clear as Eve", 0},
    {"Todo calculado", "Calculated", "Termina el juego con el Prof. Ching", "Clear as Professor Ching", 0},
    {"¡Tiene una pistola!", "He's Got A Gun!", "Termina el juego con Dweeb", "Clear as Dweeb", 0},
    {"Ven a jugar con nosotros", "Come Play With Us", "Termina el juego con Danny y Demi", "Clear as Danny & Demi", 0},
    {"¡Come plomo!", "Eat Lead!", "Mata a un asesino solo con pistola y metralleta", "Kill an assassin using only Gun and Machine Gun", 0},
    {"Experto en demoliciones", "Demolitions Expert", "Mata a un asesino solo con granadas y lanzacohetes", "Kill an assassin using only Grenade and Rocket Launcher", 0},
    {"Cazavampiros novato", "Hobbyist Vampire Hunter", "Con el látigo, daña 10 veces al rival en un asalto", "While holding Whip, damage the enemy 10 times in a round", 0},
    {"Por TODOS los medios", "By ANY Means", "Mata a un asesino sin armas (solo comida y carbón)", "Kill an assassin without weapons, except food and coal", 0},
    {"Seguro de helicóptero", "Helicopter Insurance", "Mata a John Smith antes de caer al comedor", "Kill John Smith before falling into the dining room", 0},
    {"Planta sótano", "Going Down", "Agota el tiempo en el foso de granadas del rascacielos", "Time-out in the grenade pit of the Skyscraper", 0},
    {"Salvar a la ballena", "Saving The Whale", "Mata a Betty Doe en 90 segundos", "Kill Betty Doe in 90 seconds", 0},
    {"Sube el nivel del mar", "Rising Sea Levels", "Agota el tiempo en el acuario", "Time-out in the Aquarium", 0},
    {"¡Vuela, precioso!", "Fly, My Pretty", "Tira a Dweeb del avión", "Throw Dweeb off the Plane", 0},
    {"¡Pum, de juguete!", "It's Only A Toy!", "Dispara una pistola de juguete en el circo", "Fire a toy gun in the Circus", 0},
    {"Burlado oficialmente", "Officially Outfoxed", "Mata a un asesino sin recibir daño", "Kill an assassin without taking damage", 0},
    {"Acme, burlado", "Acme Outfoxed", "Mata a Mr. Acme sin recibir daño", "Kill Mr. Acme without taking damage", 0},
    {"Casi difícil", "Almost Hard", "Termina el juego con algo más de dificultad que Normal", "Clear the game a little harder than Normal", 0},
    {"La siguiente en Normal", "Next Time On Normal", "Termina el juego en Fácil (o en cualquier otra dificultad)", "Clear the game on Easy (or any other difficulty)", 0},
    /* propios (14-11-2026, del usuario) */
    {"Asalto al tren del moreno", "Great Train Robbery", "Tira a Bernard del tren", "Throw Bernard off the Train", 0},
    {"Toma, te lo devuelvo", "Return To Sender", "Haz que el Prof. Ching se coma una explosión tuya (granada o cohete) en su fábrica", "Make Professor Ching eat one of your explosions (grenade or rocket) in his Factory", 0},
    {"Daros un bañito", "Bath Time", "Haz que los gemelos caigan del barco", "Make the twins fall off the Boat", 0},
};
/* logro de "terminar con" cada personaje, por numero de personaje del juego */
static const int clear_ach[8] = {0, 3, 1, 2, 4, 5, 6, -1};

static unsigned char got[ACH_COUNT];
static char path[600];
static int in_round, hurt, prev_e1, prev_e2, prev_t, cleared_game, resume;
static unsigned held;      /* armas principales que ha llevado el 1P en el asalto (bit = numero de arma) */
static int whip_hits;      /* veces que el rival pierde energia mientras el 1P lleva el latigo */
static int prev_ammo, prev_toy, prev_st2, armed_t;
static int cheat_now, round_cheat, run_cheat;   /* trucos puestos ahora / en este asalto / en esta partida */   /* pistola de juguete que llevaba el 1P en el fotograma anterior (1-3, o 0) */
static int W16(const unsigned char *m, unsigned a) { return m[a] << 8 | m[a + 1]; }
static unsigned L32(const unsigned char *m, unsigned a) { return (unsigned)m[a] << 24 | m[a + 1] << 16 | m[a + 2] << 8 | m[a + 3]; }
#define W(n) (1u << (n))
/* hay una explosion (clase 0x1A) que acaba de dar al luchador f */
static int blast_on(const unsigned char *m, unsigned f) {
    for (unsigned o = 0x208d06; o + 0xb4 <= 0x220000; o += 0xb4)
        if (W16(m, o + 0x12) == 0x1a && L32(m, o + 0x58) == f) return 1;
    return 0;
}
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
    for (int i = 0; i < ACH_COUNT; i++) if (got[i]) fprintf(f, "%d %s\n", i, list[i].t_en);
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
void ach_cheats(int on) { cheat_now = on != 0; }
void ach_reset(int how) {
    prev_e1 = prev_e2 = -1;
    if (how != ACH_REWIND) in_round = 0;   /* al cargar, el asalto de antes ya no vale (puede ser otro) */
    resume = how != ACH_NEW;
}

void ach_frame(const unsigned char *m, int diff) {
    int allowed = diff >= 0;   /* como en RA, mas facil que Normal no cuenta (salvo "La siguiente en Normal") */
    int e1 = m[0x208d59], e2 = m[0x208e0d], rival = m[0x20807b] >> 4, me = m[0x20807b] & 15, t = m[0x208092] << 8 | m[0x208093];
    if (prev_e1 < 0) {
        /* tras cargar (o rebobinar hasta un asalto ya acabado) a mitad de asalto: se sigue contando. Lo que pide el asalto entero (sin recibir dano,
           solo ciertas armas, sin armas) ya no se puede saber y no cuenta; tiempo, sitios y terminar el juego si. */
        if (resume && !in_round && e1 > 0 && e2 > 0 && t > 0) {
            in_round = 1; hurt = 1; cleared_game = 0; held = ~0u; whip_hits = 0; prev_toy = 0; prev_ammo = 0;
        }
        resume = 0; prev_e1 = e1; prev_e2 = e2; prev_t = t; return;
    }
    if (!in_round && e1 == 0xf0 && e2 == 0xf0 && (prev_e1 == 0 || prev_e2 == 0)) {
        int fresh = 1;   /* primer combate de una partida: ningun rival vencido aun -> partida nueva, sin trucos de antes */
        in_round = 1; hurt = 0; cleared_game = 0; held = 0; whip_hits = 0; prev_toy = 0; prev_ammo = 0;
        for (int k = 0; k < 7; k++) if (m[0x20807c + k * 2] != 0xff) fresh = 0;
        round_cheat = 0;
        if (fresh) run_cheat = 0;
    }
    if (cheat_now) round_cheat = run_cheat = 1;
    if (round_cheat) allowed = 0;
    if (in_round) {
        int wpn = m[0x208d65];
        if (e1 < prev_e1) hurt = 1;
        if (wpn < 32 && !(NOT_PRIMARY & W(wpn))) held |= W(wpn);
        if (wpn == 8 && e2 < prev_e2 && e2 > 0 && allowed && ++whip_hits == 10) unlock(9);
        {   /* pistola de juguete (circo): la que llevaba el 1P y le baja la municion (al disparar pasa a 0 y la
               puede soltar en el mismo fotograma: por eso se mira cual llevaba en el anterior) */
            unsigned o = L32(m, 0x208d88); int ammo = m[0x208d67], k = 0;
            if (o >= 0x200000 && o < 0x280000) { k = W16(m, o + 0x26); if (k < 1 || k > 3) k = 0; }
            if (rival == 1 && prev_toy && ammo < prev_ammo && allowed) unlock(16);
            prev_toy = k; prev_ammo = ammo;
        }
        {   /* caidas del rival (estado $4E del luchador 2P, 0x208E08): tren 133 (cae por delante o por detras del tren,
               luego 134 y vuelve en la vagoneta), barco 130 (al agua, luego 131) */
            int st = W16(m, 0x208e08);
            if (allowed && rival == 3 && st == 133 && prev_st2 != 133) unlock(21);
            if (allowed && rival == 6 && st == 130 && prev_st2 != 130) unlock(23);
            prev_st2 = st;
        }
        /* Toma, te lo devuelvo: en la fabrica, al Prof. Ching (rival 4) le quita energia una EXPLOSION (objeto clase 0x1A cuyo
           +0x58 es el luchador al que ha dado: 0x208DBA) mientras el 1P lleva, o ha llevado hace menos de 3 s, una granada
           (2) o el lanzacohetes (4): la ha provocado el jugador. Los objetos van de 0xB4 en 0xB4 desde los luchadores. */
        if (wpn == 2 || wpn == 4) armed_t = 180; else if (armed_t) armed_t--;
        if (allowed && rival == 4 && e2 < prev_e2 && armed_t && blast_on(m, 0x208dba)) unlock(22);
        /* Fly, My Pretty: Dweeb (el rival del avion) cae al vacio */
        if (rival == 5 && e2 > 0 && (short)W16(m, 0x208dd2) < -0x40 && allowed) unlock(15);
        if (t == 0 && prev_t > 0) {                         /* se acaba el tiempo (el juego deja al 1P a 0 a la vez) */
            in_round = 0;
            if (allowed && rival == 2) unlock(14);
            if (allowed && rival == 0 && W16(m, 0x208d1e) < 0x0280 && W16(m, 0x208d1a) < 0x01c0) unlock(12);   /* Going Down */
        } else if (e2 == 0 && prev_e2 > 0 && e1 > 0) {     /* el 1P gana el asalto */
            in_round = 0;
            if (allowed) {
                if (!hurt) unlock(17);
                if (rival == 0 && W16(m, 0x208d1e) >= 0x0740) unlock(11);   /* Helicopter Insurance: aun en la azotea */
                if (held && !(held & ~(W(3) | W(6)))) unlock(7);     /* Eat Lead!: solo pistola y metralleta */
                /* Demolitions Expert: granada y lanzacohetes. En la mansion no: a Mr. Acme solo se le puede dar con el
                   lanzamisiles, y saltaba solo con terminar el juego */
                if (held && !(held & ~(W(2) | W(4))) && rival != 7) unlock(8);
                if (wpn == 8 && whip_hits >= 9) unlock(9);           /* el golpe final tambien cuenta */
                if (!held) unlock(10);                               /* By ANY Means */
                if (!hurt && rival == 7) unlock(18);
                if (rival == 2 && t >= 12000 - 90 * 60) unlock(13);
            }
            if (rival == 7) {   /* terminar el juego: tambien en Facil, para "La siguiente en Normal" */
                int all = 1;
                for (int k = 0; k < 7; k++) if (m[0x20807c + k * 2] == 0xff && m[0x20807d + k * 2] != 7) all = 0;
                if (all && !cleared_game && !run_cheat) {
                    cleared_game = 1;
                    unlock(20);
                    if (allowed && me < 8 && clear_ach[me] >= 0) unlock(clear_ach[me]);
                    if (diff > 0) unlock(19);   /* "Casi dificil": algo mas dificil que Normal */
                }
            }
        } else if (e1 == 0 || e2 == 0) in_round = 0;
    }
    prev_e1 = e1; prev_e2 = e2; prev_t = t;
}
/* Orden en la lista del menu (el numero de cada logro no cambia: es el que va en logros.txt):
   1) acciones (en cualquier escenario), 2) los de cada escenario, en el orden de los escenarios (rascacielos, circo,
   acuario, tren, fabrica, avion, barco, mansion), 3) terminar el juego con cada personaje y, al final, el de Facil
   ("La siguiente en Normal") y el de Dificil ("Casi dificil"). */
static const unsigned char order[ACH_COUNT] = {
    7, 8, 9, 10, 17,
    11, 12, 16, 13, 14, 21, 22, 15, 23, 18,
    0, 1, 2, 3, 4, 5, 6, 20, 19};
int ach_order(int pos) { return pos >= 0 && pos < ACH_COUNT ? order[pos] : 0; }
int ach_got(int i) { return got[i]; }
int ach_todo(int i) { return (list[i].flags & ACH_TODO) != 0; }
const char *ach_title(int i) { return TR(list[i].t_es, list[i].t_en); }
const char *ach_desc(int i) { return TR(list[i].es, list[i].en); }
int ach_total(void) { int n = 0; for (int i = 0; i < ACH_COUNT; i++) n += got[i]; return n; }
