#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char*m;
static void S16(unsigned a,int v){m[a]=v>>8;m[a+1]=v;}
static unsigned char snap[4][0x20000];
int main(int c,char**v){ char err[128]; nb2_load(v[1],err,128); nb2_set_wide(1); nb2_set_cheats(1,-1); nb2_reset();
 FILE*f=fopen("/tmp/mans/mansion.sav","rb"); static unsigned char b[1<<22]; size_t n=fread(b,1,sizeof b,f); fclose(f);
 int P[4][2]={{400,367},{500,367},{500,447},{560,447}};
 for(int s=0;s<4;s++){ nb2_state_load(b,n); m=nb2_mem();
  for(int i=0;i<90;i++){ S16(0x208d06+0x14,P[s][0]);S16(0x208d06+0x18,P[s][1]); S16(0x208dba+0x14,P[s][0]);S16(0x208dba+0x18,P[s][1]); nb2_set_inputs(0,0,0); nb2_frame(); m=nb2_mem();}
  memcpy(snap[s],m+0x200000,0x20000);}
 for(int a=0;a<0x20000;a+=2){ int w[4]; for(int s=0;s<4;s++) w[s]=(short)(snap[s][a]<<8|snap[s][a+1]);
   int dx1=w[1]-w[0], dy=w[2]-w[1], dx2=w[3]-w[2];
   if((abs(dx1)==100 && abs(dx2)==60) || (dx1==0&&dy!=0&&dx2==0&&abs(dy)>40&&abs(dy)<90)) printf("%06x: %d %d %d %d\n",0x200000+a,w[0],w[1],w[2],w[3]); }
}
