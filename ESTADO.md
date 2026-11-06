# Estado del proyecto: The Outfoxies para Android

## Punto de partida para un chat nuevo
- Repositorio privado. Cada `push` a `main` compila y publica el APK en Releases (`build-N`).
- La versión sale de `OFX_VERSION` en `.github/workflows/android.yml`.
- Para trabajar: clonar el repo y pedir al usuario `outfxies.zip` (no está en el repositorio).
- Prueba sin pantalla: `cmake -S . -B build && cmake --build build`, y luego
  `build/headless outfxies.zip <frames> <guardar cada N> <frame de la moneda> <carpeta>`.

- Compilar sin Gradle (si Maven Central no está accesible): `SDK=... SDL=... tools/build_apk_local.sh <código> <versión>`;
  necesita build-tools 35, plataforma 34 y NDK, y una copia de SDL `release-2.30.12`.

## Decisiones
- La ROM y cualquier dato derivado nunca se suben ni van en el APK.
- CPU con Musashi: el juego se ejecuta desde la ROM, sin traducirlo a C.
- Solo arm64-v8a, para las consolas actuales.

## Versión 0.1.0
Funciona en servidor: arranque, protección, demo, monedas, selección de personaje y combate.
Sin sonido.
- build-1 (6-11-2026): compilada por GitHub Actions a la primera. El usuario la ha probado en consola: "va perfecto".
- Comprobado en servidor: el núcleo da imágenes idénticas al prototipo de la prueba de viabilidad.

## Versión 0.2.0: sonido
- CPU de sonido C75 (M37702) en `third_party/m37710`: núcleo de MAME pasado a C. Los opcodes (`m37710op.h`) se generan
  casi tal cual desde MAME; `m37710.c` lleva estado, puertos, temporizadores, ADC e interrupciones.
- Chip C352 en `third_party/c352` (84 kHz, se usan los canales delanteros como en MAME).
- `nb2.c`: la 68020 y la C75 se ejecutan intercaladas (8 trozos por línea). Mapa de la C75: C352 0x2000,
  RAM compartida 0x4000-0xBFFF (bytes cruzados con 0x200000 de la 68020), `ou1spr0.5b` en 0x200000, BIOS en 0xC000.
  IRQ0 e IRQ2 a 60 Hz. Control en 0xF00016 (bit 0: reset y marcha). Mandos y moneda ya entran por los puertos P6/P7 de la C75.
- Si al zip le falta `c75.bin`, `ou1spr0.5b` u `ou1voi0.6n`, el juego arranca mudo con la simulación antigua de mandos.
- `headless` guarda también `audio.wav`. Comprobado en servidor: suena desde la demo, monedas, selección y combate van bien.
- build-3 probada en consola (6-11-2026): "va perfecto", sonido de gran calidad.

## Revisión gráfica (6-11-2026)
- Cambios a mitad de pantalla: el juego pone siempre la interrupción de línea en la 224 (fin de la imagen),
  así que dibujar el fotograma entero da lo mismo que MAME. Nada que hacer.
- Sombras: en minutos de demo y combate no se pinta ningún píxel de sombra. MAME las hace con color | 0x800;
  nosotros a mitad de brillo. Se deja así.
- Franja azul en el borde derecho del escenario de la ballena (cámara al fondo derecho): sale de la capa ROZ 1
  porque la ROZ 0 se acaba ahí. Nuestro C169 coincide con el de MAME, así que MAME la muestra igual.
  El usuario no recuerda si la recreativa la tiene: se deja como MAME.

## Versión 0.3.0: menú
- `src/app/menu.c`: menú sin SDL (lienzo 480x270). Al arrancar y en partida con moneda + start 1 s, Esc o Atrás.
  Imagen (nítida, suavizada, scanlines), formato (4:3, 4:3 escala exacta, pantalla completa), volumen,
  botones (reasignables, gatillos incluidos) y cambiar ROM. Guarda en `opciones.txt`.
- Cambiar ROM: `main.c` manda el mensaje 0x8001; `GameActivity` abre `RomPickerActivity` (proceso aparte `:picker`)
  con `change=true` y cierra el proceso del juego. La ROM vieja se conserva hasta validar la nueva.
- Letra: `tools/make_font.py` genera `src/app/font.h` desde DejaVu Sans Bold.

## Pendiente, por orden
1. Probar el menú en consola.
2. Valorar 16:9 de verdad (ver más escenario): la capa ROZ tiene mapa de sobra, pero hay que ver si el juego
   coloca sprites fuera de los 288 píxeles y cómo quedan el HUD y las capas fijas.

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
