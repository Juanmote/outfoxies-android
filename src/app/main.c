/* The Outfoxies para Android y escritorio: ventana, mandos, sonido, menu de opciones y ritmo de juego. */
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "menu.h"

#define MAX_PADS 8
#define MSG_CHANGE_ROM 0x8001   /* lo recoge GameActivity.onUnhandledMessage */

static SDL_GameController *pads[MAX_PADS];
static char eeprom_path[512], config_path[512];
static unsigned char eeprom_last[NB2_EEPROM_SIZE];
static SDL_AudioDeviceID audio_dev;
static SDL_Texture *tex, *mtex, *scan;
static uint32_t menu_px[MENU_W * MENU_H];

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
    SDL_SetTextureScaleMode(tex, cfg.filter == FILT_SHARP ? SDL_ScaleModeNearest : SDL_ScaleModeLinear);
}
static SDL_Texture *make_scanlines(SDL_Renderer *r) {
    /* 4 filas por linea del juego: la ultima oscura y la anterior un poco */
    static Uint32 px[NB2_H * 4];
    SDL_Texture *t = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 1, NB2_H * 4);
    if (!t) return NULL;
    for (int i = 0; i < NB2_H * 4; i++) px[i] = (i % 4 == 3) ? 0x80000000 : (i % 4 == 2 ? 0x30000000 : 0);
    SDL_UpdateTexture(t, NULL, px, 4);
    SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(t, SDL_ScaleModeNearest);
    return t;
}
static void draw_game(SDL_Renderer *ren) {
    SDL_Rect d;
    game_rect(ren, &d);
    SDL_RenderCopy(ren, tex, NULL, &d);
    if (cfg.filter == FILT_SCANLINES && scan) SDL_RenderCopy(ren, scan, NULL, &d);
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
        case SDL_CONTROLLER_BUTTON_B: return menu_input(MI_BACK);
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

int main(int argc, char **argv) {
    char rom[512], err[128] = "";
    SDL_Window *win; SDL_Renderer *ren;
    Uint64 last, freq; double acc = 0;
    int run = 1, frames = argc > 2 ? atoi(argv[2]) : 0, count = 0, hold = 0, coin_prev = 0, guard = 0;
    int in_menu = frames ? 0 : 1, started = 0, changing_rom = 0, can_rom = 0;
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
    snprintf(rom, sizeof rom, "%s/rom.zip", SDL_AndroidGetInternalStoragePath());
    snprintf(eeprom_path, sizeof eeprom_path, "%s/eeprom.bin", SDL_AndroidGetInternalStoragePath());
    snprintf(config_path, sizeof config_path, "%s/opciones.txt", SDL_AndroidGetInternalStoragePath());
    can_rom = 1;
#else
    snprintf(rom, sizeof rom, "%s", argc > 1 ? argv[1] : "outfxies.zip");
    snprintf(eeprom_path, sizeof eeprom_path, "outfoxies_eeprom.bin");
    snprintf(config_path, sizeof config_path, "outfoxies_opciones.txt");
#endif
    config_load(config_path);
    if (!nb2_load(rom, err, sizeof err)) {
        char msg[256];
#ifdef __ANDROID__
        remove(rom);    /* la proxima vez vuelve a pedir el archivo */
#endif
        snprintf(msg, sizeof msg, "El archivo elegido no sirve: %s.\nHace falta el romset outfxies.zip (World, OU2).", err);
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies", msg, NULL);
        SDL_Quit();
        return 1;
    }
    eeprom_load();
    nb2_reset();
    audio_open();

    win = SDL_CreateWindow("The Outfoxies", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1152, 864,
                           SDL_WINDOW_RESIZABLE
#ifdef __ANDROID__
                           | SDL_WINDOW_FULLSCREEN_DESKTOP
#endif
    );
    ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC) : NULL;
    tex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, NB2_W, NB2_H) : NULL;
    mtex = ren ? SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, MENU_W, MENU_H) : NULL;
    if (!tex || !mtex) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "The Outfoxies", SDL_GetError(), NULL);
        SDL_Quit();
        return 1;
    }
    SDL_SetTextureBlendMode(mtex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(mtex, SDL_ScaleModeLinear);
    scan = make_scanlines(ren);
    apply_config();
    for (int i = 0; i < SDL_NumJoysticks(); i++) if (SDL_IsGameController(i)) pad_add(i);
    if (in_menu) menu_open(0, can_rom);
    else audio_pause(0);

    freq = SDL_GetPerformanceFrequency();
    last = SDL_GetPerformanceCounter();
    tick_prev = SDL_GetTicks();
    while (run) {
        SDL_Event e;
        Uint64 now;
        const unsigned char *rgb = NULL;
        int steps = 0, coin = 0, both = 0, open_menu = 0;
        Uint32 tick = SDL_GetTicks();

        while (SDL_PollEvent(&e)) {
            switch (e.type) {
            case SDL_QUIT: run = 0; break;
            case SDL_APP_WILLENTERBACKGROUND: eeprom_save(); audio_pause(1); break;
            case SDL_APP_DIDENTERFOREGROUND:
                last = SDL_GetPerformanceCounter(); acc = 0;
                if (!in_menu) audio_pause(0);
                break;
            case SDL_CONTROLLERDEVICEADDED: pad_add(e.cdevice.which); break;
            case SDL_CONTROLLERDEVICEREMOVED: pad_remove(e.cdevice.which); break;
            }
            if (in_menu && !changing_rom) {
                int r = menu_event(&e);
                if (r == MR_CHANGED) { apply_config(); config_save(config_path); }
                else if (r == MR_PLAY) { in_menu = 0; guard = 15; started = 1; last = SDL_GetPerformanceCounter(); acc = 0; audio_pause(0); }
                else if (r == MR_QUIT) run = 0;
                else if (r == MR_CHANGE_ROM) {
#ifdef __ANDROID__
                    eeprom_save();
                    changing_rom = 1;
                    SDL_AndroidSendMessage(MSG_CHANGE_ROM, 0);   /* Java abre el selector y cierra este proceso */
#endif
                }
            } else if (!in_menu && e.type == SDL_KEYDOWN && !e.key.repeat &&
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
            if (started) {   /* en pausa se ve el juego detras, oscurecido */
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

        for (int i = 0; i < MAX_PADS; i++) {
            if (!pads[i]) continue;
            if (padbtn_down(pads[i], cfg.map[ACT_COIN])) {
                coin = 1;
                if (padbtn_down(pads[i], cfg.map[ACT_START])) both = 1;
            }
        }
        if (SDL_GetKeyboardState(NULL)[SDL_SCANCODE_5]) coin = 1;
        /* moneda + start mantenidos 1 s: menu. Moneda sola: credito. */
        hold = both ? hold + 1 : 0;
        if (hold > 60) open_menu = 1;
        if (open_menu) {
            hold = 0; in_menu = 1; started = 1;
            audio_pause(1);
            eeprom_save();
            menu_open(1, can_rom);
            continue;
        }
        if (coin && !coin_prev && !both && !guard) nb2_coin();
        coin_prev = coin;
        if (guard) { guard--; nb2_set_inputs(0, 0, 0); }
        else nb2_set_inputs((unsigned char)((both ? 0 : pad_bits(pads[0])) | key_bits()), pad_bits(pads[1]), 0);

        now = SDL_GetPerformanceCounter();
        acc += (double)(now - last) / (double)freq;
        last = now;
        if (acc > period * 4) acc = period * 4;
        if (frames) acc = period;   /* modo de prueba: sin esperar al reloj */
        while (acc >= period && steps < 3) {
            rgb = nb2_frame();
            audio_push();
            acc -= period; steps++; count++;
            if (count % 600 == 0) eeprom_save();
        }
        if (rgb) {
            SDL_UpdateTexture(tex, NULL, rgb, NB2_W * 3);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
            SDL_RenderClear(ren);
            draw_game(ren);
            SDL_RenderPresent(ren);
        } else SDL_Delay(1);
        if (frames && count >= frames) run = 0;
    }
    eeprom_save();
    if (audio_dev) SDL_CloseAudioDevice(audio_dev);
    SDL_Quit();
    exit(0);    /* en Android cierra tambien el proceso */
}
