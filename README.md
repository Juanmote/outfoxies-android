# The Outfoxies Remastered para Android

Launcher que ejecuta el arcade *The Outfoxies* (Namco, 1994, placa NB-2) desde la ROM del usuario.

**Este repositorio no contiene ningún archivo del juego.** Hace falta el romset `outfxies.zip`, versión World (OU2).

## Controles
| Acción | Mando | Teclado (PC) |
|---|---|---|
| Moverse | Cruceta / stick izquierdo | Flechas |
| Botones 1, 2 y 3 | A, B, X | Z, X, C |
| Empezar / pausa | START | Intro |
| Menú | SELECT o Atrás | Esc |
| Rebobinar (mantener) | L2 | Retroceso |
| Avance rápido (mantener) | R2 | Tab |

No hay que meter moneda: START mete el crédito y empieza (o entra el 2P). En combate, START pausa.
Cada mando conectado es un jugador: el primero es el 1P y el segundo el 2P.
Todos los botones se pueden cambiar en el menú (Controles).

Controles táctiles: cruceta, disparo, salto, START y MENU, en tres tamaños (o desactivados) desde Controles.
Se esconden al usar un mando y vuelven al tocar la pantalla.

Partidas guardadas: 5 ranuras (Opciones > Partidas; en partida, Guardar y cargar partida). En Android están en
`Android/data/com.juanmote.outfoxies/files/partidaN.sav`.

## Pantalla de inicio
Al abrir la app sale el launcher: la portada y siete botones, con **Jugar** en el centro:
**Opciones · Logros · Galería · Jugar · Manual · Editor de ropa · Salir**. Se manejan con el mando o tocándolos.
En pantallas casi cuadradas (8:7, 4:3) van en dos filas.
- **Opciones**: partidas, pantalla, dificultad (y daño de 25 % en 25 %), sonido y vibración, controles, trucos, idioma
  y juego en red.
- **Logros**: los 21 de RetroAchievements y 3 propios, por tipos.
- **Galería** y **Manual**: visores de imágenes; los packs (`galeria.zip`, `manual.zip`) se eligen con el selector de
  archivos la primera vez.
- **Editor de ropa**: cada prenda de cada personaje con su color, viendo el resultado en directo. Los packs son
  archivos `.txt` (`ropas/` en la carpeta de la app) que se importan y exportan; la app trae 6 hechos por la
  comunidad (Gótica, Negativa, Pinky, Vampiros, Mortal Kombat y Street Fighter).

## Menú de partida
En partida, SELECT (o Atrás) abre el menú de pausa con las mismas opciones, guardar y cargar, y volver a la pantalla
de inicio.
- Pantalla: formato (se elige solo según la pantalla hasta que lo cambies: 16:9 panorámico, 4:3, 4:3 con escala exacta, 4:3 estirado u 8:7), filtro (nítida, suavizada,
  scanlines, CRT...), colores, bandas y ropa de cada jugador (la de la recreativa o un pack), con vista previa.
- En 16:9 los combates muestran más escenario a los lados. En 8:7 el marcador se recoloca y la presentación de las
  teles llena la pantalla.
- Vibración (regulable o desactivable): al disparar, golpear, saltar, recibir daño y con las explosiones.
- Trucos (para probar): invulnerable, munición infinita, puño de la muerte, inmunidad tras golpe, armas, dos armas
  y escenario del siguiente combate.
- Juego en red: dos consolas en la misma wifi, o por internet con un código.
- Idioma: menú en español, inglés, portugués o italiano; textos del juego en inglés original, castellano, portugués
  o italiano.
Todo se guarda en `opciones.txt` dentro de la app.

## Extras
El manual (castellano, inglés, portugués e italiano) y la galería para los botones **Manual** y **Galería** se bajan
aparte, de las Releases del repositorio [Extras_Ports](https://github.com/Juanmote/extras_ports).

## Estado
Ver [`ESTADO.md`](ESTADO.md).

## Créditos
- CPU de sonido M37702 y chip PCM C352: portados a C desde MAME (BSD-3-Clause; R. Belmont, Karl Stenerud, superctr).
- CPU: [Musashi](https://github.com/kstenerud/Musashi), de Karl Stenerud (licencia MIT).
- Vídeo y hardware: basados en la documentación del driver `namconb1` de [MAME](https://github.com/mamedev/mame) (BSD-3-Clause).
- [SDL2](https://www.libsdl.org/), [miniz](https://github.com/richgel999/miniz) y [stb_image](https://github.com/nothings/stb) (dominio público).
- Rótulos de los botones: Inter Display Black Italic (SIL Open Font License), convertida a imagen (`tools/make_launcher.py`).
- Letra del menú: DejaVu Sans Bold convertida a mapa de bits (`tools/make_font.py`).
- Datos de los personajes del manual: Wikipedia en castellano (artículo "The Outfoxies", CC BY-SA).
- Iconos de logros: los de [RetroAchievements](https://retroachievements.org/game/19839) y tres propios con el mismo estilo.
- *The Outfoxies* es © Bandai Namco Entertainment. Proyecto de aficionados sin afiliación.
