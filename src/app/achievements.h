/* Logros locales (lista de RetroAchievements, sin conexion). */
#ifndef OFX_ACH_H
#define OFX_ACH_H
#define ACH_COUNT 24
void ach_init(const char *dir);             /* carga logros.txt de esa carpeta */
#define ACH_NEW 0                           /* partida nueva o reinicio: se olvida el asalto en curso */
#define ACH_REWIND 1                        /* al rebobinar: el asalto en curso sigue contando igual */
#define ACH_LOAD 2                          /* tras cargar partida: si es a mitad de asalto, sigue contando, pero lo que
                                               depende de todo el asalto (sin dano, armas usadas) ya no */
void ach_reset(int how);
void ach_frame(const unsigned char *mem, int diff);  /* tras cada fotograma; diff: -1 mas facil que Normal, 0 Normal, 1 mas dificil */
int ach_pop(void);                          /* siguiente logro recien conseguido, o -1 */
int ach_order(int pos);                     /* numero del logro que va en ese puesto de la lista del menu */
int ach_got(int i);
int ach_todo(int i);                        /* aun no se puede conseguir (en estudio) */
int ach_total(void);
const char *ach_title(int i);                /* en el idioma del menu */
const char *ach_desc(int i);
#endif
