/* Namco C352 (port a C del de MAME, BSD-3-Clause). */
#ifndef C352_C_H
#define C352_C_H
#include <stdint.h>
void c352_init(const uint8_t *samples, uint32_t size);
void c352_reset(void);
void c352_set_gains(int voices_0_15, int voices_16_31);   /* 0-256 */
uint32_t c352_active(void);
void c352_render(int16_t *stereo, int n);          /* n muestras estereo a reloj/divisor */
uint16_t c352_read(uint32_t word_offset);
void c352_write(uint32_t word_offset, uint16_t data, uint16_t mask);
#include <stddef.h>
size_t c352_state_size(void);
void c352_state_save(void *dst);
void c352_state_load(const void *src);
#endif
