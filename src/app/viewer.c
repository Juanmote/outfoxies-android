/* Visor de la galeria y del manual. Ver viewer.h.
   Mando: izquierda/derecha o L1/R1 pasan, A acerca (dos pasos y vuelve), con zoom la cruceta y el stick mueven la
   imagen, X cambia de pack, B cierra. Tactil: deslizar o tocar los bordes pasa, doble toque o pellizco acercan,
   arrastrar mueve. El pie y los botones van en el borde de abajo y se ocultan a los 2 s. */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <sys/stat.h>
#include "viewer.h"
#include "menu.h"
#include "miniz.h"
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#include "stb_image.h"

#define VIEW_MAX 256
#define OW 480                   /* ancho del lienzo de los textos (el alto sigue la forma de la pantalla) */
static struct {
    int kind, prompt, waiting;   /* abierto (VIEW_*), sin pack: pregunta (VIEW_*), esperando al selector (VIEW_*) */
    time_t wait_since;
    mz_zip_archive zip; int zip_open;
    int n, entry[VIEW_MAX]; char name[VIEW_MAX][80];
    int cur;
    SDL_Texture *tex; int tw, th;
    double zoom, cx, cy;
    Uint32 hud_until; int hud_seen;
    int fingers, moved; float down_x, down_y; Uint32 last_tap;
    SDL_Texture *ui; uint32_t *uipx; int uh;
    Uint32 poll;
} V;

static void pack_path(int kind, char *out, size_t n) {
    snprintf(out, n, "%s/%s", menu_save_dir(), kind == VIEW_MANUAL ? "manual.zip" : "galeria.zip");
}
int viewer_active(void) { return V.kind || V.prompt; }
static void hud(void) { V.hud_until = SDL_GetTicks() + 2000; }

