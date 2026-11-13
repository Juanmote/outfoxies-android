/* caidas: lleva al combate del escenario ESC, teletransporta al rival por x y mira su estado y su y */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "achievements.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_cheats(1,atoi(getenv("ESC"))); nb2_reset(); m=nb2_mem();
  int fr=0,t0=-1; size_t len=0;
  for(;;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    nb2_set_inputs(p1,0,0); nb2_frame();
    if(t0>=0 && fr-t0==(getenv("OFF")?atoi(getenv("OFF")):200)) break;
  }
  printf("rival %d yo %d  P1 %d,%d  P2 %d,%d estado %d\n", m[0x20807b]>>4, m[0x20807b]&15,(short)W16(P1+0x14),(short)W16(P1+0x18),(short)W16(P2+0x14),(short)W16(P2+0x18),W16(P2+0x4e));
  if(getenv("SHOT")) shot(getenv("SHOT"));
  unsigned char *st=nb2_state_save(&len,1); unsigned char *cp=malloc(len); memcpy(cp,st,len);
  int x0=atoi(getenv("X0")),x1=atoi(getenv("X1")),dx=atoi(getenv("DX")), dy=getenv("DY")?atoi(getenv("DY")):0;
  for(int x=x0;x<=x1;x+=dx){
    nb2_state_load(cp,len); m=nb2_mem();
    ach_init("/tmp/achtest"); ach_reset(2); int y=(short)W16(P2+0x18)+dy; S16(P2+0x14,x); S16(P2+0x18,y);
    int seen[256]={0}; char buf[400]; int n=0; int miny=99999, e0=m[0x208e0d];
    for(int f=0;f<240;f++){ nb2_set_inputs(0,0,0); nb2_frame(); ach_frame(m,0); {int a=ach_pop(); if(a>=0) printf("LOGRO %d %s\n",a,ach_title(a));} if(getenv("SF")&&f==atoi(getenv("SF"))){char nn[64];sprintf(nn,"cx%d.ppm",x);shot(nn);} int s=W16(P2+0x4e)&255; int yy=(short)W16(P2+0x18); if(yy<miny)miny=yy;
      if(!seen[s]){seen[s]=1; n+=sprintf(buf+n,"%d@%d(y%d) ",s,f,yy); if(n>330)break;} }
    printf("x %5d y %5d: miny %6d energia %x->%x  %s\n",x,y,miny,e0,m[0x208e0d],buf);
  }
}
