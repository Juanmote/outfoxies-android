/* The Outfoxies para Android y escritorio: ventana, mandos, sonido, menu de opciones y ritmo de juego. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nb2.h"
#include "menu.h"
#include "touch.h"
#include "achievements.h"
#include "miniz.h"
#include "launcher.h"

#define MAX_PADS 8

static SDL_GameController *pads[MAX_PADS];
static char eeprom_path[512], config_path[512], data_dir[512];
static unsigned char eeprom_last[NB2_EEPROM_SIZE];
static SDL_AudioDeviceID audio_dev;
static SDL_Texture *tex, *mtex, *scan;
static uint32_t menu_px[MENU_MAXW * MENU_MAXH];
static int tex_w = NB2_W;      /* ancho de la ultima imagen subida (288 o 384) */
/* 8:7: en combate se recorta el 4:3 a 248 columnas (el nucleo recoloca el marcador); fuera de combate (presentacion,
   teles, titulo, eleccion) se ensena el 4:3 entero con relleno arriba y abajo (las bandas), para no cortar las teles. */
static int fight87;
/* 8:7 en la escena de las teles: el nucleo da NB2_VEXT filas mas por arriba y por abajo (el decorado prolongado) y la
   imagen llena la pantalla sin bandas. La textura del juego mide NB2_H + 2 * NB2_VEXT de alto: el juego va siempre en
   las filas NB2_VEXT..; las de mas solo se ensenan cuando tall87 vale 1. */
static int tall87;
#define TEX_H (NB2_H + 2 * NB2_VEXT)
static void upload_frame(const unsigned char *raw);
static void update_fight87(void) { const unsigned char *m = nb2_mem(); fight87 = m[0x208d59] || m[0x208e0d]; }

static void eeprom_load(void) {
    FILE *f = fopen(eeprom_path, "rb");
    if (!f) return;
    if (fread(nb2_eeprom(), 1, NB2_EEPROM_SIZE, f) != NB2_EEPROM_SIZE) { /* archivo incompleto: el juego lo rehace */ }
    fclose(f);
    memcpy(eeprom_last, nb2_eeprom(), NB2_EEPROM_SIZE);
}
static void eeprom_save(void) {
    FILE *f;
    if (!memcmp(eeprom_last, nb2_eeprom(), NB2_EEPROM_SIZE)) return;
    f = fopen(eeprom_path, "wb");
    if (!f) return;
    fwrite(nb2_eeprom(), 1, NB2_EEPROM_SIZE, f);
    fclose(f);
    memcpy(eeprom_last, nb2_eeprom(), NB2_EEPROM_SIZE);
}

/* ---------- sonido ---------- */
static void audio_open(void) {
    SDL_AudioSpec want, have;
    if (!nb2_has_sound() || SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) return;
    SDL_zero(want);
    want.freq = NB2_AUDIO_RATE; want.format = AUDIO_S16SYS; want.channels = 2; want.samples = 1024;
    /* SDL convierte de 84 kHz a lo que use el dispositivo */
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
}
static void audio_pause(int p) {
    if (!audio_dev) return;
    SDL_PauseAudioDevice(audio_dev, p);
    if (p) SDL_ClearQueuedAudio(audio_dev);
}
static void audio_push(void) {
    static short tmp[NB2_AUDIO_MAX * 2];
    int n;
    const short *a = nb2_audio(&n);
    const Uint32 frame_bytes = NB2_AUDIO_RATE / 60 * 4;
    if (!audio_dev || n <= 0) return;
    /* si el reloj de audio va mas lento que el juego, se descarta un trozo para no acumular retraso */
    if (SDL_GetQueuedAudioSize(audio_dev) > frame_bytes * 6) return;
    if (cfg.volume < 100) {
        for (int i = 0; i < n * 2; i++) tmp[i] = (short)(a[i] * cfg.volume / 100);
        a = tmp;
    }
    SDL_QueueAudio(audio_dev, a, (Uint32)n * 4);
}

/* ---------- mandos ---------- */
static void pad_add(int index) {
    SDL_GameController *c = SDL_GameControllerOpen(index);
    if (!c) return;
    for (int i = 0; i < MAX_PADS; i++) if (pads[i] == c) return;
    for (int i = 0; i < MAX_PADS; i++) if (!pads[i]) { pads[i] = c; return; }
    SDL_GameControllerClose(c);
}
static void pad_remove(SDL_JoystickID id) {
    for (int i = 0; i < MAX_PADS; i++)
        if (pads[i] && SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(pads[i])) == id) {
            SDL_GameControllerClose(pads[i]);
            pads[i] = NULL;
            for (int k = i; k + 1 < MAX_PADS; k++) pads[k] = pads[k + 1];
            pads[MAX_PADS - 1] = NULL;
            return;
        }
}
static int padbtn_down(SDL_GameController *c, int b) {
    if (!c) return 0;
    if (b == PADBTN_L2) return SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 16000;
    if (b == PADBTN_R2) return SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 16000;
    return SDL_GameControllerGetButton(c, (SDL_GameControllerButton)b);
}
static unsigned char pad_bits(SDL_GameController *c) {
    enum { DEAD = 14000 };
    unsigned char b = 0;
    int x, y;
    if (!c) return 0;
    x = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
    y = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_RIGHT) || x > DEAD) b |= NB2_RIGHT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_LEFT) || x < -DEAD) b |= NB2_LEFT;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_DOWN) || y > DEAD) b |= NB2_DOWN;
    if (SDL_GameControllerGetButton(c, SDL_CONTROLLER_BUTTON_DPAD_UP) || y < -DEAD) b |= NB2_UP;
    if (padbtn_down(c, cfg.map[ACT_B1])) b |= NB2_B1;
    if (padbtn_down(c, cfg.map[ACT_B2])) b |= NB2_B2;
    if (padbtn_down(c, cfg.map[ACT_B3])) b |= NB2_B3;
    if (padbtn_down(c, cfg.map[ACT_START])) b |= NB2_START;
    return b;
}
static unsigned char key_bits(void) {
    const Uint8 *k = SDL_GetKeyboardState(NULL);
    unsigned char b = 0;
    if (k[SDL_SCANCODE_RIGHT]) b |= NB2_RIGHT;
    if (k[SDL_SCANCODE_LEFT]) b |= NB2_LEFT;
    if (k[SDL_SCANCODE_DOWN]) b |= NB2_DOWN;
    if (k[SDL_SCANCODE_UP]) b |= NB2_UP;
    if (k[SDL_SCANCODE_Z]) b |= NB2_B1;
    if (k[SDL_SCANCODE_X]) b |= NB2_B2;
    if (k[SDL_SCANCODE_C]) b |= NB2_B3;
    if (k[SDL_SCANCODE_RETURN]) b |= NB2_START;
    return b;
}

