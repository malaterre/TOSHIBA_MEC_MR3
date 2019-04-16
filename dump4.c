#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <byteswap.h>

typedef struct S {
  char buf[32];
} S;

typedef struct /* __attribute__((__packed__))*/ I {
#if 0
  uint32_t k1;
#else
  uint16_t k11;
  uint16_t k12;
#endif
#if 0
  uint32_t k2;
#else
  uint16_t k21;
  uint16_t k22;
#endif
  uint16_t len;
  uint16_t k4;
  unsigned char separator[20];
} I;

static const unsigned char magic2[] = {0,0,0,0,0,0,0,0,0xc,0,0,0,0,0,0,0,0,0,0,0};

static int debug = 0;
static void dump2file(const char * in, int len )
{
  char buffer[512];
  sprintf( buffer, "out%04d", debug );
  ++debug;
  FILE * f = fopen( buffer, "wb" );
  fwrite( in, 1, len, f );
  fclose( f );
}

enum Type{
  FLOAT1     = 0x01, // float single precision. 55ff ?
  WSTRING    = 0x03, // ISO-8859-1 ?
  FLOAT5     = 0x05, // float single precision. a806 seems to refers to FOV (700d,1005)
  VECT3FLOAT = 0x06, // float single precision x 3. 6719/671a/671b Orientation Vector (700d,1002)
  CHARACTER_SET = 0x23, // 17f2 seems to store the character set used / to use ? Stored as UTF-16 ?
  STRING     = 0x2c, // ASCII (UTF-8 ?) string
};
static void print_float( const float * buffer, int len)
{
  const int  m = sizeof(float);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m;i++) {
 if(i)
  printf(",");
      printf("%f",buffer[i]);
  }
  printf("]");
}

static void print_wstring( const char * buffer, int len)
{
  const char * iso = (char*)buffer + 7;
  const int diff = len - (7 + 9 + 3);
  const char * next = buffer + 7 + 9 + 3;
  //dump2file( buffer, len );
  //assert(0);
  int len2 = (unsigned char)buffer[3];
  int len3 = (unsigned char)buffer[5];
  int len4 = (unsigned char)buffer[17];
  assert(len2 + 4 == len);
  assert( len3 == 9 );
  assert( len4 == diff );
  printf(" [%.*s : %.*s] #%d", len3, iso, len4, next, len);
}
static void print(int type, char *buffer, int len)
{
  switch(type)
  {
    case FLOAT1:
      assert( len == 4 );
      print_float( (float*)buffer, len);
      break;
    case WSTRING:
      print_wstring( buffer, len);
      break;
    case FLOAT5:
      assert( len == 4 || len == 8 );
      print_float( (float*)buffer, len);
      break;
    case VECT3FLOAT:
      assert( len % 12 == 0 );
      print_float( (float*)buffer, len);
      break;
    case STRING:
      //assert( buffer[len-1] == 0 );
      printf(" [%.*s] #%d", len, buffer, len);
      //printf(" [%s]", buffer);
      break;
    case 0xc3:
    case CHARACTER_SET:
      dump2file(buffer, len );
      break;
    default:
      printf(" [??] #%d", len);
  }
}

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
  assert( sizeof(si.separator) == 20 );
  assert( sizeof(magic2) == 20 );
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
      //printf("  #k11: 0x%04x #k12: 0x%04x #k21: 0x%02x #k22: 0x%04x #k3: %04d", si.k11, si.k12, si.k21 >> 8, si.k22, si.len );
      printf("  #k11: 0x%04x #k12: 0x%04x #k21: 0x%02x #k22: 0x%04x ", si.k11, si.k12, si.k21 >> 8, si.k22 );
      assert( si.k12 <= 0x3 );
      assert( ( si.k21 & 0x00ff ) == 0x0 );
      assert( si.len <= 8192 );
      //printf("  #k1: 0x%08lx #k2: 0x%08x", si.k1, si.k2 );
//      printf("  #Pos: %7ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos, si.len, si.len );
      int b = memcmp(si.separator, magic2, sizeof(magic2) );
      assert( b == 0 );
      assert( si.len <= sizeof buffer );
      nread = fread( buffer, 1, si.len, in );
      assert( nread == (size_t)si.len );
      print( si.k21 >> 8, buffer, nread );
//      if( si.k2 == 0xff002c00 )
// printf("  buffer: [%.*s]", nread,  buffer );
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
