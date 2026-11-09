/* The Outfoxies para Android y escritorio: ventana, mandos, sonido, menu de opciones y ritmo de juego. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "nb2.h"
#include "menu.h"

#define MAX_PADS 8

static SDL_GameController *pads[MAX_PADS];
static char eeprom_path[512], config_path[512], data_dir[512];
static unsigned char eeprom_last[NB2_EEPROM_SIZE];
static SDL_AudioDeviceID audio_dev;
static SDL_Texture *tex, *mtex, *scan;
static uint32_t menu_px[MENU_W * MENU_H];
static int tex_w = NB2_W;      /* ancho de la ultima imagen subida (288 o 384) */

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
static void game_rect(SDL_Renderer *r, SDL_Rect *d) {
    int w, h;
    SDL_GetRendererOutputSize(r, &w, &h);
    if (cfg.aspect == ASP_FULL) { d->x = d->y = 0; d->w = w; d->h = h; return; }
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
    d->w = w; d->h = w * MENU_H / MENU_W;
    if (d->h > h) { d->h = h; d->w = h * MENU_W / MENU_H; }
    d->x = (w - d->w) / 2; d->y = (h - d->h) / 2;
}
static void apply_config(void) {
    nb2_set_wide(cfg.aspect == ASP_169);
    nb2_set_cheats((cfg.invuln ? NB2_CHEAT_INVULN : 0) | (cfg.ammo ? NB2_CHEAT_AMMO : 0) |
                   (cfg.onehit ? NB2_CHEAT_ONEHIT : 0), cfg.stage);
    SDL_SetTextureScaleMode(tex, cfg.filter == FILT_SHARP ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
}
/* Scanlines a la resolucion real de la pantalla. Antes se estiraba una textura de 4 filas por linea
   a una altura que casi nunca es multiplo de 224, y unas lineas salian de 1 pixel y otras de 2.
   Ahora la textura mide exactamente lo que el juego en pantalla y cada fila lleva una sombra de perfil
   senoidal (oscura en el borde entre lineas del juego y clara en el centro): al no tener armonicos,
   todas las lineas se ven iguales aunque la escala no sea entera. */
static int scan_h;
static void update_scanlines(SDL_Renderer *r, int h) {
    const double PI = 3.14159265358979;
    double ppl = (double)h / NB2_H, strength = 0.55;
    Uint32 *px;
    if (h == scan_h) return;
    if (scan) SDL_DestroyTexture(scan);
    scan = NULL; scan_h = h;
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
static void draw_game(SDL_Renderer *ren) {
    SDL_Rect d, src = {0, 0, tex_w, NB2_H};
    game_rect(ren, &d);
    SDL_RenderCopy(ren, tex, &src, &d);
    if (cfg.filter == FILT_SCANLINES) {
        update_scanlines(ren, d.h);
        if (scan) SDL_RenderCopy(ren, scan, NULL, &d);
    }
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
    config_load(config_path);
    if (!nb2_load(rom, err, sizeof err)) {
        char msg[256];
#ifdef __ANDROID__
        remove(rom);    /* la proxima vez vuelve a pedir el archivo */
#endif
        snprintf(msg, sizeof msg, "El archivo elegido no sirve: %s.\nHace falta el romset outfxies.zip (World, OU2).", err);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies Remastered", msg, NULL);
        SDL_Quit();
        return 1;
    }
    eeprom_load();
    nb2_reset();
    audio_open();

    win = SDL_CreateWindow("The Outfoxies Remastered", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1152, 864,
                           SDL_WINDOW_RESIZABLE
#ifdef __ANDROID__
                           | SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
    );
    ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, NB2_WIDE_W, NB2_H) : NULL;
    mtex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MENU_W, MENU_H) : NULL;
    if (!tex || !mtex) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies Remastered", SDL_GetError(), NULL);
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(mtex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(mtex, SDL_ScaleModeLinear);
    apply_config();
    for (int i = 0; i < SDL_NumJoysticks(); i++) if (SDL_IsGameController(i)) pad_add(i);
    if (in_menu) menu_open(0);
    else audio_pause(0);

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
            }
            if (in_menu) {
                int r = menu_event(&e);
                if (r == MR_CHANGED) { apply_config(); config_save(config_path); }
                else if (r == MR_SAVE || r == MR_LOAD) {
                    char m[64];
                    int ok = r == MR_SAVE ? save_slot(menu_slot()) : load_slot(menu_slot());
                    if (r == MR_LOAD && ok) {
                        const unsigned char *m = nb2_mem();
                        rw_clear(); paused = 0; start_seq[0] = start_seq[1] = -1;
                        /* una partida guardada es de una partida de verdad si tiene la lista de rivales */
                        for (int p = 0; p < 2; p++) real_game[p] = (m[RAM_PLAYING] & (1 << p)) && (m[0x20807d] | m[0x20807f]);
                        vibration_reset();
                    }
                    snprintf(m, sizeof m, ok ? (r == MR_SAVE ? "Partida guardada en la ranura %d" : "Partida cargada de la ranura %d")
                                             : (r == MR_SAVE ? "No se pudo guardar en la ranura %d" : "La ranura %d no vale para esta versión"),
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

        if (in_menu) {
            SDL_Rect d;
            menu_tick((int)(tick - tick_prev));
            tick_prev = tick;
            menu_render(menu_px);
            SDL_UpdateTexture(mtex, NULL, menu_px, MENU_W * 4);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            if (started) {   /* en el menu de partida se ve el juego detras, oscurecido */
                draw_game(ren);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, 6, 6, 10, 170);
                SDL_RenderFillRect(ren, NULL);
            }
            menu_rect(ren, &d);
            SDL_RenderCopy(ren, mtex, NULL, &d);
            SDL_RenderPresent(ren);
            continue;
        }
        tick_prev = tick;

        /* botones especiales: se miran en todos los mandos */
        {
            int mb = any_pad(ACT_MENU);
            if (mb && !prev_menu) open_menu = 1;
            prev_menu = mb;
        }
        if (open_menu) {
            in_menu = 1; started = 1;
            audio_pause(1);
            eeprom_save();
            menu_open(1);
            continue;
        }
        b1 = (unsigned char)(pad_bits(pads[0]) | key_bits());
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
            if (rw_pop()) {
                nb2_set_inputs(0, 0, 0);
                rgb = nb2_frame();
                track_players();
                vibration_reset();
            }
            acc = 0;
            snprintf(osd_msg, sizeof osd_msg, rw_count ? "« Rebobinando" : "« No se puede rebobinar más");
            osd_until = tick + 300;
        } else if (paused) {
            acc = 0;
            SDL_Delay(5);
        } else {
            int n_ff = ff ? 4 : 0;
            if (rewinding) { rewinding = 0; audio_pause(0); acc = 0; }
            if (ff) { acc = period; snprintf(osd_msg, sizeof osd_msg, "» Avance rápido"); osd_until = tick + 300; }
            while ((acc >= period && steps < 3) || (n_ff && steps < n_ff)) {
                nb2_set_inputs(b1 | start_step(0), b2 | start_step(1), 0);
                nb2_set_render(!n_ff || steps == n_ff - 1);
                rgb = nb2_frame();
                track_players();
                if (!n_ff) { audio_push(); vibration_frame(b1, b2); } else vibration_reset();
                if (acc >= period) acc -= period;
                steps++; count++;
                if (++emu_frames % RW_EVERY == 0) rw_push();
                if (count % 600 == 0) eeprom_save();
            }
            nb2_set_render(1);
            if (n_ff) acc = 0;
        }
        if (rgb) {
            SDL_Rect sr = {0, 0, nb2_width(), NB2_H};
            tex_w = sr.w;
            SDL_UpdateTexture(tex, &sr, rgb, nb2_width() * 3);
        }
        if (rgb || paused || !SDL_TICKS_PASSED(tick, osd_until)) {
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            draw_game(ren);
            if (paused || !SDL_TICKS_PASSED(tick, osd_until)) {
                SDL_Rect d;
                menu_render_osd(menu_px, paused, SDL_TICKS_PASSED(tick, osd_until) ? NULL : osd_msg);
                SDL_UpdateTexture(mtex, NULL, menu_px, MENU_W * 4);
                menu_rect(ren, &d);
                SDL_RenderCopy(ren, mtex, NULL, &d);
            }
            SDL_RenderPresent(ren);
        } else SDL_Delay(1);
        if (frames && count >= frames) run = 0;
    }
    eeprom_save();
    if (haptic) SDL_HapticClose(haptic);
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    SDL_Quit();
    exit(0);    /* en Android cierra tambien el proceso */
}
