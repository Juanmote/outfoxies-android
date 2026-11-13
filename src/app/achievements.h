/* Logros locales (lista de RetroAchievements, sin conexion). */
#ifndef OFX_ACH_H
#define OFX_ACH_H
#define ACH_COUNT 20
void ach_init(const char *dir);             /* carga logros.txt de esa carpeta */
#define ACH_NEW 0                           /* partida nueva o reinicio: se olvida el asalto en curso */
#define ACH_REWIND 1                        /* al rebobinar: el asalto en curso sigue contando igual */
#define ACH_LOAD 2                          /* tras cargar partida: si es a mitad de asalto, sigue contando, pero lo que
                                               depende de todo el asalto (sin dano, armas usadas) ya no */
void ach_reset(int how);
void ach_frame(const unsigned char *mem, int allowed);  /* tras cada fotograma */
int ach_pop(void);                          /* siguiente logro recien conseguido, o -1 */
int ach_got(int i);
int ach_todo(int i);                        /* aun no se puede conseguir (en estudio) */
int ach_points(int i);
int ach_total(void);
const char *ach_title(int i);
const char *ach_desc(int i);
#endif
