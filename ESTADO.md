# Estado del proyecto: The Outfoxies para Android

## Punto de partida para un chat nuevo
- Repositorio privado. Cada `push` a `main` compila y publica el APK en Releases (`build-N`).
- La versión sale de `OFX_VERSION` en `.github/workflows/android.yml`.
- Para trabajar: clonar el repo y pedir al usuario `outfxies.zip` (no está en el repositorio).
- Prueba sin pantalla: `cmake -S . -B build && cmake --build build`, y luego
  `build/headless outfxies.zip <frames> <guardar cada N> <frame de la moneda> <carpeta>`.
  Con `OFX_WIDE=1` delante, las capturas salen en 16:9 (384x224).
  `OFX_TRUCOS=7 OFX_ESCENARIO=3 OFX_LOG=1` activa trucos, elige escenario y apunta energía, arma y rival.

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
  Imagen (nítida, suavizada, scanlines), formato, volumen y botones (reasignables, gatillos incluidos).
  Guarda en `opciones.txt`.
- Letra: `tools/make_font.py` genera `src/app/font.h` desde DejaVu Sans Bold.

## Versión 0.4.0: 16:9, scanlines nuevas y sin "Cambiar ROM"
- Quitado "Cambiar ROM" del menú (solo hay un romset válido). El selector de `RomPickerActivity` solo sale
  la primera vez o si la ROM guardada no sirve.
- Scanlines: antes una textura de 4 filas por línea se estiraba a una altura no múltiplo de 224 y salían
  líneas de grosor distinto. Ahora `update_scanlines()` (main.c) hace una textura de la altura exacta del juego
  en pantalla, con sombra de perfil senoidal promediada por píxel. Comprobado con números y con una captura a 1080p.
- 16:9 de verdad ("16:9 panorámico", formato 3 en `opciones.txt`; por defecto en instalaciones nuevas):
  - El núcleo dibuja 384x224 (48 columnas más por lado; mismo píxel que el 4:3, así que es 16:9 exacto).
    Los arrays de vídeo siempre miden 384; la columna x de pantalla va en `x + EXT`. `nb2_set_wide()` / `nb2_width()`.
  - ROZ, capas 0-3 y sprites se amplían. Las ventanas (C116 y las de sprites) que ocupan toda la pantalla
    se abren a los laterales. Las capas fijas 4-5 (marcador y textos) se quedan en los 288 del centro.
  - Las capas 0-3 SÍ hacen falta anchas: el primer plano del escenario del dirigible es capa de tiles.
  - Escenas pensadas solo para 288 (presentación con las teles, título, elegir personaje): en los laterales hay
    negro o restos de memoria de vídeo. Cada fotograma se mira si los laterales tienen al menos un 60 % de
    píxeles no negros; si no, se tapan de negro y la escena se ve 4:3 centrada. Cambia tras 4 fotogramas seguidos.
  - Comprobado en servidor: con 4:3 las capturas son idénticas byte a byte a la versión anterior. En 16:9 la demo
    del dirigible (de día y de noche) se ve entera, con los sprites en los laterales y el marcador centrado.
    Solo se ha visto el primer escenario: falta probar los demás en consola.

## Versión 0.5.0: menú de trucos (para probar)
- Página "Trucos" en el menú: invulnerable, munición infinita, puño de la muerte y escenario del siguiente combate.
  Se guardan en `opciones.txt` (`truco_*`). Se aplican al final de cada fotograma en `apply_cheats()` (nb2.c).
- Direcciones (la energía viene de la base de trucos de MAME 2003-Plus; el resto, halladas con pruebas en servidor):
  - Luchadores contados desde su byte de energía: 1P 0x208D59, 2P 0x208E0D (0xF0 llena). +0x0C arma (0 ninguna),
    +0x0E munición. Comprobado: pistola 3 con 7 balas, lanzacohetes 4 con 3, granada 2 con 1.
  - Orden de combates en 0x20807C: 7 parejas (estado, rival); 0xFF pendiente, 0x00 vencido. 0x20808F = combate actual.
    0x20807B: rival actual (nibble alto) y personaje del 1P (nibble bajo). El rival cambia en el fotograma en que
    se elige el combate; el escenario se carga 25-70 fotogramas después.
  - Rivales y escenarios: 0 John Smith rascacielos, 1 Eve circo, 2 Betty Joe acuario, 3 Bernard White tren,
    4 Prof. Ching fábrica, 5 Dweeb avión, 6 Danny y Demi barco, 7 Mr. Ache mansión. Con John, el orden es 5,2,3,4,6,1,7.
- Cómo funciona cada truco:
  - Invulnerable: si la energía del humano baja (y no es 0), vuelve a 0xF0.
  - Munición: se recuerda la munición con la que se cogió el arma y se mantiene.
  - Puño de la muerte: si la energía de un rival de la máquina baja, se pone a 0. El combate termina bien.
  - Escenario: cuando el juego elige rival, se intercambia en la lista con el pedido si sigue pendiente.
    No hace nada si ya se le ganó o si es el personaje del jugador.
- Quién es humano: el 1P siempre; el 2P cuando pulsa START. (No se encontró un indicador fiable en la RAM.)
- Comprobado en servidor: con los trucos, el bot de prueba pasa 7 combates seguidos; con escenario = tren, el primer
  combate es en el tren. Sin trucos, las imágenes siguen idénticas byte a byte.
- Sin ver: qué hay después del 7.º rival (jefe final) — el bot se atasca en la mansión.

## Pendiente, por orden
1. Probar los trucos en consola y, con ellos, el 16:9 en todos los escenarios: buscar bordes raros en los laterales, sprites que
   aparezcan o desaparezcan de golpe en el borde, o escenas que parpadeen entre 4:3 y 16:9.
2. Comprobar las scanlines nuevas en consola.

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
