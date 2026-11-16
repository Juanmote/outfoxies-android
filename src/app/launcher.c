/* Launcher (pantalla de inicio). Ver launcher.h.
   Colocacion:
   - pantallas anchas (16:9, 16:10...): la portada llena la pantalla y los 10 botones van en una fila abajo, con JUGAR
     mas grande: OPCIONES CONTROLES PARTIDAS GALERIA [JUGAR] MANUAL ROPA LOGROS TRUCOS SALIR.
   - pantallas casi cuadradas (8:7, 4:3; LAUNCH_TWO_ROWS): dos filas, para que los botones no queden diminutos:
     PARTIDAS GALERIA [JUGAR] MANUAL LOGROS arriba y OPCIONES CONTROLES ROPA TRUCOS SALIR abajo. De fondo va la portada casi cuadrada
     (cover87), que llena la pantalla; se corta casi todo por abajo, donde estan los botones.
   Los botones son translucidos (BTN_ALPHA) para que se vea la portada detras. El elegido se ve opaco, algo mas grande
   y con brillo (late despacio). */
#define BTN_ALPHA 105
#include <math.h>
#include "launcher.h"
#include "menu.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"
#include "launcher_gfx.h"

typedef struct { SDL_Texture *t; int w, h; } img;
static img cover, cover87, panel[LB_COUNT], label[LANG_COUNT][LB_COUNT];
static SDL_Texture *fade;        /* degradado vertical de transparente a oscuro */
static int ready;

static img load(SDL_Renderer *r, const unsigned char *d, int n) {
    img m = {NULL, 0, 0};
    int c;
    unsigned char *px = stbi_load_from_memory(d, n, &m.w, &m.h, &c, 4);
    if (!px) return m;
    m.t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, m.w, m.h);   /* bytes R,G,B,A */
    if (m.t) {
        SDL_UpdateTexture(m.t, NULL, px, m.w * 4);
        SDL_SetTextureBlendMode(m.t, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(m.t, SDL_ScaleModeLinear);
    }
    stbi_image_free(px);
    return m;
}
int launcher_init(SDL_Renderer *r) {
#define L(a) load(r, a, (int)sizeof a)
    static Uint32 g[256];
    cover = L(lg_cover); cover87 = L(lg_cover87);
    for (int i = 0; i < LB_COUNT && i < LG_N; i++) {
        panel[i] = load(r, lg_panel[i], lg_panel_n[i]);
        label[LANG_ES][i] = load(r, lg_text_es[i], lg_text_es_n[i]);
        label[LANG_EN][i] = load(r, lg_text_en[i], lg_text_en_n[i]);
        label[LANG_PT][i] = load(r, lg_text_pt[i], lg_text_pt_n[i]);
        label[LANG_IT][i] = load(r, lg_text_it[i], lg_text_it_n[i]);
    }
#undef L
    for (int i = 0; i < 256; i++) g[i] = (Uint32)i << 24 | 0x06070c;
    fade = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, 256);
    if (fade) { SDL_UpdateTexture(fade, NULL, g, 4); SDL_SetTextureBlendMode(fade, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(fade, SDL_ScaleModeLinear); }
    ready = cover.t != NULL;
    for (int i = 0; i < LB_COUNT; i++) for (int l = 0; l < LANG_COUNT; l++) if (!panel[i].t || !label[l][i].t) ready = 0;
    return ready;
}

