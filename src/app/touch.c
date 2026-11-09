/* Controles tactiles.
   Graficos: la cruceta y los botones de la imagen que paso el usuario (tools/make_touch.py los mete en touch_gfx.h).
   Colocacion (cfg.touch: 0 no, 1 pequenos, 2 medianos, 3 grandes; el tamano va segun el alto de la pantalla):
   - cruceta abajo a la izquierda;
   - abajo a la derecha disparo (boton 1) y salto (boton 2), el de salto en la esquina y el de disparo un poco mas
     arriba, en arco, para el pulgar derecho;
   - START y MENU mas pequenos, arriba a la derecha (no tapan el marcador, que va en el centro).
   Al tocar, se ilumina lo que se pulsa: en la cruceta solo el brazo de la direccion (o los dos en diagonal).
   Se esconden en cuanto se usa un mando o el teclado y vuelven al tocar la pantalla. */
#include <math.h>
#include <string.h>
#include "touch.h"
#include "menu.h"
#include "nb2.h"
#include "miniz.h"
#include "touch_gfx.h"

enum { C_DPAD, C_B1, C_B2, C_START, C_MENU, C_COUNT };
static SDL_Texture *tx[C_COUNT];
static int visible = -1;                   /* -1: aun sin decidir (se ve si no hay mandos al arrancar) */
#define MAX_F 10
static struct { SDL_FingerID id; int on, ctl; float x, y; } fing[MAX_F];
static SDL_FRect rc[C_COUNT];             /* donde se dibuja cada control */
static int lay_w, lay_h, lay_size;

static SDL_Texture *load(SDL_Renderer *r, const unsigned char *z, size_t zn, int w, int h) {
    mz_ulong n = (mz_ulong)w * h * 4;
    unsigned char *px = SDL_malloc(n);
    SDL_Texture *t = NULL;
    if (px && mz_uncompress(px, &n, z, (mz_ulong)zn) == MZ_OK) {
        t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);   /* bytes R,G,B,A */
        if (t) {
            SDL_UpdateTexture(t, NULL, px, w * 4);
            SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
            SDL_SetTextureScaleMode(t, SDL_ScaleModeLinear);
        }
    }
    SDL_free(px);
    return t;
}
void touch_init(SDL_Renderer *r) {
    tx[C_DPAD] = load(r, tg_cruceta, sizeof tg_cruceta, TG_CRUCETA_W, TG_CRUCETA_H);
    tx[C_B1] = load(r, tg_disparo, sizeof tg_disparo, TG_DISPARO_W, TG_DISPARO_H);
    tx[C_B2] = load(r, tg_salto, sizeof tg_salto, TG_SALTO_W, TG_SALTO_H);
    tx[C_START] = load(r, tg_start, sizeof tg_start, TG_START_W, TG_START_H);
    tx[C_MENU] = load(r, tg_menu, sizeof tg_menu, TG_MENU_W, TG_MENU_H);
}
static void layout(SDL_Renderer *r) {
    int w, h;
    SDL_GetRendererOutputSize(r, &w, &h);
    if (w == lay_w && h == lay_h && cfg.touch == lay_size) return;
    lay_w = w; lay_h = h; lay_size = cfg.touch;
    {
        static const float dsz[4] = {0, 0.30f, 0.38f, 0.46f};
        float D = h * dsz[cfg.touch < 1 ? 1 : cfg.touch > 3 ? 3 : cfg.touch];
        float B = D * 0.42f, S = B * 0.62f, m = h * 0.035f;
        rc[C_DPAD] = (SDL_FRect){m, h - m - D, D, D};
        rc[C_B2] = (SDL_FRect){w - m - B, h - m - B, B, B};
        rc[C_B1] = (SDL_FRect){w - m - B * 2.15f, h - m - B * 1.45f, B, B};
        rc[C_MENU] = (SDL_FRect){w - m - S, m, S, S};
        rc[C_START] = (SDL_FRect){w - m - S * 2.2f, m, S, S};
    }
}
/* que control hay bajo el dedo (para los botones, un circulo algo mayor que el dibujo) */
static int hit(float x, float y) {
    for (int c = C_B1; c < C_COUNT; c++) {
        float cx = rc[c].x + rc[c].w / 2, cy = rc[c].y + rc[c].h / 2, rr = rc[c].w * 0.62f;
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < rr * rr) return c;
    }
    {
        float cx = rc[C_DPAD].x + rc[C_DPAD].w / 2, cy = rc[C_DPAD].y + rc[C_DPAD].h / 2, rr = rc[C_DPAD].w * 0.62f;
        if ((x - cx) * (x - cx) + (y - cy) * (y - cy) < rr * rr) return C_DPAD;
    }
    return -1;
}
/* direccion de la cruceta: 8 sectores de 45 grados; en el centro, nada */
static unsigned dpad_bits(float x, float y) {
    float cx = rc[C_DPAD].x + rc[C_DPAD].w / 2, cy = rc[C_DPAD].y + rc[C_DPAD].h / 2;
    float dx = x - cx, dy = y - cy, ax = fabsf(dx), ay = fabsf(dy);
    unsigned b = 0;
    if (ax * ax + ay * ay < (rc[C_DPAD].w * 0.12f) * (rc[C_DPAD].w * 0.12f)) return 0;
    if (ax > ay * 0.414f) b |= dx > 0 ? NB2_RIGHT : NB2_LEFT;
    if (ay > ax * 0.414f) b |= dy > 0 ? NB2_DOWN : NB2_UP;
    return b;
}
int touch_event(const SDL_Event *e, SDL_Renderer *r) {
    int i;
    float x, y;
    if (e->type != SDL_FINGERDOWN && e->type != SDL_FINGERMOTION && e->type != SDL_FINGERUP) return 0;
    if (cfg.touch == 0) return 1;
    layout(r);
    x = e->tfinger.x * lay_w; y = e->tfinger.y * lay_h;
    for (i = 0; i < MAX_F; i++) if (fing[i].on && fing[i].id == e->tfinger.fingerId) break;
    if (e->type == SDL_FINGERDOWN) {
        visible = 1;
        if (i == MAX_F) for (i = 0; i < MAX_F && fing[i].on; i++) {}
        if (i == MAX_F) return 1;
        fing[i].on = 1; fing[i].id = e->tfinger.fingerId; fing[i].ctl = hit(x, y);
    } else if (i == MAX_F) return 1;
    if (e->type == SDL_FINGERUP) { fing[i].on = 0; return 1; }
    fing[i].x = x; fing[i].y = y;
    /* la cruceta se queda con su dedo aunque se salga; entre botones se puede deslizar */
    if (e->type == SDL_FINGERMOTION && fing[i].ctl != C_DPAD) fing[i].ctl = hit(x, y) == C_DPAD ? -1 : hit(x, y);
    return 1;
}
static unsigned state_of(int c) {
    unsigned b = 0;
    for (int i = 0; i < MAX_F; i++) {
        if (!fing[i].on || fing[i].ctl != c) continue;
        switch (c) {
        case C_DPAD: b |= dpad_bits(fing[i].x, fing[i].y); break;
        case C_B1: b |= NB2_B1; break;
        case C_B2: b |= NB2_B2; break;
        case C_START: b |= NB2_START; break;
        case C_MENU: b |= TOUCH_MENU; break;
        }
    }
    return b;
}
unsigned touch_state(void) {
    unsigned b = 0;
    if (cfg.touch == 0) return 0;
    for (int c = 0; c < C_COUNT; c++) b |= state_of(c);
    /* direcciones opuestas a la vez (dos dedos en la cruceta) no tienen sentido */
    if ((b & (NB2_LEFT | NB2_RIGHT)) == (NB2_LEFT | NB2_RIGHT)) b &= ~(NB2_LEFT | NB2_RIGHT);
    if ((b & (NB2_UP | NB2_DOWN)) == (NB2_UP | NB2_DOWN)) b &= ~(NB2_UP | NB2_DOWN);
    return b;
}
void touch_hide(void) { visible = 0; touch_release_all(); }
void touch_show(void) { visible = 1; }
void touch_release_all(void) { memset(fing, 0, sizeof fing); }

