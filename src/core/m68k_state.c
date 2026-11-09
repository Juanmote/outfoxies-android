/* Estado de la 68020 (Musashi) para guardar/cargar partida y rebobinar.
   Se copia la parte del nucleo que no son punteros (tablas de ciclos y callbacks van al final del struct
   y no se tocan, asi que un guardado vale aunque el sistema cargue el programa en otra direccion). */
#include <stddef.h>
#include <string.h>
#include "m68kcpu.h"
#include "m68k_state.h"

size_t m68k_state_size(void) { return offsetof(m68ki_cpu_core, cyc_instruction); }
void m68k_state_save(void *dst) { memcpy(dst, &m68ki_cpu, m68k_state_size()); }
void m68k_state_load(const void *src) { memcpy(&m68ki_cpu, src, m68k_state_size()); }
