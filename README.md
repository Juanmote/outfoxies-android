# The Outfoxies para Android

Launcher que ejecuta el arcade *The Outfoxies* (Namco, 1994, placa NB-2) desde la ROM del usuario.

**Este repositorio no contiene ningún archivo del juego.** Hace falta el romset `outfxies.zip`, versión World (OU2).

## Controles
| Acción | Mando | Teclado (PC) |
|---|---|---|
| Moverse | Cruceta / stick izquierdo | Flechas |
| Botones 1, 2 y 3 | A, B, X | Z, X, C |
| Moneda | SELECT | 5 |
| Empezar | START | Intro |
| Menú de opciones | SELECT + START 1 s, o Atrás | Esc |

Cada mando conectado es un jugador: el primero es el 1P y el segundo el 2P.
Los botones de acción, moneda y start se pueden cambiar en el menú (Botones).

## Menú
Sale al arrancar y en partida (pausa). Opciones: imagen (nítida, suavizada o con scanlines),
formato (4:3, 4:3 con escala exacta o pantalla completa), volumen, botones y cambiar ROM.
Se guardan en `opciones.txt` dentro de la app.

## Estado
Ver [`ESTADO.md`](ESTADO.md).

## Créditos
- CPU de sonido M37702 y chip PCM C352: portados a C desde MAME (BSD-3-Clause; R. Belmont, Karl Stenerud, superctr).
- CPU: [Musashi](https://github.com/kstenerud/Musashi), de Karl Stenerud (licencia MIT).
- Vídeo y hardware: basados en la documentación del driver `namconb1` de [MAME](https://github.com/mamedev/mame) (BSD-3-Clause).
- [SDL2](https://www.libsdl.org/) y [miniz](https://github.com/richgel999/miniz).
- Letra del menú: DejaVu Sans Bold convertida a mapa de bits (`tools/make_font.py`).
- *The Outfoxies* es © Bandai Namco Entertainment. Proyecto de aficionados sin afiliación.
