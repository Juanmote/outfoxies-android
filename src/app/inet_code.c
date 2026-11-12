/* Codigo de partida por internet (ver inet.c): sin SDL ni red, para que tambien lo use headless (menu.c). */
#include <string.h>
#include <ctype.h>
#include "inet.h"
#define BASE_PORT 47800
static const char ALPHA[] = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
#define CODE_XOR 0xA5C3E1F7u

/* ---------- codigo ---------- */
static unsigned check4(unsigned ip, int k) {
    unsigned s = 7;
    for (int i = 0; i < 8; i++) s = s * 5 + ((ip >> (i * 4)) & 15);
    return (s + k * 3) & 15;
}
void inet_code_encode(unsigned ip, int k, char *out) {
    unsigned long long v = ((unsigned long long)(ip ^ CODE_XOR) << 8) | (unsigned)(k & 15) << 4 | check4(ip, k);
    for (int i = 0; i < 8; i++) out[i] = ALPHA[(v >> (35 - i * 5)) & 31];
    out[8] = 0;
}
int inet_code_char(int c) {           /* posicion en el alfabeto, o -1 */
    const char *p;
    c = toupper(c);
    if (c == '0') c = 'O'; if (c == '1') c = 'I';
    if (c == 'O') c = 'Q';            /* por si acaso: nunca se dan, se leen como la letra mas parecida */
    if (c == 'I') c = 'J';
    p = c ? strchr(ALPHA, c) : NULL;
    return p ? (int)(p - ALPHA) : -1;
}
char inet_code_letter(int i) { return ALPHA[i & 31]; }
int inet_code_decode(const char *s, unsigned *ip, int *port) {
    unsigned long long v = 0; int n = 0;
    for (; *s && n < 8; s++) {
        int d;
        if (*s == '-' || *s == ' ') continue;
        if ((d = inet_code_char((unsigned char)*s)) < 0) return 0;
        v = v << 5 | (unsigned)d; n++;
    }
    if (n != 8) return 0;
    {
        unsigned x = (unsigned)(v >> 8) ^ CODE_XOR; int k = (int)(v >> 4) & 15;
        if ((v & 15) != check4(x, k)) return 0;
        *ip = x; *port = BASE_PORT + k;
    }
    return 1;
}

