/* tr(): texto del menu en el idioma elegido. Castellano e ingles van en el codigo (TR(castellano, ingles)); el portugues y el
   italiano salen de lang_ui.h (tools/make_lang_ui.py), buscando por el texto en castellano. Como los textos son
   literales, cada puntero se busca una vez y se guarda en una tabla pequena. Si falta la traduccion, el ingles. */
#include <string.h>
#include <stdint.h>
#include "menu.h"
#include "lang_ui.h"

#define NUI (sizeof lang_ui / sizeof lang_ui[0])
#define CACHE 1024
static struct { const char *es; const char *t[2]; } cache[CACHE];

static int find(const char *es) {
    int lo = 0, hi = (int)NUI - 1;
    while (lo <= hi) {
        int m = (lo + hi) / 2, c = strcmp(es, lang_ui[m][0]);
        if (!c) return m;
        if (c < 0) hi = m - 1; else lo = m + 1;
    }
    return -1;
}
const char *tr(const char *es, const char *en) {
    int l = cfg.lang;
    if (l == LANG_ES || !es) return es;
    if (l != LANG_PT && l != LANG_IT) return en;
    {
        unsigned h = (unsigned)(((uintptr_t)es >> 3) * 2654435761u) % CACHE, n = 0;
        while (cache[h].es && cache[h].es != es && n++ < 8) h = (h + 1) % CACHE;
        if (cache[h].es != es) {
            int i = find(es);
            if (cache[h].es) return i >= 0 ? lang_ui[i][l == LANG_PT ? 1 : 2] : en;   /* tabla llena: sin guardar */
            cache[h].es = es;
            cache[h].t[0] = i >= 0 ? lang_ui[i][1] : en;
            cache[h].t[1] = i >= 0 ? lang_ui[i][2] : en;
        }
        return cache[h].t[l == LANG_PT ? 0 : 1];
    }
}
