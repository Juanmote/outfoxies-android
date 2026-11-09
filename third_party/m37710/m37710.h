/* Mitsubishi M37702 (Namco C75): CPU de sonido. Port a C del nucleo m37710 de MAME (BSD-3-Clause,
   R. Belmont y Karl Stenerud). Solo lo que usa la placa NB-2: puertos, temporizadores, ADC e interrupciones. */
#ifndef M37710_C_H
#define M37710_C_H
#include <stdint.h>

/* Lineas de interrupcion (mismo orden que MAME) */
enum {
    M37710_LINE_DMA3 = 0, M37710_LINE_DMA2, M37710_LINE_DMA1, M37710_LINE_DMA0,
    M37710_LINE_ADC,
    M37710_LINE_UART1XMIT, M37710_LINE_UART1RECV, M37710_LINE_UART0XMIT, M37710_LINE_UART0RECV,
    M37710_LINE_TIMERB2, M37710_LINE_TIMERB1, M37710_LINE_TIMERB0,
    M37710_LINE_TIMERA4, M37710_LINE_TIMERA3, M37710_LINE_TIMERA2, M37710_LINE_TIMERA1, M37710_LINE_TIMERA0,
    M37710_LINE_IRQ2, M37710_LINE_IRQ1, M37710_LINE_IRQ0,
    M37710_LINE_WATCHDOG, M37710_LINE_DEBUG, M37710_LINE_BRK, M37710_LINE_ZERODIV, M37710_LINE_RESET
};

/* Lo pone quien usa la CPU: ROM interna de 16 KB (0xC000-0xFFFF) */
void m37710_set_internal_rom(const uint8_t *rom);
void m37710_reset(void);
int m37710_execute(int cycles);     /* ciclos de instruccion (reloj / 2); devuelve los ejecutados */
void m37710_assert_irq(int line);   /* equivale a HOLD_LINE de MAME */
void m37710_abort_slice(void);      /* termina la ejecucion actual cuanto antes */

/* Callbacks que implementa la placa */
uint8_t c75_ext_r8(uint32_t a);
void c75_ext_w8(uint32_t a, uint8_t v);
uint16_t c75_ext_r16(uint32_t a);              /* a par */
void c75_ext_w16(uint32_t a, uint16_t v);      /* a par */
uint8_t c75_port_in(int port);
void c75_port_out(int port, uint8_t v);
uint16_t c75_analog(int ch);
#include <stddef.h>
size_t m37710_state_size(void);
void m37710_state_save(void *dst);
void m37710_state_load(const void *src);
#endif
