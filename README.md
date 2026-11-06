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
| Salir | SELECT + START 1,5 s | Esc |

Cada mando conectado es un jugador: el primero es el 1P y el segundo el 2P.

## Estado
Ver [`ESTADO.md`](ESTADO.md).

## Créditos
- CPU de sonido M37702 y chip PCM C352: portados a C desde MAME (BSD-3-Clause; R. Belmont, Karl Stenerud, superctr).
- CPU: [Musashi](https://github.com/kstenerud/Musashi), de Karl Stenerud (licencia MIT).
- Vídeo y hardware: basados en la documentación del driver `namconb1` de [MAME](https://github.com/mamedev/mame) (BSD-3-Clause).
- [SDL2](https://www.libsdl.org/) y [miniz](https://github.com/richgel999/miniz).
- *The Outfoxies* es © Bandai Namco Entertainment. Proyecto de aficionados sin afiliación.