/* ---------- imagen ---------- */
static void game_rect_in(int w, int h, SDL_Rect *d);
/* rectangulo del juego dentro de area (la pantalla entera o la vista previa del menu) */
static void game_rect(SDL_Rect area, SDL_Rect *d) {
    int w = area.w, h = area.h;
    game_rect_in(w, h, d);
    d->x += area.x; d->y += area.y;
}
static void game_rect_in(int w, int h, SDL_Rect *d) {
    if (cfg.aspect == ASP_FULL) { d->x = d->y = 0; d->w = w; d->h = h; return; }
    if (cfg.aspect == ASP_87 && tex_w == NB2_W) {
        /* 8:7: en combate el 4:3 recortado 20 pixeles por lado sin deformar (el nucleo recoloca el marcador, nb2_set_hud87);
           fuera de combate, el 4:3 entero a la misma anchura (queda sitio arriba y abajo para las bandas) */
        int bw = h * 8 / 7, bh = h;
        if (bw > w) { bw = w; bh = w * 7 / 8; }
        if (!fight87 && !tall87) { d->w = bw; d->h = bw * 3 / 4; }
        else { d->w = bw; d->h = bh; }
        d->x = (w - d->w) / 2; d->y = (h - d->h) / 2;
        return;
    }
    if (tex_w == NB2_WIDE_W) {     /* 384x224 con el pixel del 4:3: justo 16:9 */
        d->h = h; d->w = h * 16 / 9;
        if (d->w > w) { d->w = w; d->h = w * 9 / 16; }
        d->x = (w - d->w) / 2; d->y = (h - d->h) / 2;
        return;
    }
    d->h = h;
    if (cfg.aspect == ASP_43_INT && h >= NB2_H) d->h = h / NB2_H * NB2_H;
    d->w = d->h * 4 / 3;
    if (d->w > w) { d->w = w; d->h = w * 3 / 4; }
    d->x = (w - d->w) / 2; d->y = (h - d->h) / 2;
}
static void menu_rect(SDL_Renderer *r, SDL_Rect *d) {
    int w, h;
    SDL_GetRendererOutputSize(r, &w, &h);
    menu_layout(w, h);
    d->w = w; d->h = w * menu_ch / menu_cw;
    if (d->h > h) { d->h = h; d->w = h * menu_cw / menu_ch; }
    d->x = (w - d->w) / 2; d->y = (h - d->h) / 2;
}
static void apply_config(void) {
    nb2_set_hud87(cfg.aspect == ASP_87);
    nb2_set_wide(cfg.aspect == ASP_169);
    nb2_set_game_lang(cfg.game_lang);
    nb2_set_outfit(0, cfg.outfit); nb2_set_outfit(1, cfg.outfit_rival);
    nb2_set_cheats((cfg.invuln ? NB2_CHEAT_INVULN : 0) | (cfg.ammo ? NB2_CHEAT_AMMO : 0) |
                   (cfg.onehit ? NB2_CHEAT_ONEHIT : 0) | ((cfg.noifr & 1) ? NB2_CHEAT_NOIFR_HUMAN : 0) |
                   ((cfg.noifr & 2) ? NB2_CHEAT_NOIFR_CPU : 0), cfg.stage);
    nb2_set_difficulty(cfg.difficulty);
    nb2_set_volumes(cfg.music, cfg.sfx);
    nb2_set_weapon_pack(cfg.weapons);
    SDL_SetTextureScaleMode(tex, cfg.filter == FILT_SHARP ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
}
/* Scanlines a la resolucion real de la pantalla. Antes se estiraba una textura de 4 filas por linea
   a una altura que casi nunca es multiplo de 224, y unas lineas salian de 1 pixel y otras de 2.
   Ahora la textura mide exactamente lo que el juego en pantalla y cada fila lleva una sombra de perfil
   senoidal (oscura en el borde entre lineas del juego y clara en el centro): al no tener armonicos,
   todas las lineas se ven iguales aunque la escala no sea entera. */
static int scan_h;
static void update_scanlines(SDL_Renderer *r, int h, int lines) {
    const double PI = 3.14159265358979;
    double ppl = (double)h / lines, strength = 0.55;
    Uint32 *px;
    static int scan_lines;
    if (h == scan_h && lines == scan_lines) return;
    if (scan) SDL_DestroyTexture(scan);
    scan = NULL; scan_h = h; scan_lines = lines;
    if (h <= 0) return;
    /* con menos de 2 pixeles por linea no se puede dibujar la sombra sin que se deforme */
    if (ppl < 2.0) return;
    if (ppl < 3.0) strength *= ppl - 2.0;
    px = malloc(sizeof(Uint32) * h);
    scan = px ? SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, h) : NULL;
    if (!scan) { free(px); return; }
    for (int y = 0; y < h; y++) {
        /* media de la sombra sobre toda la altura del pixel (t en lineas del juego; entero = borde) */
        double t0 = y / ppl, t1 = (y + 1) / ppl;
        double avg = (sin(2.0 * PI * t1) - sin(2.0 * PI * t0)) / (2.0 * PI * (t1 - t0));
        int a = (int)(255.0 * strength * (0.5 + 0.5 * avg) + 0.5);
        px[y] = (Uint32)a << 24;
    }
    SDL_UpdateTexture(scan, NULL, px, 4);
    SDL_SetTextureBlendMode(scan, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(scan, SDL_ScaleModeNearest);
    free(px);
}
/* ---------- colores ----------
   Como en Master of Darkness, pero sobre el RGB de 8 bits de la recreativa: cada modo cambia los niveles de
   cada componente, la saturacion (alrededor de la luminancia) y un tinte. Sepia y pantalla verde solo usan la
   luminancia. Se aplica a la imagen ya dibujada, antes de subirla a la textura. */
static const unsigned char *last_rgb, *last_raw;
static unsigned char col_lv[256];
static int col_sat = 256, col_mode = -1;
static unsigned char col_y[256][3];
static int clamp255(int v) { return v < 0 ? 0 : v > 255 ? 255 : v; }
static void colors_prepare(void) {
    if (col_mode == cfg.colors) return;
    col_mode = cfg.colors;
    for (int c = 0; c < 256; c++) {
        double v = c / 255.0;
        switch (col_mode) {
        case COL_TV: col_lv[c] = (unsigned char)(16 + c * 224 / 255); break;                 /* negros levantados, menos contraste */
        case COL_VIVID: col_lv[c] = (unsigned char)clamp255((int)((c - 128) * 1.15 + 128.5)); break;   /* mas contraste */
        case COL_NIGHT: col_lv[c] = (unsigned char)(6 + c * 230 / 255); break;
        default: col_lv[c] = (unsigned char)c;
        }
        if (col_mode == COL_SEPIA) {
            col_y[c][0] = (unsigned char)clamp255((int)(255 * (0.10 + 0.95 * v)));
            col_y[c][1] = (unsigned char)clamp255((int)(255 * (0.06 + 0.80 * v)));
            col_y[c][2] = (unsigned char)clamp255((int)(255 * (0.03 + 0.58 * v)));
        } else if (col_mode == COL_GREEN) {   /* cuatro tonos de verde, como una portatil antigua */
            static const unsigned g4[4] = {0x0F380F, 0x306230, 0x8BAC0F, 0x9BBC0F};
            int sh = c < 40 ? 0 : c < 110 ? 1 : c < 185 ? 2 : 3;
            col_y[c][0] = g4[sh] >> 16; col_y[c][1] = (g4[sh] >> 8) & 255; col_y[c][2] = g4[sh] & 255;
        }
    }
    col_sat = col_mode == COL_TV ? 210 : col_mode == COL_VIVID ? 328 : col_mode == COL_NIGHT ? 230 : 256;
}
static unsigned char col_buf[NB2_WIDE_W * NB2_H * 3], col_buf2[2 * NB2_VEXT * NB2_W * 3];
static const unsigned char *colors_apply_to(unsigned char *d, const unsigned char *src, int n);
static const unsigned char *colors_apply(const unsigned char *src, int n) { return colors_apply_to(col_buf, src, n); }
static const unsigned char *colors_apply_to(unsigned char *d, const unsigned char *src, int n) {
    unsigned char *d0 = d;
    colors_prepare();
    if (col_mode == COL_ORIGINAL) return src;
    for (int i = 0; i < n; i++, src += 3, d += 3) {
        int r = src[0], g = src[1], b = src[2], y;
        if (col_mode == COL_SEPIA || col_mode == COL_GREEN) {
            y = (77 * r + 150 * g + 29 * b) >> 8;
            d[0] = col_y[y][0]; d[1] = col_y[y][1]; d[2] = col_y[y][2];
            continue;
        }
        r = col_lv[r]; g = col_lv[g]; b = col_lv[b];
        y = (77 * r + 150 * g + 29 * b) >> 8;
        r = y + (((r - y) * col_sat) >> 8); g = y + (((g - y) * col_sat) >> 8); b = y + (((b - y) * col_sat) >> 8);
        if (col_mode == COL_TV) { r += 8; b -= 6; }                        /* algo calida */
        else if (col_mode == COL_NIGHT) {                                   /* sombras frias, luces calidas */
            int t = 255 - y;
            r -= 22 * t / 255; g -= 6 * t / 255; b += 26 * t / 255;
        }
        d[0] = (unsigned char)clamp255(r); d[1] = (unsigned char)clamp255(g); d[2] = (unsigned char)clamp255(b);
    }
    return d0;
}
/* sube un fotograma del nucleo a la textura (con los colores elegidos) y, en 8:7 con las teles, sus franjas de arriba y abajo */
static void upload_frame(const unsigned char *raw) {
    SDL_Rect sr = {0, NB2_VEXT, nb2_width(), NB2_H};
    const unsigned char *vb = cfg.aspect == ASP_87 ? nb2_vborder() : NULL;
    tex_w = sr.w; last_raw = raw;
    update_fight87();
    last_rgb = colors_apply(raw, sr.w * NB2_H);
    SDL_UpdateTexture(tex, &sr, last_rgb, sr.w * 3);
    tall87 = vb != NULL && sr.w == NB2_W;
    if (tall87) {
        SDL_Rect a = {0, 0, NB2_W, NB2_VEXT}, b = {0, NB2_VEXT + NB2_H, NB2_W, NB2_VEXT};
        const unsigned char *c = colors_apply_to(col_buf2, vb, 2 * NB2_VEXT * NB2_W);
        SDL_UpdateTexture(tex, &a, c, NB2_W * 3);
        SDL_UpdateTexture(tex, &b, c + NB2_VEXT * NB2_W * 3, NB2_W * 3);
    }
}

/* ---------- bandas "ambiente" ----------
   Con el juego en 4:3, los lados vacios toman los colores de la escena: la imagen reducida a 16x12 bloques de
   color medio, oscurecida y estirada (suavizada) por toda la pantalla, por debajo del juego. */
#define AMB_W 16
#define AMB_H 12
static SDL_Texture *tex_amb;
static void draw_ambient(SDL_Renderer *r, SDL_Rect area) {
    static Uint32 px[AMB_W * AMB_H];
    int w = tex_w;
    if (!last_rgb) return;
    if (!tex_amb) {
        tex_amb = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, AMB_W, AMB_H);
        if (!tex_amb) return;
        SDL_SetTextureScaleMode(tex_amb, SDL_ScaleModeLinear);
    }
    for (int by = 0; by < AMB_H; by++) for (int bx = 0; bx < AMB_W; bx++) {
        int x0 = bx * w / AMB_W, x1 = (bx + 1) * w / AMB_W, y0 = by * NB2_H / AMB_H, y1 = (by + 1) * NB2_H / AMB_H;
        unsigned sr = 0, sg = 0, sb = 0, n = 0;
        for (int y = y0; y < y1; y += 2) for (int x = x0; x < x1; x += 2) {
            const unsigned char *p = last_rgb + (y * w + x) * 3;
            sr += p[0]; sg += p[1]; sb += p[2]; n++;
        }
        if (!n) n = 1;
        px[by * AMB_W + bx] = 0xff000000u | (sr * 45 / 100 / n) << 16 | (sg * 45 / 100 / n) << 8 | (sb * 45 / 100 / n);
    }
    SDL_UpdateTexture(tex_amb, NULL, px, AMB_W * 4);
    SDL_RenderCopy(r, tex_amb, NULL, &area);
}
/* ---------- filtro CRT (el de Master of Darkness) ----------
   Por GPU: la imagen se dibuja a 3 columnas por pixel y k filas por linea, se multiplica por una mascara (perfil del
   haz en cada linea y franjas de fosforo rojo, verde y azul) y se le suma una copia borrosa (el resplandor, que
   tambien devuelve el brillo que quita la mascara). El curvo dibuja el resultado en una rejilla abombada con las
   esquinas mas oscuras. */
