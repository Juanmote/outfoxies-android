/* Banco de pruebas del logro de las pistolas de juguete del circo (11-11-2026) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "achievements.h"
#include "menu.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static unsigned L32(unsigned a){return (unsigned)m[a]<<24|m[a+1]<<16|m[a+2]<<8|m[a+3];}
static void S32(unsigned a,unsigned v){m[a]=v>>24;m[a+1]=v>>16;m[a+2]=v>>8;m[a+3]=v;}
static unsigned P1=0x208d06, P2=0x208dba;
int main(int c,char**v){
  char err[128]; int frames=atoi(v[2]);
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  nb2_set_cheats(1,1); nb2_reset(); m=nb2_mem();
  config_defaults(); ach_init("/tmp/ach5");
  int t0=0, fired=0;
  for(int fr=0;fr<frames;fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    if(fr>=560&&fr<566) p1=0x10;
    if(fr>=600 && m[0x208d59] && m[0x208e0d]){
      unsigned held=L32(P1+0x82);
      S32(P2+0x14, 0x00400000);
      if(held==0xffffffff||!held){
        for(unsigned a=0x208000;a<0x220000;a+=2)
          if((W16(a+0x10)&0xc000)==0xc000&&W16(a+0x12)==0x14&&L32(a+0x82)==0xffffffff&&W16(a+0x26)>=1&&W16(a+0x26)<=3){ memcpy(m+P1+0x14,m+a+0x14,8); break; }
        p1=(fr%10)<3?0x04:0; t0=fr;
      } else { int k=fr-t0; if(k==1) printf("%d cogida juguete %d mun %d\n",fr,W16(held+0x26),W16(held+0x60)); if(k>20 && k%30<3) p1=0x10; }
    }
    if(0) { printf("%d rival %d:",fr,m[0x20807b]>>4); for(unsigned a=0x208000;a<0x220000;a+=2) if(W16(a+0x12)==0x14&&W16(a+0x5e)<0x15&&(W16(a+0x10)&0xc000)==0xc000) printf(" [t%d j%d own%x]",W16(a+0x5e),W16(a+0x26),L32(a+0x82)); printf("\n"); }
    if(fr>=10530&&fr<10700){ unsigned o=L32(P1+0x82); printf("  %d o=%x ammo=%d p1=%02x",fr,o,m[0x208d67],p1); if(o!=0xffffffff) printf(" f10=%04x 60=%d 26=%d",W16(o+0x10),W16(o+0x60),W16(o+0x26)); printf(" 20d572:f10=%04x\n",W16(0x20d572+0x10)); }
    if(fr%100==0) for(unsigned a=0x208000;a<0x220000;a+=2) if(W16(a+0x12)==0x14&&W16(a+0x5e)<0x15&&W16(a+0x26)>=1&&W16(a+0x26)<=3&&W16(a+0x5e)==1) { printf("%d juguete %06x j%d f10 %04x own %x pos %04x,%04x\n",fr,a,W16(a+0x26),W16(a+0x10),L32(a+0x82),W16(a+0x14),W16(a+0x18)); }
    nb2_set_inputs(p1,0,0);
    nb2_frame();
    ach_frame(m,1); int r; while((r=ach_pop())>=0) printf("%d logro %d %s\n",fr,r,ach_title(r));
  }
}
