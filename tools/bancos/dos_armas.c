/* Banco de pruebas de "Dos armas" (11-11-2026): el 1P coge armas (se teletransporta encima y se agacha), boton 3
   andando, dispara y cambia; apunta la mano, la reserva y la municion de cada una. ESC=rival (escenario), CHAR=n, WIDE=1 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static unsigned L32(unsigned a){return (unsigned)m[a]<<24|m[a+1]<<16|m[a+2]<<8|m[a+3];}
static void S32(unsigned a,unsigned v){m[a]=v>>24;m[a+1]=v>>16;m[a+2]=v>>8;m[a+3]=v;}
static unsigned P1=0x208d06, P2=0x208dba, RES=0x27ff00;
static int pick(void){
  for(unsigned a=0x208000;a<0x220000;a+=2)
    if((W16(a+0x10)&0xc000)==0xc000&&W16(a+0x12)==0x14&&L32(a+0x82)==0xffffffff&&m[a+0x5f]<8&&a!=L32(RES)){ memcpy(m+P1+0x14,m+a+0x14,8); return 1; }
  return 0;
}
static void shot(int fr,const char*t){ char n[64]; sprintf(n,"/tmp/%05d_%s.ppm",fr,t); FILE*f=fopen(n,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){
  char err[128]; int frames=atoi(v[2]);
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  if(getenv("WIDE")) nb2_set_wide(1);
  nb2_set_cheats(1,getenv("ESC")?atoi(getenv("ESC")):-1); nb2_set_two_weapons(1); nb2_reset(); m=nb2_mem();
  int step=0,t0=0;
  for(int fr=0;fr<frames;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(fr>=420&&fr<420+n*12&&(fr-420)%12<4) p1=0x01; }
    if(fr>=560&&fr<566) p1=0x10;
    if(fr==1500) printf("personaje 9c=%d 7b=%02x\n",W16(P1+0x9c),m[0x20807b]);
    if(fr>=600 && m[0x208d59] && m[0x208e0d]){
      unsigned held=L32(P1+0x82); int k=fr-t0;
      S32(P2+0x14, 0x00400000);
      switch(step){
      case 0: case 2: if(held==0xffffffff||!held){ pick(); p1=(fr%10)<3?0x04:0; } else { step++; t0=fr; printf("%d cogida %08x tipo %d\n",fr,held,W16(held+0x5e)); } break;
      case 1: case 3: case 5: case 7:
        if(k<12) p1=0x01;                 /* se levanta andando */
        if(k>=12&&k<16) p1=0x01|NB2_B3;   /* boton 3 andando */
        if(k==40){ printf("%d tras B3: mano %08x res %08x  reserva(tipo+2)=%d arma=%d\n",fr,L32(P1+0x82),L32(RES),nb2_reserve_weapon(0),m[0x208d65]); shot(fr,"cambio"); step++; t0=fr; }
        break;
      case 4: case 6: case 8:
        if(k>5&&k<100&&k%25<3) p1=0x10;
        if(k==120){ printf("%d [9c=%d slot0 %08x slot1 %08x] mano %08x mun %d | res %08x mun %d\n",fr,W16(P1+0x9c),L32(0x23540e),L32(0x235412),L32(P1+0x82),m[0x208d67],L32(RES),L32(RES)!=0xffffffff?W16(L32(RES)+0x60):-1); step++; t0=fr; }
        break;
      }
    }
    nb2_set_inputs(p1,0,0);
    nb2_frame();
    if(getenv("ST")&&fr>2300&&fr<2420) printf("   %d 4e=%d in=%02x\n",fr,W16(P1+0x4e),p1);
  }
}
