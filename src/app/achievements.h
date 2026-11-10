/* Logros locales (lista de RetroAchievements, sin conexion). */
#ifndef OFX_ACH_H
#define OFX_ACH_H
#define ACH_COUNT 20
void ach_init(const char *dir);             /* carga logros.txt de esa carpeta */
void ach_reset(void);                       /* tras cargar partida o rebobinar: se olvida el asalto en curso */
void ach_frame(const unsigned char *mem, int allowed);  /* tras cada fotograma */
int ach_pop(void);                          /* siguiente logro recien conseguido, o -1 */
int ach_got(int i);
int ach_todo(int i);                        /* aun no se puede conseguir (en estudio) */
int ach_points(int i);
int ach_total(void);
const char *ach_title(int i);
const char *ach_desc(int i);
#endif
