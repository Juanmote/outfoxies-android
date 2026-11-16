# Estado del proyecto: The Outfoxies Remastered para Android

## Punto de partida para un chat nuevo
- Repositorio privado. Cada `push` a `main` compila y publica el APK en Releases (`build-N`).
- La versión sale de `OFX_VERSION` en `.github/workflows/android.yml`.
- Para trabajar: clonar el repo y pedir al usuario `outfxies.zip` (no está en el repositorio; está en los archivos
  del Project "Outfoxies" de claude.ai).
- Prueba sin pantalla: `cmake -S . -B build && cmake --build build`, y luego
  `build/headless outfxies.zip <frames> <guardar cada N> <frame de la moneda> <carpeta>`.
  `OFX_LOAD=partida3.sav` arranca desde una partida guardada del usuario (las de la carpeta de la app).
  `OFX_WIDE=2` ensena los laterales del 16:9 sin taparlos (para ver que dibuja el juego de verdad).
  Con `OFX_WIDE=1` delante, las capturas salen en 16:9 (384x224).
  `OFX_TRUCOS=7 OFX_ESCENARIO=3 OFX_LOG=1` activa trucos, elige escenario y apunta energía, arma y rival.
  `OFX_DIF=0/2` dificultad fácil/difícil. Depuración (ver cabecera de `tools/headless.c`): sprites, capas, trazas de
  escrituras con PC, scroll y volcado de memoria; `tools/dis.c` desensambla ese volcado.
- Referencia: MAME 0.264 se instala con `apt install mame`; `tools/mame_escenario.lua` lleva el juego a un escenario
  y saca capturas (sirve para saber si algo es del juego original).

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

## Versión 0.7.1: arreglo del cuelgue con cohetes (9-11-2026)
- Síntoma (consola): al disparar el lanzacohetes el juego se colgaba (acuario: imagen congelada) o el jugador
  se quedaba sin responder (avión). Culpable: el truco de munición infinita, NO el 16:9 (se reproduce igual en 4:3).
- Por qué: el contador del luchador (+0x0E desde la energía = palabra +0x60 del objeto luchador) es una copia que
  el juego rehace cada fotograma (0x97E0) desde el objeto del arma (puntero en +0x82 del luchador; el arma apunta a
  su dueño en su +0x82). En el lanzacohetes el número de cohetes va ligado a objetos: subirlo (en la copia o en el
  objeto del arma, da igual) hace que el juego busque un cohete que no existe y rompa la lista de objetos
  (0x208232; un "next" acaba en 0x0020FFFF y la rutina 0xB390 da vueltas para siempre).
- Arreglo: ya no se toca la RAM. Con el truco, los 6 "subq.w #1,($60,A5)" que gastan munición (0x15044, 0x15C12,
  0x1925E, 0x19352, 0x19458, 0x19582) pasan a "jsr $0F00.w"; en 0xF00 (hueco 0x400-0xFFF lleno de 0xFF) va una rutina
  que solo gasta si A5 (o su +0x82) no es el luchador humano (1P 0x208D06; 2P 0x208DBA cuando ha pulsado START).
  Es lo que el propio juego hace con el cohete en un modo especial ($5C,A6 = 8, en 0x19578).
- Las partidas guardadas después del cuelgue ya están rotas (no se pueden arreglar); las de antes sirven.
- Mansión (vídeo del usuario): la "ola" verde es la espuma del extintor (es del juego). El jugador que se queda tumbado
  dentro de la vitrina probablemente es el mismo fallo (extintor + munición infinita).
- Comprobado en servidor: estado del bot justo antes del cohete del avión; con el truco, 8 disparos seguidos sin
  romper nada y la munición quieta; sin truco, igual que el juego original.

## Versión 0.8.0: arreglos de 16:9, dificultad, colores, filtros y menú de pantalla (9-11-2026)
- Tren: las montañas (capa 1) se escriben en bloques de 4 columnas (32 px) justo al llegar al borde derecho del 4:3
  (rutina 0x362EC; el bloque es 0x3630E-0x3639C, cada 64 fotogramas). En 16:9 se veían columnas viejas a la derecha.
  Parche: al empezar el escenario (0x362E8, `jsr $0E00.w`) se escriben ya 2 bloques; desde ahí va 64 px por delante.
  OJO: 0x362EC es destino de la tabla de saltos del objeto; no se puede tapar con un jsr largo.
  Las partidas guardadas ya dentro del tren no lo notan hasta el siguiente tren.
- Tren: el muñeco rosa de la izquierda es un objeto que el juego deja fuera del 4:3 con una paleta de magenta puro.
  En los laterales no se dibujan los píxeles de sprite de magenta puro (R y B >= 0xF0, G < 0x10).
- Fábrica: con el zoom más lejano el edificio mide justo el 4:3. La ciudad de fuera es la capa 0 (46 columnas desde el
  borde izquierdo del 4:3): a la izquierda no había nada (negro). El reflejo de la mansión se ha generalizado: un fondo
  de 35-48 columnas seguidas que cubre el 4:3 y empieza o acaba en su borde se refleja en el lado que falta.
- Barco, agua: el agua oscura es un dibujo de 18 losetas repartido entre las capas 1 y 2, con 4 columnas vacías donde
  64 no da para otra vuelta; en 16:9 se veía el cielo a través. Ahora `scan_tmaps()` (cada fotograma en 16:9) mira
  qué columnas tienen dibujo; si una capa es periódica, los huecos de hasta 6 columnas junto a su dibujo que no tapa
  ninguna otra capa parcial se rellenan siguiendo la repetición (solo en los laterales).
- Barco, olas: 16 objetos fijos en el mundo cada 0x70 desde x = -0x100 (rutina 0x17E00; dan la vuelta sumando 0x700 en
  0x17E64). Cubrían 336 px con el zoom más lejano. En 16:9 se crean 19 desde -0x1A0 (vuelta de 0x850).
- Mansión, la mujer que no se queda sin balas: era el truco de munición infinita. Sin 2P, la rutina comparaba con
  0xFFFFFFFF, que es justo lo que llevan en +0x82 las armas sin dueño: gastaba como si fuera humana. Ahora es 1.
- Mansión, "luces" de la planta de arriba: los focos con resplandor también salen en MAME; son del juego. La línea del
  vídeo del usuario se ve al rebobinar; no se ha reproducido jugando normal (pedir otro vídeo si sigue).
- Dificultad (menú principal; `dificultad=` en opciones.txt; `nb2_set_difficulty`): al final de cada fotograma se
  escala la energía perdida de cada luchador. Fácil: el humano recibe x1/2 y la máquina x2. Difícil: al revés.
  Las fracciones van en medios puntos. No toca energías a 0 ni subidas. No va en las partidas (no cambia su tamaño).
  Comprobado con el bot en el tren: normal pierde 36 y quita 24; fácil 18 y 48; difícil 72 y 12.
- Menú "Pantalla": formato, filtro, colores y bandas laterales. Filtros nuevos (van detrás para no cambiar
  opciones.txt): nítida suavizada, CRT y CRT curvo (los de Master of Darkness, por GPU). Colores: original, tele
  antigua, vivos, nocturna, sepia y pantalla verde (sobre el RGB final, en `colors_apply`). Bandas: negras o
  ambiente (la escena reducida a 16x12 y oscurecida, solo cuando la imagen no llena la pantalla).
- Escritorio: `OFX_SHOT=captura.ppm outfoxies rom.zip <fotogramas>` guarda la pantalla al terminar (para probar
  filtros con `SDL_VIDEODRIVER=offscreen SDL_RENDER_DRIVER=software`).
- Comprobado en servidor: en 4:3 sin trucos, imagen y sonido idénticos byte a byte a la 0.7.1 (las 4 partidas y
  arranque + bot). En 16:9, circo, acuario, avión y mansión idénticos a la 0.7.1.

## Versión 0.10.0: traducción, logros, 8:7 sin cortes, ropa gótica (10-11-2026)
- Traducción al castellano de los textos del juego (Idioma → Textos del juego). `tools/make_lang.py outfxies.zip`
  genera `src/core/lang_es.h` (203 escrituras, 4,4 KB) y comprueba anchos y punteros contra la ROM:
  - Biografías (0xAD47C): una tira, '\\\\' = salto, líneas de 24 (la primera 25); mismo número de líneas y mismo
    largo por línea (ritmo de las teles). Nombres en rojo = byte 1-26, 0xFB ':', 0xEE '.'.
  - Mensajes fijos 0x91C64-0x9201C: cabecera de 7 bytes (bytes 2-3 = dirección en la capa de texto, se recoloca para
    centrar) + texto + FF. Si no cabe, va a la zona libre y se cambian los punteros absolutos que lo usan.
  - Mensajes de Mr. Acme 0x925FC-0x92BC8: tabla de punteros 0x48770-0x488F8; todos a la zona libre 0x0F2000.
    La 1.ª lista de nombres es a quién habla ("BUEN TRABAJO, SR.SMITH."), la 2.ª el objetivo.
  - En pantalla, loseta = 0x340 + byte (presentación, encargos) o 0x520 + byte (mensajes fijos); misma letra.
    Tildes: bytes 0x7B-0x7F, 0x60, 0x5B, 0x5D → Á É Í Ó Ú Ñ ¿ ¡, dibujadas en nb2.c (lang_make_glyphs) a partir de
    A E I O U N ? ! de la fuente (letra a 5 filas + acento; ¿ ¡ girados).
  - Sin traducir: PUSH 1P AND/OR 2P BUTTON (dibujo, losetas 0x499A-0x49AF), PUSH 2P, WIN/WINS, aviso legal, créditos.
  - Documento de revisión (inglés / castellano): Claude Docs "The Outfoxies Remastered — Traducción de los textos".
- Logros (`src/app/achievements.c`, menú Logros, `logros.txt` junto a las partidas): los 20 de RetroAchievements
  (juego 19839), locales, sin iconos aún. Funcionan 12: terminar con cada personaje (7), Officially Outfoxed, Acme
  Outfoxed, The Outfoxiest (Difícil), Saving The Whale (Betty en 90 s), Rising Sea Levels (tiempo agotado en el
  acuario). No cuentan con trucos ni en Fácil. Aviso "Logro: …" 4 s. En estudio (8): los de armas (Eat Lead!,
  Demolitions Expert, Hobbyist Vampire Hunter, By ANY Means, toy guns del circo) y los de sitios (Helicopter
  Insurance, Going Down, Fly My Pretty). Hace falta el número de cada arma (+0x0C del luchador; vistos 3, 4, 9, 18).
  RAM nueva: 0x208092 (palabra) = tiempo que queda en fotogramas (12000 = 200,0). Al llegar a 0 el juego pone a 0 la
  energía del 1P en ese mismo fotograma.
  Iconos: el servidor no puede bajar de media.retroachievements.org; hay que traerlos con el navegador del usuario.
- 8:7: en combate el marcador (capa de losetas 3, filas 0-51) se separa y se recoloca dentro de las columnas
  20-267 (draw_hud87): tiempo a tamaño normal 9 filas más abajo, cajones de arma al 55 % encima, barras/nombres/balas
  pegados al tiempo. Fuera de combate se ve el 4:3 entero a la anchura del 8:7 (franjas arriba y abajo).
- Ropa del jugador 1 (Pantalla → Ropa del jugador 1: Original / Gótica): banco de sprites 1, se recolorea al
  dibujar (outfit_color): piel (tono 5-45°, clara) y pelo rubio intactos; lo rojo → carmesí, otros colores → negro
  carbón, lo apagado → grises.
- PUSH 2P (losetas 0x49D8-0x49DD, alterna con CREDIT): en combate solo los 140 primeros fotogramas (dos veces).
- Tren al empezar: la capa de montañas tiene 48 columnas con dibujo solo abajo; el reflejo de fondos ahora pide
  tiles >= n*12 (antes n*24) y admite 35-63 columnas. El hueco del inicio ya no se ve.
