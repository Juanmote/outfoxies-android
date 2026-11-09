// license:BSD-3-Clause
// copyright-holders:R. Belmont, superctr
/* Namco C352: 32 voces PCM. Port a C del c352 v2.0 de MAME. Salida a reloj/divisor (84 kHz en NB-2). */
#include <string.h>
#include "c352.h"

typedef int16_t s16; typedef uint16_t u16; typedef uint32_t u32; typedef int32_t s32; typedef uint8_t u8; typedef int8_t s8;
enum {
    FLG_BUSY = 0x8000, FLG_KEYON = 0x4000, FLG_KEYOFF = 0x2000, FLG_LOOPTRG = 0x1000, FLG_LOOPHIST = 0x0800,
    FLG_FM = 0x0400, FLG_PHASERL = 0x0200, FLG_PHASEFL = 0x0100, FLG_PHASEFR = 0x0080, FLG_LDIR = 0x0040,
    FLG_LINK = 0x0020, FLG_NOISE = 0x0010, FLG_MULAW = 0x0008, FLG_FILTER = 0x0004, FLG_REVLOOP = 0x0003,
    FLG_LOOP = 0x0002, FLG_REVERSE = 0x0001
};
typedef struct {
    u32 pos, counter;
    s16 sample, last_sample;
    u16 regs[8];            /* vol_f, vol_r, freq, flags, wave_bank, wave_start, wave_end, wave_loop */
    u8 curr_vol[4];
} voice_t;
#define VOL_F regs[0]
#define VOL_R regs[1]
#define FREQ regs[2]
#define FLAGS regs[3]
#define BANK regs[4]
#define START regs[5]
#define END regs[6]
#define LOOP regs[7]

static voice_t v_[32];
static s16 mulaw[256];
static u16 rnd, control;
static const u8 *rom; static u32 rom_size;

static void fetch_sample(voice_t *v) {
    v->last_sample = v->sample;
    if (v->FLAGS & FLG_NOISE) {
        rnd = (rnd >> 1) ^ ((-(rnd & 1)) & 0xfff6);
        v->sample = rnd;
    } else {
        s8 s = (v->pos & 0xffffff) < rom_size ? (s8)rom[v->pos & 0xffffff] : 0;
        u16 pos = v->pos & 0xffff;
        v->sample = (v->FLAGS & FLG_MULAW) ? mulaw[s & 0xff] : (s16)(s * 256);
        if ((v->FLAGS & FLG_LOOP) && (v->FLAGS & FLG_REVERSE)) {
            if ((v->FLAGS & FLG_LDIR) && pos == v->LOOP) v->FLAGS &= ~FLG_LDIR;
            else if (!(v->FLAGS & FLG_LDIR) && pos == v->END) v->FLAGS |= FLG_LDIR;
            v->pos += (v->FLAGS & FLG_LDIR) ? -1 : 1;
        } else if (pos == v->END) {
            if ((v->FLAGS & FLG_LINK) && (v->FLAGS & FLG_LOOP)) {
                v->pos = ((u32)v->START << 16) | v->LOOP; v->FLAGS |= FLG_LOOPHIST;
            } else if (v->FLAGS & FLG_LOOP) {
                v->pos = (v->pos & 0xff0000) | v->LOOP; v->FLAGS |= FLG_LOOPHIST;
            } else {
                v->FLAGS |= FLG_KEYOFF; v->FLAGS &= ~FLG_BUSY; v->sample = 0;
            }
        } else v->pos += (v->FLAGS & FLG_REVERSE) ? -1 : 1;
    }
}
static void ramp(voice_t *v, int ch, u8 val) {
    s16 d = v->curr_vol[ch] - val;
    if (d != 0) v->curr_vol[ch] += (d > 0) ? -1 : 1;
}

