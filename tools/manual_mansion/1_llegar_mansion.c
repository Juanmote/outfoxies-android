#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
static unsigned char *m;
static void shot(const char*t){ FILE*f=fopen(t,"wb"); fprintf(f,"P6 %d 224 255\n",nb2_width()); fwrite(nb2_redraw(),1,nb2_width()*224*3,f); fclose(f);}
int main(int c,char**v){
  char err[128]; if(!nb2_load(v[1],err,sizeof err)) return 1;
  nb2_set_wide(1); nb2_set_cheats(1,-1); nb2_reset(); m=nb2_mem();
  int fights=0, prev=0, done=0; size_t len;
  for(int fr=0;fr<atoi(v[2]);fr++){
    int p1=0;
    if(fr==200||fr==260) nb2_coin();
    if(fr>=320&&fr<326) p1=NB2_START;
    if(fr>=560&&fr<566) p1=0x10;
    int in=m[0x208d59]&&m[0x208e0d];
    if(in && !prev){ fights++; printf("f%d combate rival %d tabla:",fr,m[0x20807b]>>4); for(int k=0;k<7;k++) printf(" %02x/%d",m[0x20807c+2*k],m[0x20807d+2*k]); printf("\n");
       if((m[0x20807b]>>4)==7){ done=fr; } }
    if(in && (m[0x20807b]>>4)!=7 && fr%60==0){ /* marcar todos los asesinos vencidos y KO al rival */
       for(int k=0;k<7;k++) if(m[0x20807d+2*k]!=7) m[0x20807c+2*k]=0;
       m[0x208e0d]=0; }
    if(done && (fr-done)%100==0 && fr-done<=3000){ char nm[64]; sprintf(nm,"/tmp/mans/m%05d.ppm",fr-done); shot(nm);
       if(fr-done==600){ unsigned char*st=nb2_state_save(&len,1); FILE*f=fopen("/tmp/mans/mansion.sav","wb"); fwrite(st,1,len,f); fclose(f);} }
    prev=in;
    nb2_set_inputs(p1,0,0); nb2_frame(); m=nb2_mem();
  }
}