- Botón 3 fuera del menú (la placa lo tiene, el juego no lo usa).
- Mansión: la sangre bajo el helicóptero sale en vídeos del original: es del juego.
- headless: OFX_ES=1, OFX_ROPA=1, OFX_HUD87=1, OFX_ACH=carpeta (prueba de logros), t= en OFX_LOG.
- Juego en red local: el repo Mckeijas/Leyendas-de-Mystara no es accesible (privado o nombre distinto). Plan: ver
  respuesta del 10-11-2026 (lockstep por UDP en la wifi, estado inicial del anfitrión, comprobación de RAM).

## Versión 0.11.0: packs de armas, logros de armas, 8:7 lleno, volver al inicio (10-11-2026)
- Armas (tabla de tipos): el luchador lleva tipo + 2 en +0x0C desde su energía (0x208D65 el 1P). Tipos: 0 granada,
  1 pistola, 2 lanzacohetes, 3 katana, 4 metralleta, 6 látigo, 7 lanzallamas, 8-10 las 3 pistolas de juguete del circo
  (al crearse pasan a pistola, tipo 1, con $26 = 1-3; rutina 0x198A8), 11 un objeto de la fábrica (sin identificar),
  12 carbón (tren), 16 tarta, 17 sopa, 18 cesta de fruta, 19 y 20 especiales (forzarlos cuelga el juego). 5, 13-15
  vacíos. Tabla por tipo en 0x443A6 (16 bytes, +0x0A munición). Partidas del usuario: granada, pistola, lanzacohetes,
  katana, metralleta, látigo, lanzallamas, sopa y cesta (en la de la tarta el 1P ya no la llevaba; la sopa la tenía el 2P).
- Objetos arma = clase 0x14 ($12); al crearse la rutina 0x185E8 copia el tipo pedido con "move.w ($7e,A5),($5e,A5)"
  en 0x185FC. Packs (`nb2_set_weapon_pack`, menú principal "Armas", `armas=` en opciones.txt): esa instrucción pasa a
  "jsr $0D00.w" + nop; la rutina de 0xD00 traduce el tipo con una tabla de 0x15 bytes en 0xD80. 0xFF = el arma se borra
  nada más crearse (salta a 0x1AE48 como hace el juego con "bra $1ae48"). Packs: de fuego (pistola, metralleta),
  cuerpo a cuerpo (katana, látigo), explosivas (granada, lanzacohetes, lanzallamas), sin armas. La comida, el carbón,
  las pistolas de juguete y el objeto de la fábrica se quedan siempre. Probado: 6 combates seguidos con cada pack.
  headless: `OFX_ARMAS=n` (todas al tipo n) o lista de 21 tipos; `OFX_ARMLOG=1` apunta los tipos que van saliendo.
- Logros de armas ya funcionan: Eat Lead!, Demolitions Expert, Hobbyist Vampire Hunter (10 veces que baja la energía
  del rival con el látigo en la mano) y By ANY Means (sin armas principales; comida y carbón sí). Quedan 4 en estudio
  (helicóptero, foso de granadas, avión, pistolas de juguete). De momento los trucos y los packs NO impiden los logros
  (para probarlos); solo Fácil. Volver a ponerlo como RA cuando estén probados.
- Logros: la descripción va en hasta 3 líneas (se cortaba); la lista enseña 8.
- Menú: "Armas" y "Volver a la pantalla de inicio" (en partida: reinicia la recreativa, `MR_TITLE`). Volumen y
  vibración pasan a la página "Sonido y vibración" para que quepa todo.
- INSERT COIN del 1P en la pantalla de CONTINUE? (losetas 0x49B0-0x49B8, abajo a la izquierda): ya no se dibuja.
  PUSH 2P ya no se dibuja nunca.
- Ropa gótica: la piel oscura (Bernard White) se pintaba de negro. Ahora la piel es tono 5-45°, R ≥ G ≥ B,
  saturación 15-88 y brillo ≥ 60 (antes 15-65 y ≥ 120). Los pelos castaños también se quedan.
- 8:7: fuera de combate también se recorta a 248 columnas para llenar la pantalla, salvo si la pantalla es casi toda
  negra (≥ 55 % de píxeles negros: logo de Namco, título, elegir personaje), donde se ve el 4:3 entero (las franjas no se
  notan y no se corta el logo). Las biografías de las teles (capa de losetas 1) se corren 8 px a la derecha para que no
  se corte la primera letra (`text_shift87`); los textos de las capas 4-5 que empiecen antes de la columna 20, también.
- Sangre del helicóptero: el juego tiene en el test "BLOOD COLOR" (RED / GREEN / YELLOW; RAM de ajustes A6+0x42 =
  EEPROM 0x40; tabla de opciones en 0xAF5C2). Nuestra EEPROM tiene 0 = RED, así que no es eso. Falta partida guardada
  justo antes de que el helicóptero aplaste a los Acme para compararlo con MAME (`apt install mame`).

## Versión 0.12.0: menú nuevo, REMASTERED en el título, ropas, sonido separado y 8:7 (10-11-2026)
- Título: letrero REMASTERED en pixel art bajo el logo (`tools/make_title.py` → `src/core/title_gfx.h`; letras dibujadas
  a mano con polígonos, A con vértice de verdad; textura del panel azul agrietado del usuario, `assets/art/paneles.jpg`).
  Se pone al final de `render_frame` (`title_overlay`) si 24 puntos del logo coinciden (con el brillo escalado, para
  seguir los fundidos; también con el logo subiendo). Icono nuevo con el letrero (`tools/make_icon.py captura.ppm`).
- Menú: lienzo que toma la forma de la pantalla (`menu_layout`: 480x270 en 16:9; 400 de ancho y más alto en 8:7/4:3).
  Al arrancar, la portada del usuario de fondo (`assets/art/portada.jpg` → `tools/make_art.py` → `src/app/art_gfx.h`,
  256 colores) y la demo de la recreativa andando detrás (sin sonido) para la vista previa. "Partidas" junta guardar y
  cargar. "Salir del juego" siempre; "Volver a la pantalla de inicio" reinicia y vuelve al menú de inicio.
  Las filas se pueden tocar con el dedo (`menu_tap`); los controles táctiles se ven más transparentes en el menú
  (tapaban "Salir del juego" en la Pocket).
- Pantalla: vista previa (`menu_preview_rect`; main.c dibuja el juego con formato, filtro, colores y bandas). Al
  cambiar algo en partida se vuelve a dibujar el último fotograma (`nb2_redraw`). Bandas: Negras, Ambiente, Portada
  (por defecto; clave `bandas2`).
- 8:7: fuera de combate el 4:3 entero con las bandas arriba y abajo (ya no se recorta: las teles se cortaban);
  `text_shift87` queda desactivado. En combate, lo que las capas fijas 4-5 pintan junto al marcador (ILIMITADA del
  lanzamisiles final) se mueve con las piezas de `draw_hud87`.
- Avisos (logros, guardar...): arriba en el centro, más grandes, sobre el lienzo que cubre toda la pantalla.
- Sonido: música y efectos por separado. El driver toca la música (orden del canal 0, 0x200000) en las voces 0-15 del
  C352 y los efectos en las 16-31 (visto con `OFX_SND=1`). `nb2_set_volumes` → ganancia por voz en c352.c.
- Truco "Inmunidad tras golpe" (Normal / quitada al jugador / al rival / a todos): tras un golpe el juego pone el bit 13
  de ($6C) del luchador y cuenta 60 fotogramas en ($30) (rutina 0xA702); se pone el contador a 0.
  Probado: con el truco el 1P recibe 7 golpes en vez de 2 en la misma partida.
- Ropa: del 1P (banco 1) y del rival (banco 2) por separado, solo en combate (la elección de personaje queda original).
  Estilos: Original, Gótica, Negativa, Pinky, Vampiro. Piel y pelo por entradas de la paleta de cada personaje
  (`ch_parts`, vistas con `OFX_ROW=banco*16+fila`): John piel 0x40-47 pelo 0x20-25; Eve 0x30-37 / 0x10-17; Betty
  0x20-27 / 0x30-37; Bernard 0x30-37; Ching 0x40-47; Dweeb 0x30-37 / 0x40-44. Danny y Demi y Mr. Acme: por color.
  Las entradas 0x90+ (efectos, sangre 0xB8-0xBA) y la 1 (contorno) no se tocan.
- Armas: las partidas nuevas del usuario dan pistola de juguete (pasa a pistola, tipo 1) y carbón (tipo 12). El cohete
  de la fábrica no es un arma (el luchador no tiene arma: es un objeto del escenario); se queda en todos los packs.
- Sangre del helicóptero: los colores de la sangre se ponen al empezar cada escenario (rutinas 0x3B2E y 0x3C26, según
  BLOOD COLOR: paleta 0x420-0x423 y entradas 0xB8-0xBA de cada luchador), pero en la partida del usuario justo antes de
  que caiga no sale ningún objeto de sangre. Probablemente la versión World (OU2) la quita: falta comparar con la
  japonesa (outfxiesj, OU1). Pedir ese romset.
- headless: `OFX_SND=1` (voces del C352), `OFX_WATCH=dir,len` (bytes por fotograma), `OFX_ROW`, `OFX_ROPA2`;
  `OFX_TRUCOS` admite 8 (sin inmunidad jugador) y 16 (rival). Escritorio: `OFX_WIN=1240x1080`, `OFX_LOAD=partida.sav`,
  `OFX_MENUSHOT=menu.ppm OFX_MENUKEYS=ddo OFX_MENUWAIT=n` (captura del menú).
- Comprobado en servidor: 4:3 sin opciones, imagen y sonido idénticos byte a byte a la 0.11.0 en 4 partidas.

## Versión 0.13.0: launcher con botones, teles a pantalla completa en 8:7, ayudas del menú y ROMs japonesas (10-11-2026)
- Launcher (`src/app/launcher.c`, arte de `tools/make_launcher.py` → `src/app/launcher_gfx.h`): al arrancar ya no hay lista,
  hay botones como en Re-Master-ed of Darkness. Portada del usuario de fondo (JPEG a 1600 px, se descodifica con
  `third_party/stb/stb_image.h`) y 7 botones hechos con sus paneles de cristal (`assets/art/paneles.jpg`: azul, acero,
  verde, oro, plata, rojo y uno morado = el azul con el tono girado). El rótulo va aparte (castellano e inglés, Inter
  Display Black Italic): cambiar un texto no duplica paneles. `make_launcher.py <carpeta>` guarda los botones montados.
  - Botones: OPCIONES (pantalla, sonido y vibración, idioma, dificultad, armas), CONTROLES, PARTIDAS, JUGAR, LOGROS,
    TRUCOS, SALIR. Pantallas anchas: una fila abajo con JUGAR grande en el centro. Pantallas casi cuadradas
    (`LAUNCH_TWO_ROWS`: menos de 3:2, o sea 8:7 y 4:3): dos filas (PARTIDAS JUGAR LOGROS / OPCIONES CONTROLES TRUCOS
    SALIR) y la portada arriba, con el logo entero por encima de los botones y fundido a oscuro por debajo.
  - `menu.c`: `launcher` / `at_launch` / `launch_sel`; `menu_at_launcher()`, `menu_launch_sel()`, `menu_launch_tap()`.
    Con el launcher, la página principal (`PG_MAIN`) es la de OPCIONES (`build()`); `back_to()` devuelve pantalla, sonido
    e idioma a OPCIONES y el resto a los botones. B o Atrás en el launcher lleva a SALIR (no cierra sin querer).
    Los botones se tocan con el dedo; en el launcher no se dibujan los controles táctiles.
  - El menú de partida (SELECT) no cambia. `art_gfx.h` y `make_art.py` ya no existen: la portada de las bandas sale del
    mismo JPEG (`launcher_cover`).
