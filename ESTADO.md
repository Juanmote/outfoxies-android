# Estado del proyecto: The Outfoxies Remastered para Android

## Punto de partida para un chat nuevo
- Repositorio privado. Cada `push` a `main` compila y publica el APK en Releases (`build-N`).
- La versión sale de `OFX_VERSION` en `.github/workflows/android.yml`.
- Para trabajar: clonar el repo y pedir al usuario `outfxies.zip` (no está en el repositorio).
- Prueba sin pantalla: `cmake -S . -B build && cmake --build build`, y luego
  `build/headless outfxies.zip <frames> <guardar cada N> <frame de la moneda> <carpeta>`.
  `OFX_LOAD=partida3.sav` arranca desde una partida guardada del usuario (las de la carpeta de la app).
  `OFX_WIDE=2` ensena los laterales del 16:9 sin taparlos (para ver que dibuja el juego de verdad).
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
  (Desde 0.6.0 el menú se abre con SELECT; ver abajo.)
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
  - Rivales y escenarios: 0 John Smith rascacielos, 1 Eve circo, 2 Betty Doe acuario, 3 Bernard White tren,
    4 Prof. Ching fábrica, 5 Dweeb avión, 6 Danny y Demi barco, 7 Mr. Acme mansión. Con John, el orden es 5,2,3,4,6,1,7.
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

## Versión 0.6.0: START sin moneda, partidas guardadas, rebobinar, avance rápido y arreglos de 16:9
- Controles (`main.c`): ya no hay botón de moneda. START: si el jugador está en combate de una partida de verdad,
  pausa; si no, mete crédito (si no hay) y pulsa START a los 20 fotogramas, repitiendo a los 45 y 70 si aún no
  ha entrado (el juego ignora START hasta ~14 fotogramas después de la moneda). SELECT abre el menú.
  - RAM: 0x208079 bit 0/1 = jugador 1/2 en partida (la demo pone 3); 0x208129 créditos. 0x20816F NO sirve
    para saber si hay combate (vale 1 o 3 según el caso). La demo no se distingue por RAM de forma fiable:
    `real_game[p]` se lleva en `main.c` (empieza cuando entra tras nuestro START, acaba cuando se borra el bit).
- Estado completo (`nb2_state_save/load`, comprimido con miniz): RAM en zonas fijas (tabla `ram_zone`), 68020
  (`m68k_state.c`, parte del struct sin punteros), C75 (`m37710_state_*`), C352 (`c352_state_*`) y variables de
  `nb2.c` (`NB2_STATE_VARS`). Comprobado: tras cargar, 600 fotogramas con imagen y sonido idénticos. ~67 KB, ~4 ms.
  Si cambian esas variables, los guardados viejos dejan de cargar (la cabecera lleva los tamaños).
- Partidas guardadas: 5 ranuras; en Android en el almacenamiento compartido de la app para poder mandarlas.
- Rebobinar (L2): anillo de 400 estados, uno cada 3 fotogramas (~20 s). Al rebobinar se carga uno por pantalla y se
  ejecuta 1 fotograma para tener imagen. Avance rápido (R2): 4 fotogramas por pantalla, sin dibujar los 3 primeros
  (`nb2_set_render(0)`) y sin sonido.
- Truco de munición: ahora se repone tras medio segundo sin cambiar el contador (o al quedar 1 bala en armas de
  10 o menos). Antes se forzaba cada fotograma; puede que eso colgara el barco al disparar (no reproducido).
- 16:9, parches en el programa (`wide_patches`, se ponen y quitan al cambiar de formato; 4:3 sigue idéntico):
  - Visibilidad de objetos (rutina 0xB390): el juego proyecta en 3D y descarta lo que pasa de ±144 px (0x90).
    Plano principal: salto a 0x0FF000 que compara con dz*4/3; otros planos: `addi.w #$90` → `#$c0` en 0xB50E.
  - Cámara (rutina 0x2786): limita la cámara con semiancho 0x90 - z contra la tabla de límites 0x40220 + 8*escenario.
    Salto a 0x0FF020 que usa 4/3 de eso: la vista de 384 ya no se sale del escenario.
  - Hueco libre para rutinas: 0x0F1700-0x0FFFFF (0xFF).