static void set_picture(SDL_Renderer *r, unsigned char *data, size_t size) {
    int w = 0, h = 0, c, maxs = 4096;
    SDL_RendererInfo ri;
    unsigned char *px;
    if (V.tex) { SDL_DestroyTexture(V.tex); V.tex = NULL; }
    px = data ? stbi_load_from_memory(data, (int)size, &w, &h, &c, 4) : NULL;
    if (!px) return;
    if (SDL_GetRendererInfo(r, &ri) == 0 && ri.max_texture_width > 0) maxs = ri.max_texture_width;
    while (w > maxs || h > maxs) {           /* mas grande de lo que admite la GPU: a la mitad */
        int nw = w / 2, nh = h / 2;
        for (int y = 0; y < nh; y++) for (int x = 0; x < nw; x++) memcpy(px + (y * nw + x) * 4, px + (y * 2 * w + x * 2) * 4, 4);
        w = nw; h = nh;
    }
    V.tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
    if (V.tex) {
        SDL_UpdateTexture(V.tex, NULL, px, w * 4);
        SDL_SetTextureBlendMode(V.tex, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(V.tex, SDL_ScaleModeLinear);
    }
    V.tw = w; V.th = h;
    stbi_image_free(px);
}
static void show(SDL_Renderer *r, int i) {
    size_t size = 0; void *d;
    if (!V.n) return;
    V.cur = (i + V.n) % V.n;
    V.zoom = 1; V.cx = V.cy = 0.5;
    d = mz_zip_reader_extract_to_heap(&V.zip, (mz_uint)V.entry[V.cur], &size, 0);
    set_picture(r, d, size);
    if (d) mz_free(d);
    hud();
}
static void close_zip(void) { if (V.zip_open) mz_zip_reader_end(&V.zip); V.zip_open = 0; V.n = 0; }
static void viewer_close(void) {
    close_zip();
    if (V.tex) SDL_DestroyTexture(V.tex);
    V.tex = NULL; V.kind = V.prompt = V.waiting = 0; V.fingers = 0;
}
static int name_cmp(const void *a, const void *b) { return strcmp(V.name[*(const int *)a], V.name[*(const int *)b]); }
static int load_pack(int kind) {
    char p[700]; int total, order[VIEW_MAX], n = 0;
    static int e2[VIEW_MAX]; static char n2[VIEW_MAX][80];
    close_zip();
    pack_path(kind, p, sizeof p);
    memset(&V.zip, 0, sizeof V.zip);
    if (!mz_zip_reader_init_file(&V.zip, p, 0)) return 0;
    V.zip_open = 1;
    total = (int)mz_zip_reader_get_num_files(&V.zip);
    for (int i = 0; i < total && n < VIEW_MAX; i++) {
        char nm[512]; const char *base, *dot;
        if (mz_zip_reader_is_file_a_directory(&V.zip, i)) continue;
        mz_zip_reader_get_filename(&V.zip, i, nm, sizeof nm);
        base = strrchr(nm, '/'); base = base ? base + 1 : nm;
        dot = strrchr(base, '.');
        if (!dot || base[0] == '.') continue;
        if (SDL_strcasecmp(dot, ".jpg") && SDL_strcasecmp(dot, ".jpeg") && SDL_strcasecmp(dot, ".png")) continue;
        snprintf(V.name[n], sizeof V.name[n], "%.*s", (int)(dot - base), base);
        V.entry[n] = i; order[n] = n; n++;
    }
    qsort(order, (size_t)n, sizeof order[0], name_cmp);
    for (int i = 0; i < n; i++) { e2[i] = V.entry[order[i]]; memcpy(n2[i], V.name[order[i]], 80); }
    memcpy(V.entry, e2, sizeof(int) * (size_t)n); memcpy(V.name, n2, 80 * (size_t)n);
    V.n = n;
    if (!n) close_zip();
    return n > 0;
}
void viewer_open(SDL_Renderer *r, int kind) {
    V.prompt = 0; V.waiting = 0; V.fingers = 0;
    if (load_pack(kind)) { V.kind = kind; show(r, 0); }
    else { V.kind = 0; V.prompt = kind; }
}
static void pick_file(int kind) {
    char p[700]; struct stat st;
    pack_path(kind, p, sizeof p);
    V.wait_since = stat(p, &st) == 0 ? st.st_mtime : 0;
    V.waiting = kind; V.poll = SDL_GetTicks();
#ifdef __ANDROID__
    SDL_AndroidSendMessage(0x8007, kind);   /* GameActivity abre el selector y copia el .zip a la carpeta de la app */
#endif
}
/* esperando al selector: cuando el archivo aparece (o cambia) y se puede abrir, se ve */
static void check_waiting(SDL_Renderer *r) {
    char p[700]; struct stat st; int kind = V.waiting;
    if (!kind || SDL_GetTicks() - V.poll < 500) return;
    V.poll = SDL_GetTicks();
    pack_path(kind, p, sizeof p);
    if (stat(p, &st) != 0 || st.st_mtime == V.wait_since) return;
    if (load_pack(kind)) { V.waiting = 0; V.prompt = 0; V.kind = kind; show(r, 0); }
}
void viewer_resume(SDL_Renderer *r) { V.fingers = 0; check_waiting(r); }

static SDL_Rect pic_rect(int ww, int wh) {
    SDL_Rect q = {0, 0, 0, 0};
    double s, hx, hy;
    if (!V.tw || !V.th) return q;
    s = (double)ww / V.tw < (double)wh / V.th ? (double)ww / V.tw : (double)wh / V.th;
    s *= V.zoom;
    q.w = (int)(V.tw * s + 0.5); q.h = (int)(V.th * s + 0.5);
    hx = (double)ww / q.w / 2; hy = (double)wh / q.h / 2;
    V.cx = hx >= 0.5 ? 0.5 : V.cx < hx ? hx : V.cx > 1 - hx ? 1 - hx : V.cx;
    V.cy = hy >= 0.5 ? 0.5 : V.cy < hy ? hy : V.cy > 1 - hy ? 1 - hy : V.cy;
    q.x = (int)(ww / 2.0 - V.cx * q.w); q.y = (int)(wh / 2.0 - V.cy * q.h);
    return q;
}
static void zoom_step(void) {
    V.zoom = V.zoom < 1.4 ? 1.6 : V.zoom < 2.2 ? 2.4 : 1;
    if (V.zoom == 1) V.cx = V.cy = 0.5;
}
static const char *title(int kind) { return kind == VIEW_MANUAL ? TR("MANUAL", "MANUAL") : TR("GALERÍA", "GALLERY"); }

/* lienzo de los textos: OW de ancho y el alto de la forma de la pantalla */
static void ui_begin(SDL_Renderer *r, int ww, int wh) {
    int uh = OW * wh / (ww ? ww : 1);
    if (uh < 200) uh = 200;
    if (uh > 600) uh = 600;
    if (uh != V.uh || !V.ui) {
        if (V.ui) SDL_DestroyTexture(V.ui);
        free(V.uipx);
        V.uh = uh; V.uipx = malloc((size_t)OW * uh * 4);
        V.ui = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, OW, uh);
        if (V.ui) { SDL_SetTextureBlendMode(V.ui, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(V.ui, SDL_ScaleModeNearest); }
    }
    memset(V.uipx, 0, (size_t)OW * V.uh * 4);
}
static void ui_end(SDL_Renderer *r) {
    if (!V.ui) return;
    SDL_UpdateTexture(V.ui, NULL, V.uipx, OW * 4);
    SDL_RenderCopy(r, V.ui, NULL, NULL);
}
#define BOX(x, y, w, h, c, a) menu_ext_rect(V.uipx, OW, V.uh, x, y, w, h, c, a)
#define TXT(x, y, s, c) menu_ext_text(V.uipx, OW, V.uh, x, y, s, c, 0)
#define TXTB(x, y, s, c) menu_ext_text(V.uipx, OW, V.uh, x, y, s, c, 1)
#define TW(s) menu_ext_text_w(s, 0)
#define TWB(s) menu_ext_text_w(s, 1)

void viewer_draw(SDL_Renderer *r) {
    int ww, wh;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    check_waiting(r);
    if (V.kind && V.tex) {
        SDL_Rect q = pic_rect(ww, wh);
        SDL_SetRenderDrawColor(r, 8, 6, 10, 255);
        SDL_RenderClear(r);
        SDL_RenderCopy(r, V.tex, NULL, &q);
    } else {   /* pregunta: sobre el launcher oscurecido */
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 190);
        SDL_RenderFillRect(r, NULL);
    }
    ui_begin(r, ww, wh);
    if (!V.ui || !V.uipx) return;
    if (V.prompt) {
        int bw = 464, bh = 132, bx = (OW - bw) / 2, by = (V.uh - bh) / 2;
        const char *t1 = V.waiting ? TR("Esperando el archivo...", "Waiting for the file...")
                                   : TR("No hay ningún pack instalado. Elige su archivo .zip:", "No pack installed. Choose its .zip file:");
        const char *t2 = V.prompt == VIEW_MANUAL ? "manual.zip" : "galeria.zip";
        char t3[120];
        BOX(bx, by, bw, bh, 0x140c18, 240);
        BOX(bx, by, bw, 2, 0xe8a030, 255); BOX(bx, by + bh - 2, bw, 2, 0xe8a030, 255);
        TXTB((OW - TWB(title(V.prompt))) / 2, by + 10, title(V.prompt), 0xffd040);
        TXT((OW - TW(t1)) / 2, by + 42, t1, 0xffffff);
        snprintf(t3, sizeof t3, TR("Se copia como %s en la carpeta de la app", "It is copied as %s into the app folder"), t2);
        TXT((OW - TW(t3)) / 2, by + 60, t3, 0xb8b0a8);
        BOX(bx + 20, by + 88, 170, 28, 0x8a1a1a, 255);
        BOX(bx + bw - 190, by + 88, 170, 28, 0x2a2430, 255);
        TXT(bx + 20 + (170 - TW(TR("A: Elegir archivo", "A: Choose file"))) / 2, by + 95, TR("A: Elegir archivo", "A: Choose file"), 0xffffff);
        TXT(bx + bw - 190 + (170 - TW(TR("B: Volver", "B: Back"))) / 2, by + 95, TR("B: Volver", "B: Back"), 0xffffff);
    } else if ((Sint32)(SDL_GetTicks() - V.hud_until) < 0) {
        char t[200]; const char *s = V.name[V.cur];
        const char *cl = TR("B  Cerrar", "B  Close"), *ch = TR("X  Cambiar pack", "X  Change pack");
        int w;
        while (*s >= '0' && *s <= '9') s++;
        while (*s == ' ' || *s == '-' || *s == '_') s++;
        snprintf(t, sizeof t, "%s   < %d/%d >", *s ? s : V.name[V.cur], V.cur + 1, V.n);
        w = TW(t) + 20;
        BOX((OW - w) / 2, V.uh - 22, w, 18, 0, 190); TXT((OW - w) / 2 + 10, V.uh - 20, t, 0xffffff);
        BOX(OW - TW(cl) - 20, V.uh - 22, TW(cl) + 14, 18, 0, 190); TXT(OW - TW(cl) - 13, V.uh - 20, cl, 0xffffff);
        BOX(6, V.uh - 22, TW(ch) + 14, 18, 0, 190); TXT(13, V.uh - 20, ch, 0xb8b0a8);
    }
    ui_end(r);
}