- 8:7, teles: el decorado (plano ROZ 0, 864x672) se acaba en los bordes del 4:3. `tv_vborder()` (nb2.c) inventa
  `NB2_VEXT` = 19 filas por arriba y por abajo: arriba siguen los ladrillos (se copian de 64 px más abajo, su periodo) en
  las columnas que acaban en ladrillo y se refleja la pared en el resto (a la izquierda solo 21 px, para no reflejar la
  tele de arriba); abajo siguen los tablones de la mesa (periodo medido). `nb2_vborder()` las da y `main.c`
  (`upload_frame`, `tall87`) las pone en una textura de 262 filas: la escena llena el 8:7 con todo a su tamaño.
  Con la ventana a medio abrir (cortinillas) van en negro. El resto de escenas sin combate siguen con bandas.
- Menú: las ayudas pasan por `help_text()` (hasta 3 líneas, con margen: 82 % del lienzo estrecho); el pie baja en el
  lienzo alto (`foot_y`). `row()` acorta la etiqueta con "..." si no cabe con su valor (antes se pisaban: "Inmunidad
  tras golpe" con "Quitada (todos)"). Valores nuevos de ese truco: Normal / Tú no / El rival no / Nadie; filas de la
  página de trucos más anchas.
- ROMs japonesas (outfxiesj y outfxiesja; el usuario las pasó). Las dos llevan el MISMO programa (OU1); outfxiesja solo
  cambia las ROMs de gráficos (otro reparto de los datos: cargadas como las nuestras salen rotas). Diferencias del
  programa OU1 frente al World OU2 (comparación completa, con el desensamblado alineado):
  - Presentación de las teles: un objeto (clase 0, rutina 0x29100) escribe una frase en japonés en vertical a la
    derecha por cada personaje. World empieza esa rutina con `jmp $4e34` (borrarse). Con 3 `nop` en 0x29100 de la World
    vuelve a salir (comprobado). Otra clase de objeto (0x2c, rutina 0x3943C, dibuja bloques de losetas en la capa fija
    0x684140 según el personaje) también lleva ese `jmp` en World; con `nop` no cambió nada en 9000 fotogramas de bot
    (no se ha visto dónde sale).
  - Título: Japón pinta el subtítulo en katakana (`lea $94b62,A0; jsr $4482` en 0x37152, una tira de 16x2 losetas en el
    mapa ROZ); World no tiene esas dos instrucciones.
  - Biografías distintas (Bernard White, Prof. Ching, Danny y Demi: las japonesas son más bestias), aviso legal
    "solo para Japón" y la marca de versión de la EEPROM (palabra 0x20: 0 World, 1 Japón; solo sirve para rehacer los
    ajustes si no coincide, 0xB05A0).
  - NADA de sangre, de la mansión ni del helicóptero: ese código es idéntico. Tampoco hay un ajuste de región.
    El programa japonés arranca en nuestro núcleo (zip con los `ou1_mpr*` renombrados a `ou2_mpr*`), pero los parches
    (16:9, traducción, munición, armas) usan direcciones de la World: de 0x29100 en adelante van 6-8 bytes desplazadas.
  - Sangre del helicóptero: sigue sin explicar. Hace falta la partida guardada justo antes y saber de qué vídeo sale.
- Hojas de ropas (`tools/make_outfit_sheets.py`): un PNG por estilo con los 7 personajes en 5 posturas, sacadas del
  juego (headless `OFX_STYLES=banco`: el mismo fotograma con las 5 ropas y solo ese banco de sprites sobre magenta).
  Hallazgo: Mr. y Mrs. Acme van en el banco 0 (con objetos del escenario), no en el 2: la ropa del rival NO les afecta.
- headless: `OFX_STICK=n` (el 1P se coloca donde el rival cada 20 fotogramas y pega; llega hasta la pelea con los Acme),
  `OFX_STYLES`, `OFX_VB=1` (capturas con las franjas del 8:7), `OFX_DECOR=archivo.ppm` (decorado de las teles).
  Escritorio: `OFX_MENUGAME=1` (con `OFX_MENUSHOT`, captura el menú de partida).
- APK compilada en el servidor con `tools/build_apk_local.sh` (dl.google.com es accesible: cmdline-tools + `sdkmanager
  "build-tools;35.0.0" "platforms;android-34" "ndk;27.0.12077973"`; SDL con `git clone -b release-2.30.12`).
- Comprobado en servidor: 4:3 sin opciones y 16:9 con traducción y ropa, imagen y sonido idénticos byte a byte a la 0.12.0.

## Versión 0.14.0: botones translúcidos, portada del 8:7, JUGAR arranca la recreativa, Armas en Trucos (11-11-2026)
- Launcher: los botones son translúcidos (`BTN_ALPHA` 178 de 255 en `launcher.c`; el rótulo va opaco) y algo más pequeños
  (una fila: 202/286 px sobre 1920; dos filas: 246/310 sobre 1240). El elegido sigue opaco, un 10 % mayor y con brillo.
  CONTROLES: marco de acero con el cristal amarillo (`recolor_center` en `make_launcher.py`; antes azul). `keep_main`
  quita los restos del panel de al lado que se colaban en el recorte de la plantilla.
- 8:7 y 4:3: portada casi cuadrada del usuario (`assets/art/portada87.jpg` → `lg_cover87`, 1280 px). Llena la pantalla;
  lo que sobra de alto se corta 1/5 por arriba y 4/5 por abajo (donde van los botones). OJO: esa imagen dice RECOMP y la
  ancha REMASTERED (pendiente de que el usuario decida).
- JUGAR: la recreativa se reinicia al pulsarlo (`nb2_reset` en `MR_PLAY` si `!started`), así empieza en el logo de Namco.
  La demo sigue andando sin sonido detrás del launcher solo para la vista previa de Pantalla. Cargar partida no reinicia.
- Trucos: "Armas" pasa a la página de trucos (`CH_WEAPONS`; ya no está en OPCIONES ni en la lista del menú de partida).
  `config_reset_cheats()` se llama al final de `config_load`: cada arranque de la app empieza sin trucos, con todas las
  armas y el escenario en Normal (lo que haya en `opciones.txt` de trucos se ignora).
- Sangre del helicóptero (vídeo del usuario, Arcade Archives de PS4, minuto 26:14): unos 2 s después de que el helicóptero
  caiga sobre Mr. Acme sale una línea roja de 2 px de alto a ras de suelo, centrada bajo el helicóptero, que se ensancha
  hasta unos 100 px y se queda (charco de sangre). En nuestro núcleo, con `partida3.sav` del usuario (dejándola correr, el
  helicóptero cae hacia el fotograma 690) NO sale. Mirado entre los fotogramas 700 y 1000: no hay ningún sprite plano en
  la lista (`OFX_SPR`), ninguna escritura en las capas de losetas, y en la paleta solo cambia el ciclo de grises
  0x692-0x697. O sea, el juego no llega a crear el charco: no es un fallo de dibujo. Falta saber si MAME lo saca con
  nuestra ROM (sería un fallo de emulación) o si es cosa de la versión de Arcade Archives. El bot no pasa del interior de
  la mansión, así que hay que llevar MAME hasta ahí de otra forma (p. ej. volcar la RAM de nuestra partida en MAME).
  MAME se instala en `/usr/games/mame`.

## Versión 0.15.0: daño a medida y botones del launcher (11-11-2026)
- Daño a medida: dos filas nuevas bajo "Dificultad" (en OPCIONES y en el menú de partida): "Daño que recibes" y "Daño
  que haces", del 50 % al 200 % de 25 en 25 (`dano_propio=` y `dano_rival=` en opciones.txt; `cfg.dmg_own`, `cfg.dmg_rival`).
  "Dificultad" pasa a ser un atajo: Fácil = 50/200, Normal = 100/100, Difícil = 200/50; con otros valores pone
  "A medida" (`config_difficulty()`; ya no existe `cfg.difficulty`; el `dificultad=` de los opciones.txt viejos se lee).
  Núcleo: `nb2_set_damage(propio, rival)`; `apply_difficulty` escala la energía perdida con el porcentaje y guarda la
  fracción en cuartos de punto (`nb2_set_difficulty` sigue para headless). Con 2 humanos, los dos son "propio".
  Logros: no cuentan si es más fácil que Normal (recibes < 100 o haces > 100); The Outfoxiest pide 200/50 o más duro.
  headless: `OFX_DANO=propio,rival`. Comprobado con el bot en el tren: con 150/75 los golpes de 14 y 22 puntos al
  jugador quitan 21 y 33, y los de 22 al rival quitan 17; Fácil y Difícil dan lo mismo que antes.
- Launcher: JUGAR (centro) lleva el panel del marco de acero con su cristal azul de la plantilla, y CONTROLES el panel
  amarillo sin marco (se quita el recoloreado amarillo del de acero; `recolor_center` queda sin usar). Botones más
  pequeños para tapar menos a los personajes: una fila 168/236 px sobre 1920 (antes 202/286) y 18 px más abajo;
  dos filas 204/258 sobre 1240 (antes 246/310). El degradado oscuro de detrás también es más bajo y suave.
- 0.15.1: portada nueva del 8:7 del usuario (cuadrada, 2048 px, ya dice REMASTERED: decidido) y botones del launcher
  más transparentes (`BTN_ALPHA` 105; el elegido, opaco del todo). El tamaño de los botones le parece bien.
- Juego en red: el repo del amigo es `Mckeijas/dias-de-furia` (público; Kotlin + LibretroDroid). Su carpeta
  `proyecto-android/app/src/main/java/com/diasdefuria/red/` (unas 2.000 líneas) hace retransmisión desde el anfitrión:
  solo el anfitrión emula; copia la imagen con PixelCopy de la SurfaceView, la comprime en H.264 (MediaCodec, 60 fps) y
  la manda por TCP (puerto 47800) con el sonido en AAC; los invitados mandan botones y cruceta. Descubrimiento en la wifi
  por NSD; por internet, UPnP y un código con IP y puerto. Los invitados no necesitan la ROM. Protocolo en `Protocolo.kt`.
  Para traerlo aquí: añadir Kotlin al Gradle, adaptar Anfitrion/Retransmision/AudioRed a `GameActivity` (SDLActivity:
  la SurfaceView es `SDLActivity.mSurface`), meter los botones del invitado como jugador 2 por JNI (`nb2_set_inputs`),
  sacar el sonido de nuestro mezclador por JNI, y una pantalla de invitado (su `InvitadoActivity` usa Compose) más las
  entradas "Crear partida / Unirse" en el launcher. SIN EMPEZAR.

## Versión 0.16.0: juego en red, fase 1 (wifi local) (11-11-2026)
- `src/app/net.c` / `net.h`, todo en C dentro de la app (no se ha traído el Kotlin de Días de Furia: nuestra app es SDL
  y el núcleo es nuestro, así que sale más sencillo y mejor mandar el fotograma del núcleo que copiar la pantalla).
  Mismo modelo y mismos mensajes que `red/Protocolo.kt` de Mckeijas: solo el anfitrión ejecuta el juego, manda imagen
  y sonido y recibe los botones del invitado, que entran como jugador 2 (`b2 |= net_host_inputs()` en main.c).
  - TCP, puerto 47800, mensajes [tipo:1][longitud:4][datos]: HOLA "OFXN"+versión, ENTRADAS bits, PING/PONG, ADIOS,
    BIENVENIDO, VERSION, VIDEO, AUDIO, FIN. `NET_VERSION` 1 (subirla si cambia el formato).
  - VIDEO: el fotograma del núcleo (288 o 384 x 224, RGB, antes de colores y filtros) SIN pérdida: XOR con el fotograma
    anterior desplazado (dx, dy) y deflate nivel 1 (miniz). `find_shift` busca el desplazamiento del fondo (±12, ±8) en
    una rejilla de 1 de cada 8 píxeles. Medido en el tren (lo peor: varios planos y zoom): unos 20 KB por fotograma
    (~10 Mbit/s; sin el desplazamiento eran 35 KB); pantallas quietas, 300 bytes. Fotograma clave al entrar y si la
    cola de salida pasa de 600 KB (se tira el vídeo atrasado). El invitado le pone SU formato, filtro, colores y bandas
    (`upload_net`); si el anfitrión va en 16:9 llegan 384 columnas.
  - AUDIO: el sonido del núcleo a la mitad de frecuencia (42 kHz) en ADPCM IMA de 4 bits (42 KB/s).
  - Encontrarse: UDP. El anfitrión se anuncia por difusión cada segundo (al 47801) y contesta a las preguntas "OFXQ"
    (al 47800); el invitado pregunta por difusión y además una a una a las 254 direcciones de su red (`udp_sweep`,
    por si el router o el aparato tiran la difusión). `GameActivity` coge un `MulticastLock` de la wifi. Permisos
    nuevos en el manifiesto: INTERNET, ACCESS_NETWORK_STATE, ACCESS_WIFI_STATE, CHANGE_WIFI_MULTICAST_STATE.
  - Hilos: uno que acepta y lee (anfitrión) o lee (invitado), uno que envía la cola (anfitrión) y uno de UDP.
- Menú: OPCIONES → "Juego en red" (`PG_NET`: Crear partida / Unirse a una partida / Volver; `MR_NET_*` los atiende
  main.c; `menu_net_state()` le pasa el estado). El anfitrión crea la partida, vuelve y pulsa JUGAR; el invitado entra
  con START. El invitado no ejecuta la recreativa: bucle aparte en main.c (`net_mode() == NET_GUEST`): pinta lo que
  llega, manda botones (cruceta, 1, 2, 3 y START), "Esperando al anfitrión..." si no llega imagen en 0,7 s (anfitrión
  en el menú o en el launcher), y MENU o perder la conexión lo devuelve a la pantalla de inicio.
- Limitaciones de la fase 1: solo wifi local; el invitado necesita la app con su ROM (la app la pide al abrir, aunque
  de invitado no se usa); START del invitado en combate pausa la partida como el de un 2P local; el anfitrión no puede
  abrir la partida a mitad de juego (solo desde OPCIONES del launcher); sin indicador de retraso (hay `net_guest_ping`).
- Fase 2 (pendiente): internet (UPnP y código de partida como `Internet.kt`), H.264 por hardware si el caudal sin
  pérdida no cabe, invitado sin ROM, botón propio en el launcher.
- Pruebas en escritorio (dos procesos): `OFX_NET=host outfoxies rom.zip <fotogramas>` y
  `OFX_NET=127.0.0.1 OFX_NETSTART=1 OFX_SHOT=inv.ppm outfoxies rom.zip <fotogramas>`; `OFX_NETLOG=1` (tamaño del vídeo
  cada 300 fotogramas), `OFX_NETLOCAL=1` (anuncios también a 127.0.0.1). Comprobado: el invitado ve la partida y entra
  como jugador 2; ida y vuelta de `xor_shift` exacta con 60 desplazamientos al azar. SIN PROBAR EN CONSOLAS.

## Versión 0.17.0: dos armas, escenario en red, formato del invitado, submenú Dificultad, 4 logros más (11-11-2026)
- Prueba del usuario de la 0.16.0 (Odin 3 + Classic, misma wifi, consolas juntas): va perfecto, sin retraso apreciable.
  Fallos que vio: el invitado en 8:7 veía el 16:9 del anfitrión, y SELECT del invitado lo sacaba de la partida.
- Juego en red (`NET_VERSION` 2: hay que actualizar las dos consolas):
  - Formato del invitado: manda su formato (mensaje FORMATO = 6: 0 4:3, 1 16:9, 2 8:7) al entrar y cuando cambia. Si no
    es el del anfitrión, este dibuja el fotograma otra vez a su medida (`nb2_render_for(wide, hud87)`: cambia `ext` y
    `hud87` un momento, sin tocar los parches del programa; guarda y devuelve lo que toca `render_frame`). Con el
    anfitrión en 4:3 no se puede dar 16:9 (los laterales no están preparados): va 4:3. En 8:7 llega el marcador
    recolocado y, en las teles, las franjas de arriba y abajo (imagen de 262 filas). VIDEO lleva un byte más de
    banderas (bit 0 = hay combate: el invitado en 8:7 recorta 20 columnas por lado).
  - SELECT del invitado abre su menú (`menu_open(2)`: Continuar, Pantalla, Sonido, Controles, Idioma, Salir de la partida
    en red, Salir del juego) sin desconectarse: mientras está abierto sigue llegando la imagen (`guest_idle`).
  - Escenario del combate entre dos (página Juego en red → Escenario: Al azar / los 8). En VS el juego baraja una lista
    (0x13B2C, 11 cambios al azar) y guarda el primero en 0x20805C (escenario + 1, mismo orden que los rivales).
    `nb2_set_vs_stage`: 0x13B94 pasa a `jsr $0B00.w; nop` y la rutina de 0xB00 devuelve el byte de 0xB1F si no es 0xFF.
    Probado con los 8. Vale también con dos mandos en la misma consola (`red_escenario=` en opciones.txt).
  - Encontrarse: las direcciones de la consola salen de todas sus interfaces (`SIOCGIFCONF`; getifaddrs no está antes de
    Android 7): la difusión va a la general y a la de cada red, y el barrido de 254 direcciones a cada red. Así funciona
    también cuando una de las consolas crea la red (punto de acceso): su interfaz (ap0, wlan1...) no tiene ruta a internet
    y antes no se veía. SIN PROBAR con punto de acceso.
  - headless: `OFX_P2=n` (en el fotograma n entra el 2P con moneda y START), `OFX_VS=rival`.
- Dos armas (OPCIONES → "Dos armas", `dos_armas=`; también en el menú de partida). Botón 3 ("Botón 3 (cambiar arma)" ya
  sale en Controles; táctil: el botón de disparo en ámbar, encima del de salto, solo con la opción puesta):
  - El juego ya lleva un arma "en el bolsillo" a los gemelos: al coger otra guarda la vieja (0xCFEA, hueco 0x235412 +
    jugador*8) y la saca sola cuando se quedan sin la de la mano (0x97FE; 0x23540E es su hueco de la mano). Esa arma es un
    objeto en estado "llevada" ($4E = 1, $82 = el luchador): va pegada, no se ve, no se puede coger y no gasta.
  - Lo mismo para cualquiera con el hueco `TW_RES` = 0x27FF00 (+4 el 2P; RAM que el juego no usa en 9000 fotogramas de
    prueba, va en partidas y rebobinado). `two_weapons_frame()` (nb2.c, al final de cada fotograma) mira el flanco del
    bit B3 de cada jugador humano: guarda el arma (luchador $82 = -1, $5E = $60 = 0) o la cambia por la reserva
    ($82 = reserva, $5E = tipo + 2, $60 = munición; el juego rehace $5E/$60 desde $82 en 0x97C4). Gemelos: también su
    hueco de la mano; con el suyo propio llevan tres.
  - Solo de pie quieto o andando (estado $4E 0 o 2); si no, espera hasta 30 fotogramas. Cambiar agachado con arma (0x29)
    dejaba al luchador atascado. Estados vistos: 0 quieto, 2 andar, 10 girar, 14/15 salto, 29 aterrizar, 39 coger,
    41 agachado con arma, 38/40/42 caer del escenario.
  - Marcador: un cajón pequeño (55 %) con la reserva bajo el cajón del arma (1P x 94, 2P x 176, y 40; en 8:7 bajo el
    tiempo). El dibujo se copia del propio cajón cuando ese tipo de arma estuvo en la mano (`tw_hud`).
  - Vibración corta al cambiar (`NB2_EV_SWAP`). En red el botón 3 del invitado ya viaja.
  - Probado (`tools/bancos/dos_armas.c`): pistola + pistola (acuario, circo, barco), lanzacohetes + lanzacohetes (fábrica)
    y con los gemelos: dispara con la de la mano, la de la reserva no gasta, y se cambian bien varias veces.
  - Banco de pruebas: `tools/bancos/mk.sh prueba.c salida` compila contra el núcleo sin SDL. Para coger armas: teletransportar
    al 1P encima (+0x14 x, +0x18 y, palabras; y crece hacia arriba) y pulsar abajo.
- Submenú "Dificultad" (OPCIONES y menú de partida): Dificultad, Daño que recibes, Daño que haces.
- Logros: los 4 que faltaban ya se pueden conseguir (20 de 20):
  - Helicopter Insurance: ganar a John Smith con el 1P aún en la azotea (y >= 0x0740; azotea 0x0777-0x0990, comedor
    0x0640-0x06C7; todos caen al comedor cuando explota el helicóptero, hacia el segundo 90).
  - Going Down: tiempo agotado en el rascacielos con el 1P en el foso de granadas (sótano a la izquierda del garaje,
    7 granadas en y 0x67, x 0x150-0x1B0): y < 0x0280 y x < 0x01C0.
  - Fly, My Pretty: en el avión, la y del rival baja de -0x40 (cae al vacío; el juego lo devuelve con menos energía, no es KO).
  - It's Even Funnier The Third Time!: en el circo, el 1P dispara las 3 pistolas de juguete ($26 del objeto = 1-3;
    salen hacia el segundo 170 abajo del circo; al disparar la munición pasa de 7 a 0).
  - Comprobados con headless (`OFX_ACH`, `OFX_POKE` para llevar a los luchadores al sitio) salvo el de las pistolas
    (el bot solo llegó a disparar 2: la lógica se vio con el banco `tools/bancos/pistolas_juguete.c`).
