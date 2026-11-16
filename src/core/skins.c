/* Packs de ropa a medida. Ver skins.h. Las prendas son las de ESTADO.md 0.18.0 (vistas pintando cada bloque de la
   paleta con OFX_ROW / OFX_IDX). pal_rom: donde esta la paleta del personaje en el programa (256 colores R,G,B; hallado
   comparando la paleta del banco 1 en combate con el programa): la usa el editor para dibujar al personaje. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "skins.h"

const skin_char skin_chars[SKIN_CHARS] = {
    {"john", "JOHN SMITH", 0xE0F00, 7, {
        {"chaqueta", "Chaqueta", "Jacket", {{0x30, 0x37}, {0x80, 0x87}}},
        {"camiseta", "Camiseta", "T-shirt", {{0x50, 0x53}}},
        {"dibujo", "Dibujo de la camiseta", "T-shirt print", {{0x90, 0x9F}}},
        {"vaqueros", "Vaqueros", "Jeans", {{0x10, 0x17}, {0x70, 0x77}}},
        {"zapatillas", "Zapatillas", "Sneakers", {{0x60, 0x63}}},
        {"pelo", "Pelo", "Hair", {{0x20, 0x25}}},
        {"piel", "Piel", "Skin", {{0x40, 0x47}}}}},
    {"eve", "EVE", 0xE1200, 5, {
        {"top", "Top", "Top", {{0x50, 0x57}}},
        {"pantalon", "Pantalón corto y botas", "Shorts and boots", {{0x20, 0x27}}},
        {"medias", "Medias", "Tights", {{0x40, 0x47}}},
        {"pelo", "Pelo", "Hair", {{0x10, 0x17}}},
        {"piel", "Piel", "Skin", {{0x30, 0x37}}}}},
    {"betty", "BETTY DOE", 0xE3900, 5, {
        {"traje", "Traje", "Suit", {{0x02, 0x08}, {0x0C, 0x0E}}},
        {"botones", "Botones", "Buttons", {{0x09, 0x0B}}},
        {"zapatos", "Zapatos", "Shoes", {{0x10, 0x17}}},
        {"pelo", "Pelo", "Hair", {{0x30, 0x37}}},
        {"piel", "Piel", "Skin", {{0x20, 0x27}}}}},
    {"bernard", "BERNARD WHITE", 0xE3600, 5, {
        {"traje", "Traje", "Suit", {{0x10, 0x17}}},
        {"camisa", "Camisa y corbata", "Shirt and tie", {{0x28, 0x2C}}},
        {"guante", "Guante", "Glove", {{0x48, 0x4F}}},
        {"zapatos", "Zapatos", "Shoes", {{0x50, 0x53}, {0x1F, 0x1F}}},
        {"piel", "Piel", "Skin", {{0x30, 0x37}}}}},
    {"ching", "PROF. CHING", 0xE3C00, 10, {
        {"bata", "Bata", "Robe", {{0x30, 0x37}}},
        {"pantalon", "Pantalón", "Trousers", {{0x10, 0x17}}},
        {"zapatos", "Zapatos", "Shoes", {{0x18, 0x1F}}},
        {"piel", "Piel", "Skin", {{0x40, 0x47}}},
        {"silla", "Carrocería de la silla", "Chair body", {{0x48, 0x4F}}},
        {"brazos", "Brazos mecánicos", "Mechanical arms", {{0x38, 0x3F}}},
        {"ruedas", "Ruedas y juntas", "Wheels and joints", {{0x20, 0x27}}},
        {"neumaticos", "Neumáticos", "Tyres", {{0x60, 0x67}}},
        {"pilotos", "Pilotos", "Lights", {{0x68, 0x6F}}},
        {"ribete", "Ribete", "Trim", {{0x80, 0x87}}}}},
    {"dweeb", "DWEEB", 0xE3F00, 5, {
        {"traje", "Traje y sombrero", "Suit and hat", {{0x10, 0x15}}},
        {"camisa", "Camisa", "Shirt", {{0x20, 0x26}}},
        {"corbata", "Corbata", "Tie", {{0x02, 0x04}}},
        {"pelo", "Pelo", "Fur", {{0x40, 0x44}}},
        {"piel", "Piel", "Skin", {{0x30, 0x37}}}}},
    {"gemelos", "DANNY Y DEMI", 0xE4200, 9, {
        {"abrigo_demi", "Abrigo de Demi", "Demi's coat", {{0x30, 0x37}}},
        {"abrigo_danny", "Abrigo de Danny", "Danny's coat", {{0x38, 0x3F}}},
        {"botas_demi", "Botas de Demi", "Demi's boots", {{0x40, 0x44}}},
        {"zapatos_danny", "Zapatos de Danny", "Danny's shoes", {{0x48, 0x4B}}},
        {"calcetines_danny", "Calcetines de Danny", "Danny's socks", {{0x58, 0x5B}}},
        {"pelo_demi", "Pelo de Demi", "Demi's hair", {{0x20, 0x27}}},
        {"pelo_danny", "Pelo de Danny", "Danny's hair", {{0x28, 0x2F}}},
        {"piel_demi", "Piel de Demi", "Demi's skin", {{0x10, 0x17}}},
        {"piel_danny", "Piel de Danny", "Danny's skin", {{0x18, 0x1F}}}}},
};

void skin_pack_clear(skin_pack *p) { unsigned v = p->version; memset(p, 0, sizeof *p); p->version = v + 1; }

int skin_pack_load(skin_pack *p, const char *path) {
    char line[256];
    FILE *f = fopen(path, "r");
    skin_pack_clear(p);
    if (!f) return 0;
    {   /* nombre: el del archivo sin carpeta ni .txt */
        const char *b = strrchr(path, '/'), *dot;
        b = b ? b + 1 : path; dot = strrchr(b, '.');
        snprintf(p->name, sizeof p->name, "%.*s", (int)(dot && dot > b ? dot - b : (long)strlen(b)), b);
    }
    while (fgets(line, sizeof line, f)) {
        char *eq = strchr(line, '='), *dot = strchr(line, '.');
        unsigned col;
        if (line[0] == '#' || !eq || !dot || dot > eq) continue;
        *eq = 0; *dot = 0;
        if (sscanf(eq + 1, " %x", &col) != 1) continue;
        for (int c = 0; c < SKIN_CHARS; c++) if (!strcmp(line, skin_chars[c].slug))
            for (int k = 0; k < skin_chars[c].nparts; k++) if (!strcmp(dot + 1, skin_chars[c].parts[k].slug)) {
                p->set[c][k] = 1; p->color[c][k] = col & 0xffffff;
            }
    }
    fclose(f);
    p->version++;
    return 1;
}

