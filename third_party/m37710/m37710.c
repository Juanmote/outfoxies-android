// license:BSD-3-Clause
// copyright-holders:R. Belmont, Karl Stenerud
/* Port a C del nucleo m37710 de MAME para el M37702M2 (Namco C75).
   Los opcodes (m37710op.h) son los de MAME casi sin cambios; aqui van el estado,
   el mapa interno (puertos, ADC, temporizadores, control de interrupciones, RAM y ROM)
   y el bucle de ejecucion. */
#include <stdint.h>
#include <string.h>
#include "m37710.h"

#define logerror(...) ((void)0)

/* ---------- estado de la CPU ---------- */
static uint32_t m_a, m_b, m_ba, m_bb, m_x, m_y, m_xh, m_yh, m_s, m_pc, m_ppc, m_pg, m_dt, m_dpr;
static uint32_t m_flag_e, m_flag_m, m_flag_x, m_flag_n, m_flag_v, m_flag_d, m_flag_i, m_flag_z, m_flag_c;
static uint32_t m_line_irq, m_ipl, m_ir, m_im, m_im2, m_im3, m_im4, m_irq_delay;
static int m_ICount;
static uint32_t m_source, m_destination, m_stopped;
static int m_abort;

typedef void (*opcode_func)(void);
static const opcode_func *m_opcodes, *m_opcodes42, *m_opcodes89;
static int (*m_execute)(int);

/* ---------- perifericos internos ---------- */
static const uint8_t *irom;
static uint8_t iram[0x200];
static uint8_t m_port_regs[11], m_port_dir[11];
static uint8_t m_ad_control, m_ad_sweep;
static uint16_t m_ad_result[8];
static int ad_left;                                  /* relojes hasta fin de conversion, 0 = parado */
static uint8_t m_uart_mode[2], m_uart_baud[2], m_uart_ctrl_reg0[2], m_uart_ctrl_reg1[2];
static uint8_t m_count_start, m_one_shot_start, m_up_down_reg;
static uint16_t m_timer_reg[8];
static uint8_t m_timer_mode[8];
static int64_t timer_left[8], timer_reload[8];       /* en relojes; reload 0 = parado */
static uint8_t m_proc_mode, m_watchdog_freq;
#define M37710_MASKABLE_INTERRUPTS (M37710_LINE_RESET + 1 - 5)
#define M37710_INTERRUPT_MAX (M37710_LINE_RESET + 1)
static uint8_t m_int_control[M37710_MASKABLE_INTERRUPTS];

static const int m37710_irq_vectors[M37710_INTERRUPT_MAX] = {
    0xffce, 0xffd0, 0xffd2, 0xffd4, 0xffd6, 0xffd8, 0xffda, 0xffdc, 0xffde, 0xffe0,
    0xffe2, 0xffe4, 0xffe6, 0xffe8, 0xffea, 0xffec, 0xffee, 0xfff0, 0xfff2, 0xfff4,
    0xfff6, 0xfff8, 0xfffa, 0xfffc, 0xfffe
};
/* registros 0x70-0x7F -> linea */
static const int intctl_line[16] = {
    M37710_LINE_ADC, M37710_LINE_UART0XMIT, M37710_LINE_UART0RECV, M37710_LINE_UART1XMIT, M37710_LINE_UART1RECV,
    M37710_LINE_TIMERA0, M37710_LINE_TIMERA1, M37710_LINE_TIMERA2, M37710_LINE_TIMERA3, M37710_LINE_TIMERA4,
    M37710_LINE_TIMERB0, M37710_LINE_TIMERB1, M37710_LINE_TIMERB2, M37710_LINE_IRQ0, M37710_LINE_IRQ1, M37710_LINE_IRQ2
};

static void m37710i_update_irqs(void);
static void m37710i_interrupt_software(uint32_t vector);
static void m37710i_set_execution_mode(uint32_t mode);
static void m37710_set_irq_line(int line, int state);
enum { CLEAR_LINE = 0, ASSERT_LINE = 1 };

/* ---------- puertos ---------- */
static uint8_t get_port_reg(int p) {
    uint8_t d = m_port_dir[p];
    if (d != 0xff) return (c75_port_in(p) & ~d) | (m_port_regs[p] & d);
    return m_port_regs[p];
}
static void set_port_reg(int p, uint8_t data) {
    uint8_t d = m_port_dir[p];
    if (d != 0) c75_port_out(p, data & d);
    m_port_regs[p] = data;
}

