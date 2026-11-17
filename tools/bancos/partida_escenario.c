/* Lleva a un escenario (ESC=rival 0-7, CHAR=n pulsaciones en la eleccion) y guarda la partida 300 fotogramas despues
   de empezar el combate: partida_escenario outfxies.zip salida.sav (captura en 16:9 con laterales en /tmp/as/pos.ppm) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_wide(2);
  nb2_set_cheats(1,atoi(getenv("ESC"))); if(getenv("ROPA")) nb2_set_outfit(0,atoi(getenv("ROPA"))); nb2_reset(); m=nb2_mem();
  int t0=-1;
  for(int fr=0;;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    nb2_set_inputs(p1,0,0); nb2_frame();
    if(t0>=0 && fr-t0==300) break;
  }
  printf("ch %02x p1 %d,%d p2 %d,%d estado %d\n",m[0x20807b],(short)W16(0x208d06+0x14),(short)W16(0x208d06+0x18),(short)W16(0x208dba+0x14),(short)W16(0x208dba+0x18),W16(0x208d06+0x4e));
  size_t len; unsigned char *st=nb2_state_save(&len,1); FILE*f=fopen(v[2],"wb"); fwrite(st,1,len,f); fclose(f);
  shot("/tmp/as/pos.ppm");
}
