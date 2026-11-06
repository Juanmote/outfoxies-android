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
Sin sonido. Pendiente de probar en la consola. El workflow de GitHub aún no se ha ejecutado nunca.

Comprobado con la ROM en servidor:
- Los bits 0x01 y 0x02 del mando son derecha e izquierda, en la pantalla de selección.
- El núcleo da imágenes idénticas al prototipo con el que se hizo la prueba de viabilidad.

Sin comprobar: arriba, abajo y qué hace cada botón; se han tomado de MAME.

## Pendiente, por orden
1. Probar la APK en la Odin 3 y en la Retroid Pocket Classic: velocidad y mandos.
2. Sonido: emular C75 (M37702) y C352.
3. Comprobar contra el juego real: columna azul lisa en el lado derecho de un escenario del museo,
   sombras (aproximadas a mitad de brillo), cambios a mitad de pantalla (no se dibuja por líneas).
4. Launcher con opciones, y valorar 16:9 y más resolución.

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
