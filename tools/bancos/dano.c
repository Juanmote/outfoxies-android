/* Dano base (D0 de la rutina 0x305A) de cada golpe sin arma: el 1P pega al rival de la maquina, que se pone pegado */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "m68k.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static unsigned L32(unsigned a){return (unsigned)m[a]<<24|m[a+1]<<16|m[a+2]<<8|m[a+3];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static void S32(unsigned a,unsigned v){m[a]=v>>24;m[a+1]=v>>16;m[a+2]=v>>8;m[a+3]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_cheats(1,atoi(getenv("ESC"))); if(getenv("ARMA")){ static unsigned char map[0x15]; memset(map,atoi(getenv("ARMA")),sizeof map); nb2_set_weapon_map(map);} nb2_reset(); m=nb2_mem();
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
  unsigned a6=m68k_get_reg(NULL,M68K_REG_A6);
  int dif=W16(0x62054+2*W16(a6+0x10a)) ; /* tabla en ROM: la ROM esta en mem? */
  printf("1P 9c=%d rival 9c=%d A6=%06x dif_idx=%d prog_idx($98)=%d\n",W16(P1+0x9c),W16(P2+0x9c),a6,W16(a6+0x10a),W16(a6+0x98));
  if(getenv("ARMA")){
    int got=0;
    for(int k=0;k<900&&!got;k++){
      int p1=0;
      if(L32(P1+0x82)!=0xffffffff && L32(P1+0x82)) { got=1; break; }
      for(unsigned a=0x208000;a<0x220000;a+=2)
        if((W16(a+0x10)&0xc000)==0xc000&&W16(a+0x12)==0x14&&L32(a+0x82)==0xffffffff&&m[a+0x5f]<30){ if(k%40==0) memcpy(m+P1+0x14,m+a+0x14,8); break; }
      p1=(k%10)<3?0x04:0;
      S16(P2+0x14, W16(P1+0x14)+700);
      nb2_set_inputs(p1,0,0); nb2_frame();
    }
    for(int k=0;k<40;k++){ nb2_set_inputs(0,0,0); nb2_frame(); }
    printf("arma en la mano: tipo %d municion %d\n", W16(P1+0x5e)-2, W16(P1+0x60));
  }
  unsigned char *st=nb2_state_save(&len,1); unsigned char *cp=malloc(len); memcpy(cp,st,len);
  for(int s=2;s<c;s++){
    nb2_state_load(cp,len); m=nb2_mem();
    int dir = getenv("DIR")?atoi(getenv("DIR")):1;
    char buf[256]; strcpy(buf,v[s]); int f=0, hits=0; unsigned e0=L32(P2+0x52);
    printf("[%s]", v[s]);
    for(char*t=strtok(buf," ");t;t=strtok(NULL," ")){
      int k=strtol(t,0,16), n=atoi(strchr(t,':')+1);
      for(int i=0;i<n;i++,f++){
        if(hits==0){ S16(P2+0x14, W16(P1+0x14)+dir*(getenv("GAP")?atoi(getenv("GAP")):24)); S16(P2+0x18, W16(P1+0x18)); }
        nb2_set_inputs(k,0,0); nb2_frame();
        unsigned e=L32(P2+0x52);
        if(e!=e0){ double l=(double)(e0-e)/65536.0; int dr=W16(P2+0x9c); unsigned a6=0x208000; int T=W16(0x6206a+0)*0; (void)T; printf(" f%d %.3f",f,l); e0=e; hits++; }
      }
    }
    printf("\n");
  }
}
