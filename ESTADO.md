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
- Pendiente de probar en consola (latencia y si va fluido).

## Pendiente, por orden
1. Probar el sonido en consola. Si hay cortes o retraso, ajustar `audio_push` en `main.c` (cola de unos 6 fotogramas).
2. Comprobar contra el juego real: columna azul lisa en el lado derecho de un escenario del museo,
   sombras (aproximadas a mitad de brillo), cambios a mitad de pantalla (no se dibuja por líneas).
3. Launcher con opciones, y valorar 16:9 y más resolución.

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