void c352_init(const uint8_t *samples, uint32_t size) {
    int j = 0;
    rom = samples;
    rom_size = size;
    for (int i = 0; i < 128; i++) {
        mulaw[i] = j << 5;
        if (i < 16) j += 1; else if (i < 24) j += 2; else if (i < 48) j += 4; else if (i < 100) j += 8; else j += 16;
    }
    for (int i = 0; i < 128; i++) mulaw[i + 128] = (~mulaw[i]) & 0xffe0;
    c352_reset();
}
void c352_reset(void) { memset(v_, 0, sizeof v_); rnd = 0x1234; control = 0; }

void c352_render(int16_t *out, int n) {
    for (int i = 0; i < n; i++) {
        int o[4] = {0, 0, 0, 0};
        for (int j = 0; j < 32; j++) {
            voice_t *v = &v_[j];
            s16 s = 0;
            if (v->FLAGS & FLG_BUSY) {
                s32 next = v->counter + v->FREQ;
                if (next & 0x10000) fetch_sample(v);
                if ((next ^ v->counter) & 0x18000) {
                    ramp(v, 0, v->VOL_F >> 8); ramp(v, 1, v->VOL_F & 0xff);
                    ramp(v, 2, v->VOL_R >> 8); ramp(v, 3, v->VOL_R & 0xff);
                }
                v->counter = next & 0xffff;
                s = v->sample;
                if (!(v->FLAGS & FLG_FILTER)) s = v->last_sample + ((s32)v->counter * (v->sample - v->last_sample) >> 16);
            }
            o[0] += (((v->FLAGS & FLG_PHASEFL) ? -s : s) * v->curr_vol[0]) >> 8;
            o[2] += (((v->FLAGS & FLG_PHASERL) ? -s : s) * v->curr_vol[2]) >> 8;
            o[1] += (((v->FLAGS & FLG_PHASEFR) ? -s : s) * v->curr_vol[1]) >> 8;
            o[3] += (((v->FLAGS & FLG_PHASEFR) ? -s : s) * v->curr_vol[3]) >> 8;
        }
        /* MAME solo saca los canales delanteros 0 y 1 en NB-2 */
        out[i * 2] = (s16)(o[0] >> 3);
        out[i * 2 + 1] = (s16)(o[1] >> 3);
    }
}

uint16_t c352_read(uint32_t offset) {
    if (offset < 0x100) return v_[offset / 8].regs[offset % 8];
    if (offset == 0x200) return control;
    return 0;
}
void c352_write(uint32_t offset, uint16_t data, uint16_t mask) {
    if (offset < 0x100) {
        u16 *r = &v_[offset / 8].regs[offset % 8];
        *r = (*r & ~mask) | (data & mask);
    } else if (offset == 0x200) {
        control = (control & ~mask) | (data & mask);
    } else if (offset == 0x202) {
        if (mask != 0xffff) return;
        for (int i = 0; i < 32; i++) {
            voice_t *v = &v_[i];
            if (v->FLAGS & FLG_KEYON) {
                v->pos = ((u32)v->BANK << 16) | v->START;
                v->sample = v->last_sample = 0;
                v->counter = 0xffff;
                v->FLAGS |= FLG_BUSY;
                v->FLAGS &= ~(FLG_KEYON | FLG_LOOPHIST);
                v->curr_vol[0] = v->curr_vol[1] = v->curr_vol[2] = v->curr_vol[3] = 0;
            }
            if (v->FLAGS & FLG_KEYOFF) {
                v->FLAGS &= ~(FLG_BUSY | FLG_KEYOFF);
                v->counter = 0xffff;
            }
        }
    }
}

/* ---------- guardar/cargar estado ---------- */
size_t c352_state_size(void) { return sizeof v_ + sizeof rnd + sizeof control; }
void c352_state_save(void *dst) {
    u8 *p = dst;
    memcpy(p, v_, sizeof v_); p += sizeof v_;
    memcpy(p, &rnd, sizeof rnd); p += sizeof rnd;
    memcpy(p, &control, sizeof control);
}
void c352_state_load(const void *src) {
    const u8 *p = src;
    memcpy(v_, p, sizeof v_); p += sizeof v_;
    memcpy(&rnd, p, sizeof rnd); p += sizeof rnd;
    memcpy(&control, p, sizeof control);
}