static SDL_Texture *tex_crt, *tex_mask, *tex_glow, *tex_sharp;
static int crt_w, crt_h, sharp_w, sharp_h;
static void crt_textures(SDL_Renderer *r, int gw, int gh, int k) {
    int w = gw * 3, h = gh * k;
    Uint32 *px;
    if (tex_crt && crt_w == w && crt_h == h) return;
    if (tex_crt) SDL_DestroyTexture(tex_crt);
    if (tex_mask) SDL_DestroyTexture(tex_mask);
    if (tex_glow) SDL_DestroyTexture(tex_glow);
    tex_crt = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    tex_glow = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, gw / 2, gh / 2);
    tex_mask = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, w, h);
    crt_w = w; crt_h = h;
    if (!tex_crt || !tex_glow || !tex_mask || !(px = malloc(sizeof(Uint32) * w * h))) return;
    for (int y = 0; y < h; y++) {
        double t = ((y % k) + 0.5) / k - 0.5;
        double beam = 0.38 + 0.62 * exp(-t * t / (2 * 0.2 * 0.2));   /* claro en el centro de la linea */
        for (int x = 0; x < w; x++) {
            double m[3] = {0.78, 0.78, 0.78};
            Uint32 c = 0xff000000u;
            m[x % 3] = 1.0;                                            /* franja de fosforo */
            for (int ch = 0; ch < 3; ch++) c |= (Uint32)clamp255((int)(255 * beam * m[ch] + 0.5)) << (16 - 8 * ch);
            px[y * w + x] = c;
        }
    }
    SDL_UpdateTexture(tex_mask, NULL, px, w * 4);
    free(px);
    SDL_SetTextureBlendMode(tex_mask, SDL_BLENDMODE_MOD);
    SDL_SetTextureScaleMode(tex_crt, SDL_ScaleModeLinear);
    SDL_SetTextureScaleMode(tex_glow, SDL_ScaleModeLinear);
}
static void draw_crt(SDL_Renderer *r, SDL_Rect dst, SDL_Rect src, int curved) {
    int k = dst.h / src.h;
    if (k < 3) k = 3;
    if (k > 6) k = 6;
    crt_textures(r, src.w, src.h, k);
    if (!tex_crt || !tex_mask || !tex_glow) { SDL_RenderCopy(r, tex, &src, &dst); return; }
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);                 /* resplandor: la imagen a media escala */
    SDL_SetRenderTarget(r, tex_glow);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
    SDL_RenderCopy(r, tex, &src, NULL);
    SDL_SetRenderTarget(r, tex_crt);                                   /* imagen x mascara + resplandor */
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
    SDL_RenderCopy(r, tex, &src, NULL);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(r, tex_mask, NULL, NULL);
    SDL_SetTextureBlendMode(tex_glow, SDL_BLENDMODE_ADD);
    SDL_SetTextureAlphaMod(tex_glow, 72);
    SDL_RenderCopy(r, tex_glow, NULL, NULL);
    SDL_SetTextureAlphaMod(tex_glow, 255);
    SDL_SetRenderTarget(r, NULL);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
    if (!curved) { SDL_RenderCopy(r, tex_crt, NULL, &dst); return; }
    {
        enum { GX = 32, GY = 24 };
        static SDL_Vertex v[(GX + 1) * (GY + 1)];
        static int idx[GX * GY * 6];
        const double bend = 0.045;
        int n = 0;
        for (int j = 0; j <= GY; j++)
            for (int i = 0; i <= GX; i++) {
                double u = (double)i / GX, w = (double)j / GY, x = u * 2 - 1, y = w * 2 - 1;
                double xs = x * (1 - bend * y * y), ys = y * (1 - bend * x * x), r2 = x * x * y * y;
                int lum = clamp255((int)(255 * (1 - 0.35 * r2 - 0.06 * (x * x + y * y))));
                SDL_Vertex *p = &v[j * (GX + 1) + i];
                p->position.x = (float)(dst.x + (xs + 1) * 0.5 * dst.w);
                p->position.y = (float)(dst.y + (ys + 1) * 0.5 * dst.h);
                p->color.r = p->color.g = p->color.b = (Uint8)lum; p->color.a = 255;
                p->tex_coord.x = (float)u; p->tex_coord.y = (float)w;
            }
        for (int j = 0; j < GY; j++)
            for (int i = 0; i < GX; i++) {
                int a = j * (GX + 1) + i, b = a + 1, c = a + GX + 1, d = c + 1;
                idx[n++] = a; idx[n++] = b; idx[n++] = c; idx[n++] = b; idx[n++] = d; idx[n++] = c;
            }
        SDL_RenderGeometry(r, tex_crt, v, (GX + 1) * (GY + 1), idx, n);
    }
}
/* parte de la imagen del juego que se ensena: toda, salvo en 8:7 (se quitan 20 columnas por lado; el pixel del
   4:3 es un 3,7 % mas ancho que alto, asi que 248x224 se ven justo como 8:7) */
