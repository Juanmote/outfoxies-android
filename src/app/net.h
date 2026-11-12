/* Juego en red, fase 1 (wifi local). Mismo modelo que Dias de Furia, de Mckeijas ("retransmision desde el anfitrion"):
   solo el anfitrion ejecuta el juego; manda la imagen y el sonido al invitado y recibe sus botones, que entran como
   jugador 2. No puede desincronizarse. Ver net.c. */
#ifndef OFX_NET_H
#define OFX_NET_H
enum { NET_OFF, NET_HOST, NET_SEARCH, NET_GUEST };
/* formato de pantalla del invitado (se lo dice al anfitrion, que le dibuja la imagen a su medida) */
enum { NET_FMT_43, NET_FMT_169, NET_FMT_87 };
/* banderas de cada fotograma */
#define NET_FL_FIGHT 1                   /* hay combate (el 8:7 recorta 20 columnas por lado) */
int net_mode(void);
int net_connected(void);                 /* anfitrion: hay invitado; invitado: sigue conectado */
void net_stop(void);                     /* cierra todo y vuelve a NET_OFF */
const char *net_status(int english);     /* una linea para el menu */
const char *net_inet_status(int english);   /* anfitrion: el codigo para internet o por que no hay ("" si no toca) */

/* anfitrion */
int net_host_start(void);                /* 1 si ha podido abrir el puerto */
void net_host_frame(const unsigned char *rgb, int w, int h, int flags, const short *audio, int audio_frames, int audio_rate);
int net_host_guest_format(void);         /* NET_FMT_* del invitado */
unsigned char net_host_inputs(void);     /* botones del invitado (bits NB2_*, START incluido) */
void net_host_offer(int sock);           /* servidor intermedio: una conexion ya hecha de un invitado */
int net_host_event(void);                /* 1 = acaba de entrar el invitado, 2 = se ha ido, 0 = nada (se consume) */

/* invitado */
int net_search_start(void);              /* empieza a buscar partidas en la wifi */
int net_found(char *ip, int n);          /* 1 y la IP de la primera partida encontrada */
int net_join(const char *ip);            /* 1 si ha entrado */
int net_join_addr(const char *host, int port);  /* igual, con puerto (internet: el del codigo); IP o nombre, IPv4 o IPv6 */
void net_guest_inputs(unsigned char bits);
void net_guest_format(int fmt);          /* NET_FMT_*: se manda al entrar y cuando cambia */
int net_guest_frame(unsigned char *rgb, int *w, int *h, int *flags);   /* 1 si hay imagen nueva (RGB24, hasta 384x262) */
int net_guest_audio(short *out, int max_frames);          /* estereo a la frecuencia del nucleo; devuelve fotogramas */
int net_guest_ping(void);                /* ms de ida y vuelta, -1 si aun no se sabe */
#endif
