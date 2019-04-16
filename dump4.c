#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <byteswap.h>

typedef struct S {
  char buf[32];
} S;

typedef struct I {
  uint32_t k1;
  uint32_t k2;
  uint16_t len;
  uint16_t k4;
  unsigned char separator[22-4];
} I;

static const unsigned char magic2[] = {0,0,0,0,0,0,0,0,0xc,0,0,0,0,0,0,0,0,0,0,0};

int main(int argc, char * argv[])
{
  if( argc < 2 ) return 1;
  const char * filename = argv[1];
  FILE * in = fopen( filename, "rb" );
  int i,r = 0;
  unsigned char buffer[5000*2];
  long sz;
  size_t nread;

  fseek(in, 0L, SEEK_END);
  sz = ftell(in);
  fseek(in, 0L, SEEK_SET);
  printf("Size: %ld\n", sz);

  S s;
  assert( sizeof(s) == 32 );
  I si;
  assert( sizeof(si) == 32 );
  char buf[512];


  /* TODO what to do with hypo tag + type because of:
    #k1: 0x000017e3 #k2: 0xff002400
    #k1: 0x000017e3 #k2: 0xff002a00
    */


  bool lastgroup = false;
  //for( r = 0; r < 6; ++r )
  while( !lastgroup )
  {
    uint32_t nitems;
    fread(&nitems, 1, sizeof nitems, in);
    if( nitems == 1 ) {
       // special case to handle last element ?
      printf("<#last element coming: %08x>\n", nitems);
      lastgroup = true;
      fread(&nitems, 1, sizeof nitems, in);
    }
    printf("Group %d #Items: %u\n", r,  nitems );
    ++r;
    for( i = 0; i < nitems; ++i )
    {
      memset(&s, 0, sizeof s);
      long pos = ftell(in);
      fread(&s, 1, sizeof s, in);
      memcpy(&si, &s, sizeof s);
      //printf("  #k1: %08ld 0x%08lx #k2:%016u 0x%08x", si.k1, si.k1, si.k2, si.k2 );
      //printf("  #k0: 0x%08lx #k2: 0x%08x #k3: 0x%04x k4: 0x%04x", si.k1, si.k2, si.len, si.k4 );
      //printf("  #Pos: %08ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos, si.len, si.len );
      assert( si.k4 == 0 );
      if( si.k4 != 0 ) {
        assert( si.k4 == 0xff00 );
        memcpy(buf, &s, sizeof s);
        fread(buf+ sizeof s, 1, 4, in);
        memcpy(&si, buf+ 4, sizeof s);
        uint32_t debug;
        memcpy(&debug, buf, 4);
        printf(" #debug: %08x\n", debug);

      }
      //printf("  #k1: 0x%08lx #k2: 0x%08x #k3: 0x%04x k4: 0x%04x", si.k1, si.k2, si.len, si.k4 );
      printf("  #k1: 0x%08lx #k2: 0x%08x #k3: %04d", si.k1, si.k2, si.len );
      assert( si.len <= 8192 );
      //printf("  #k1: 0x%08lx #k2: 0x%08x", si.k1, si.k2 );
//      printf("  #Pos: %7ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos, si.len, si.len );
      int b = memcmp(si.separator, magic2, sizeof(magic2) );
      assert( b == 0 );
      assert( si.len <= sizeof buffer );
      nread = fread( buffer, 1, si.len, in );
      assert( nread == (size_t)si.len );
      //if( si.k1 == 0xff002c00 ) printf("  buffer: [%.*s]", nread,  buffer );
      printf("\n" );
    }
  }

  long pos = ftell(in);
  printf("pos: 0x%08x 0x%08x\n", pos, sz);
  int ret = feof(in);
  printf("feof: %d\n", ret);
  fclose(in);
  return 0;
}
