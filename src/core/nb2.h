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
unsigned char *nb2_eeprom(void);
void nb2_set_wide(int on);                              /* 1 = 16:9 (ve mas escenario a los lados) */
int nb2_width(void);
unsigned char *nb2_mem(void);

/* Trucos (para pruebas). stage: -1 normal, o el rival 0-7 cuyo escenario se quiere en el siguiente combate:
   0 John Smith (rascacielos), 1 Eve (circo), 2 Betty Doe (acuario), 3 Bernard White (tren),
   4 Prof. Ching (fabrica), 5 Dweeb (avion), 6 Danny y Demi (barco), 7 Mr. Acme (mansion). */
#define NB2_CHEAT_INVULN 1
#define NB2_CHEAT_AMMO   2
#define NB2_CHEAT_ONEHIT 4
void nb2_set_cheats(int flags, int stage);

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
const short *nb2_audio(int *frames);
#endif
