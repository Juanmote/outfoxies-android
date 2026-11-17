/* Busca un asiento del escenario (metralleta 128, cabina 69, o ST=estado) desde una partida: teletransporta al 1P por
   la rejilla X0..X1/DX, Y0..Y1/DY y pulsa cada tecla (K=hex para una sola). Guarda la partida ya sentado en OUT.
   Metralleta del avion (cupula de arriba): x 600-618, y 330 andando a la derecha (K=01). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
static unsigned char *load(const char*p,long*n){FILE*f=fopen(p,"rb");fseek(f,0,2);*n=ftell(f);fseek(f,0,0);unsigned char*b=malloc(*n);fread(b,1,*n,f);fclose(f);return b;}
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  if(getenv("ROPA")) nb2_set_outfit(0,atoi(getenv("ROPA")));
  if(getenv("PACK")) { extern int nb2_dummy; }
  nb2_set_cheats(1,-1); nb2_reset();
  long n; unsigned char *b=load(v[2],&n);
  int x0=atoi(getenv("X0")),x1=atoi(getenv("X1")),dx=atoi(getenv("DX")),y0=atoi(getenv("Y0")),y1=atoi(getenv("Y1")),dy=atoi(getenv("DY"));
  int keys[8]={0x04,0x08,0x10,0x20,0x01,0x02}, nk=6; if(getenv("K")){nk=1;keys[0]=strtol(getenv("K"),0,16);}
  for(int y=y0;y<=y1;y+=dy) for(int x=x0;x<=x1;x+=dx) for(int k=0;k<nk;k++){
    nb2_state_load(b,n); m=nb2_mem();
    S16(0x208d06+0x14,x); S16(0x208d06+0x18,y);
    for(int f=0;f<90;f++){
      int in = f>20 && (f%30)<10 ? keys[k] : 0;
      nb2_set_inputs(in,0,0); nb2_frame();
      int s=W16(0x208d06+0x4e);
      if(s==128||s==69||(getenv("ST")&&s==atoi(getenv("ST")))){ printf("x%d y%d tecla %02x estado %d\n",x,y,keys[k],s);
        for(int g=0;g<40;g++){nb2_set_inputs(0,0,0);nb2_frame();}
        char nm[64]; sprintf(nm,"/tmp/as/seat_%d.ppm",s); shot(nm);
        size_t len; unsigned char *st=nb2_state_save(&len,1); sprintf(nm,"%s",getenv("OUT")?getenv("OUT"):"/tmp/as/seat.sav"); FILE*f2=fopen(nm,"wb"); fwrite(st,1,len,f2); fclose(f2);
        return 0; }
    }
  }
  puts("nada");
}
