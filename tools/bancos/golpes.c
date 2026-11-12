/* Compilar con el gancho de instrucciones de Musashi (ver ESTADO.md 0.21.0): cada .c del nucleo con
   -DM68K_INSTRUCTION_HOOK=M68K_OPT_SPECIFY_HANDLER "-DM68K_INSTRUCTION_CALLBACK(pc)=hook(pc)". ESC=escenario ARMA=tipo. */
/* Registra cada llamada a las rutinas de dano 0x305A / 0x30C4: D0, quien llama, victima y objeto que golpea */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nb2.h"
#include "m68k.h"
static unsigned char *m;
static int W16(unsigned a){return m[a]<<8|m[a+1];}
static unsigned L32(unsigned a){return (unsigned)m[a]<<24|m[a+1]<<16|m[a+2]<<8|m[a+3];}
static unsigned P1=0x208d06, P2=0x208dba;
static int frame;
static int cnt[64][0x40000/64];
void hook(unsigned pc){
  if(pc!=0x305a && pc!=0x30c4) return;
  unsigned sp=m68k_get_reg(NULL,M68K_REG_SP), ret=m68k_read_memory_32(sp);
  unsigned d0=m68k_get_reg(NULL,M68K_REG_D0), a5=m68k_get_reg(NULL,M68K_REG_A5);
  unsigned a0=m68k_get_reg(NULL,M68K_REG_A0),a1=m68k_get_reg(NULL,M68K_REG_A1),a2=m68k_get_reg(NULL,M68K_REG_A2),a3=m68k_get_reg(NULL,M68K_REG_A3),a4=m68k_get_reg(NULL,M68K_REG_A4);
  if(m && a5>=0x200000 && a5<0x220000)
  printf("f%d %s d0=%u ret=%06x st=%d y=%d vict=%s ch=%d A0=%06x[cl%02x t%d] A1=%06x A2=%06x[cl%02x t%d] A3=%06x[cl%02x] A4=%06x[cl%02x t%d]\n",
    frame, pc==0x305a?"305a":"30c4", d0, ret, W16(a5+0x4e),(short)W16(a5+0x18), a5==P1?"1P":a5==P2?"2P":"?", W16(a5+0x9c),
    a0, a0>=0x200000&&a0<0x220000?W16(a0+0x12):0, a0>=0x200000&&a0<0x220000?W16(a0+0x5e):0, a1,
    a2, a2>=0x200000&&a2<0x220000?W16(a2+0x12):0, a2>=0x200000&&a2<0x220000?W16(a2+0x5e):0,
    a3, a3>=0x200000&&a3<0x220000?W16(a3+0x12):0,
    a4, a4>=0x200000&&a4<0x220000?W16(a4+0x12):0, a4>=0x200000&&a4<0x220000?W16(a4+0x5e):0);
}
int main(int c,char**v){
  char err[128];
  if(!nb2_load(v[1],err,sizeof err)){puts(err);return 1;}
  int esc=getenv("ESC")?atoi(getenv("ESC")):-1;
  nb2_set_cheats(NB2_CHEAT_INVULN|NB2_CHEAT_NOIFR_HUMAN, esc);
  if(getenv("ARMA")){ static unsigned char map[0x15]; memset(map,atoi(getenv("ARMA")),sizeof map); nb2_set_weapon_map(map);}
  nb2_reset(); m=nb2_mem();
  int N=atoi(v[2]);
  for(frame=0;frame<N;frame++){
    int p1=0;
    if(frame==200||frame==260) nb2_coin();
    if(frame>=320&&frame<326) p1=NB2_START;
    { int n=getenv("CHAR")?atoi(getenv("CHAR")):0; if(frame>=420&&frame<420+n*12&&(frame-420)%12<4) p1=0x01; }
    if(frame>=560&&frame<566) p1=0x10;
    nb2_set_inputs(p1,0,0); nb2_frame(); m=nb2_mem();
  }
}
