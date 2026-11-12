#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
int main(int c,char**v){
  char err[128]; if(!nb2_load(v[1],err,sizeof err)) return 1;
  nb2_set_cheats(1,atoi(getenv("ESC"))); nb2_reset(); m=nb2_mem();
  int fr=0,t0=-1;
  for(;;fr++){ int p1=0;
    if(fr==200||fr==260) nb2_coin(); if(fr>=320&&fr<326) p1=NB2_START;
    { int n=atoi(getenv("CHAR")); if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    nb2_set_inputs(p1,0,0); nb2_frame(); if(t0>=0&&fr-t0==500) break; }
  size_t len; unsigned char*st=nb2_state_save(&len,1),*cp=malloc(len); memcpy(cp,st,len);
  const char*sc[]={"walk","run"}; 
  for(int s=0;s<2;s++){ nb2_state_load(cp,len); m=nb2_mem();
    S16(P1+0x14,600); 
    int xs[80];
    for(int f=0;f<70;f++){ int k = s==0?0x02 : (f<3||f>=6)?0x02:0; S16(P2+0x14,W16(P1+0x14)+900); nb2_set_inputs(k,0,0); nb2_frame(); xs[f]=(short)W16(P1+0x14); }
    printf("9c=%d %s: v(30-60)=%.2f px/f  estado %d  x:",W16(P1+0x9c),sc[s],(xs[30]-xs[60])/30.0,W16(P1+0x4e)); for(int f=8;f<20;f++) printf(" %d",xs[f]-xs[f-1]); printf("\n"); }
}
