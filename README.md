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
Todos los botones se pueden cambiar en el menú (Botones).

Partidas guardadas: 5 ranuras en el menú (Guardar / Cargar partida). En Android están en
`Android/data/com.juanmote.outfoxies/files/partidaN.sav`.

## Menú
Sale al arrancar y en partida (pausa). Opciones: imagen (nítida, suavizada o con scanlines),
formato (16:9 panorámico, 4:3, 4:3 con escala exacta o 4:3 estirado), volumen, vibración y botones.
En 16:9 los combates muestran más escenario a los lados; las pantallas de presentación se ven en 4:3
(la de las teles con pared a los lados).
Vibración (regulable o desactivable): al disparar, golpear, saltar, recibir daño y con las explosiones.
Trucos (para probar): invulnerable, munición infinita, puño de la muerte (los rivales caen de un golpe)
y elegir el escenario del siguiente combate.
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
