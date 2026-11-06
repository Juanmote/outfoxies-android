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

## Pendiente, por orden
1. Sonido (siguiente tarea): emular la CPU C75 (Mitsubishi M37702, BIOS `c75.bin`, programa `ou1spr0.5b`)
   y el chip PCM C352 (muestras `ou1voi0.6n`). Referencia: driver `namconb1` de MAME y sus dispositivos m37710 y c352.
   La C75 comparte la RAM 0x200000-0x207FFF con la CPU principal y lee los mandos y monedas en la placa real;
   ahora `nb2.c` los simula escribiendo en 0x206001/2/4 y 0x20601E. Al emularla habrá que pasar los mandos por sus puertos.
   Salida con SDL_audio.
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