- Comprobado en servidor: sin opciones nuevas, 4:3 y 16:9 con traducción, imagen y sonido idénticos byte a byte a la 0.16.0.
  Red en escritorio: invitado en 8:7 y en 4:3 con el anfitrión en 16:9 (capturas bien).

## Versión 0.27.0: launcher de 7 botones, portugués e italiano, packs de ropa incluidos, logros y manual (16-11-2026)
- Launcher: 7 botones, OPCIONES LOGROS GALERÍA [JUGAR] MANUAL EDITOR-DE-ROPA SALIR (`LB_*` en ese orden; una fila
  212/286 sobre 1920; dos filas GALERÍA [JUGAR] MANUAL / OPCIONES LOGROS ROPA SALIR). CONTROLES, PARTIDAS y TRUCOS van
  dentro de OPCIONES (con el launcher, todas vuelven a OPCIONES salvo LOGROS, que vuelve a los botones). Rótulos en 4
  idiomas, "EDITOR DE ROPA" en dos líneas (`make_launcher.py` admite '\n'); paneles: opciones azul, logros plata,
  galería turquesa, jugar acero, manual naranja, ropa rosa, salir rojo.
- Idiomas del menú: `LANG_ES, LANG_EN, LANG_PT, LANG_IT`. `TR(es, en)` es ahora `tr()` (`src/app/lang_ui.c`): castellano
  o inglés tal cual; portugués e italiano buscando el texto en castellano en `lang_ui.h` (búsqueda binaria + caché por
  puntero). Las tablas `x[2][N]` de menu.c se leen con `TR(x[0][i], x[1][i])`; net.c y achievements.c también.
  Traducciones en `tools/lang_ui_pt_it.py`; `tools/make_lang_ui.py` genera el .h y falla si falta algún texto
  (`tools/lang_extract.py` los saca del código) o no coinciden los %d/%s. Letra: añadidas àâãçèêìòôõù y mayúsculas.
- Textos del juego: `cfg.game_lang` = `GLANG_EN/ES/PT/IT` (0-3). `tools/make_lang.py` genera `src/core/lang_game.h`
  con `lang_es`, `lang_pt` y `lang_it` (portugués e italiano en `tools/lang_game_pt_it.py`). nb2.c guarda una copia del
  programa (1 MB) antes del primer parche y al cambiar de idioma devuelve lo del anterior. Letras nuevas en los bytes
  0x61-0x6B (minúsculas, que el juego no escribe: comprobado contando losetas en demo y combate): À È Ì Ò Ù Ã Õ Â Ê Ô Ç,
  hechas de la fuente del juego (acento grave, circunflejo, virgulilla; la Ç es la C entera con la cedilla en la fila 8).
  Mensajes fijos que no caben y no tienen punteros: hay que acortarlos (EACH ARE DEAD: "AMBOS MORTOS...", "ENTRAMBI MORTI.").