static SDL_Rect game_src(void) {
    SDL_Rect s = {0, NB2_VEXT, tex_w, NB2_H};
    if (cfg.aspect == ASP_87 && tex_w == NB2_W && fight87) { s.x = 20; s.w = NB2_W - 40; }
    else if (cfg.aspect == ASP_87 && tex_w == NB2_W && tall87) { s.y = 0; s.h = TEX_H; }
    return s;
}
static void draw_game_in(SDL_Renderer *ren, SDL_Rect area) {
    SDL_Rect d, src = game_src();
    game_rect(area, &d);
    if (d.w < area.w - 2 || d.h < area.h - 2) {
        if (cfg.bands == BAND_AMBIENT) draw_ambient(ren, area);
        else if (cfg.bands == BAND_COVER) launcher_cover(ren, area, 150);
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_NONE);
    if ((cfg.filter == FILT_CRT || cfg.filter == FILT_CRT_CURVED) && SDL_RenderTargetSupported(ren)) {
        draw_crt(ren, d, src, cfg.filter == FILT_CRT_CURVED);
        return;
    }
    if (cfg.filter == FILT_SHARP_SMOOTH && SDL_RenderTargetSupported(ren)) {
        /* nitida suavizada: se agranda a escala entera sin suavizar y luego se reduce suavizando */
        int k = (d.h + src.h - 1) / src.h;
        if (k < 1) k = 1;
        if (k > 8) k = 8;
        if (!tex_sharp || sharp_w != src.w * k || sharp_h != src.h * k) {
            if (tex_sharp) SDL_DestroyTexture(tex_sharp);
            sharp_w = src.w * k; sharp_h = src.h * k;
            tex_sharp = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, sharp_w, sharp_h);
            if (tex_sharp) SDL_SetTextureScaleMode(tex_sharp, SDL_ScaleModeLinear);
        }
        if (tex_sharp) {
            SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
            SDL_SetRenderTarget(ren, tex_sharp);
            SDL_RenderCopy(ren, tex, &src, NULL);
            SDL_SetRenderTarget(ren, NULL);
            SDL_RenderCopy(ren, tex_sharp, NULL, &d);
            return;
        }
    }
    SDL_SetTextureScaleMode(tex, cfg.filter == FILT_SHARP ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
    SDL_RenderCopy(ren, tex, &src, &d);
    if (cfg.filter == FILT_SCANLINES) {
        update_scanlines(ren, d.h, src.h);
        if (scan) SDL_RenderCopy(ren, scan, NULL, &d);
    }
}
static void draw_game(SDL_Renderer *ren) {
    SDL_Rect a = {0, 0, 0, 0};
    SDL_GetRendererOutputSize(ren, &a.w, &a.h);
    draw_game_in(ren, a);
}

/* ---------- entrada del menu ---------- */
static int menu_event(SDL_Event *e) {
    static int stick_dir = -1;
    switch (e->type) {
    case SDL_CONTROLLERBUTTONDOWN:
        if (menu_waiting_button()) {
            int b = e->cbutton.button;
            if (b >= SDL_CONTROLLER_BUTTON_DPAD_UP && b <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT) return MR_NONE;
            return menu_bind(b);
        }
        switch (e->cbutton.button) {
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return menu_input(MI_UP);
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return menu_input(MI_DOWN);
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return menu_input(MI_LEFT);
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return menu_input(MI_RIGHT);
        case SDL_CONTROLLER_BUTTON_A: case SDL_CONTROLLER_BUTTON_START: return menu_input(MI_OK);
        case SDL_CONTROLLER_BUTTON_B: case SDL_CONTROLLER_BUTTON_BACK: return menu_input(MI_BACK);
        }
        break;
    case SDL_CONTROLLERAXISMOTION: {
        int v = e->caxis.value, a = e->caxis.axis, dir = -1;
        if (menu_waiting_button()) {
            if (a == SDL_CONTROLLER_AXIS_TRIGGERLEFT && v > 16000) return menu_bind(PADBTN_L2);
            if (a == SDL_CONTROLLER_AXIS_TRIGGERRIGHT && v > 16000) return menu_bind(PADBTN_R2);
            return MR_NONE;
        }
        if (a == SDL_CONTROLLER_AXIS_LEFTY) dir = v < -20000 ? MI_UP : v > 20000 ? MI_DOWN : -2;
        else if (a == SDL_CONTROLLER_AXIS_LEFTX) dir = v < -20000 ? MI_LEFT : v > 20000 ? MI_RIGHT : -2;
        if (dir == -2) { stick_dir = -1; return MR_NONE; }
        if (dir >= 0 && dir != stick_dir) { stick_dir = dir; return menu_input(dir); }
        break;
    }
    case SDL_KEYDOWN:
        switch (e->key.keysym.sym) {
        case SDLK_UP: return menu_input(MI_UP);
        case SDLK_DOWN: return menu_input(MI_DOWN);
        case SDLK_LEFT: return menu_input(MI_LEFT);
        case SDLK_RIGHT: return menu_input(MI_RIGHT);
        case SDLK_RETURN: case SDLK_z: return menu_input(MI_OK);
        case SDLK_ESCAPE: case SDLK_AC_BACK: case SDLK_BACKSPACE: case SDLK_x: return menu_input(MI_BACK);
        }
        break;
    }
    return MR_NONE;
}