/* ---------- temporizadores ---------- */
static void recalc_timer(int t) {
    static const int tscales[4] = {2, 16, 64, 512};
    if (!((m_count_start >> t) & 1)) return;
    int tval = m_timer_reg[t];
    if (tval == 0 && (m_timer_mode[t] & 0xc0) == 0) return;   /* igual que MAME */
    if ((m_timer_mode[t] & 3) == 0) {
        timer_reload[t] = (int64_t)tscales[m_timer_mode[t] >> 6] * (tval + 1);
        timer_left[t] = timer_reload[t];
    }
}

/* ---------- ADC ---------- */
static void ad_control_w(uint8_t data) {
    if ((data & 0x40) && !(m_ad_control & 0x40)) {
        ad_left = 57 * 2 * ((data & 0x80) ? 2 : 4);
        if (data & 0x10) data &= 0xf8;
    } else if (!(data & 0x40)) ad_left = 0;
    m_ad_control = data;
}
static void ad_timer_cb(void) {
    int line = m_ad_control & 7;
    m_ad_result[line] = c75_analog(line);
    if (m_ad_control & 0x10) m_ad_control = (m_ad_control & 0xf8) | ((line + 1) & 7);
    if ((m_ad_control & 0x08) || ((m_ad_control & 0x10) && line != (m_ad_sweep & 3) * 2 + 1))
        ad_left = 57 * 2 * ((m_ad_control & 0x80) ? 2 : 4);
    else {
        m37710_set_irq_line(M37710_LINE_ADC, ASSERT_LINE);
        m_ad_control &= 0xbf;
    }
}

/* ---------- registros internos 0x00-0x7F ---------- */
static uint8_t sfr_r(uint32_t a) {
    if (a >= 0x02 && a <= 0x15 && !(a & 1)) { int o = (a - 2) >> 1, p = o & ~1; return (o & 1) ? m_port_dir[p] : get_port_reg(p); }
    if (a >= 0x03 && a <= 0x11 && (a & 1)) { int o = (a - 3) >> 1, p = (o & ~1) + 1; return (o & 1) ? m_port_dir[p] : get_port_reg(p); }
    if (a >= 0x20 && a <= 0x2f) { uint16_t r = m_ad_result[(a - 0x20) >> 1]; return (a & 1) ? r >> 8 : r & 0xff; }
    if (a >= 0x46 && a <= 0x55) { uint16_t r = m_timer_reg[(a - 0x46) >> 1]; return (a & 1) ? r >> 8 : r & 0xff; }
    if (a >= 0x56 && a <= 0x5d) return m_timer_mode[a - 0x56];
    if (a >= 0x70) return m_int_control[intctl_line[a - 0x70]];
    switch (a) {
    case 0x1e: return m_ad_control;
    case 0x1f: return m_ad_sweep;
    case 0x30: return m_uart_mode[0];
    case 0x34: return m_uart_ctrl_reg0[0];
    case 0x35: return m_uart_ctrl_reg1[0];
    case 0x38: return m_uart_mode[1];
    case 0x3c: return m_uart_ctrl_reg0[1];
    case 0x3d: return m_uart_ctrl_reg1[1];
    case 0x40: return m_count_start;
    case 0x44: return m_up_down_reg & 0x1f;
    case 0x5e: return m_proc_mode & 0xf7;
    case 0x61: return m_watchdog_freq;
    }
    return 0;
}
static void sfr_w(uint32_t a, uint8_t v) {
    if (a >= 0x02 && a <= 0x15 && !(a & 1)) { int o = (a - 2) >> 1, p = o & ~1; if (o & 1) m_port_dir[p] = v; else set_port_reg(p, v); return; }
    if (a >= 0x03 && a <= 0x11 && (a & 1)) { int o = (a - 3) >> 1, p = (o & ~1) + 1; if (o & 1) m_port_dir[p] = v; else set_port_reg(p, v); return; }
    if (a >= 0x46 && a <= 0x55) {
        int t = (a - 0x46) >> 1;
        m_timer_reg[t] = (a & 1) ? (m_timer_reg[t] & 0x00ff) | (v << 8) : (m_timer_reg[t] & 0xff00) | v;
        return;
    }
    if (a >= 0x56 && a <= 0x5d) { m_timer_mode[a - 0x56] = v; return; }
    if (a >= 0x70) { m_int_control[intctl_line[a - 0x70]] = v; m37710i_update_irqs(); return; }
    switch (a) {
    case 0x1e: ad_control_w(v); break;
    case 0x1f: m_ad_sweep = v; break;
    case 0x30: m_uart_mode[0] = v; break;
    case 0x31: m_uart_baud[0] = v; break;
    case 0x34: m_uart_ctrl_reg0[0] = v; break;
    case 0x35: m_uart_ctrl_reg1[0] = v; break;
    case 0x38: m_uart_mode[1] = v; break;
    case 0x39: m_uart_baud[1] = v; break;
    case 0x3c: m_uart_ctrl_reg0[1] = v; break;
    case 0x3d: m_uart_ctrl_reg1[1] = v; break;
    case 0x40: {
        uint8_t prev = m_count_start; m_count_start = v;
        for (int i = 0; i < 8; i++) if (((v >> i) & 1) && !((prev >> i) & 1)) recalc_timer(i);
        break;
    }
    case 0x42: m_one_shot_start = v; break;
    case 0x44: m_up_down_reg = v; break;
    case 0x5e: m_proc_mode = v; break;
    case 0x61: m_watchdog_freq = v; break;
    }
}