- Ropa: ya no salen los estilos calculados; Ropa = Recreativa + packs. Los 6 packs del usuario (Gótica, Negativa, Pinky,
  Vampiros, Mortal Kombat, Street Fighter; `assets/ropas/`, `tools/make_packs_incluidos.py` → `packs_incluidos.h`) se
  copian a `ropas/` la primera vez (marca `.incluidos_1`) y se borran los "Pack ..." de prueba de la 0.26.0. El "Original"
  que mandó era igual a la recreativa: no va. Configuraciones antiguas: `ropa=1-4` pasa al pack con ese nombre.
- Logros 21-23: iconos con imágenes del juego dentro de la tele de los de RA (`tools/make_ach_icons_juego.py` con 3
  recortes de capturas: tren con Bernard caído delante de la locomotora, explosión en la fábrica, gemelos en el agua).
- Manual: panel "Datos" bajo el retrato de cada ficha con lo de la Wikipedia en castellano (edad, altura y peso pasados
  a metros y kilos, origen, oficio y el remate sobre la tele de Mr. Acme).
- Auditoría: sin avisos de -Wall -Wextra que importen (los de sangría y `fallthrough` comentados son a propósito);
  quitadas variables sin usar; `recv_msg` (red) ya no pierde el búfer si falla realloc; revisados los límites de lo que
  llega por red (vídeo, audio, UDP) y del lector de packs. README puesto al día.

## Versión 0.26.0: editor de ropa (botón ROPA), packs de ropa, logros por tipos, manual retocado (16-11-2026)
- Editor de ropa (`src/app/skineditor.c`, botón ROPA del launcher, panel rosa = azul girado 112°; ahora 10 botones:
  una fila OPCIONES CONTROLES PARTIDAS GALERÍA [JUGAR] MANUAL ROPA LOGROS TRUCOS SALIR (146/210 sobre 1920); dos filas
  PARTIDAS GALERÍA [JUGAR] MANUAL LOGROS / OPCIONES CONTROLES ROPA TRUCOS SALIR, ROPA debajo de JUGAR).
  - Izquierda: el personaje dibujado desde la ROM (paleta en `skin_chars[].pal_rom`, postura = losetas del 1P quieto al
    empezar, `src/app/skin_poses.h`, solo números). Derecha: una fila por prenda con su cajón de color; A abre 96 colores
    (6x16) y el personaje cambia en directo con el color bajo el cursor; X deja la prenda original; Y cambia el nombre.
    Botones: Nuevo, Abrir, Ponérmela (ropa del jugador 1), Importar, Exportar. Todo se guarda solo.
  - Pack = `ropas/<nombre>.txt` en la carpeta de la app: líneas `personaje.prenda=RRGGBB` (`src/core/skins.c`, prendas de
    la 0.18.0). Color: cada entrada de la prenda guarda su luz comparada con la media de la prenda y se pinta con el color
    elegido a esa luz (si pasa de 255, el brillo va al blanco). En el juego, `nb2_set_outfit_pack` y `pack_lut` (rehecha
    si cambia el pack, el personaje o la paleta del banco).
  - OPCIONES > Pantalla > Ropa jugador 1 / Ropa del rival: tras los 5 estilos salen los packs (`cfg.outfit_pack`, por
    nombre: `ropa_pack=` y `ropa_rival_pack=` en la configuración).
  - Android: Importar = mensaje 0x8007 con 3 (selector; un .txt o un .zip con varios .txt, se copian a ropas/);
    Exportar = 4 (ACTION_CREATE_DOCUMENT con ropa_exportar.txt). SIN PROBAR en consola.
  - `tools/packs_estilos.c`: saca "Pack Original/Gótica/Negativa/Pinky/Vampiro" de los estilos (color típico de cada
    prenda pasado por el estilo). Se parecen al estilo, no son idénticos (la Negativa no invierte las luces).
- Logros: el menú los enseña por tipos (`ach_order`): acciones, los de cada escenario en el orden de los escenarios y los
  de terminar el juego, con Fácil y Difícil al final. El número de cada logro no cambia.
- "Toma, te lo devuelvo" (22), simplificado (al usuario le saltó con un golpe cualquiera): al Prof. Ching le baja la
  energía y hay una explosión (objeto clase 0x1A, de 0xB4 en 0xB4 desde 0x208D06) cuyo +0x58 es él (0x208DBA = a quién ha
  dado), con el 1P llevando granada o lanzacohetes ahora o hace < 3 s. Comprobado con un banco: los golpes normales no
  cuentan, las explosiones sí.
- Manual: "Armas" en la portada; Mr. Acme como las fichas (foto de Mr. y Mrs. Acme en su despacho, sin las estrellas);
  katana y "Cómo se usan" con los textos del usuario; finales sin pies (fotos más grandes); MANSIÓN con la panorámica
  entera sacada del juego sin las letras (`tools/manual_mansion/`, LEEME.txt). Las capturas que faltaban (retratos,
  escenarios, teles, saldo) se recortan del manual anterior del usuario: `tools/manual_recortar_v2.py <carpeta jpg> <salida>`
  (y `make_manual.py` acepta .png en lugar de .ppm).

## Versión 0.25.0: logro del cohete definitivo, manual corregido y ampliado, packs de galería (14-11-2026)
- "Toma, te lo devuelvo" (22): el "cohete del ascensor" es el LANZACOHETES (arma tipo 2; 4 en 0x208D65) que suben los
  ascensores de la fábrica (cartel ROCKET). Saves del usuario `Cohete_logro` (partida1/2): en la 2 Ching lleva uno y se
  revienta a sí mismo (explosión clase 0x1A, 35, a los dos). Regla: Ching (rival 4) pierde >= 24 de golpe mientras el 1P
  lleva el lanzacohetes o lo llevaba hace < 3 s, y sin granada en la mano.
- Manual (30 páginas; `tools/make_manual.py`): número de página centrado abajo, correcciones del usuario (tiempo: pierde el
  de menos vida; lagarto de Eve y granadas; Bernard sin cortar la cabeza; gemelos con un arma cada uno; textos de circo,
  acuario, tren, fábrica, avión, barco y mansión), 2 páginas de armas (iconos verdes recortados del panel "Cómo jugar" de
  la galería en castellano) tras Mr. Acme, página de finales nueva (con la tele del saldo de Eve) y 3 fotos por final.
  - Fotos de los finales: saves del usuario `Final_<personaje>.sav` (ya en los créditos). Los cuadros de 128x96 están
    quietos a la izquierda mientras caen billetes: `tools/manual_fotos_finales.py` junta los fotogramas de cada cuadro y
    saca la mediana (sin billetes). Headless cada 2 fotogramas, 3200 fotogramas.
  - Retratos: `tools/manual_retratos.py` (elección de personaje con `OFX_MASK=3f`, diferencia con otros dos fotogramas).
  - (Hecha en la 0.26.0) panorámica de la MANSIÓN.
- Galería: `tools/packs/montar_galeria.py <es|en|jp>` → Outfoxies_Galeria_Castellano/English/Japon.zip (7 imágenes: folleto
  portada y reverso, panel "Cómo jugar", marquesina, caja, caja con personajes, logotipo). Pies del pack japonés en
  castellano (la letra del menú no tiene japonés).

## Versión 0.24.0: GALERÍA y MANUAL, 3 logros nuevos, lanzamisiles negro, varios logros a la vez (14-11-2026)
- Logros a la vez: `ach_pop` se llamaba cada fotograma y el 2.º aviso pisaba al 1.º al instante (los dos se guardaban, pero
  solo se veía el último). Ahora el siguiente sale cuando acaba el aviso anterior. OJO: en Fácil (o daño más fácil que
  Normal) solo cuenta "La siguiente en Normal": es lo decidido (como RA); el usuario lo vio con Eve en Fácil.
- Lanzamisiles ILIMITADO negro (partida1.sav del usuario, Eve contra Mr. Acme): era la ROPA DEL RIVAL. Contra Mr. Acme
  (rival 7) el banco de sprites 2 no es el del rival (los Acme van en el banco 0) y lleva los colores del cajón del arma y
  de los cohetes del marcador; la detección de asientos (`seat_bank`) los tomaba como el rival. Ahora con rival 7 no se
  recolorea el banco 2 ni sus asientos (`ofit2` en nb2.c).
- Logros nuevos (índices 21-23, propios; `ACH_COUNT` 24; iconos dibujados a mano con el marco de tele del 20,
  `tools/make_ach_icons_propios.py` (los escribe con un argumento), añadidos a `assets/logros/iconos.b64`):
  - 21 "Asalto al tren del moreno" (Great Train Robbery): Bernard (rival 3) cae del tren = el 2P pasa al estado $4E 133
    (luego 134 y vuelve en la vagoneta). Sale cayendo por delante y por detrás del tren.
  - 23 "Daros un bañito" (Bath Time): los gemelos (rival 6) caen del barco al agua = estado 130 (luego 131 y 63, vuelven).
  - 22 "Toma, te lo devuelvo" (Return To Sender): PROVISIONAL. En la fábrica, al Prof. Ching le baja la energía >= 24 de
    golpe sin que el 1P haya llevado en el último segundo un arma (tipos 2-12 de 0x208D65). No se ha visto el cohete del
    ascensor en el servidor: los objetos clase 0x3F de x 144 son las plataformas del ascensor (paternóster, cada 128), no
    se pueden coger. Pedir al usuario una partida guardada con el cohete en la mano o justo antes de lanzarlo.
  - Comprobados con `tools/bancos/caidas.c` (lleva el combate, teletransporta al rival por x y llama a ach_frame).
- Launcher: 9 botones. GALERÍA (panel turquesa: azul girado -22°) y MANUAL (naranja: +172°), a los lados de JUGAR.
  Una fila: OPCIONES CONTROLES PARTIDAS GALERÍA [JUGAR] MANUAL LOGROS TRUCOS SALIR (150/214 px sobre 1920). Dos filas:
  PARTIDAS GALERÍA [JUGAR] MANUAL LOGROS / OPCIONES CONTROLES TRUCOS SALIR (190/244 sobre 1240). `launcher_gfx.h` ahora
  trae tablas `lg_panel[]`, `lg_text_es[]`, `lg_text_en[]` (LG_N).
- Visor de imágenes (`src/app/viewer.c`, como el de Re-Master-ed of Darkness): packs opcionales `galeria.zip` y `manual.zip`
  (.jpg/.png; el nombre sin el número es el pie) en la carpeta de la app (la de las partidas). Sin pack, pregunta y abre el
  selector de Android (mensaje 0x8007 → `GameActivity.onUnhandledMessage`; copia en otro hilo a .tmp y renombra; el visor
  mira cada 0,5 s si el archivo ha cambiado). Mando: ←→/L1 R1 pasan, A acerca, X cambia de pack, B cierra; táctil:
  deslizar, bordes, doble toque, pellizco. Textos con la letra del menú (`menu_ext_text`); la grande ya tiene ÁÉÍÓÚÑ.
  Probado en escritorio (OFX_MENUKEYS=ro abre MANUAL). SIN PROBAR en consola el selector de archivos.
