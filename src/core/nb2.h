/* Interfaz del nucleo Namco NB-2 (The Outfoxies). */
#ifndef NB2_INTERFACE_H
#define NB2_INTERFACE_H
#define NB2_W 288
#define NB2_H 224
#define NB2_WIDE_W 384     /* 16:9 con el mismo pixel que el 4:3 */
#define NB2_EEPROM_SIZE 0x800

/* Bits de mando: valen 1 mientras se pulsa */
#define NB2_RIGHT 0x01
#define NB2_LEFT  0x02
#define NB2_DOWN  0x04
#define NB2_UP    0x08
#define NB2_B1    0x10
#define NB2_B2    0x20
#define NB2_B3    0x40
#define NB2_START 0x80

int nb2_load(const char *zip, char *err, int errlen);   /* 1 = correcto */
void nb2_reset(void);
void nb2_set_inputs(unsigned char p1, unsigned char p2, unsigned char misc);
void nb2_coin(void);
const unsigned char *nb2_frame(void);                   /* un fotograma en RGB24 */
const unsigned char *nb2_redraw(void);                  /* vuelve a dibujar el ultimo (tras cambiar ropa, formato...) */
unsigned char *nb2_eeprom(void);
/* 8:7, escena de las teles: filas de mas por arriba y por abajo (el decorado prolongado) para llenar la pantalla sin
   bandas. nb2_vborder() da NB2_VEXT filas de arriba y NB2_VEXT de abajo (288 x RGB), o NULL si el fotograma no es de esa escena. */
#define NB2_VEXT 19
const unsigned char *nb2_vborder(void);
void nb2_set_hud87(int on);                              /* 1 = formato 8:7: el marcador se recoloca */
/* Ropa: player 0 = jugador 1 (banco de sprites 1), 1 = rival (banco 2). Solo en combate. */
enum { NB2_OUTFIT_ORIGINAL, NB2_OUTFIT_GOTHIC, NB2_OUTFIT_NEGATIVE, NB2_OUTFIT_PINKY, NB2_OUTFIT_VAMPIRE, NB2_OUTFIT_COUNT };
void nb2_set_outfit(int player, int style);
void nb2_set_game_lang(int es);                          /* 1 = textos del juego en castellano */
void nb2_set_wide(int on);                              /* 1 = 16:9 (ve mas escenario a los lados) */
int nb2_width(void);
unsigned char *nb2_mem(void);

/* Trucos (para pruebas). stage: -1 normal, o el rival 0-7 cuyo escenario se quiere en el siguiente combate:
   0 John Smith (rascacielos), 1 Eve (circo), 2 Betty Doe (acuario), 3 Bernard White (tren),
   4 Prof. Ching (fabrica), 5 Dweeb (avion), 6 Danny y Demi (barco), 7 Mr. Acme (mansion). */
#define NB2_CHEAT_INVULN 1
#define NB2_CHEAT_AMMO   2
#define NB2_CHEAT_ONEHIT 4
#define NB2_CHEAT_NOIFR_HUMAN 8   /* sin los segundos de invencibilidad tras recibir dano: jugadores */
#define NB2_CHEAT_NOIFR_CPU 16    /* ... y rivales de la maquina */
void nb2_set_cheats(int flags, int stage);
/* Packs de armas: tabla de 0x15 bytes (tipo de arma del juego -> tipo que aparece) o NULL = armas originales */
void nb2_set_weapon_map(const unsigned char *map);
/* Packs de armas: cada arma que aparece se cambia por una del pack (al crearse; las que ya estan no cambian).
   La comida y el carbon se quedan en todos. */
enum { NB2_WEAPONS_ALL, NB2_WEAPONS_FIRE, NB2_WEAPONS_MELEE, NB2_WEAPONS_EXPLOSIVE, NB2_WEAPONS_NONE, NB2_WEAPONS_COUNT };
void nb2_set_weapon_pack(int pack);
/* Dificultad: escala el dano de cada golpe. Facil: el humano recibe la mitad y hace el doble.
   Dificil: recibe el doble y hace la mitad. Normal: como el original. */
enum { NB2_DIFF_EASY, NB2_DIFF_NORMAL, NB2_DIFF_HARD };
void nb2_set_difficulty(int d);

/* Estado completo comprimido (partidas guardadas y rebobinado). level: 1 rapido ... 9 pequeno.
   Devuelve un bloque de malloc (liberar con free) o NULL. Cargar devuelve 0 si el bloque no vale. */
#include <stddef.h>
unsigned char *nb2_state_save(size_t *len, int level);
int nb2_state_load(const unsigned char *buf, size_t len);
void nb2_set_render(int on);    /* 0: no dibuja la imagen (avance rapido); el juego avanza igual */

/* Sucesos del ultimo fotograma (para la vibracion) */
#define NB2_EV_EXPLOSION 1
unsigned nb2_events(void);

/* Sonido: estereo 16 bits a NB2_AUDIO_RATE. nb2_audio da lo generado en el ultimo fotograma. */
#define NB2_AUDIO_RATE 84000
#define NB2_AUDIO_MAX 2048
int nb2_has_sound(void);
/* Volumen de musica y efectos (0-100). Voces 0-15 del C352 = musica (orden 0 del juego), 16-31 = efectos. */
void nb2_set_volumes(int music, int sfx);
const short *nb2_audio(int *frames);
#endif