int skin_pack_save(const skin_pack *p, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return 0;
    fprintf(f, "# The Outfoxies Remastered: pack de ropa \"%s\"\n# personaje.prenda=RRGGBB (lo que no sale, como la recreativa)\n", p->name);
    for (int c = 0; c < SKIN_CHARS; c++)
        for (int k = 0; k < skin_chars[c].nparts; k++)
            if (p->set[c][k]) fprintf(f, "%s.%s=%06X\n", skin_chars[c].slug, skin_chars[c].parts[k].slug, p->color[c][k] & 0xffffff);
    return fclose(f) == 0;
}

int skin_part_of(int ch, int idx) {
    if (ch < 0 || ch >= SKIN_CHARS) return -1;
    for (int k = 0; k < skin_chars[ch].nparts; k++)
        for (int t = 0; t < 3; t++) {
            const unsigned char *r = skin_chars[ch].parts[k].rng[t];
            if ((r[0] || r[1]) && idx >= r[0] && idx <= r[1]) return k;
        }
    return -1;
}
static int lum(const unsigned char *c) { return (c[0] * 77 + c[1] * 151 + c[2] * 28) >> 8; }
/* luz media de la prenda (sin los casi negros, que son sombras o contorno) */
static int part_lum(int ch, int k, const unsigned char *pal, int *best) {
    long s = 0; int n = 0;
    for (int i = 2; i < 256; i++) if (skin_part_of(ch, i) == k) { int l = lum(pal + i * 3); if (l > 12) { s += l; n++; } }
    if (!n) return 0;
    s /= n;
    if (best) {   /* la entrada con la luz mas cercana a la media: su color es el "tipico" */
        int bd = 9999; *best = -1;
        for (int i = 2; i < 256; i++) if (skin_part_of(ch, i) == k) { int d = abs(lum(pal + i * 3) - (int)s); if (d < bd) { bd = d; *best = i; } }
    }
    return (int)s;
}
unsigned skin_part_color(int ch, int k, const unsigned char *pal) {
    int b = -1;
    part_lum(ch, k, pal, &b);
    if (b < 0) return 0x808080;
    return (unsigned)pal[b * 3] << 16 | pal[b * 3 + 1] << 8 | pal[b * 3 + 2];
}
void skin_apply(const skin_pack *p, int ch, const unsigned char *pal, unsigned char *out, unsigned char *changed) {
    int ref[SKIN_MAXPARTS];
    memcpy(out, pal, 256 * 3);
    memset(changed, 0, 256);
    if (!p || ch < 0 || ch >= SKIN_CHARS) return;
    for (int k = 0; k < skin_chars[ch].nparts; k++) ref[k] = p->set[ch][k] ? part_lum(ch, k, pal, NULL) : 0;
    for (int i = 2; i < 256; i++) {
        int k = skin_part_of(ch, i);
        if (k < 0 || !p->set[ch][k] || ref[k] <= 0) continue;
        {
            /* luz que toca: la del color elegido por lo que la entrada es mas clara u oscura que la media de la prenda
               (asi un traje blanco pasado a negro queda negro, con sus brillos en gris oscuro). Se pinta con el tono del
               color elegido a esa luz; si no cabe (colores claros), se aclara hacia el blanco */
            unsigned c = p->color[ch][k];
            double T[3] = {(double)((c >> 16) & 255), (double)((c >> 8) & 255), (double)(c & 255)}, o[3];
            double tl = (T[0] * 77 + T[1] * 151 + T[2] * 28) / 256.0, l = lum(pal + i * 3), r = ref[k], ld, g, mx;
            if (tl < 1) { T[0] = T[1] = T[2] = 1; tl = 1; }
            ld = tl * l / r;
            g = ld / tl;
            for (int j = 0; j < 3; j++) o[j] = T[j] * g;
            mx = o[0] > o[1] ? (o[0] > o[2] ? o[0] : o[2]) : (o[1] > o[2] ? o[1] : o[2]);
            if (mx > 255) {
                double ol, a;
                for (int j = 0; j < 3; j++) o[j] *= 255 / mx;
                ol = (o[0] * 77 + o[1] * 151 + o[2] * 28) / 256.0;
                a = ol < 255 ? (ld - ol) / (255 - ol) : 0; if (a < 0) a = 0; if (a > 1) a = 1;
                for (int j = 0; j < 3; j++) o[j] += (255 - o[j]) * a;
            }
            for (int j = 0; j < 3; j++) out[i * 3 + j] = (unsigned char)(o[j] < 0 ? 0 : o[j] > 255 ? 255 : o[j] + 0.5);
            changed[i] = 1;
        }
    }
}
