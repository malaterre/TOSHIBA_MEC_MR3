//#define __STDC_WANT_LIB_EXT1__ 1
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <byteswap.h>
#include <math.h>

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

enum Type {
  UNK1          = 0x01, // 55ff ?
  UNK2          = 0x02, // 
  WSTRING       = 0x03, // ISO-8859-1 ?
  UNK4          = 0x04, // 
  VECT2FLOAT    = 0x05, // float single precision x 2. a806 seems to refers to FOV (700d,1005)
  VECT3FLOAT    = 0x06, // float single precision x 3. 6719/671a/671b Orientation Vector (700d,1002)
  FLOAT8        = 0x08, // 0x55f9 patient weight / 55f8 Patient height * 100 (in cm)
  UNKB          = 0x0b, // 
  DATETIME      = 0x0e, // Date/Time stored as ASCII
  UNK20         = 0x20, // USAN string ???
  UNK21         = 0x21, // 3a5e ??
  UINT16        = 0x22, // 1bc3 contains a 64x64x 16bits icon image (most likely either bytes or ushort)
  CHARACTER_SET = 0x23, // 17f2 seems to store the character set used / to use ? Stored as UTF-16 ?
  UNK24         = 0x24, // 
  UNK25         = 0x25, // 
  FLOAT28       = 0x28, // float single precision. afea ??
  DOUBLE        = 0x29, // 0x13ec is Imaging Frequency
  BOOL          = 0x2a, // BOOL stored as UINT32
  STRING        = 0x2c, // ASCII (UTF-8 ?) string
  UNK31         = 0x31, // 
  UNK32         = 0x32, // 
  UNKBA         = 0xba, // 
  STRING41      = 0xc1, // 6 bytes strings, with 41 padding. 0xa965 ?
  UNKC2         = 0xc2, // 66 / 396, all multiple of 11 ??
  UNKD0         = 0xd0,
#if 0
 #type: 0x01 
 #type: 0x02 
 #type: 0x03 
 #type: 0x04 
 #type: 0x05 
 #type: 0x06 
 #type: 0x08 
 #type: 0x0b 
 #type: 0x0e 
 #type: 0x20 
 #type: 0x21 
 #type: 0x22 
 #type: 0x23 
 #type: 0x24 
 #type: 0x25 
 #type: 0x28 
 #type: 0x29 
 #type: 0x2a 
 #type: 0x2c 
 #type: 0x31 
 #type: 0x32 
 #type: 0x40 
 #type: 0x41 
 #type: 0x43 
 #type: 0x44 
 #type: 0x46 
 #type: 0x5e 
 #type: 0x5f 
 #type: 0x70 
 #type: 0x72 
 #type: 0xb8 
 #type: 0xb9 
 #type: 0xba 
 #type: 0xbb 
 #type: 0xc1 
 #type: 0xc2 
 #type: 0xc3 
 #type: 0xd0 
#endif
};
static bool iszero( float f )
{
  char buf0[4];
  char buf1[4];
  float zero = 0;
  memcpy( buf0, &zero, sizeof zero );
  memcpy( buf1, &f, sizeof f );
  int b = memcmp( buf0, buf1, 4 );
//  if( f == 0.0 ) assert( b == 0 );
  return true;
}

static void print_float( const float * buffer, int len)
{
  const int  m = sizeof(float);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
#if 0
      const float cur = buffer[i];
#else
      float cur = -1;
      memcpy( &cur, buffer+i, sizeof cur);
#endif
      assert( iszero(cur) && isfinite(cur) && !isnan(cur) );
      printf("%f", cur);
  }
  printf("] #%d", len);
}
static void print_double( const double * buffer, int len)
{
  const int  m = sizeof(double);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const double cur = buffer[i];
      assert( isfinite(cur) && !isnan(cur) );
      printf("%g", cur);
  }
  printf("] #%d", len);
}
static void print_uint16( const uint16_t * buffer, int len)
{
  const int  m = sizeof(uint16_t);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const uint16_t cur = buffer[i];
      printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_int32( const int32_t * buffer, int len)
{
  const int  m = sizeof(int32_t);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const int32_t cur = buffer[i];
      printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_int64( const int64_t * buffer, int len)
{
  const int  m = sizeof(int64_t);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const int64_t cur = buffer[i];
      printf("%lld", cur);
  }
  printf("] #%d", len);
}
static void print_uint64( const uint64_t * buffer, int len)
{
  const int  m = sizeof(uint64_t);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const uint64_t cur = buffer[i];
      printf("%llu", cur);
  }
  printf("] #%d", len);
}
static void print_uint32( const uint32_t * buffer, int len)
{
  const int  m = sizeof(uint32_t);
  assert( len % m == 0 );
  int i;
  printf(" [");
  for (i=0;i < len / m; i++) {
      if(i) printf(",");
      const uint32_t cur = buffer[i];
      printf("%u", cur);
  }
  printf("] #%d", len);
}
static void print_hex( const unsigned char * buffer, int len)
{
  int i;
  printf(" [");
  for (i=0;i < len ; i++) {
      const unsigned char cur = buffer[i];
      if(i) printf("\\");
      printf("%02x", cur);
  }
  printf("] #%d", len);
}