- Cortinillas (ventana C116 que crece desde el centro): se estiran a 384 en 16:9.
- Laterales: se enseñan siempre que hay energía en algún marcador (combates, demo incluida). Fuera de combate van
  en negro salvo las filas en que los dos bordes del 4:3 tienen el mismo color liso (mesa y baldas de las teles),
  que se prolongan con ese color. Se quitó la detección por píxeles (era la que pasaba circo y fábrica a 4:3).
- Herramientas de prueba (no están en el repo, se rehacen fácil): volcado del mapa ROZ, render por capas,
  traza de escrituras con PC.

## Versión 0.7.0: Remastered, vibración y arreglos de 16:9 (9-11-2026)
- Nombre de la app: "The Outfoxies Remastered". Icono: el logo del título sobre negro (también icono adaptativo).
- Nombres corregidos en el menú: Betty Doe y Mr. Acme.
- Sprites en los laterales: TODOS los escenarios ponen la ventana de recorte de sprites de 1 a 288 (no de 0 a 287),
  y solo se abría a los laterales si empezaba en 0. Por eso desaparecían en los bordes los peces del acuario, las nubes
  y el helicóptero del avión, las cajas de la fábrica y el personaje al caer del tren. Ahora en 16:9 se abre si empieza
  en 0-2 o acaba en 285-287. En 4:3 no cambia nada.
- Mansión: el paisaje del fondo es la capa 0 y solo tiene dibujo en 36 columnas (288 px); el resto del mapa está vacío.
  En 16:9 se rellena reflejándolo en cada borde (árboles y palmeras en espejo). Solo si la capa tiene dibujo justo en
  el ancho del 4:3 y ocupa casi toda la pantalla (el marcador también mide 288 pero no se refleja).
- Textos de monedas: "INSERT COIN" (pequeño 0x49C7-0x49CF y grande 0x4954-0x495F), "CREDIT n" (0x4941-0x4945) y
  "CREDITS n" (0x4946-0x494B) no se dibujan (se buscan por el patrón de losetas en las capas 4 y 5). Quitarlos también
  arregla el parpadeo de las baldas prolongadas en las teles. "PUSH 2P" (0x49D8-0x49DD) se deja.
- Teles (presentación): es el plano ROZ 0 (atributo 0x0211; losetas 0x09B3 en (0,0) y 0x0AFC en (50,2)); el decorado mide
  864x672 y se ve a 1/3. En los laterales: ladrillos a la derecha (los de arriba a la derecha, periodo 96x64, empalman
  con el borde porque 864 es múltiplo de 96), pared oscura reflejada a la izquierda y baldas prolongadas donde los dos
  bordes tienen el mismo color.
- Vibración (menú "Vibración": No, 25 %, 50 % por defecto, 75 %, 100 %; `vibracion=` en opciones.txt). En el mando de
  cada jugador; si el del 1P no tiene motor, el vibrador de la consola (SDL_Haptic). Solo en partida de verdad:
  daño recibido (energía propia baja), golpe dado (baja la del rival), disparo (baja la munición del arma),
  salto (botón 2 en combate) y explosiones (el juego pide el sonido 0x18; `nb2_events()`).
- Órdenes de sonido: rutina 0x1810, una palabra por canal en 0x200000-0x20003F (nibble alto 4 = sonar, 0 = parar).
- Fábrica: el enemigo que se queda atascado junto a la caja hace lo mismo en 4:3 sin parches (fotogramas idénticos)
  y se suelta solo al rato: es del juego original.
- Comprobado en servidor con las 5 partidas del usuario. En 4:3 la imagen es idéntica a la 0.6.0 salvo los textos de monedas.

## Pendiente, por orden
1. Barco: las olas son sprites que el juego coloca solo en el ancho del 4:3 (desde x=0 de pantalla); falta buscar
   la rutina y ampliarla. La franja oscura de abajo (capa de tiles) también empieza en x=0.
2. Tren: comprobar en consola que el personaje ya no desaparece al caer (era el recorte de sprites).
   La capa ROZ se va reescribiendo por columnas (rutina de estampar tiles en 0xE702-0xE7xx); falta ampliar
   el margen para que los laterales no enseñen columnas viejas. Y la escena del helicóptero que te devuelve al tren.
3. Cuelgue del barco al disparar: pedir una partida guardada justo antes.
4. Vibración: comprobar en la consola que vibra (mando integrado o vibrador del sistema) y si el salto es el botón 2.
5. Comprobar en consola: START/pausa, SELECT, guardar/cargar, rebobinar, avance rápido.

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
