/* Juego en red por internet: puerto del router (UPnP), IP publica y codigo de partida. Ver inet.c. */
#ifndef OFX_INET_H
#define OFX_INET_H
enum { INET_IDLE, INET_WORKING, INET_OK, INET_MANUAL, INET_CGNAT, INET_FAIL };
void inet_host_begin(void);              /* en segundo plano: UPnP + IP publica (lo llama net_host_start) */
void inet_host_end(void);                /* quita el puerto del router (lo llama net_stop) */
int inet_host_state(char *code, int n);  /* INET_*; code = "ABCD-EFGH" si ya se sabe */
void inet_code_encode(unsigned ip, int k, char *out);              /* 8 letras + 0 */
int inet_code_decode(const char *s, unsigned *ip, int *port);       /* 1 si el codigo es valido */
int inet_code_char(int c);               /* posicion de una letra en el alfabeto del codigo, o -1 */
char inet_code_letter(int i);            /* letra numero i (0-31) */
#endif