/* ---------- START, pausa y avisos ----------
   Direcciones de RAM (halladas en pruebas en el servidor, ver ESTADO.md):
   0x208079 bit 0/1 = jugador 1/2 en partida (la demo pone los dos); 0x208129 = creditos;
   energia de cada jugador en 0x208D59 / 0x208E0D (0 fuera de combate).
   Como la demo tambien marca a los jugadores, "partida de verdad" se lleva aqui: empieza cuando el
   jugador entra tras nuestro START y acaba cuando el juego le quita la marca. */
#define RAM_PLAYING 0x208079
#define RAM_CREDITS 0x208129
static const unsigned ram_energy[2] = {0x208d59, 0x208e0d};
static int paused, start_seq[2] = {-1, -1}, real_game[2];
static char osd_msg[96];
static Uint32 osd_until;
static void osd(const char *m) { snprintf(osd_msg, sizeof osd_msg, "%s", m); osd_until = SDL_GetTicks() + 2000; }
static void track_players(void) {
    const unsigned char *m = nb2_mem();
    for (int p = 0; p < 2; p++) if (!(m[RAM_PLAYING] & (1 << p))) real_game[p] = 0;
}
static void start_pressed(int p) {
    const unsigned char *m = nb2_mem();
    if (paused || (real_game[p] && m[ram_energy[p]] != 0)) {   /* en combate: pausa */
        paused = !paused;
        audio_pause(paused);
        return;
    }
    if (start_seq[p] < 0) start_seq[p] = 0;   /* fuera de combate: credito (si hace falta) y START */
}
/* devuelve el bit de START que hay que mandar al juego este fotograma. El juego no acepta START hasta
   unos 14 fotogramas despues de la moneda: se pulsa a los 20 y, si el jugador aun no ha entrado, se repite
   dos veces mas (a los 45 y a los 70). */
static unsigned char start_step(int p) {
    int f = start_seq[p], ph = (f - 20) % 25;
    if (f < 0) return 0;
    if (f == 0 && nb2_mem()[RAM_CREDITS] == 0) nb2_coin();
    if (f > 26 && (nb2_mem()[RAM_PLAYING] & (1 << p))) { real_game[p] = 1; ph = 99; }   /* ya esta dentro */
    start_seq[p] = f + 1 >= 90 || ph == 99 ? -1 : f + 1;
    return (f >= 20 && ph >= 0 && ph < 6) ? NB2_START : 0;
}

/* ---------- vibracion ----------
   Se vibra en el mando de cada jugador (SDL_GameControllerRumble). Si ese mando no tiene motor y es el
   del jugador 1, se usa el vibrador de la consola (SDL_Haptic: en Android es el servicio de vibracion).
   Sucesos, mirados al final de cada fotograma (solo en partida de verdad, no en la demo):
   - recibir dano: baja la energia propia (lo mas fuerte, segun el dano)
   - golpear: baja la energia del rival
   - disparar: baja la municion del arma que se lleva (+0x0C arma, +0x0E municion)
   - saltar: se pulsa el boton 2 en combate
   - explosiones del escenario: el juego pide el sonido de explosion (nb2_events) */
static SDL_Haptic *haptic;
static int haptic_tried;
static void rumble(int p, int strength, int ms) {
    int s = strength * cfg.vibration / 100;
    Uint16 v;
    if (s <= 0) return;
    if (s > 100) s = 100;
    v = (Uint16)(s * 65535 / 100);
    if (pads[p] && SDL_GameControllerRumble(pads[p], v, v, (Uint32)ms) == 0) return;
    if (p != 0) return;
    if (!haptic_tried) {
        haptic_tried = 1;
        if (SDL_InitSubSystem(SDL_INIT_HAPTIC) == 0 && SDL_NumHaptics() > 0 && (haptic = SDL_HapticOpen(0)) &&
            SDL_HapticRumbleInit(haptic) != 0) { SDL_HapticClose(haptic); haptic = NULL; }
    }
    if (haptic) SDL_HapticRumblePlay(haptic, s / 100.0f, (Uint32)ms);
}
static int vib_energy[2], vib_weapon[2], vib_ammo[2], vib_jump[2];
static void vibration_frame(unsigned char in1, unsigned char in2) {
    const unsigned char *m = nb2_mem();
    unsigned ev = nb2_events();
    int str[2] = {0, 0}, len[2] = {0, 0};
#define VIB(p, st, ms) do { if ((st) > str[p]) { str[p] = (st); len[p] = (ms); } } while (0)
    for (int p = 0; p < 2; p++) {
        const unsigned char *e = m + ram_energy[p], *other = m + ram_energy[1 - p];
        int jump = ((p ? in2 : in1) & NB2_B2) != 0;
        if (real_game[p] && !paused && cfg.vibration) {
            if (vib_energy[p] && *e < vib_energy[p]) {                     /* dano recibido */
                int d = vib_energy[p] - *e;
                VIB(p, 65 + (d > 35 ? 35 : d), 200);
            }
            if (vib_energy[1 - p] && *other < vib_energy[1 - p]) VIB(p, 50, 90);             /* golpe dado */
            if (*e && e[0x0c] && e[0x0c] == vib_weapon[p] && e[0x0e] < vib_ammo[p]) VIB(p, 35, 60);   /* disparo */
            if (*e && jump && !vib_jump[p]) VIB(p, 22, 45);                                    /* salto */
            if (ev & NB2_EV_EXPLOSION) VIB(p, 75, 260);
        }
        vib_jump[p] = jump;
        if (str[p]) rumble(p, str[p], len[p]);
    }
    for (int p = 0; p < 2; p++) { vib_energy[p] = m[ram_energy[p]]; vib_weapon[p] = m[ram_energy[p] + 0x0c]; vib_ammo[p] = m[ram_energy[p] + 0x0e]; }
#undef VIB
}
static void vibration_reset(void) {
    const unsigned char *m = nb2_mem();
    for (int p = 0; p < 2; p++) { vib_energy[p] = m[ram_energy[p]]; vib_weapon[p] = m[ram_energy[p] + 0x0c]; vib_ammo[p] = m[ram_energy[p] + 0x0e]; }
}

