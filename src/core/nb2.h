/* Interfaz del nucleo Namco NB-2 (The Outfoxies). */
#ifndef NB2_INTERFACE_H
#define NB2_INTERFACE_H
#define NB2_W 288
#define NB2_H 224
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

/* Sonido: estereo 16 bits a NB2_AUDIO_RATE. nb2_audio da lo generado en el ultimo fotograma. */
#define NB2_AUDIO_RATE 84000
#define NB2_AUDIO_MAX 2048
int nb2_has_sound(void);
const short *nb2_audio(int *frames);
#endif