- Manual: `tools/make_manual.py <carpeta capturas> <salida>` → PDF de 28 páginas 1920x1080 y `manual.zip` (el pack).
  Portada, historia, cómo se juega (daños), 7 fichas + comparativa + Mr. Acme, 8 escenarios y finales (huecos "FOTO
  PENDIENTE": el bot no llega a derribar el helicóptero; fotos y resumen de cada final, del usuario: `fin_<nombre>.png`).
  Capturas: retratos de la elección de personaje sin cursor (`OFX_MASK=3f`, lo demás oscurecido), escenarios en 16:9 a los
  400 fotogramas del combate (`tools/bancos/esc.c`), teles de Mr. Acme. Nada de eso va al repositorio ni al APK.

## Versión 0.23.0: agua del acuario como el original, logros traducidos y nuevos (13-11-2026)
- Acuario (partida del usuario `partida1.sav`, captura 13-11): con el agua, la pantalla entera salía verde. Comparado
  con MAME 0.264 (instalado con apt en el servidor: `/usr/games/mame`, script Lua que mete moneda, START, invulnerable
  y escenario 2 con las mismas direcciones que nuestros trucos; capturas cada 300 fotogramas): en el original solo es
  verde lo que queda por debajo del nivel del agua. El juego lo hace con la interrupción de línea (nivel 5, rutina
  0x1714 -> tabla en 0x1732 -> 0x173C): en la línea `c116reg(5) - 32` cambia el color de los dos planos ROZ
  (0x740002 = (A6+0x186) | 6 y 0x740012 = (A6+0x1AC) | 7, paleta verde). Nosotros pintábamos el fotograma entero al
  final con los registros ya cambiados. Ahora `nb2_frame` guarda los registros del ROZ (0x740000-0x74001F) al empezar
  cada línea visible y `draw_roz` pinta por tramos de líneas con sus registros. Comprobado contra MAME en varias
  alturas del agua; los otros 7 escenarios en 16:9 (4000 fotogramas, con las teles) salen idénticos byte a byte.
  Depuración: `OFX_TR`/`OFX_LO`/`OFX_HI` ahora dicen la línea (`L183`) y la interrupción de línea.
- Logros:
  - Nombres en castellano (con el menú en inglés, los de RA). Ya no se cortan a 21 letras: `row` acorta por lo que
    mide el texto, y todos caben (medido con la fuente: hueco 215 px con el valor seleccionado).
  - Sin puntos (el usuario: no aportan nada). Abajo solo "N de 21 logros".
  - Pistolas de juguete: basta con disparar una ("¡Pum, de juguete!" / It's Only A Toy!). Se mira la que llevaba el
    1P en el fotograma anterior (al disparar se queda sin munición). Comprobado con `tools/bancos/pistolas_juguete.c`.
  - Difícil -> "Casi difícil": salta al terminar con cualquier ajuste algo más duro que Normal (recibes más del 100 %
    o haces menos, sin que lo otro sea más fácil). `ach_frame(mem, diff)`: -1 más fácil, 0 Normal, 1 más difícil.
  - Nuevo (índice 20): "La siguiente en Normal" (Next Time On Normal): terminar el juego, también en Fácil. Icono
    propio (tele del icono 13 con pantalla verde "FACIL" tachado, flecha y "NORMAL"; `/tmp/icon20.py` del 13-11,
    escrito en `assets/logros/iconos.b64` en el sitio del 21, "High Roller", que no se usaba).
  - headless: `OFX_ACHDIF=-1/0/1`. Probado: en Fácil solo el nuevo; en Normal el del personaje; más difícil, también
    "Casi difícil".
- Pendiente: el lanzamisiles ilimitado del final (Mr. Acme) se ve negro, icono y cohetes. El bot no llega a cogerlo:
  hace falta una partida guardada del usuario en ese momento (y saber qué ropa y pack de armas lleva).

## Versión 0.22.0: arreglo de logros (13-11-2026)
- Prueba del usuario: saltaron Action Man (John Smith), Eat Lead!, Hobbyist Vampire Hunter, By ANY Means y Helicopter
  Insurance. NO saltaron: terminar con Bernard, Betty y Eve, ni Rising Sea Levels.
- Terminar con cada personaje: la lista de rivales (0x20807C, parejas estado/rival) NO pone 0x00 al vencido: pone el
  número del personaje que lo ha vencido (0xFF = pendiente). Con John Smith (el 0) coincidía. Ahora "vencido" = != 0xFF.
  Comprobado con headless (Dweeb, la lista tocada con OFX_POKE): antes nada, ahora "He's Got A Gun!".
- Rising Sea Levels (y cualquier logro): al rebobinar o cargar partida se llamaba `ach_reset`, que olvidaba el asalto
  en curso: lo que pasara después en ese asalto no contaba. Ahora `ach_reset(ACH_NEW / ACH_REWIND / ACH_LOAD)`:
  rebobinar mantiene el asalto; cargar a mitad de asalto lo retoma, pero sin los que dependen del asalto entero
  (Officially/Acme Outfoxed, Eat Lead!, Demolitions Expert, By ANY Means: `hurt = 1`, `held = ~0`). La lógica del
  acuario era correcta (comprobada con OFX_POKE del reloj y con OFX_ACHRES).
- headless: `OFX_ACHRES=n` (en el fotograma n hace lo mismo que al cargar partida).
- The Outfoxiest: el original SÍ tiene dificultad, en el menú de test del operador (no lo ve el jugador): 0x20810A =
  0/1/2 (fácil/normal/difícil), sale de la configuración guardada (rutina 0x12C0). Solo se ha visto que cambia el daño:
  tabla 0x62054 = 320/348/384 (rutinas 0x305A y 0x30C4). El logro de RA se refiere a ese ajuste. Nuestro Difícil
  (200/50) es mucho más duro. Pendiente de que el usuario decida.

## Versión 0.21.0: internet por servidor intermedio, daño de objetos lanzados (12-11-2026)
- Prueba del usuario: con su router no hay forma de entrar desde fuera (CGNAT). Se descarta seguir por ahí.
- Servidor intermedio (`inet.c`, `relay_loop`): si no se ha podido abrir el puerto (CGNAT, dos routers, sin UPnP, datos
  móviles o sin IP pública), el anfitrión se conecta HACIA FUERA a un servidor de túneles con el protocolo de bore
  (github.com/ekzhang/bore; por defecto `bore.pub:7835`, cambiable con `red_servidor=host:puerto` en opciones.txt).
  Control: JSON terminado en byte 0. `{"Hello":0}` → `{"Hello":puerto}`; luego "Heartbeat" cada 0,5 s y
  `{"Connection":"uuid"}` por invitado; el anfitrión abre otra conexión al 7835, manda `{"Accept":"uuid"}` y esa conexión
  es ya la del invitado (`net_host_offer` → `host_thread`, igual que un accept). Sin latidos 15 s o caída: reconecta
  (el puerto y el código cambian). 4 fallos seguidos: vuelve al mensaje de antes (CGNAT, datos...).
  - Código: el mismo de 8 letras con "IP" 0.0.x.x = puerto del servidor (0.x.x.x no existe en internet). El invitado lo
    resuelve con `inet_code_target` (nombre del servidor + puerto); `net_join_addr` usa getaddrinfo (nombres, IPv4 e IPv6).
  - Menú: "Código para internet: XXXX-XXXX (por el servidor intermedio bore.pub)". Fallo al entrar: avisa que el código
    cambia cada vez.
  - Probado en escritorio con `tools/fake_bore.py` (servidor bore mínimo): `OFX_RELAY=127.0.0.1:7835 OFX_FAKEIF=movil
    OFX_NET=host` y el invitado con `OFX_NET=codigo:...`: entra y ve la partida. `OFX_NORELAY=1` lo desactiva.
    SIN PROBAR con bore.pub de verdad (el servidor de Claude no llega a internet): no se sabe su caudal ni su retraso
    (está fuera de España; esperar algo más de retraso que en la wifi). Si bore.pub no vale: otro servidor bore
    (`bore server` en cualquier PC con el puerto abierto, p. ej. el de Mckeijas) con `red_servidor=`.
  - IPv6: no hecho. Haría falta un código de ~28 letras y casi todos los routers bloquean la entrada por IPv6.
- Guía (documento "Personajes"): daño de la granada (explosión 35, como el cohete; la granada al golpear 1), sopa 10,
  frutas de la cesta 0/1/2 (7 frutas, subtipos 7-13: la 7 quita 0, la 12 quita 2, el resto 1), carbón 1, tarta 0,
  caer al vacío 25 (0x7A98). Sin identificar: 20 en el tren (0x7B1C, en salto) y 10 al caer encima (0xCD68).
  Método: `tools/bancos/golpes.c` con el gancho de instrucciones de Musashi en 0x305A (D0, quién llama, víctima, objeto)
  y todas las armas cambiadas por un tipo (`ARMA=`): la máquina las lanza al jugador (invulnerable). Rutinas: 0xCE00
  arma que golpea ($5C del arma), 0xD040 explosión (clase 0x1A), 0xD5B4/0xD61A puño (clase 0x3E: daño de la tabla
  0x61F0C + personaje*4 + $7E) y placaje, 0xD6AA clase 0x3F (15).
- Iconos de logros: HECHOS. Sacados con el Chrome del usuario de media.retroachievements.org/Badge/<id>.png (ids en el orden
  de achievements.c: 291765 291755 291754 291761 291766 291759 291757 291760 291758 291763 291756 291769 291762 291768
  291767 291902 291764 291904 291901 291905, y 291903 "High Roller", que no usamos), reducidos a 32x32 RGB555 en el
  navegador, comprimidos y pegados en GitHub (`assets/logros/iconos.b64`; el portapapeles fue la única forma: una URL con
  los datos bloquea la extensión). `tools/make_ach_icons.py` → `src/app/ach_icons.h`. En Logros, cada fila lleva su icono
  a 16x16 con marco oscuro; sin conseguir, en gris oscuro. Debajo, el del logro elegido a 32x32 junto a la descripción
  (`wrap_dx` corre el texto). El aviso "Logro: ..." lleva el icono a 32x32 (`menu_set_osd_icon`: solo si el texto del
  aviso es el de ese logro).
- Internet: el usuario lo da por ZANJADO (12-11-2026). Con su router no entra nadie, pero creando la partida Mckeijas
  funciona. No seguir con IPv6 ni con otros servidores salvo que lo pida.

## Versión 0.20.0: ropa en asientos del escenario, cajón de reserva, sangre, internet con CGNAT/datos (12-11-2026)
- Prueba del usuario: charco del helicóptero "exacto" de tamaño y aspecto; lo quiere un poco antes y más lento.
  Ahora empieza a los 10 + 90 fotogramas (antes 10 + 120) y crece 1 px por fotograma (antes 2).
- "Dos armas" también se quita al arrancar la app, como los demás trucos (`config_reset_cheats`).
- Cajón de la reserva: se copiaba 29x31 del cajón grande, que en realidad mide 30x32 (marco de 2 px por lado; el
  derecho y el de abajo se quedaban a medias) y encima se le ponía un borde oscuro que pisaba la fila de abajo del
  grande. Ahora se guarda 30x32 y el pequeño (18x19, 1 px por debajo) se monta con el mismo marco de 2 px (cada
  anillo del anillo igual del grande) y el cristal reducido con media de color. En 8:7 la parte del cajón en
  `draw_hud87` también pasa a 30x32.
- Ropa en los sitios donde el escenario dibuja al luchador (metralletas y puesto del piloto del avión, cañón del
  circo...): ahí el luchador no es un sprite de su banco, es parte del dibujo de losetas del escenario, con una paleta
  de losetas que el juego llena al empezar con los colores del personaje (John en la metralleta: 0x1909-0x192D = sus
  0x10-17, 0x20-24, 0x30-37, 0x40-47, 0x50-53, 0x60-62; el rival en 0x194B-0x1972; copias también en el banco de
  sprites 8 y en 0x1B-0x1F). `seat_update`/`seat_bank` (nb2.c) buscan en todas las paletas salvo los bancos 1 y 2
  tramos de 3 a 8 colores seguidos iguales a colores seguidos de un luchador (2-0x8F; sin negro ni magenta), los
  alargan hasta su bloque de 8 con colores que no estén en su paleta y exigen 2 partes distintas y como mucho 72
  colores por banco (en los cambios de escena el juego copia paletas enteras; en la fábrica hay rampas sueltas que
  coinciden por casualidad). Se rehace por banco cuando cambian sus colores (suma de comprobación). Solo con ropa
  puesta y en combate. Visto: metralleta (estado 128) y cabina (estado 69) del avión con la ropa vampiro. Circo:
  el banco 0x0C lleva pelo y camiseta de John (seguramente el cañón; no se ha llegado a ver dentro).
  También el tren (banco 0x0B con 6 partes del rival). headless/bancos: `OFX_SEAT=1` apunta lo encontrado.
- Internet (prueba del usuario: en su wifi salía CGNAT; con datos móviles decía "el router no tiene UPnP"):
  - Datos móviles: si no hay red local (wifi, cable, punto de acceso) y sí rmnet/ccmni/pdp/seth → "con datos móviles
    no se puede entrar: únete tú a la del otro, o usad Tailscale" (`INET_MOBILE`). "Abierta en la wifi" ya no sale con
    la IP de datos (`local_ip` devuelve 0).
  - Si la IP externa del router (UPnP) es 192.168/10/172.16 → `INET_DOUBLE` (dos routers en cadena: abrir el 47800
    en el del operador). Si es 100.64/10 o no cuadra con ipify → CGNAT.
  - Tailscale/ZeroTier/WireGuard: si hay una interfaz tun*/tailscale*/zt*/wg* con IPv4 (mejor 100.64/10), el código
    lleva ESA IP y el puerto 47800 sin UPnP (`INET_VPN`). Con Tailscale en las dos consolas (misma cuenta o
    compartiendo el aparato) se juega aunque haya CGNAT o datos en los dos lados. SIN PROBAR en consola.
  - Escritorio: `OFX_FAKEIF=vpn|movil`. En la página Juego en red la pista gris solo sale si cabe (se pisaba).
- Datos de personajes (documento "Personajes" en Claude Docs, actualizado):
  - Tabla de movimiento 0x61AE2: registros de 8 x (velocidad, aceleración) SEGUIDOS del estado al que valen
    (fin -1). Estados: 2 andar, 3 (?), 5 correr (doble toque), 17 (?), 20 (todos 48), 24 salto con carrerilla,
    87/88 placaje y su salida, 110/111 copias de andar/correr, 114/115 retroceso al recibir, 123/131 (?).
    Correr: John 64/24, Eve 80/20, Betty 48/256 (al instante), Bernard 96/11, Ching 128/16, Dweeb 72/22, DyD 56/24
    (velocidad/16 = px por fotograma; la aceleración va en 1/128 px por fotograma, medido).
  - Otros estados vistos: 6 frenar, 7-9 agacharse, 13 colgado, 19 trapecio, 21-23 salto andando, 31 deslizarse,
    69 cabina del avión, 75/76 puñetazo, 79/80 agachado, 82 en el aire, 127 salto desde agachado, 128 metralleta.
  - Daño base (D0 de 0x305A; el arma lo lleva el objeto que golpea en $5C): puñetazo 1 (Bernard 3), placaje Bernard
    12, Ching y DyD 6, John y Dweeb 5, Eve y Betty 4; pistola 20, cohete 35, katana 15, lanzallamas 10, látigo 5,
    metralleta 2. Medido con `tools/bancos/dano.c` (rival pegado; con ARMA=n todas las armas de ese tipo) y
    comprobado con la fórmula (vs Betty en normal, rival de la máquina al principio: 1,2824 por punto).
    Granada y objetos lanzados: sin medir (el bot no los lanza bien).
  - `0x93190`: tabla de rutinas por clase de objeto ($12): clase 0x14 arma 0x185E8, 0x56 0x224F6, 0x98 0x29100.
  - Bancos nuevos en `tools/bancos`: rejilla.c (busca sitios donde el 1P entra en un estado raro teletransportándolo),
    dano.c, movs.c (estados por combinación de botones), vel.c. `mk.sh` enlaza inet_code.c y Musashi.

## Versión 0.19.0: sangre del helicóptero, juego en red por internet, datos de los personajes (12-11-2026)
- Sangre del helicóptero (mansión): nuestro núcleo no crea el charco, así que se dibuja (`heli_blood`/`heli_frame` en nb2.c).
  Con `partida3.sav` del usuario: el helicóptero estrellado es el sprite de formato 0x381 con paleta 0 (banco 0, el de los
  Acme y los objetos) y solo en la mansión (rival 7). Aparece en el fotograma ~660 de esa partida, cae en ~10 fotogramas y
  se queda con la base en vpos + 101/256 de su alto (y = 207; el sprite sigue por debajo, tapado por el suelo). A los
  10 + 120 fotogramas sale una línea roja de 2 px a ras de suelo, centrada, que crece 2 px por fotograma hasta 100 px (todo
  a escala del sprite: 160 de ancho en esa partida). `render_sprites` apunta lo que ocupa (`bb_*`). No va en el estado de
  las partidas (si se carga una partida con el helicóptero ya en el suelo, el charco vuelve a salir a los 2 s).
  headless: `OFX_HELI=1` (posición por fotograma), `OFX_OBJS=n` (objetos de la RAM), `OFX_IDX=banco` (ver 0.18.0).
- Juego en red por internet (fase 2, `src/app/inet.c`, `inet_code.c`):
  - Al crear partida se abre además, en segundo plano, el puerto en el router con UPnP (SSDP → descripción → servicio
    WANIPConnection/WANPPPConnection → AddPortMapping del 47800+k al 47800 de la consola, k = 0..15 si está ocupado; 4 h y si
    no, permanente) y se pregunta la IP pública (GetExternalIPAddress; si no hay UPnP, api.ipify.org por HTTP). Al cerrar la
    partida o la app, DeletePortMapping. Probado con un router UPnP simulado (`fake_igd.py`, no está en el repo): puerto
    ocupado → 47801, CGNAT detectado, borrado al salir.
  - Código: 8 letras (IP 32 bits + k 4 bits + comprobación 4 bits, XOR 0xA5C3E1F7, base 32 sin 0/1/I/O), se ve como
    ABCD-EFGH en Juego en red. Estados: preparando, código (UPnP bien), código + "abre el puerto 47800 TCP" (sin UPnP),
    CGNAT (IP del router privada o distinta de ipify: que cree la partida el otro), sin internet.
  - Unirse: Juego en red → "Unirse por internet (código)" → página de 8 casillas (←→ casilla, ↑↓ letra, A siguiente /
    Conectar). El último código se guarda (`red_codigo=`). `net_join_addr(ip, puerto)`, 5 s de espera.
  - Si la red no da abasto (más de 120 KB en cola), el anfitrión no manda la imagen de ese fotograma (la siguiente va
    contra la última enviada): por internet lento baja la cadencia en vez de cortarse. El sonido va siempre.
  - Escritorio: `OFX_NET=codigo:ABCDEFGH` entra con un código (127.0.0.1 = 5MB8-D7SG). SIN PROBAR por internet de verdad.
- Datos de los personajes para el manual (de las tablas de la ROM; ver documento "Personajes" en Claude Docs):
  - Andar: tabla 0x61AE4 (lista por estado $4E, 8 personajes x (velocidad, aceleración), fin con -1); velocidad/16 = px por
    fotograma. Estado 2 (andar): John 35, Eve 38, Betty 34, Bernard 36, Ching 28, Dweeb 32, Danny y Demi 33.
  - Salto: velocidad inicial 0x622B0 (largo 16.16 por personaje): 5,75 / 6,5 / 5,5 / 5,56 / 6,25 / 6,38 / 5,44 (Acme 9,56);
    gravedad medida 0,28 px/fotograma². Control en el aire 0x622D0 (+personaje*32: 4 aceleraciones y 4 máximos).
  - Daño recibido (0x3058 rutina, 0x305A): base × dificultad (0x62054: 320/348/384 /2) × defensa del que recibe (0x6205A:
    John 209, Eve 217, Betty 225, Bernard 192, Ching 250, Dweeb 230, DyD 256, Acme 256; /2) y, si el que recibe es la
    máquina, × 0x6206A[($98,A6)] / 64; si es humano, × 4. Energía en el luchador +$52 (largo 16.16); 0x208D59 es una copia.
  - Puntería (aleatoria entre mínimo y máximo, 65536 = 360°): pistola 0x61F34 + personaje*32 + dirección*8 + 4, lanzacohetes
    lo mismo + 0, metralleta 0x62034 + personaje*4. Pistola de frente: John ±5°, Eve ±7°, Betty ±2°, Bernard -5..0°, Ching
    ±12°, Dweeb ±15°, DyD ±10°. (Rutinas 0x1D180 pistola/metralleta y 0x1A2AE cohetes.)
  - Altura en losetas de 16 (escaleras): 0x61EFC: 4 4 3 4 3 2 3 (Acme 4).
  - Puño (medido con el bot, no de tabla): daño base 1 todos salvo Bernard 3. Sin confirmar.
  - Munición inicial por arma (0x443A6 + tipo*16, +0x0A): pistola 7, cohetes 3, metralleta 80, lanzallamas 300, cesta 6.
- Comprobado en servidor: 4:3 sin opciones, imagen y sonido idénticos a la 0.18.0; mansión idéntica hasta que cae el helicóptero.

## Versión 0.18.0: Dificultad solo como submenú, Dos armas en Trucos, sin mansión en VS, partes de cada ropa (12-11-2026)
- Prueba del usuario de la 0.17.0: dos armas "perfecto"; juego en red bien también con la Odin 3 creando la red (punto de
  acceso) y la Classic unida a ella.
- Menú: la fila "Dificultad" (OPCIONES y menú de partida) ya no enseña valor ni flechas: solo entra al submenú (si no, se
  cambiaba desde fuera y no se veían los ajustes a mano). La ayuda dice la dificultad actual y los dos porcentajes.
- "Dos armas" pasa a la página Trucos (`CH_TWO_WEAPONS`; filas a 18 px para que quepan 8). Sigue guardándose en
  opciones.txt (`dos_armas=`), NO se borra al arrancar como los demás trucos.
- Escenario VS (red y dos mandos) sin la mansión: está hecha para jugar contra la máquina y entre dos falla (si se bloquea la
  puerta de entrada el otro jugador no entra). El selector va Al azar + 7 escenarios (`red_escenario=7` viejo pasa a azar).
  La rutina de 0xB00 va siempre puesta: el escenario fijo está ahora en 0xB3F; al azar, si el juego saca la mansión (8)
  se sortea otro con su generador (0x2908, andi 7 + 1) hasta que no lo sea. Probado forzando 8 en 10 partidas: salen 1-7.
- Partes de la paleta de cada luchador (banco 1, entradas de 8 en 8; headless `OFX_IDX=banco` guarda `f*.idx` con la entrada
  de cada píxel; hojas con `tools/make_palette_sheets.py`). El contorno es la 1 (Betty: 0xF0). Lo mismo que se ve en una
  parte suele estar en dos bloques (lado de delante y de detrás), con los mismos colores:
  - John Smith: vaqueros 0x10-17 + 0x70-77; chaqueta de cuero 0x30-37 + 0x80-87; camiseta 0x50-53; zapatillas 0x60-63;
    dibujo de la camiseta 0x90-9F (OJO: zona de efectos que hoy no se toca); pelo 0x20-24; piel 0x40-47.
  - Prof. Ching: ropa (bata/torso) 0x30-37; pantalón 0x10-17; zapato 0x18-1F; piel 0x40-47. La silla: carrocería 0x48-4F,
    brazos mecánicos 0x38-3F, ruedas y juntas 0x20-27, neumáticos 0x60-67, pilotos rojos 0x68-6F, ribete rojo 0x80-87.
  - Dweeb: traje y sombrero 0x10-15 (un solo bloque: chaqueta y pantalón no se separan); camisa 0x20-26; corbata 0x02-04;
    piel 0x30-37; pelo 0x40-44.
  - Eve: top 0x50-57; pantalón corto y botas 0x20-27 (juntos); medias 0x40-47; pelo 0x10-17; piel 0x30-37.
  - Betty Doe: traje (chaqueta y falda juntas) 0x02-07 con sus sombras 0x08 y 0x0C-0E; botones 0x09-0B; zapatos 0x10-17;
    piel 0x20-27; pelo 0x30-37.
  - Danny y Demi (cada uno por separado): abrigo de Demi 0x30-37, de Danny 0x38-3F; botas de Demi 0x40-44, zapatos de Danny
    0x48-4B, calcetines de Danny 0x58-5B; pelo de Demi 0x20-27, de Danny 0x28-2F; piel de Demi 0x10-17, de Danny 0x18-1F.
  - Bernard White: traje (chaqueta y pantalón juntos) 0x10-17; camisa y corbata 0x28-2C; guante 0x48-4F; zapatos 0x50-53
    (+0x1F); piel 0x30-37.
  - Mr. y Mrs. Acme: banco 0, sin mirar.
- Comprobado en servidor: 4:3 sin opciones, imagen y sonido idénticos byte a byte a la 0.17.0.

## Estudio: llevar dos armas y cambiar con el botón 3 (11-11-2026) — HECHO en la 0.17.0 (ver arriba)
- VIABLE, dificultad media. Lo que se ha encontrado (traza de escrituras en +0x82 del luchador rival, 0x208E3C):
  - Coger arma: rutina hacia 0xCE1E-0xCED0 (A5 = luchador, ($58,A5) = el objeto arma candidato). Hace:
    luchador.$5E = arma.$5E + 2; luchador.$60 = arma.$60 (munición); luchador.$82 = arma; arma.$82 = luchador;
    arma.$76 = 1 (en la mano); y salta por tipo con la tabla de 0xCED0 (`jmp ([8,PC,D0*4])`).
  - Soltar (0xA96A): arma.$76 = 2, luchador.$82 = -1, $5E = 0, $60 = 0. Lanzar (0xA992): arma.$76 = 4, arma.$82 =
    luchador, arma.$7E = dirección (tabla 0x40950 según ($6B,A5)); el tipo 0x14 (cesta) crea proyectiles aparte.
  - Crear un objeto: `jsr $4b30` (lo deja en A0) y se le pone la clase en ($12,A0) (las armas son la clase 0x14; el
    tipo pedido va en ($7E) y la rutina 0x185E8 lo copia a ($5E)). Borrar un arma recién creada: `bra $1ae48`.
- Plan: opción en el menú ("Dos armas"). Con el botón 3 (la placa lo tiene y el juego no lo usa): si llevas arma y la
  reserva está vacía, se guarda (tipo y munición) y el objeto se borra; si hay arma en la reserva, se crea un objeto
  arma de ese tipo, se le pone la munición guardada y se hace lo mismo que la rutina de coger (0xCE80 en adelante).
  Con las dos, se intercambian. Todo con una rutina propia en el hueco libre de la ROM, llamada cuando la app pone una
  marca en RAM. Los gemelos (personaje 6): dos reservas, el botón 3 las va rotando.
- Por comprobar antes de darlo por bueno: (1) cuántos fotogramas tarda en inicializarse el arma creada antes de poder
  "cogerla"; (2) el lanzacohetes (sus cohetes son objetos ligados: ver 0.7.1; quizá no se pueda guardar);
  (3) comida, carbón, pistolas de juguete y tipos 19-20 (no guardarlos); (4) qué pasa al morir, al cambiar de combate
  y en las partidas guardadas (la reserva tendría que ir en el estado); (5) enseñar la reserva en el marcador
  (dibujarla nosotros junto al cajón del arma); (6) en red, el botón 3 del invitado ya viaja.

## Versión 0.9.0: controles táctiles, menú en inglés, 8:7 y arreglos de 16:9 (9-11-2026)
- Controles táctiles (`src/app/touch.c`): cruceta abajo a la izquierda; disparo (botón 1) y salto (botón 2) abajo a la
  derecha en arco; START y MENU (antes PAUSE) pequeños arriba a la derecha. Tres tamaños (Controles → Controles táctiles:
  No, Pequeños, Medianos por defecto, Grandes; `tactil=` en opciones.txt). Al tocar se ilumina el botón; en la cruceta solo
  el brazo de la dirección (dos en diagonal; 8 sectores de 45°). Se esconden al usar un mando o el teclado y vuelven al
  tocar; al arrancar se ven si no hay mandos. En el menú también sirven: cruceta = flechas, disparo/START = aceptar,
  salto/MENU = volver (se mandan como teclas con windowID `TOUCH_KEY`).
  Gráficos: `assets/touch/*.png`, recortados de la imagen del usuario (fondo negro → transparente); el botón MENU es el de
  PAUSE con el texto borrado y "MENU" en letra de píxeles. `tools/make_touch.py` los pasa a `src/app/touch_gfx.h`
  (RGBA con zlib, se descomprime con miniz).
  Pruebas en escritorio: `OFX_FINGERS="0.11,0.62 0.8,0.72"` deja esos dedos apoyados.
- Menú en español e inglés (Idioma, en el menú principal; `idioma=`). Sin opciones.txt se coge el idioma del sistema.
  Textos con `TR("es", "en")` y tablas `[LANG_COUNT][...]` en menu.c. La página "Botones" pasa a llamarse "Controles".
- Formato 8:7 recortado (`formato=4`): el 4:3 sin las 20 columnas de cada lado (248x224 con el píxel del 4:3 = 8:7 sin
  deformar). Se pierde el borde del marcador y, en la presentación, la primera letra de algunas líneas.
- Tren: el objeto con paleta magenta ahora se descarta entero si cae fuera del 4:3 y más de 1/3 de sus píxeles son
  magenta (antes solo esos píxeles: quedaba el contorno). La "figura medio transparente" que sigue saliendo es el
  luchador que vuelve en la vagoneta tras caer del tren: el juego lo hace parpadear (un fotograma sí y otro no) y en 4:3
  está fuera de la pantalla. Es del juego; se deja.
- Rascacielos: el cielo (capa 0) mide 46 columnas (368 px) y se desplaza; en 16:9 quedaba un hueco negro a un lado u
  otro. El reflejo de fondos de 35-48 columnas ya no exige que empiecen o acaben en el borde del 4:3.
  Comprobado: de ~20 columnas negras por captura a 0-2; en 4:3 y en el centro, todo idéntico a la 0.8.0.
- headless: `OFX_CHAR=n` (elige personaje: n pulsaciones a la derecha), `OFX_POKE=fot:dir=val,...;fot:...` (escribe en
  memoria), `OFX_PAL=n` (cambia los colores del banco de sprites n, para saber quién lo usa).

## Estudios (9-11-2026)
- Paletas de personajes: VIABLE. Cada luchador tiene su banco de sprites de 256 colores (en el tren: banco 1 el 1P,
  banco 2 el rival). Se puede recolorear al dibujar (como los filtros de color, pero solo en ese banco) o reescribir la
  RAM de paleta tras cargarla el juego. Falta: sacar la paleta de cada personaje y diseñar las alternativas.
- Logros: la lista de RetroAchievements (juego 19839, 20 logros) se puede hacer como logros locales con lo que ya
  sabemos de la RAM (personaje, rival, energía, tiempo, armas). Sincronizar con la web de RA no es viable: exige un
  cliente aprobado por RA y su mapa de memoria de arcade (FBNeo), que no coincide con el nuestro.
- Traducir el juego: VIABLE. Los textos van en ASCII dentro del programa (0x91C6A-0x92BAF encargos y escenarios,
  0xAD487-0xADB00 biografías y final, 0x48AFC créditos): unos 4 KB. Líneas de 24 caracteres, '_' = espacio, '\\' = salto
  de línea; nombres en otra codificación (letra 1-26). En pantalla: losetas 0x300 + ASCII (rojo) / 0x320 (verde).
  Para tildes y ñ habría que añadir letras (dibujarlas nosotros en losetas libres). Hay textos que son dibujos
  (INSERT COIN, CREDIT, SELECT YOUR CHARACTER parece texto: comprobar).
- "REMASTER" en el título: el logo es el plano ROZ 1 y sus letras (T,H,E,O,U,F,X,I,S) no tienen R, M ni A. Se podría
  dibujar encima con una imagen propia (R, E y M de la imagen del usuario "OUTFOXIES RECOMP", A hecha a partir de la R).
- 8:7: hecho (ver arriba).

## Pendiente, por orden
0. Probar la 0.21.0: juego por internet con el servidor intermedio (crear partida en casa: debe salir "por el servidor
   intermedio"; el otro escribe el código). Ver si va fluido. Probar la 0.20.0: ropa en metralleta/cabina del avión y cañón del circo, cajón de la reserva, charco (antes y más lento), Tailscale.
   Probar la 0.19.0: juego por internet. Probar la 0.18.0: Dificultad (solo entra), Dos armas en Trucos, escenario en red sin mansión.
   De la 0.17.0 aún sin confirmar: (las dos actualizadas: el protocolo de red ha cambiado): formato 8:7 del invitado,
   SELECT del invitado, Escenario en red, y una consola creando la red (punto de acceso). Dos armas (botón 3, táctil,
   cajón de la reserva, gemelos). Submenú Dificultad. Los 4 logros nuevos.
   Pendiente de la 0.12.0-0.15.x: vista previa, música/efectos, inmunidad, ropas, ILIMITADA, teles en 8:7, Armas en Trucos.
1. Juego en red por internet: ZANJADO por el usuario (crea la partida Mckeijas). No tocar salvo que lo pida. Luego: invitado sin ROM, H.264 si el caudal sin pérdida no da.
2. Sangre del helicóptero: HECHA en la 0.19.0 (dibujada; ver arriba). Que el usuario la compare con el vídeo.
3. Ropas: con las partes de la 0.18.0 (y la hoja "Partes de ropa por personaje"), ropas a medida (un color por prenda);
   el usuario está preparando los estilos. Manual y galería: HECHOS en la 0.24.0 (visor y manual.zip); falta el pack de la
   galería (imágenes del usuario, como MoD_Galeria con `tools/packs/montar_galeria.py` de Master of Darkness) y las fotos
   de los finales para el manual. Decidir si los Acme (banco 0)
   también cambian.
4. Extras japoneses (si el usuario quiere): frases en japonés de las teles (3 `nop` en 0x29100), katakana del título.
5. Logros: probar la 0.24.0 (los 3 nuevos; "Toma, te lo devuelvo" provisional: pedir partida con el cohete del ascensor).
   Probar la 0.23.0 (terminar con Bernard/Betty/Eve, Sube el nivel del mar, pistola de juguete, Casi
   difícil, La siguiente en Normal). Lanzamisiles ilimitado negro: esperar partida guardada del usuario. Iconos: hechos en la 0.21.0. Siguiente: revisión de los logros con el usuario
   y, cuando estén probados, que no cuenten con trucos (main.c, ach_frame). Cuando los logros estén probados: que no cuenten con trucos (main.c, ach_frame).
6. Mansión: la línea al subir en ascensor (si se ve sin rebobinar, pedir vídeo). Tren: helicóptero que te devuelve.
7. Dos armas: sonido al cambiar (el juego toca uno por tipo al coger, 0xCED0 → rutina 0x1810; habría que llamarla desde
   una rutina en el hueco de la ROM).

## Notas de hardware
Ver los comentarios de `src/core/nb2.c`. Resumen:
- Programa 1 MB (palabras alternas, `ou2_mpru` primero). Datos 1 MB en 0x400000 (`ou1dat0`+`ou1dat1`, bytes intercambiados).
- Protección en 0xC00000: 0xC00002 = 0x0186 y 0xC00004 = aleatorio.
- Interrupciones: 0xF00000 nivel VBL, 0xF00002 nivel POS, reconocimiento en 0xF00004 y 0xF00006.
- Memoria compartida con la CPU de sonido: 0x206002 P1, 0x206004 P2, 0x206001 sistema, 0x20601E monedas.
- Capas 0-3: `x + scroll + 44 + ajuste` (4, 2, 1, 0) e `y + scroll + 24`.