typedef struct {
    SDL_Rect b[LB_COUNT];           /* botones */
    SDL_Rect csrc, cdst;            /* portada */
    SDL_Rect fade;                  /* zona del degradado; por debajo, oscuro liso si solid */
    int fade_max, solid;
    img *img;                       /* que portada se usa */
} layout_t;
static void layout(int ww, int wh, layout_t *L) {
    const double pa = panel[0].w ? (double)panel[0].h / panel[0].w : 0.5;     /* alto / ancho de un panel */
    if (!LAUNCH_TWO_ROWS(ww, wh)) {
        /* una fila, medidas sobre una pantalla de 1920x1080 */
        double k = ww / 1920.0 < wh / 1080.0 ? ww / 1920.0 : wh / 1080.0;
        int bw = (int)(212 * k), pw = (int)(286 * k), gap = (int)(16 * k);    /* 7 botones */
        int total = bw * (LB_COUNT - 1) + pw + gap * (LB_COUNT - 1), x = (ww - total) / 2, cy = wh - (int)(70 * k);
        for (int i = 0; i < LB_COUNT; i++) {
            int w = i == LB_PLAY ? pw : bw, h = (int)(w * pa);
            L->b[i] = (SDL_Rect){x, cy - h / 2 - (i == LB_PLAY ? (int)(10 * k) : 0), w, h};
            x += w + gap;
        }
        /* la portada llena la pantalla; si la pantalla es mas ancha que el dibujo solo se corta por abajo (el logo entero) */
        L->csrc = (SDL_Rect){0, 0, cover.w, cover.h};
        if ((long)ww * cover.h > (long)wh * cover.w) L->csrc.h = (int)((long)cover.w * wh / ww);
        else { L->csrc.w = (int)((long)cover.h * ww / wh); L->csrc.x = (cover.w - L->csrc.w) / 2; }
        L->cdst = (SDL_Rect){0, 0, ww, wh};
        L->fade = (SDL_Rect){0, wh - (int)(220 * k), ww, (int)(220 * k)};
        L->fade_max = 120; L->solid = 0; L->img = &cover;
    } else {
        /* dos filas, medidas sobre una pantalla de 1240 de ancho */
        double k = ww / 1240.0 < wh / 930.0 ? ww / 1240.0 : wh / 930.0;
        int bw = (int)(200 * k), bh = (int)(bw * pa), pw = (int)(256 * k), ph = (int)(pw * pa);
        int gapb = (int)(24 * k), gapa = (int)(24 * k);
        int yb = wh - (int)(20 * k) - bh;                              /* fila de abajo */
        int ya = yb - (int)(12 * k) - ph;                              /* JUGAR */
        int totb = 4 * bw + 3 * gapb, tota = 2 * bw + pw + 2 * gapa, x = (ww - totb) / 2;
        static const int rowa[3] = {LB_GALLERY, LB_PLAY, LB_MANUAL};
        static const int rowb[4] = {LB_OPTIONS, LB_ACH, LB_SKINS, LB_QUIT};
        img *c = cover87.t ? &cover87 : &cover;
        for (int i = 0; i < 4; i++) { L->b[rowb[i]] = (SDL_Rect){x, yb, bw, bh}; x += bw + gapb; }
        x = (ww - tota) / 2;
        for (int i = 0; i < 3; i++) {
            if (rowa[i] == LB_PLAY) { L->b[LB_PLAY] = (SDL_Rect){x, ya, pw, ph}; x += pw + gapa; }
            else { L->b[rowa[i]] = (SDL_Rect){x, ya + (ph - bh) / 2, bw, bh}; x += bw + gapa; }
        }
        /* la portada casi cuadrada llena la pantalla. Si sobra alto, se corta sobre todo por abajo (ahi van los botones) y
           un poco por arriba; si sobra ancho, por los dos lados igual */
        L->img = c;
        L->csrc = (SDL_Rect){0, 0, c->w, c->h};
        if ((long)ww * c->h > (long)wh * c->w) { L->csrc.h = (int)((long)c->w * wh / ww); L->csrc.y = (c->h - L->csrc.h) / 5; }
        else { L->csrc.w = (int)((long)c->h * ww / wh); L->csrc.x = (c->w - L->csrc.w) / 2; }
        L->cdst = (SDL_Rect){0, 0, ww, wh};
        L->fade = (SDL_Rect){0, ya - (int)(70 * k), ww, wh - ya + (int)(70 * k)};
        L->fade_max = 130; L->solid = 0;
    }
}
void launcher_cover(SDL_Renderer *r, SDL_Rect area, int dim) {
    SDL_Rect s = {0, 0, cover.w, cover.h};
    if (!cover.t || area.w <= 0 || area.h <= 0) return;
    if ((long)area.w * cover.h > (long)area.h * cover.w) { s.h = (int)((long)cover.w * area.h / area.w); s.y = (cover.h - s.h) / 2; }
    else { s.w = (int)((long)cover.h * area.w / area.h); s.x = (cover.w - s.w) / 2; }
    SDL_SetTextureColorMod(cover.t, dim, dim, dim);
    SDL_RenderCopy(r, cover.t, &s, &area);
    SDL_SetTextureColorMod(cover.t, 255, 255, 255);
}
static SDL_Rect grow(SDL_Rect r, double f) {
    int dx = (int)(r.w * (f - 1) / 2), dy = (int)(r.h * (f - 1) / 2);
    return (SDL_Rect){r.x - dx, r.y - dy, r.w + 2 * dx, r.h + 2 * dy};
}
static void button(SDL_Renderer *r, int i, SDL_Rect d, int lum, int glow, int alpha) {
    img *t = &label[cfg.lang >= 0 && cfg.lang < LANG_COUNT ? cfg.lang : LANG_ES][i];
    SDL_Rect td;
    SDL_SetTextureColorMod(panel[i].t, lum, lum, lum);
    SDL_SetTextureAlphaMod(panel[i].t, alpha);          /* el cristal translucido; el rotulo siempre opaco para leerlo */
    SDL_RenderCopy(r, panel[i].t, NULL, &d);
    SDL_SetTextureAlphaMod(panel[i].t, 255);
    if (glow > 0) {                                   /* brillo: el mismo panel sumado encima */
        SDL_SetTextureBlendMode(panel[i].t, SDL_BLENDMODE_ADD);
        SDL_SetTextureAlphaMod(panel[i].t, glow);
        SDL_RenderCopy(r, panel[i].t, NULL, &d);
        SDL_SetTextureAlphaMod(panel[i].t, 255);
        SDL_SetTextureBlendMode(panel[i].t, SDL_BLENDMODE_BLEND);
    }
    td.w = (int)((long)t->w * d.w / panel[i].w); td.h = (int)((long)t->h * d.w / panel[i].w);
    td.x = d.x + (d.w - td.w) / 2; td.y = d.y + (d.h - td.h) / 2;
    SDL_SetTextureColorMod(t->t, lum, lum, lum);
    SDL_RenderCopy(r, t->t, NULL, &td);
}
void launcher_draw(SDL_Renderer *r, int sel, int dimmed, Uint32 ticks) {
    layout_t L; int ww, wh;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    SDL_SetRenderDrawColor(r, 6, 7, 12, 255);
    SDL_RenderClear(r);
    if (!ready) return;
    layout(ww, wh, &L);
    SDL_SetTextureColorMod(L.img->t, 255, 255, 255);
    SDL_RenderCopy(r, L.img->t, &L.csrc, &L.cdst);
    if (fade) {
        SDL_SetTextureAlphaMod(fade, L.fade_max);
        SDL_RenderCopy(r, fade, NULL, &L.fade);
    }
    if (L.solid && L.fade.y + L.fade.h < wh) {
        SDL_Rect s = {0, L.fade.y + L.fade.h, ww, wh - L.fade.y - L.fade.h};
        SDL_SetRenderDrawColor(r, 6, 7, 12, 255);
        SDL_RenderFillRect(r, &s);
    }
    for (int i = 0; i < LB_COUNT; i++) if (i != sel || dimmed) button(r, i, L.b[i], dimmed ? 150 : 225, 0, BTN_ALPHA);
    if (!dimmed && sel >= 0 && sel < LB_COUNT) {
        double beat = 0.5 + 0.5 * sin(ticks / 380.0);
        button(r, sel, grow(L.b[sel], 1.10), 255, 55 + (int)(60 * beat), 255);
    }
    if (dimmed) {                                       /* pagina abierta encima: todo oscurecido para que se lea */
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 6, 7, 12, 150);
        SDL_RenderFillRect(r, NULL);
    }
}
int launcher_hit(SDL_Renderer *r, int x, int y) {
    layout_t L; int ww, wh;
    if (!ready) return -1;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    layout(ww, wh, &L);
    for (int i = 0; i < LB_COUNT; i++) {
        SDL_Point p = {x, y};
        if (SDL_PointInRect(&p, &L.b[i])) return i;
    }
    return -1;
}
