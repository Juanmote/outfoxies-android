#ifndef OFX_M68K_STATE_H
#define OFX_M68K_STATE_H
#include <stddef.h>
size_t m68k_state_size(void);
void m68k_state_save(void *dst);
void m68k_state_load(const void *src);
#endif
