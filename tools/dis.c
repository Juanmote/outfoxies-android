/* Desensamblador de la 68020 para volcados de memoria de headless (OFX_DUMP=mem.bin):
   gcc -O1 -w -Ithird_party/musashi -o dis tools/dis.c third_party/musashi/m68kdasm.c third_party/musashi/m68kcpu.c \
       third_party/musashi/m68kops.c third_party/musashi/softfloat/softfloat.c -lm
   dis mem.bin <desde_hex> <hasta_hex> */
#include <stdio.h>
#include <stdlib.h>
#include "m68k.h"
static unsigned char *m;
unsigned int m68k_read_disassembler_8(unsigned int a){return m[a&0xffffff];}
unsigned int m68k_read_disassembler_16(unsigned int a){a&=0xffffff;return m[a]<<8|m[a+1];}
unsigned int m68k_read_disassembler_32(unsigned int a){a&=0xffffff;return m[a]<<24|m[a+1]<<16|m[a+2]<<8|m[a+3];}
unsigned int m68k_read_memory_8(unsigned int a){return 0;} unsigned int m68k_read_memory_16(unsigned int a){return 0;} unsigned int m68k_read_memory_32(unsigned int a){return 0;}
void m68k_write_memory_8(unsigned int a,unsigned int v){} void m68k_write_memory_16(unsigned int a,unsigned int v){} void m68k_write_memory_32(unsigned int a,unsigned int v){}
int main(int c,char**v){FILE*f=fopen(v[1],"rb");m=malloc(0x1000000);fread(m,1,0x1000000,f);
unsigned a=strtoul(v[2],0,16),e=strtoul(v[3],0,16);char b[256];
while(a<e){int n=m68k_disassemble(b,a,M68K_CPU_TYPE_68EC020);printf("%06X: %s\n",a,b);a+=n;}}