/* ---------- bus ---------- */
static uint32_t m37710_read_8(uint32_t a) {
    a &= 0xffffff;
    if (a < 0x80) return sfr_r(a);
    if (a < 0x280) return iram[a - 0x80];
    if (a >= 0xc000 && a < 0x10000) return irom ? irom[a - 0xc000] : 0;
    return c75_ext_r8(a);
}
static void m37710_write_8(uint32_t a, uint32_t v) {
    a &= 0xffffff; v &= 0xff;
    if (a < 0x80) { sfr_w(a, v); return; }
    if (a < 0x280) { iram[a - 0x80] = v; return; }
    if (a >= 0xc000 && a < 0x10000) return;
    c75_ext_w8(a, v);
}
static uint32_t m37710_read_16(uint32_t a) {
    a &= 0xffffff;
    if (!(a & 1) && a >= 0x280 && !(a >= 0xc000 && a < 0x10000)) return c75_ext_r16(a);
    return m37710_read_8(a) | (m37710_read_8(a + 1) << 8);
}
static void m37710_write_16(uint32_t a, uint32_t v) {
    a &= 0xffffff;
    if (!(a & 1) && a >= 0x280 && !(a >= 0xc000 && a < 0x10000)) { c75_ext_w16(a, v & 0xffff); return; }
    m37710_write_8(a, v); m37710_write_8(a + 1, v >> 8);
}
#define m37710_read_8_immediate(A) m37710_read_8(A)
#define m37710_read_16_immediate(A) m37710_read_16(A)
#define WORD_ALIGNED(A) (!((A) & 1))

#include "m37710cm.h"
#include "m37710il.h"

#undef EXECUTION_MODE
#define EXECUTION_MODE EXECUTION_MODE_M0X0
#include "m37710op.h"
#undef EXECUTION_MODE
#define EXECUTION_MODE EXECUTION_MODE_M0X1
#include "m37710op.h"
#undef EXECUTION_MODE
#define EXECUTION_MODE EXECUTION_MODE_M1X0
#include "m37710op.h"
#undef EXECUTION_MODE
#define EXECUTION_MODE EXECUTION_MODE_M1X1
#include "m37710op.h"

static const opcode_func *const m37710i_opcodes[4] = { m37710i_opcodes_M0X0, m37710i_opcodes_M0X1, m37710i_opcodes_M1X0, m37710i_opcodes_M1X1 };
static const opcode_func *const m37710i_opcodes2[4] = { m37710i_opcodes42_M0X0, m37710i_opcodes42_M0X1, m37710i_opcodes42_M1X0, m37710i_opcodes42_M1X1 };
static const opcode_func *const m37710i_opcodes3[4] = { m37710i_opcodes89_M0X0, m37710i_opcodes89_M0X1, m37710i_opcodes89_M1X0, m37710i_opcodes89_M1X1 };
static int (*const m37710i_execute[4])(int) = { m37710i_execute_M0X0, m37710i_execute_M0X1, m37710i_execute_M1X0, m37710i_execute_M1X1 };

static void m37710i_set_execution_mode(uint32_t mode) {
    m_opcodes = m37710i_opcodes[mode];
    m_opcodes42 = m37710i_opcodes2[mode];
    m_opcodes89 = m37710i_opcodes3[mode];
    m_execute = m37710i_execute[mode];
}

static void m37710_set_irq_line(int line, int state) {
    if (line < 0 || line >= M37710_MASKABLE_INTERRUPTS) return;
    if (state == CLEAR_LINE) { LINE_IRQ &= ~(1u << line); m_int_control[line] &= ~8; }
    else { LINE_IRQ |= (1u << line); m_int_control[line] |= 8; }
}