/* ---------- partidas guardadas y rebobinado ---------- */
static int save_slot(int slot) {
    char path[600]; size_t len; FILE *f; int ok = 0;
    unsigned char *b = nb2_state_save(&len, 6);
    if (!b) return 0;
    menu_slot_path(slot, path, sizeof path);
    if ((f = fopen(path, "wb"))) { ok = fwrite(b, 1, len, f) == len; fclose(f); }
    free(b);
    return ok;
}
static int load_slot(int slot) {
    char path[600]; long len; FILE *f; unsigned char *b; int ok = 0;
    menu_slot_path(slot, path, sizeof path);
    if (!(f = fopen(path, "rb"))) return 0;
    fseek(f, 0, SEEK_END); len = ftell(f); fseek(f, 0, SEEK_SET);
    if (len > 0 && (b = malloc(len))) {
        if (fread(b, 1, len, f) == (size_t)len) ok = nb2_state_load(b, (size_t)len);
        free(b);
    }
    fclose(f);
    return ok;
}
/* anillo de estados: uno cada RW_EVERY fotogramas, RW_MAX en total (unos 20 s, ~25 MB) */
#define RW_EVERY 3
#define RW_MAX 400
static struct { unsigned char *d; size_t n; } rw[RW_MAX];
static int rw_head, rw_count;
static void rw_push(void) {
    size_t n; unsigned char *d = nb2_state_save(&n, 1);
    if (!d) return;
    free(rw[rw_head].d);
    rw[rw_head].d = d; rw[rw_head].n = n;
    rw_head = (rw_head + 1) % RW_MAX;
    if (rw_count < RW_MAX) rw_count++;
}
static int rw_pop(void) {
    int i;
    if (!rw_count) return 0;
    i = (rw_head + RW_MAX - 1) % RW_MAX;
    nb2_state_load(rw[i].d, rw[i].n);
    free(rw[i].d); rw[i].d = NULL;
    rw_head = i; rw_count--;
    return 1;
}
static void rw_clear(void) {
    for (int i = 0; i < RW_MAX; i++) { free(rw[i].d); rw[i].d = NULL; }
    rw_head = rw_count = 0;
}
/* en el menu, los controles tactiles mandan teclas (cruceta = flechas, disparo/START = aceptar, salto/MENU = volver) */
#define TOUCH_KEY 0x7ff0
static void touch_menu_keys(unsigned now, unsigned *prev) {
    static const struct { unsigned bit; SDL_Keycode k; } m[] = {
        {NB2_UP, SDLK_UP}, {NB2_DOWN, SDLK_DOWN}, {NB2_LEFT, SDLK_LEFT}, {NB2_RIGHT, SDLK_RIGHT},
        {NB2_B1, SDLK_RETURN}, {NB2_START, SDLK_RETURN}, {NB2_B2, SDLK_ESCAPE}, {TOUCH_MENU, SDLK_ESCAPE}};
    for (unsigned i = 0; i < sizeof m / sizeof m[0]; i++)
        if ((now & m[i].bit) && !(*prev & m[i].bit)) {
            SDL_Event e; SDL_zero(e);
            e.type = SDL_KEYDOWN; e.key.windowID = TOUCH_KEY; e.key.keysym.sym = m[i].k;
            SDL_PushEvent(&e);
        }
    *prev = now;
}
static int any_pad(int act) {
    for (int i = 0; i < MAX_PADS; i++) if (pads[i] && padbtn_down(pads[i], cfg.map[act])) return 1;
    return 0;
}

int main(int argc, char **argv) {
    char rom[512], err[128] = "";
    SDL_Window *win; SDL_Renderer *ren;
    Uint64 last, freq; double acc = 0;
    int run = 1, frames = argc > 2 ? atoi(argv[2]) : 0, count = 0, guard = 0, emu_frames = 0;
    int in_menu = frames ? 0 : 1, started = 0, prev_start[2] = {0, 0}, prev_menu = 0, rewinding = 0;
    unsigned touch_prev = 0, touch_now;
    const double period = 1.0 / 60.0;
    Uint32 tick_prev;

    SDL_SetHint(SDL_HINT_ACCELEROMETER_AS_JOYSTICK, "0");
    SDL_SetHint(SDL_HINT_ANDROID_TRAP_BACK_BUTTON, "1");
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS) != 0) {
        fprintf(stderr, "SDL: %s\n", SDL_GetError());
        return 1;
    }
#ifdef __ANDROID__
    snprintf(data_dir, sizeof data_dir, "%s", SDL_AndroidGetInternalStoragePath());
    snprintf(rom, sizeof rom, "%s/rom.zip", data_dir);
    snprintf(eeprom_path, sizeof eeprom_path, "%s/eeprom.bin", data_dir);
    snprintf(config_path, sizeof config_path, "%s/opciones.txt", data_dir);
#else
    snprintf(data_dir, sizeof data_dir, ".");
    snprintf(rom, sizeof rom, "%s", argc > 1 ? argv[1] : "outfxies.zip");
    snprintf(eeprom_path, sizeof eeprom_path, "outfoxies_eeprom.bin");
    snprintf(config_path, sizeof config_path, "outfoxies_opciones.txt");
#endif
#ifdef __ANDROID__
    /* las partidas guardadas van a la carpeta de la app en el almacenamiento compartido
       (Android/data/com.juanmote.outfoxies/files) para poder copiarlas al PC y mandarlas si algo falla */
    if ((SDL_AndroidGetExternalStorageState() & SDL_ANDROID_EXTERNAL_STORAGE_WRITE) && SDL_AndroidGetExternalStoragePath())
        menu_set_save_dir(SDL_AndroidGetExternalStoragePath());
    else