void touch_render(SDL_Renderer *r) {
    unsigned b;
    if (cfg.touch == 0) return;
    if (visible < 0) visible = SDL_NumJoysticks() == 0;
    if (!visible) return;
    layout(r);
    b = touch_state();
    for (int c = 0; c < C_COUNT; c++) {
        int on = c == C_DPAD ? 0 : (state_of(c) != 0);
        if (!tx[c]) continue;
        SDL_SetTextureBlendMode(tx[c], SDL_BLENDMODE_BLEND);
        SDL_SetTextureAlphaMod(tx[c], on ? 235 : 150);
        SDL_RenderCopyF(r, tx[c], NULL, &rc[c]);
        if (on) {   /* iluminado: otra pasada sumando */
            SDL_SetTextureBlendMode(tx[c], SDL_BLENDMODE_ADD);
            SDL_SetTextureAlphaMod(tx[c], 120);
            SDL_RenderCopyF(r, tx[c], NULL, &rc[c]);
        }
    }
    if (tx[C_DPAD] && (b & 15)) {
        /* brazos de la cruceta en la textura de 320x320 */
        static const SDL_Rect arm[4] = {{219, 101, 101, 115}, {0, 101, 102, 115}, {102, 217, 117, 103}, {102, 0, 117, 102}};
        float k = rc[C_DPAD].w / TG_CRUCETA_W;
        SDL_SetTextureBlendMode(tx[C_DPAD], SDL_BLENDMODE_ADD);
        SDL_SetTextureAlphaMod(tx[C_DPAD], 150);
        for (int d = 0; d < 4; d++) if (b & (1u << d)) {   /* NB2_RIGHT, LEFT, DOWN, UP */
            SDL_FRect dst = {rc[C_DPAD].x + arm[d].x * k, rc[C_DPAD].y + arm[d].y * k, arm[d].w * k, arm[d].h * k};
            SDL_RenderCopyF(r, tx[C_DPAD], &arm[d], &dst);
            SDL_RenderCopyF(r, tx[C_DPAD], &arm[d], &dst);
        }
        SDL_SetTextureBlendMode(tx[C_DPAD], SDL_BLENDMODE_BLEND);
    }
}