static void touch(SDL_Renderer *r, SDL_Event *e, int ww, int wh) {
    float x = e->tfinger.x * ww, y = e->tfinger.y * wh, ux, uy, ddx;
    Uint32 now;
    if (e->type == SDL_FINGERDOWN) {
        V.fingers++;
        V.hud_seen = (Sint32)(SDL_GetTicks() - V.hud_until) < 0;   /* los botones solo valen mientras se ven */
        hud();
        if (V.fingers == 1) { V.down_x = x; V.down_y = y; V.moved = 0; } else V.moved = 1;
        return;
    }
    if (e->type == SDL_FINGERMOTION) {
        if (V.fingers != 1) return;
        if (fabsf(x - V.down_x) > ww / 40.0f || fabsf(y - V.down_y) > wh / 40.0f) V.moved = 1;
        if (V.zoom > 1.01) {
            SDL_Rect q = pic_rect(ww, wh);
            if (q.w) V.cx -= (double)e->tfinger.dx * ww / q.w;
            if (q.h) V.cy -= (double)e->tfinger.dy * wh / q.h;
        }
        return;
    }
    if (!V.fingers) return;      /* el dedo que abrio el visor no cuenta */
    if (--V.fingers) return;
    ddx = x - V.down_x;
    if (V.moved) {
        if (V.zoom <= 1.01 && fabsf(ddx) > ww / 8.0f && V.n) show(r, V.cur + (ddx < 0 ? 1 : -1));
        return;
    }
    ux = x * OW / ww; uy = y * V.uh / wh;
    if (V.hud_seen && uy > V.uh - 26 && ux > OW - 110) { viewer_close(); return; }
    if (V.hud_seen && uy > V.uh - 26 && ux < 130) { pick_file(V.kind); return; }
    now = SDL_GetTicks();
    if (now - V.last_tap < 350) { V.last_tap = 0; zoom_step(); return; }
    V.last_tap = now;
    if (V.zoom <= 1.01 && V.n) {
        if (x < ww * 0.2f) show(r, V.cur - 1);
        else if (x > ww * 0.8f) show(r, V.cur + 1);
    }
}