#endif
    menu_set_save_dir(data_dir);
    ach_init(menu_save_dir());
    if (!config_load(config_path)) {
        /* sin idioma guardado: el del sistema (espanol si lo es, si no ingles) */
        SDL_Locale *loc = SDL_GetPreferredLocales();
        cfg.lang = (loc && loc[0].language && !strncmp(loc[0].language, "es", 2)) ? LANG_ES : (loc && loc[0].language) ? LANG_EN : LANG_ES;
        cfg.game_lang = cfg.lang == LANG_ES;
        SDL_free(loc);
    }
    if (!nb2_load(rom, err, sizeof err)) {
        char msg[256];
#ifdef __ANDROID__
        remove(rom);    /* la proxima vez vuelve a pedir el archivo */
#endif
        snprintf(msg, sizeof msg, TR("El archivo elegido no sirve: %s.\nHace falta el romset outfxies.zip (World, OU2).",
                                     "The chosen file is not valid: %s.\nThe outfxies.zip romset (World, OU2) is needed."), err);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies Remastered", msg, NULL);
        SDL_Quit();
        return 1;
    }
    eeprom_load();
    nb2_reset();
    audio_open();
    if (getenv("OFX_LOAD")) {   /* pruebas (escritorio): arranca desde una partida guardada */
        FILE *f = fopen(getenv("OFX_LOAD"), "rb"); long n; unsigned char *b;
        if (f) { fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET); b = malloc(n);
                 if (b && fread(b, 1, n, f) == (size_t)n) { nb2_state_load(b, n); real_game[0] = 1; } free(b); fclose(f); }
    }

    int win_w = 1152, win_h = 864;
    if (getenv("OFX_WIN")) sscanf(getenv("OFX_WIN"), "%dx%d", &win_w, &win_h);   /* pruebas: tamano de la ventana */
    win = SDL_CreateWindow("The Outfoxies Remastered", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w, win_h,
                           SDL_WINDOW_RESIZABLE
#ifdef __ANDROID__
                           | SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
    );
    ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, NB2_WIDE_W, TEX_H) : NULL;
    mtex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MENU_MAXW, MENU_MAXH) : NULL;
    if (!tex || !mtex) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies Remastered", SDL_GetError(), NULL);
        SDL_Quit();
        return 1;
    }
    touch_init(ren);
    launcher_init(ren);
    SDL_SetTextureBlendMode(mtex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(mtex, SDL_ScaleModeLinear);
    apply_config();
    for (int i = 0; i < SDL_NumJoysticks(); i++) if (SDL_IsGameController(i)) pad_add(i);
    if (in_menu) menu_open(0);
    else audio_pause(0);
    if (in_menu && getenv("OFX_MENUGAME")) { started = 1; menu_open(1); }   /* pruebas: el menu de partida (con OFX_MENUSHOT) */
    /* pruebas (escritorio): OFX_FINGERS="x,y x,y" (0-1) deja esos dedos apoyados */
    if (getenv("OFX_FINGERS")) {
        const char *q = getenv("OFX_FINGERS"); float fx, fy; int n = 0, k;
        while (sscanf(q, "%f,%f%n", &fx, &fy, &k) == 2) {
            SDL_Event e; SDL_zero(e);
            e.type = SDL_FINGERDOWN; e.tfinger.fingerId = ++n; e.tfinger.x = fx; e.tfinger.y = fy;
            SDL_PushEvent(&e); q += k;
        }
    }

    freq = SDL_GetPerformanceFrequency();
    last = SDL_GetPerformanceCounter();
    tick_prev = SDL_GetTicks();
    while (run) {
        SDL_Event e;
        Uint64 now;
        const unsigned char *rgb = NULL;
        int steps = 0, open_menu = 0, rew, ff;
        unsigned char b1, b2;
        Uint32 tick = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: run = 0; break;
            case SDL_APP_WILLENTERBACKGROUND: eeprom_save(); audio_pause(1); break;
            case SDL_APP_DIDENTERFOREGROUND:
                last = SDL_GetPerformanceCounter(); acc = 0;
                if (!in_menu && !paused) audio_pause(0);
                break;
            case SDL_CONTROLLERDEVICEADDED: pad_add(e.cdevice.which); break;
            case SDL_CONTROLLERDEVICEREMOVED: pad_remove(e.cdevice.which); break;
            /* mando o teclado: los controles tactiles se esconden hasta que se vuelva a tocar la pantalla
               (las teclas que mandan los propios controles tactiles en el menu llevan windowID TOUCH_KEY) */
            case SDL_CONTROLLERBUTTONDOWN: touch_hide(); break;
            case SDL_CONTROLLERAXISMOTION: if (e.caxis.value > 20000 || e.caxis.value < -20000) touch_hide(); break;
            case SDL_KEYDOWN: if (e.key.windowID != TOUCH_KEY && e.key.keysym.sym != SDLK_AC_BACK) touch_hide(); break;
            }
            touch_event(&e, ren);
            if (in_menu) {
                int r = menu_event(&e);
                if (e.type == SDL_FINGERDOWN && menu_at_launcher()) {   /* launcher: el toque va a los botones */
                    int ow, oh;
                    SDL_GetRendererOutputSize(ren, &ow, &oh);
                    r = menu_launch_tap(launcher_hit(ren, (int)(e.tfinger.x * ow), (int)(e.tfinger.y * oh)));
                    touch_release_all();
                } else if (e.type == SDL_FINGERDOWN && !touch_on_control(ren, e.tfinger.x, e.tfinger.y)) {
                    SDL_Rect md; int ow, oh;
                    menu_rect(ren, &md); SDL_GetRendererOutputSize(ren, &ow, &oh);
                    r = menu_tap((int)((e.tfinger.x * ow - md.x) * menu_cw / md.w), (int)((e.tfinger.y * oh - md.y) * menu_ch / md.h));
                }
                if (r == MR_CHANGED) {
                    apply_config(); config_save(config_path);
                    if (last_raw) upload_frame(nb2_redraw());   /* que el juego de detras del menu (y la vista previa) ya salga con lo nuevo */
                }
                else if (r == MR_TITLE) {   /* volver a la pantalla de inicio: se reinicia la recreativa */
                    eeprom_save();
                    nb2_reset();
                    rw_clear(); paused = 0; start_seq[0] = start_seq[1] = -1; real_game[0] = real_game[1] = 0;
                    ach_reset(); vibration_reset();
                    menu_open(0); started = 0; last = SDL_GetPerformanceCounter(); acc = 0;   /* al menu de inicio, con la portada */
                }
                else if (r == MR_SAVE || r == MR_LOAD) {
                    char m[64];
                    int ok = r == MR_SAVE ? save_slot(menu_slot()) : load_slot(menu_slot());
                    if (r == MR_LOAD && ok) {
                        const unsigned char *m = nb2_mem();
                        rw_clear(); paused = 0; start_seq[0] = start_seq[1] = -1; ach_reset();
                        /* una partida guardada es de una partida de verdad si tiene la lista de rivales */
                        for (int p = 0; p < 2; p++) real_game[p] = (m[RAM_PLAYING] & (1 << p)) && (m[0x20807d] | m[0x20807f]);
                        vibration_reset();
                    }
                    snprintf(m, sizeof m, ok ? (r == MR_SAVE ? TR("Partida guardada en la ranura %d", "Game saved to slot %d")
                                                              : TR("Partida cargada de la ranura %d", "Game loaded from slot %d"))
                                             : (r == MR_SAVE ? TR("No se pudo guardar en la ranura %d", "Could not save to slot %d")
                                                              : TR("La ranura %d no vale para esta versión", "Slot %d is not valid for this version")),
                             menu_slot() + 1);
                    osd(m);
                    if (ok) r = MR_PLAY;
                }
                if (r == MR_PLAY) {
                    in_menu = 0; guard = 15; started = 1; last = SDL_GetPerformanceCounter(); acc = 0;
                    prev_menu = 1; prev_start[0] = prev_start[1] = 1;   /* que el boton que cierra el menu no cuente */
                    if (!paused) audio_pause(0);
                } else if (r == MR_QUIT) run = 0;
            } else if (e.type == SDL_KEYDOWN && !e.key.repeat &&
                       (e.key.keysym.sym == SDLK_ESCAPE || e.key.keysym.sym == SDLK_AC_BACK)) open_menu = 1;
        }

        touch_now = touch_state();
        if (in_menu) {
            SDL_Rect d, ms, pv;
            int px, py, pw, ph;
            if (menu_at_launcher()) touch_prev = touch_now;   /* en el launcher no hay controles tactiles: se tocan los botones */
            else touch_menu_keys(touch_now, &touch_prev);
            menu_tick((int)(tick - tick_prev));
            tick_prev = tick;
            /* al arrancar, la demo de la recreativa sigue andando (sin sonido) para la vista previa de Pantalla */
            if (!started) {
                now = SDL_GetPerformanceCounter();
                acc += (double)(now - last) / (double)freq; last = now;
                if (acc > period * 3) acc = period * 3;
                while (acc >= period) {
                    const unsigned char *f;
                    nb2_set_inputs(0, 0, 0);
                    f = nb2_frame(); acc -= period;
                    upload_frame(f);
                }
            }
            menu_rect(ren, &d);
            menu_render(menu_px);
            ms = (SDL_Rect){0, 0, menu_cw, menu_ch};
            SDL_UpdateTexture(mtex, &ms, menu_px, menu_cw * 4);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            if (started) {   /* en el menu de partida se ve el juego detras, oscurecido */
                draw_game(ren);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 6, 6, 10, 170);
                SDL_RenderFillRect(ren, NULL);
            } else launcher_draw(ren, menu_launch_sel(), !menu_at_launcher(), tick);   /* al arrancar: el launcher */
            if (!menu_at_launcher()) SDL_RenderCopy(ren, mtex, &ms, &d);
            if (menu_preview_rect(&px, &py, &pw, &ph)) {   /* vista previa con el formato, filtro, colores y bandas elegidos */
                pv.x = d.x + px * d.w / menu_cw; pv.y = d.y + py * d.h / menu_ch;
                pv.w = pw * d.w / menu_cw; pv.h = ph * d.h / menu_ch;
                SDL_RenderSetClipRect(ren, &pv);
                draw_game_in(ren, pv);
                SDL_RenderSetClipRect(ren, NULL);
            }
            if (!menu_at_launcher()) { touch_set_alpha(110); touch_render(ren); touch_set_alpha(255); }
            /* pruebas (escritorio): OFX_MENUSHOT=archivo.ppm guarda el menu; OFX_MENUKEYS="d,d,o,r" (u d l r o=aceptar b=volver)
               se pulsan antes; OFX_MENUWAIT=n vueltas antes de la captura (por defecto 90) */
            if (getenv("OFX_MENUSHOT")) {
                static int mn;
                int wait = getenv("OFX_MENUWAIT") ? atoi(getenv("OFX_MENUWAIT")) : 90;
                if (++mn == 10 && getenv("OFX_MENUKEYS")) {
                    for (const char *k = getenv("OFX_MENUKEYS"); *k; k++) {
                        int m = *k == 'u' ? MI_UP : *k == 'd' ? MI_DOWN : *k == 'l' ? MI_LEFT : *k == 'r' ? MI_RIGHT : *k == 'o' ? MI_OK : *k == 'b' ? MI_BACK : -1;
                        if (m >= 0) { int rr = menu_input(m); if (rr == MR_CHANGED) apply_config(); }
                    }
                }
                if (mn == wait) {
                    int w, h; unsigned char *px; FILE *f;
                    SDL_GetRendererOutputSize(ren, &w, &h);
                    if ((px = malloc((size_t)w * h * 3)) && SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_RGB24, px, w * 3) == 0 &&
                        (f = fopen(getenv("OFX_MENUSHOT"), "wb"))) { fprintf(f, "P6 %d %d 255\n", w, h); fwrite(px, 1, (size_t)w * h * 3, f); fclose(f); }
                    free(px); run = 0;
                }
            }
            SDL_RenderPresent(ren);
            continue;
        }
        tick_prev = tick;

        /* botones especiales: se miran en todos los mandos */
        {
            int mb = any_pad(ACT_MENU) || (touch_now & TOUCH_MENU);
            if (mb && !prev_menu) open_menu = 1;
            prev_menu = mb;
        }
        touch_prev = touch_now;
        if (open_menu) {
            in_menu = 1; started = 1;
            audio_pause(1);
            eeprom_save();
            menu_open(1);
            continue;
        }
        b1 = (unsigned char)(pad_bits(pads[0]) | key_bits() | (touch_now & 0xff));
        b2 = pad_bits(pads[1]);
        if (guard) b1 = b2 = 0;
        for (int p = 0; p < 2; p++) {
            int st = ((p ? b2 : b1) & NB2_START) != 0;
            if (st && !prev_start[p]) start_pressed(p);
            prev_start[p] = st;
        }
        b1 &= ~NB2_START; b2 &= ~NB2_START;
        rew = any_pad(ACT_REWIND) || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_BACKSPACE];
        ff = any_pad(ACT_FF) || SDL_GetKeyboardState(NULL)[SDL_SCANCODE_TAB];
        if (guard) { guard--; rew = ff = 0; }
        if (rew && paused) { paused = 0; audio_pause(0); }

        now = SDL_GetPerformanceCounter();
        acc += (double)(now - last) / (double)freq;
        last = now;
        if (acc > period * 4) acc = period * 4;
        if (frames) acc = period;   /* modo de prueba: sin esperar al reloj */

        if (rew) {
            /* rebobinar: un estado guardado por pantalla (unas 3 veces mas rapido que el juego) */
            if (!rewinding) { rewinding = 1; audio_pause(1); }
            ach_reset();
            if (rw_pop()) {
                nb2_set_inputs(0, 0, 0);
                rgb = nb2_frame();
                track_players();
                vibration_reset();
            }
            acc = 0;
            snprintf(osd_msg, sizeof osd_msg, "%s", rw_count ? TR("« Rebobinando", "« Rewinding") : TR("« No se puede rebobinar más", "« Can't rewind any further"));
            osd_until = tick + 300;
        } else if (paused) {
            acc = 0;
            SDL_Delay(5);
        } else {
            int n_ff = ff ? 4 : 0;
            if (rewinding) { rewinding = 0; audio_pause(0); acc = 0; }
            if (ff) { acc = period; snprintf(osd_msg, sizeof osd_msg, "%s", TR("» Avance rápido", "» Fast forward")); osd_until = tick + 300; }
            while ((acc >= period && steps < 3) || (n_ff && steps < n_ff)) {
                nb2_set_inputs(b1 | start_step(0), b2 | start_step(1), 0);
                nb2_set_render(!n_ff || steps == n_ff - 1);
                rgb = nb2_frame();
                track_players();
                /* de momento los trucos y los packs de armas no impiden los logros (para poder probarlos); solo Facil */
                ach_frame(nb2_mem(), cfg.difficulty != DIF_EASY);
                {
                    int a = ach_pop();
                    if (a >= 0) {   /* aviso de logro: 4 s */
                        snprintf(osd_msg, sizeof osd_msg, "%s %s", TR("Logro:", "Achievement:"), ach_title(a));
                        osd_until = SDL_GetTicks() + 4000;
                    }
                }
                if (!n_ff) { audio_push(); vibration_frame(b1, b2); } else vibration_reset();
                if (acc >= period) acc -= period;
                steps++; count++;
                if (++emu_frames % RW_EVERY == 0) rw_push();
                if (count % 600 == 0) eeprom_save();
            }
            nb2_set_render(1);
            if (n_ff) acc = 0;
        }
        if (rgb) upload_frame(rgb);
        if (rgb || paused || !SDL_TICKS_PASSED(tick, osd_until)) {
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            draw_game(ren);
            if (paused || !SDL_TICKS_PASSED(tick, osd_until)) {
                SDL_Rect d;
                SDL_Rect ms;
                menu_rect(ren, &d);
                menu_render_osd(menu_px, paused, SDL_TICKS_PASSED(tick, osd_until) ? NULL : osd_msg);
                ms = (SDL_Rect){0, 0, menu_cw, menu_ch};
                SDL_UpdateTexture(mtex, &ms, menu_px, menu_cw * 4);
                SDL_RenderCopy(ren, mtex, &ms, &d);
            }
            touch_render(ren);
            SDL_RenderPresent(ren);
        } else SDL_Delay(1);
        if (frames && count >= frames) {
            /* modo de prueba (escritorio): OFX_SHOT=archivo.ppm guarda lo que hay en pantalla */
            const char *shot = getenv("OFX_SHOT");
            if (shot) {
                int w, h; unsigned char *px; FILE *f;
                SDL_GetRendererOutputSize(ren, &w, &h);
                SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
                SDL_RenderClear(ren);
                draw_game(ren);
                touch_render(ren);
                if ((px = malloc((size_t)w * h * 3)) && SDL_RenderReadPixels(ren, NULL, SDL_PIXELFORMAT_RGB24, px, w * 3) == 0 &&
                    (f = fopen(shot, "wb"))) {
                    fprintf(f, "P6 %d %d 255\n", w, h); fwrite(px, 1, (size_t)w * h * 3, f); fclose(f);
                }
                free(px);
            }
            run = 0;
        }
    }
    eeprom_save();
    if (haptic) SDL_HapticClose(haptic);
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    SDL_Quit();
    exit(0);    /* en Android cierra tambien el proceso */
}
