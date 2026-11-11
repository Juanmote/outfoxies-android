/* Juego en red, fase 1 (wifi local). Mismo modelo que Dias de Furia, de Mckeijas ("retransmision desde el anfitrion"):
   solo el anfitrion ejecuta el juego; manda la imagen y el sonido al invitado y recibe sus botones, que entran como
   jugador 2. No puede desincronizarse. Ver net.c. */
#ifndef OFX_NET_H
#define OFX_NET_H
enum { NET_OFF, NET_HOST, NET_SEARCH, NET_GUEST };
int net_mode(void);
int net_connected(void);                 /* anfitrion: hay invitado; invitado: sigue conectado */
void net_stop(void);                     /* cierra todo y vuelve a NET_OFF */
const char *net_status(int english);     /* una linea para el menu */

/* anfitrion */
int net_host_start(void);                /* 1 si ha podido abrir el puerto */
void net_host_frame(const unsigned char *rgb, int w, int h, const short *audio, int audio_frames, int audio_rate);
unsigned char net_host_inputs(void);     /* botones del invitado (bits NB2_*, START incluido) */
int net_host_event(void);                /* 1 = acaba de entrar el invitado, 2 = se ha ido, 0 = nada (se consume) */

/* invitado */
int net_search_start(void);              /* empieza a buscar partidas en la wifi */
int net_found(char *ip, int n);          /* 1 y la IP de la primera partida encontrada */
int net_join(const char *ip);            /* 1 si ha entrado */
void net_guest_inputs(unsigned char bits);
int net_guest_frame(unsigned char *rgb, int *w, int *h);   /* 1 si hay imagen nueva (RGB24, hasta 384x224) */
int net_guest_audio(short *out, int max_frames);          /* estereo a la frecuencia del nucleo; devuelve fotogramas */
int net_guest_ping(void);                /* ms de ida y vuelta, -1 si aun no se sabe */
#endif