int viewer_event(SDL_Renderer *r, SDL_Event *e) {
    int ww, wh, prev = 0, next = 0, ok = 0, back = 0, other = 0, zoomed = V.zoom > 1.01;
    double mx = 0, my = 0;
    if (!viewer_active()) return 0;
    SDL_GetRendererOutputSize(r, &ww, &wh);
    switch (e->type) {
    case SDL_FINGERDOWN: case SDL_FINGERUP: case SDL_FINGERMOTION:
        if (V.prompt) {
            if (e->type != SDL_FINGERDOWN) return 1;
            if (e->tfinger.x < 0.5f) ok = 1; else back = 1;
            break;
        }
        touch(r, e, ww, wh);
        return 1;
    case SDL_MULTIGESTURE:
        if (V.kind && e->mgesture.numFingers == 2) {
            V.zoom *= 1.0 + e->mgesture.dDist * 2.5;
            if (V.zoom < 1) V.zoom = 1;
            if (V.zoom > 4) V.zoom = 4;
        }
        return 1;
    case SDL_KEYDOWN: {
        SDL_Keycode k = e->key.keysym.sym;
        prev = k == SDLK_LEFT && !zoomed; next = k == SDLK_RIGHT && !zoomed;
        if (zoomed) { mx = (k == SDLK_RIGHT) - (k == SDLK_LEFT); my = (k == SDLK_DOWN) - (k == SDLK_UP); }
        ok = k == SDLK_RETURN || k == SDLK_z || k == SDLK_SPACE;
        back = k == SDLK_ESCAPE || k == SDLK_AC_BACK || k == SDLK_x || k == SDLK_BACKSPACE;
        other = k == SDLK_c;
        break;
    }
    case SDL_CONTROLLERBUTTONDOWN: {
        int b = e->cbutton.button;
        prev = (b == SDL_CONTROLLER_BUTTON_DPAD_LEFT && !zoomed) || b == SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        next = (b == SDL_CONTROLLER_BUTTON_DPAD_RIGHT && !zoomed) || b == SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        if (zoomed) {
            mx = (b == SDL_CONTROLLER_BUTTON_DPAD_RIGHT) - (b == SDL_CONTROLLER_BUTTON_DPAD_LEFT);
            my = (b == SDL_CONTROLLER_BUTTON_DPAD_DOWN) - (b == SDL_CONTROLLER_BUTTON_DPAD_UP);
        }
        ok = b == SDL_CONTROLLER_BUTTON_A || b == SDL_CONTROLLER_BUTTON_START;
        back = b == SDL_CONTROLLER_BUTTON_B || b == SDL_CONTROLLER_BUTTON_BACK;
        other = b == SDL_CONTROLLER_BUTTON_X || b == SDL_CONTROLLER_BUTTON_Y;
        break;
    }
    case SDL_CONTROLLERAXISMOTION: {
        static int dir;
        int v = e->caxis.value, d = 0;
        if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTX) {
            d = v > 20000 ? 1 : v < -20000 ? -1 : 0;
            if (zoomed && d) mx = d * 0.5;
            else if (d && d != dir) { if (d > 0) next = 1; else prev = 1; }
            dir = d;
        } else if (e->caxis.axis == SDL_CONTROLLER_AXIS_LEFTY && zoomed) {
            d = v > 20000 ? 1 : v < -20000 ? -1 : 0;
            my = d * 0.5;
        }
        break;
    }
    default:
        return e->type == SDL_KEYUP || e->type == SDL_CONTROLLERBUTTONUP;
    }
    if (V.prompt) {
        if (ok && !V.waiting) pick_file(V.prompt);
        else if (back) viewer_close();
        return 1;
    }
    if (mx || my) { V.cx += mx * 0.08 / V.zoom; V.cy += my * 0.08 / V.zoom; hud(); }
    if (prev) show(r, V.cur - 1);
    else if (next) show(r, V.cur + 1);
    else if (ok) { zoom_step(); hud(); }
    else if (other) pick_file(V.kind);
    else if (back) viewer_close();
    return 1;
}
