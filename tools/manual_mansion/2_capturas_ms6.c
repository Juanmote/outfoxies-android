#include <stdio.h>
#include <stdlib.h>
#include "nb2.h"
extern int dbg_mask;
static unsigned char*m;
static int W16(unsigned a){return (short)(m[a]<<8|m[a+1]);}
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){ char err[128]; nb2_load(v[1],err,128); nb2_set_wide(1); nb2_set_cheats(1,-1); nb2_reset();
 FILE*f=fopen("/tmp/mans/mansion.sav","rb"); static unsigned char b[1<<22]; size_t n=fread(b,1,sizeof b,f); fclose(f);
 nb2_state_load(b,n); m=nb2_mem();
 int cx=atoi(v[2]),cy=atoi(v[3]);
 for(int i=0;i<atoi(v[4]);i++){ S16(0x208170,cx); S16(0x208172,cy); nb2_set_inputs(0,0,0); nb2_frame(); m=nb2_mem();}
 printf("cam %d %d / %d %d\n",W16(0x208170),W16(0x208172),W16(0x208196),W16(0x208198));
 dbg_mask=8; shot(v[5]); }
