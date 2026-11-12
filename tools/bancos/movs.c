/* Que estado ($4E) sale con cada combinacion de botones, y cuanto avanza */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_cheats(1,atoi(getenv("ESC"))); nb2_reset(); m=nb2_mem();
  int fr=0,t0=-1; size_t len;
  for(;;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    nb2_set_inputs(p1,0,0); nb2_frame();
    if(t0>=0 && fr-t0==500) break;
  }
  printf("personaje 9c=%d\n", W16(P1+0x9c));
  unsigned char *st=nb2_state_save(&len,1); unsigned char *cp=malloc(len); memcpy(cp,st,len);
  /* guiones: lista de "tecla:fotogramas" separados por espacios en argv[2..] */
  for(int s=2;s<c;s++){
    nb2_state_load(cp,len); m=nb2_mem();
    S16(P2+0x14, W16(P1+0x14)+ (getenv("P2D")?atoi(getenv("P2D")):600));
    char buf[256]; strcpy(buf,v[s]); int prev=-1, x0=(short)W16(P1+0x14), f=0;
    printf("[%s]", v[s]);
    for(char*t=strtok(buf," ");t;t=strtok(NULL," ")){
      int k=strtol(t,0,16), n=atoi(strchr(t,':')+1);
      for(int i=0;i<n;i++,f++){
        nb2_set_inputs(k,0,0); nb2_frame();
        int e=W16(P1+0x4e); if(e!=prev){ printf(" f%d:%d(x%+d)",f,e,(short)W16(P1+0x14)-x0); prev=e; }
      }
    }
    printf(" | dx total %d\n",(short)W16(P1+0x14)-x0);
  }
}
