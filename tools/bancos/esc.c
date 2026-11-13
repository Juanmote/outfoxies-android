#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){
  char err[128]; if(!nb2_load(v[1],err,sizeof err)) return 1;
  int esc=atoi(v[2]);
  nb2_set_game_lang(1); nb2_set_wide(1); nb2_set_cheats(1,esc); nb2_reset(); m=nb2_mem();
  int t0=-1;
  for(int fr=0;fr<6000;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    if(fr>600 && t0<0 && fr%40==0){ char nm[64]; sprintf(nm,"esc/e%d_pre%05d.ppm",esc,fr); shot(nm);}
    nb2_set_inputs(p1,0,0); nb2_frame();
    if(t0>=0){ int k=fr-t0; if(k==1||k==40||k==100||k==200||k==400){ char nm[64]; sprintf(nm,"esc/e%d_%03d.ppm",esc,k); shot(nm);} if(k>400) break; }
  }
}
