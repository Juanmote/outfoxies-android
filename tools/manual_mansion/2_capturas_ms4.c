#include <stdio.h>
#include <stdlib.h>
#include "nb2.h"
extern int dbg_mask;
static unsigned char*m;
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){ char err[128]; nb2_load(v[1],err,128); nb2_set_wide(1); nb2_set_cheats(1,-1); nb2_reset();
 FILE*f=fopen("/tmp/mans/mansion.sav","rb"); static unsigned char b[1<<22]; size_t n=fread(b,1,sizeof b,f); fclose(f);
 for(int y=-48;y<=1056;y+=48){ nb2_state_load(b,n); m=nb2_mem();
  for(int x=-64;x<=1344;x+=64){ int fr=(x==-64)?120:40;
   for(int i=0;i<fr;i++){ S16(0x208d06+0x14,x);S16(0x208d06+0x18,y); S16(0x208dba+0x14,x);S16(0x208dba+0x18,y); dbg_mask=0; nb2_set_inputs(0,0,0); nb2_frame(); m=nb2_mem();}
   char nm[80]; dbg_mask=0x1008; sprintf(nm,"/tmp/pan/r_%d_%d.ppm",y,x); shot(nm); dbg_mask=8; sprintf(nm,"/tmp/pan/c_%d_%d.ppm",y,x); shot(nm); dbg_mask=0; printf("%d %d %d %d\n",y,x,(short)(m[0x208170]<<8|m[0x208171]),(short)(m[0x208172]<<8|m[0x208173])); }}
}
