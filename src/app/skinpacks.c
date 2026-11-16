/* Packs de ropa instalados. Ver skinpacks.h. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "skinpacks.h"
#include "menu.h"
#include "packs_incluidos.h"

static skin_pack packs[PACKS_MAX];
static char names[PACKS_MAX][64];
static int npacks;

void packs_dir(char *out, size_t n) {
    snprintf(out, n, "%s/ropas", menu_save_dir());
#ifdef _WIN32
    mkdir(out);
#else
    mkdir(out, 0755);
#endif
}
void packs_path(const char *name, char *out, size_t n) {
    char d[600];
    packs_dir(d, sizeof d);
    snprintf(out, n, "%s/%s.txt", d, name);
}
static int is_txt(const char *s) {
    size_t l = strlen(s);
    return l > 4 && s[0] != '.' && (!strcmp(s + l - 4, ".txt") || !strcmp(s + l - 4, ".TXT"));
}
static int cmp(const void *a, const void *b) { return strcasecmp((const char *)a, (const char *)b); }
/* Los packs que trae la app (packs_incluidos.h) se copian a la carpeta la primera vez (marca .incluidos_1); luego son
   como los demas: se pueden retocar o borrar y no vuelven. */
static void install_builtin(const char *d) {
    char p[800];
    FILE *f;
    snprintf(p, sizeof p, "%s/.incluidos_1", d);
    if ((f = fopen(p, "rb"))) { fclose(f); return; }
    {   /* los packs de prueba de la 0.26.0 (sacados de los estilos para revisarlos): los sustituyen los incluidos */
        static const char *const old[] = {"Pack Original", "Pack Gótica", "Pack Negativa", "Pack Pinky", "Pack Vampiro"};
        for (size_t i = 0; i < sizeof old / sizeof old[0]; i++) { snprintf(p, sizeof p, "%s/%s.txt", d, old[i]); remove(p); }
    }
    for (size_t i = 0; i < sizeof packs_incluidos / sizeof packs_incluidos[0]; i++) {
        snprintf(p, sizeof p, "%s/%s.txt", d, packs_incluidos[i].name);
        if ((f = fopen(p, "rb"))) { fclose(f); continue; }         /* uno del usuario con el mismo nombre se queda */
        if ((f = fopen(p, "wb"))) { fputs(packs_incluidos[i].text, f); fclose(f); }
    }
    snprintf(p, sizeof p, "%s/.incluidos_1", d);
    if ((f = fopen(p, "wb"))) fclose(f);
}
void packs_scan(void) {
    char d[600], p[800];
    char found[PACKS_MAX][64]; int n = 0;
    DIR *dir;
    struct dirent *e;
    packs_dir(d, sizeof d);
    install_builtin(d);
    if ((dir = opendir(d))) {
        while ((e = readdir(dir)) && n < PACKS_MAX)
            if (is_txt(e->d_name) && strlen(e->d_name) < 64) { snprintf(found[n], 64, "%.*s", (int)strlen(e->d_name) - 4, e->d_name); n++; }
        closedir(dir);
    }
    qsort(found, (size_t)n, 64, cmp);
    for (int i = 0; i < n; i++) {
        snprintf(p, sizeof p, "%s/%s.txt", d, found[i]);
        skin_pack_load(&packs[i], p);           /* la version sube: lo calculado con el pack de antes se rehace */
        memcpy(names[i], found[i], 64);
    }
    npacks = n;
}
int packs_count(void) { return npacks; }
const char *packs_name(int i) { return i >= 0 && i < npacks ? names[i] : ""; }
const skin_pack *packs_get(int i) { return i >= 0 && i < npacks ? &packs[i] : NULL; }
int packs_find(const char *name) {
    if (!name || !name[0]) return -1;
    for (int i = 0; i < npacks; i++) if (!strcmp(names[i], name)) return i;
    return -1;
}
unsigned packs_stamp(void) {
    char d[600], p[800];
    unsigned s = 0;
    DIR *dir;
    struct dirent *e;
    struct stat st;
    packs_dir(d, sizeof d);
    if (!(dir = opendir(d))) return 0;
    while ((e = readdir(dir))) {
        if (!is_txt(e->d_name)) continue;
        snprintf(p, sizeof p, "%s/%s", d, e->d_name);
        s = s * 31 + (unsigned)strlen(e->d_name) * 7 + (unsigned char)e->d_name[0];
        if (stat(p, &st) == 0) s = s * 31 + (unsigned)st.st_mtime + (unsigned)st.st_size;
    }
    closedir(dir);
    return s;
}
