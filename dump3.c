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
#if 0
  uint32_t k3;
#else
  uint16_t len;
  uint16_t k4;
#endif
  unsigned char separator[22-4];
} I;

typedef struct K {
  uint32_t k1;
  uint32_t k2;
  uint16_t k31;
  uint16_t k32;
  uint16_t k41;
  uint16_t k42;
  uint32_t len;
} K;

typedef struct K2 {
  uint16_t k1;
  uint16_t k2;
  uint16_t len;
  uint16_t k4;
} K2;

typedef struct __attribute__ ((__packed__)) K3
{
  uint16_t k1; // somewhat increasing
  uint16_t k2; // [0,1,2,3]
  uint16_t type;
  uint16_t flag;
  uint16_t len;
  unsigned char separator[22];
} K3;


static void print_k( K * k )
{
  printf(" k1: %08x k2: %08x k31: %x k32: %x k41: %04x k42: %04x len: %u\n", k->k1, k->k2, k->k31, k->k32, k->k41, k->k42, k->len);
}

static void print_k2( K2 * k )
{
  k->len = bswap_16(k->len);
  printf(" k1: %x k2: %x len: %u k4: %u\n", k->k1, k->k2, k->len, k->k4);
}

static void print_k3( K3 * k )
{
  printf(" k1: %x k2: %x type: %04x flag: %04x len: %u\n", k->k1, k->k2, k->type, k->flag, k->len);
}

static void parse_end( const unsigned char *in, size_t len )
{
  const unsigned char *p = in;
  const unsigned char *end = in + len;
  int i;
  assert( len == 332 );
#if 0
#if 1
  K k;
#if 0
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
  p += k.len * sizeof(uint32_t);
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
  p += k.len * sizeof(uint32_t);
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
  p += k.len * sizeof(uint32_t);
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
  p += k.len * sizeof(uint32_t);
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
#else
  for(i = 0; i < 8; ++i ) {
  memcpy( &k, p, sizeof k);
  print_k(&k);
  p += sizeof k;
  if( k.k42 == 0xff00 ) {
    printf("shift: %u\n", k.len * sizeof(uint32_t));
  p += k.len * sizeof(uint32_t);
  } else {
    printf("shift: %u\n", 24);
  p += 24;
}
  }
#endif
#else
  p += 8;
  K2 k;
  memcpy( &k, p, sizeof k);
  print_k2(&k);
  p += k.len;
  memcpy( &k, p, sizeof k);
  print_k2(&k);
  p += k.len;
  memcpy( &k, p, sizeof k);
  print_k2(&k);
#endif
#else
  p += 8;
  static const unsigned char magic[] = {0,0,0,0,0,0,0,0,0,0,0xc,0,0,0,0,0,0,0,0,0,0,0};
  K3 k;
  assert( sizeof k == 32 );
  for(i = 0; i < 10; ++i ) {
    memcpy( &k, p, sizeof k);
    print_k3(&k);
    p += sizeof k;
    p += k.len;
  }
  int b = memcmp(k.separator, magic, sizeof(magic) );

#endif
  
  assert(0);
}

int main(int argc, char * argv[])
{
  if( argc < 2 ) return 1;
  const char * filename = argv[1];
  FILE * in = fopen( filename, "rb" );
  int i,r;
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

  for( r = 0; r < 6; ++r )
  {
    uint32_t nitems;
    fread(&nitems, 1, sizeof nitems, in);
    printf("#Items:%u\n", nitems );
    for( i = 0; i < nitems; ++i )
    {
      memset(&s, 0, sizeof s);
      long pos = ftell(in);
      fread(&s, 1, sizeof s, in);
      memcpy(&si, &s, sizeof s);
      printf("  #Pos: %ld %lx #Len:%u %x\n", pos, pos, si.len, si.len );
      nread = fread( buffer, 1, si.len, in );
      if( nread != (size_t)si.len ) {
        printf("  #Expected: %u %x #Read: %lu %lx\n", si.len, si.len, nread, nread );
        assert( nread == 332 && r == 5 );
        //FILE * out = fopen("end", "wb");
        //fwrite( buffer, 1, nread, out);
        //fclose(out);
        parse_end( buffer, nread );
      }
    }
  }

  int ret = feof(in);
  printf("feof: %d\n", ret);
  fclose(in);
  return 0;
}
