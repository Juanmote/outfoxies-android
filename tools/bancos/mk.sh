#!/bin/sh
# Bancos de pruebas contra el nucleo (sin SDL): tools/bancos/mk.sh tools/bancos/dos_armas.c /tmp/dos_armas
# y luego: /tmp/dos_armas outfxies.zip <fotogramas>  (ESC=rival 0-7 elige el escenario, CHAR=n el personaje)
R=$(cd "$(dirname "$0")/../.." && pwd)
O=${TMPDIR:-/tmp}/ofx_obj; mkdir -p $O
for f in src/core/nb2.c src/core/skins.c src/core/m68k_state.c third_party/m37710/m37710.c third_party/c352/c352.c third_party/miniz/miniz.c \
         third_party/musashi/m68kcpu.c third_party/musashi/m68kops.c third_party/musashi/m68kdasm.c third_party/musashi/softfloat/softfloat.c; do
  o=$O/$(echo $f | tr / _).o
  if [ ! -f $o ] || [ $R/$f -nt $o ]; then gcc -std=c99 -O2 -w -c $R/$f -I$R/src/core -I$R/third_party/miniz -I$R/third_party/musashi -I$R/third_party/m37710 -I$R/third_party/c352 -o $o || exit 1; fi
done
gcc -O2 -w "$1" $R/src/app/achievements.c $R/src/app/menu.c $R/src/app/lang_ui.c $R/src/app/skinpacks.c $R/src/app/inet_code.c $O/*.o -I$R/src/core -I$R/src/app -I$R/third_party/musashi -I$R/third_party/miniz -lm -o "$2"