static void print_string( const char * buffer, int len)
{
  printf(" [%.*s] #%d (%d)", len, buffer, len, strnlen(buffer, len));
}
static void print_wstring( const char * buffer, int len)
{
  static const char magic[] = { 0xdf, 0xff, 0x79 };
  int b = memcmp(buffer, magic, sizeof(magic) );
  if( b == 0) {
/*
$ hexdump -C out0000 
00000000  df ff 79 17 01 09 00 49  53 4f 38 38 35 39 2d 31  |..y....ISO8859-1|
00000010  02 08 00 30 30 30 30 30  30 30 30                 |...00000000|
0000001b
*/
    assert( buffer[4] == 0x1 );
    assert( buffer[5] == 0x9 );
    assert( buffer[6] == 0x0 );
    assert( buffer[16] == 0x2 );
    assert( buffer[18] == 0x0 );
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
    assert( strncmp( iso, "ISO8859-1", 9 ) ==  0);
    printf(" [%.*s : %.*s] #%d", len3, iso, len4, next, len);
  } else {
    printf(" [%.*s] #%d (%d)", len, buffer, len, strnlen(buffer, len));
  }
}

static void print_string41( const char * buffer, int len)
{
  assert( len % 6 == 0 );
  const int n = len / 6;
  int i;
  printf(" [");
  for( i = 0; i < n; ++i ) {
      if(i) printf(",");
    const char * str = buffer + i * 6;
    assert( str[3] == 0x0 );
    const int c = str[4];
    assert( str[5] == 0x41 );
    printf("%.*s#%d", 3, str, c);
  }
  printf("] #%d", len);
}
static const unsigned char usan[] = {
  0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
  0x4e, 0x4b, 0x4e, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static void print(int type, char *buffer, int len)
{
  switch(type)
  {
    case UNK1:
      assert( len == 4 );
      //print_float( (float*)buffer, len);
      print_uint32( (uint32_t*)buffer, len);
      //print_uint16( (uint16_t*)buffer, len);
      //print_hex( buffer, len);
      break;
    case UNK2:
      assert( len % 4 == 0 );
      //print_float( (float*)buffer, len);
      print_uint16( (uint16_t*)buffer, len);
      print_uint32( (uint32_t*)buffer, len);
      print_hex( buffer, len);
      break;
    case VECT2FLOAT:
      assert( len == 8 );
      print_float( (float*)buffer, len);
      break;
    case VECT3FLOAT:
      assert( len % 12 == 0 );
      print_float( (float*)buffer, len);
      break;
    case UNK4:
      assert( len % 4 == 0 );
      print_uint32( (uint32_t*)buffer, len);
      break;
    case UNKB:
      assert( len == 12 ); // int32 x 3 ?
      print_int32( (int32_t*)buffer, len);
      break;
    case DATETIME:
      assert( len == 19 || len == 20 );
      print_string( buffer, len);
      break;
    case FLOAT8:
      assert( len == 4 );
      print_float( (float*)buffer, len);
      break;
    case UNK20:
      assert( len == 68 );
      //assert( sizeof(usan) == 68 );
      //assert( memcmp(buffer, usan, sizeof(usan) ) == 0 );
      //dump2file(buffer, len);
      print_hex(buffer, len);
      break;
    case UNK21:
      assert( len == 20 || len == 16 || len == 24 );
      print_float( (float*)buffer, len);
      print_int32( (int32_t*)buffer, len);
      print_hex( buffer, len);
      break;
    case UINT16:
      print_uint16( (uint16_t*)buffer, len);
      //print_hex( buffer, len);
      break;
    case UNK24:
      assert( len % 4 == 0 );
      print_int32( (int32_t*)buffer, len);
      break;
    case UNK25:
      assert( len == 4 || len == 512 );
      print_uint32( (uint32_t*)buffer, len);
      break;
    case UNKBA:
      assert( len == 8 ); // pair of uint32 ?
      print_uint32( (uint32_t*)buffer, len);
      break;
    case UNK31:
      assert( len == 8 || len == 16 );
      print_uint64( (uint64_t*)buffer, len);
      break;
    case UNK32:
      assert( len % 4 == 0 ); // all uint are lower than uin16_max
      print_uint32( (uint32_t*)buffer, len);
      break;
    case BOOL:
      assert( len % 4 == 0 ); // bool ?
      print_uint32( (uint32_t*)buffer, len);
      break;
    case DOUBLE:
      assert( len == 8 );
      print_double( (double*)buffer, len);
      break;
    case UNKD0:
      print_uint64( (uint64_t*)buffer, len);
      print_hex( buffer, len);
      break;
    case FLOAT28:
      assert( len % 4 == 0 );
      print_float( (float*)buffer, len);
      break;
    case WSTRING:
      print_wstring( buffer, len);
      break;
    case STRING41:
      print_string41( buffer, len);
      break;
    case UNKC2:
{
      assert( len % 11 == 0 );
      int mult = len / 11;
      assert( mult % 6 == 0 );
      //print_uint16( (uint16_t*)buffer, len);
      //dump2file(buffer, len );
      print_hex( buffer, len);
}
      break;
    case STRING:
{
      //if( len % 2 == 0 ) assert( buffer[len-1] == 0 );
      //b3d5 does not seems to contains a trailing NULL
      //size_t sl = strnlen(buffer, len);
      //printf(" [%.*s] #%d (%d)", len, buffer, len, strnlen(buffer, len));
      print_string( buffer, len );
      //printf(" [%s]", buffer);
}
      break;
#if 0
    case 0xc3:
    case CHARACTER_SET:
      dump2file(buffer, len );
      break;
#endif
    default:
      //printf(" [??] #%d", len);
      print_hex( buffer, len);
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

  //bool lastgroup = false;
  int remain = 0;
  //for( r = 0; r < 6; ++r )
  while( --remain != 0 )
  {
    uint32_t nitems;
    fread(&nitems, 1, sizeof nitems, in);
    //if( nitems == 1 || nitems == 3 ) {
    if( nitems <= 3 ) {
       // special case to handle last element ?
      printf("<#last element coming: %08x>\n", nitems);
      assert( nitems > 0 );
      remain = nitems;
      fread(&nitems, 1, sizeof nitems, in);
    }
    printf("Group %d #Items: %u\n", r,  nitems );
    ++r;
    for( i = 0; i < nitems; ++i )
    {
      memset(&s, 0, sizeof s);
      long pos = ftell(in);
      //printf("Offset 0x%x \n", pos );
      fread(&s, 1, sizeof s, in);
      memcpy(&si, &s, sizeof s);
      //printf("  #k1: %08ld 0x%08lx #k2:%016u 0x%08x", si.k1, si.k1, si.k2, si.k2 );
      //printf("  #k0: 0x%08lx #k2: 0x%08x #k3: 0x%04x k4: 0x%04x", si.k1, si.k2, si.len, si.k4 );
      //printf("  #Pos: %08ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos, si.len, si.len );
      assert( si.k4 == 0 );
#if 0
      if( si.k4 != 0 ) {
        assert( si.k4 == 0xff00 );
        memcpy(buf, &s, sizeof s);
        fread(buf+ sizeof s, 1, 4, in);
        memcpy(&si, buf+ 4, sizeof s);
        uint32_t debug;
        memcpy(&debug, buf, 4);
        printf(" #debug: %08x\n", debug);
      }
#endif
      //printf("  #k1: 0x%08lx #k2: 0x%08x #k3: 0x%04x k4: 0x%04x", si.k1, si.k2, si.len, si.k4 );
      //printf("  #k11: 0x%04x #k12: 0x%04x #k21: 0x%02x #k22: 0x%04x #k3: %04d", si.k11, si.k12, si.k21 >> 8, si.k22, si.len );
/*
 * #k22: 0x0000
 * #k22: 0x0007
 * #k22: 0x000b
 * #k22: 0x0017
 * #k22: 0x001b
 * #k22: 0x001f
 * #k22: 0xff00
 * #k22: 0xfff0
 */
      assert( si.k22 == 0x0000 // 0
           || si.k22 == 0x0007 // 7
           || si.k22 == 0x000b // 11
           || si.k22 == 0x0017 // 23
           || si.k22 == 0x001b // 27
           || si.k22 == 0x001f // 31
           || si.k22 == 0xff00 // 65280
           || si.k22 == 0xfff0 // 65520
           );
      assert( ( si.k21 & 0x00ff ) == 0x0 );
      const uint8_t type = si.k21 >> 8;
      printf("  #k11: 0x%04x #k12: 0x%01x #type: 0x%02x #k22: 0x%04x ", si.k11, si.k12, type, si.k22 );
      assert( si.k12 <= 0x3 );
      assert( si.len <= 9184 /* 8192 */ );
      //printf("  #k1: 0x%08lx #k2: 0x%08x", si.k1, si.k2 );
//      printf("  #Pos: %7ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos, si.len, si.len );
      int b = memcmp(si.separator, magic2, sizeof(magic2) );
      assert( b == 0 );
      assert( si.len <= sizeof buffer );
      nread = fread( buffer, 1, si.len, in );
      assert( nread == (size_t)si.len );
      print( type, buffer, nread );
//      if( si.k2 == 0xff002c00 )
// printf("  buffer: [%.*s]", nread,  buffer );
      printf("\n" );
    }
  }

  long pos = ftell(in);
  printf("pos: 0x%08x 0x%08x\n", pos, sz);
  assert( pos == sz || pos + 1 == sz );
  int ret = feof(in);
  printf("feof: %d\n", ret);
  fclose(in);
  return 0;
}
