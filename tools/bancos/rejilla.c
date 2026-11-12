/* Busca sitios donde el 1P cambia a un estado raro (sentarse en metralleta, cabina, canon) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
extern int dbg_spr; extern int dbg_trace; extern unsigned dbg_lo, dbg_hi; unsigned short nb2_dbg_pen(int,int);
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
static int normal(int s){ static const int ok[]={75,0,2,7,8,9,10,14,15,28,29,38,39,40,41,42,44,47,-1}; for(int i=0;ok[i]>=0;i++) if(ok[i]==s) return 1; return 0; }
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_cheats(1,atoi(getenv("ESC"))); if(getenv("ROPA")) nb2_set_outfit(0,atoi(getenv("ROPA"))); if(getenv("ROPA2")) nb2_set_outfit(1,atoi(getenv("ROPA2"))); nb2_reset(); m=nb2_mem();
  int fr=0,t0=-1; unsigned char *st=0; size_t len=0;
  for(;;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d] && t0<0) t0=fr;
    nb2_set_inputs(p1,0,0); nb2_frame();
    if(t0>=0 && fr-t0==(getenv("OFF")?atoi(getenv("OFF")):500)) break;
  }
  st=nb2_state_save(&len,1); unsigned char *cp=malloc(len); memcpy(cp,st,len);
  int x0=atoi(getenv("X0")),x1=atoi(getenv("X1")),dx=atoi(getenv("DX")),y0=atoi(getenv("Y0")),y1=atoi(getenv("Y1")),dy=atoi(getenv("DY"));
  int keys[8]={0x04,0x08,0x10,0x20,0x01,0x02}, nk=6; if(getenv("KEYS")){ nk=0; char*q=getenv("KEYS"); while(*q&&nk<8){ keys[nk++]=strtol(q,&q,16); while(*q==',')q++; } }
  int found=0;
  for(int y=y0;y<=y1;y+=dy) for(int x=x0;x<=x1;x+=dx) for(int k=0;k<nk;k++){
    nb2_state_load(cp,len); m=nb2_mem();
    S16(P1+0x14,x); S16(P1+0x18,y); S16(P2+0x14,x+400); S16(P2+0x18,y);
    for(int f=0;f<160;f++){
      int in = f>40 && (f%30)<10 ? keys[k] : 0; if(f>=150) break;
      if(getenv("TRP")){dbg_trace=2;dbg_lo=0x804000;dbg_hi=0x808000;} nb2_set_inputs(in,0,0); nb2_frame(); dbg_trace=0;
      int s=W16(P1+0x4e);
      if(!normal(s)){ printf("x%d y%d tecla %02x f%d estado %d pos %d,%d\n",x,y,keys[k],f,s,(short)W16(P1+0x14),(short)W16(P1+0x18));
        static int seen[256]; if(!seen[s&255]){ seen[s&255]=1; char n[64]; for(int g=0;g<30;g++){ if(getenv("P2N")) {S16(P2+0x14,x+60);} dbg_spr = g==29 && getenv("SPR"); nb2_set_inputs(0,0,0);nb2_frame();} dbg_spr=0; sprintf(n,"/tmp/as/estado_%d.ppm",s); shot(n); if(getenv("MAP")){ unsigned char*M=nb2_mem(); for(int i=0;i<0x40;i++){ unsigned p2=0x1900+i,o2=((p2&0x1800)<<2)|(p2&0x7ff); printf("%04x %02x%02x%02x ->",p2,M[0x800000+o2],M[0x800800+o2],M[0x801000+o2]); for(int j=0;j<256;j++){ unsigned p1=0x100+j,o1=((p1&0x1800)<<2)|(p1&0x7ff); if(M[0x800000+o1]==M[0x800000+o2]&&M[0x800800+o1]==M[0x800800+o2]&&M[0x801000+o1]==M[0x801000+o2]) printf(" %02x",j);} printf("\n"); } }
 if(getenv("CMP")){ int same=0,diff=0; for(int bk=16;bk<32;bk++){ same=0; for(int i=0;i<256;i++){ unsigned p1=0x100+i, p2=bk*256+i; unsigned o1=((p1&0x1800)<<2)|(p1&0x7ff), o2=((p2&0x1800)<<2)|(p2&0x7ff); unsigned char*M=nb2_mem(); if(M[0x800000+o1]==M[0x800000+o2]&&M[0x800800+o1]==M[0x800800+o2]&&M[0x801000+o1]==M[0x801000+o2]) same++; } printf("banco %x: %d/256 iguales al 1P\n",bk,same);} }
 if(getenv("PENS")){ int a,b,cx,cy; sscanf(getenv("PENS"),"%d,%d,%d,%d",&a,&b,&cx,&cy); for(int yy=b;yy<b+cy;yy++){ for(int xx=a;xx<a+cx;xx++) printf("%04x ",nb2_dbg_pen(xx,yy)); printf("\n"); } }} found++; break; }
    }
  }
  printf("fin %d\n",found);
}