static void m37710i_update_irqs(void) {
    int curirq, pending = LINE_IRQ, wantedIRQ = -1, curpri = 0;
    for (curirq = M37710_INTERRUPT_MAX - 1; curirq >= 0; curirq--) {
        if (pending & (1 << curirq)) {
            if (curirq < M37710_MASKABLE_INTERRUPTS) {
                int thispri = m_int_control[curirq] & 7;
                if (!FLAG_I && thispri > curpri && thispri > (int)m_ipl) { wantedIRQ = curirq; curpri = thispri; }
            } else { wantedIRQ = curirq; curpri = 7; break; }
        }
    }
    if (wantedIRQ != -1) {
        CPU_STOPPED &= ~STOP_LEVEL_WAI;
        m37710_set_irq_line(wantedIRQ, CLEAR_LINE);
        CLK(13);
        m37710i_push_8(REG_PG >> 16);
        m37710i_push_16(REG_PC);
        m37710i_push_8(m_ipl);
        m37710i_push_8(m37710i_get_reg_ps());
        FLAG_I = IFLAG_SET;
        m_ipl = curpri;
        REG_PG = 0;
        REG_PC = m37710_read_16(m37710_irq_vectors[wantedIRQ]);
    }
}

static void m37710i_interrupt_software(uint32_t vector) {
    CLK(13);
    m37710i_push_8(REG_PG >> 16);
    m37710i_push_16(REG_PC);
    m37710i_push_8(m_ipl);
    m37710i_push_8(m37710i_get_reg_ps());
    FLAG_I = IFLAG_SET;
    REG_PG = 0;
    REG_PC = m37710_read_16(vector);
}

/* ---------- interfaz ---------- */
void m37710_set_internal_rom(const uint8_t *rom) { irom = rom; }

void m37710_reset(void) {
    int i;
    for (i = 0; i < 8; i++) { timer_left[i] = timer_reload[i] = 0; }
    CPU_STOPPED = 0;
    memset(m_port_dir, 0, sizeof m_port_dir);
    m_ad_control &= 7;
    m_ad_sweep = (m_ad_sweep & ~0xdc) | 3;
    ad_left = 0;
    for (i = 0; i < 2; i++) {
        m_uart_mode[i] = 0;
        m_uart_ctrl_reg0[i] = (m_uart_ctrl_reg0[i] & 0xe0) | 8;
        m_uart_ctrl_reg1[i] = 2;
    }
    m_count_start = 0;
    m_one_shot_start &= ~0x1f;
    m_up_down_reg = 0;
    for (i = 0; i < 8; i++) m_timer_mode[i] = 0;
    m_proc_mode = 0;
    m_watchdog_freq &= ~1;
    for (i = 0; i <= M37710_LINE_TIMERA0; i++) m_int_control[i] &= ~0xf;
    for (i = M37710_LINE_IRQ2; i <= M37710_LINE_IRQ0; i++) m_int_control[i] &= ~0x3f;
    m_ipl = 0;
    FLAG_M = MFLAG_CLEAR; FLAG_X = XFLAG_CLEAR; FLAG_D = DFLAG_CLEAR; FLAG_I = IFLAG_SET;
    LINE_IRQ = 0; IRQ_DELAY = 0;
    REG_DPR = 0; REG_PG = 0; REG_DT = 0;
    REG_S = (REG_S & 0xff) | 0x100;
    REG_XH = REG_X & 0xff00; REG_X &= 0xff;
    REG_YH = REG_Y & 0xff00; REG_Y &= 0xff;
    REG_B = REG_A & 0xff00; REG_A &= 0xff;
    REG_BB = REG_BA & 0xff00; REG_BA &= 0xff;
    m37710i_set_execution_mode(EXECUTION_MODE_M0X0);
    REG_PC = m37710_read_16(0xfffe);
}

void m37710_assert_irq(int line) { m37710_set_irq_line(line, ASSERT_LINE); }
void m37710_abort_slice(void) { if (CLOCKS > 0) { m_abort += CLOCKS; CLOCKS = 0; } }

static void advance(int clocks) {
    for (int t = 0; t < 8; t++) {
        if (!timer_reload[t]) continue;
        timer_left[t] -= clocks;
        while (timer_left[t] <= 0) {
            timer_left[t] += timer_reload[t];
            m37710_set_irq_line(M37710_LINE_TIMERA0 - t, ASSERT_LINE);
        }
    }
    if (ad_left > 0) { ad_left -= clocks; if (ad_left <= 0) { ad_left = 0; ad_timer_cb(); } }
}

int m37710_execute(int cycles) {
    int done = 0;
    while (done < cycles) {
        int chunk = cycles - done, c, ran, used;
        if (chunk > 32) chunk = 32;
        m_abort = 0;
        CLOCKS = chunk;
        m37710i_update_irqs();
        c = CLOCKS;
        ran = c > 0 ? m_execute(c) : 0;
        used = chunk - c + ran - m_abort;
        if (used < 1) used = 1;
        advance(used * 2);
        done += used;
        if (m_abort) break;
    }
    return done;
}
