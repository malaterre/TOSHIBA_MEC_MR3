#include "mec_mr3_dict.h"

#include <assert.h>
#include <byteswap.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct I {
  uint32_t key;
  uint32_t type;
  uint32_t len;
  unsigned char separator[20];
} I;

static const unsigned char magic2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0xc, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0, 0,   0};

static int debug = 0;
static void dump2file(const char *in, int len) {
  char buffer[512];
  sprintf(buffer, "out%04d", debug);
  ++debug;
  FILE *f = fopen(buffer, "wb");
  fwrite(in, 1, len, f);
  fclose(f);
}

enum Type {
  UNK1 = 0x00000100,       // 55ff ?
  UNK2 = 0x00000200,       //
  WSTRING = 0x00000300,    // ISO-8859-1 ?
  UNK4 = 0x00000400,       // VM1_n
  VECT2FLOAT = 0x00000500, // float single precision x 2_n a806 seems to refers
                           // to FOV (700d,1005)
  VECT3FLOAT = 0x00000600, // float single precision x 3. 6719/671a/671b
                           // Orientation Vector (700d,1002)
  UNKB = 0x00000b00,       //
  DATETIME = 0x00000e00,   // Date/Time stored as ASCII
  UNKF = 0x00000f00,       //
  UNKD0 = 0x0007d000,
  UNKB8 = 0x000bb800,
  UNKB9 = 0x000bb900,
  UNKBA = 0x000bba00,  //
  UNKBB = 0x000bbb00,  //
  STRC1 = 0x000bc100,  // 6 bytes strings, with 41 padding. 0xa965 ?
  UNKC2 = 0x000bc200,  // 66 / 396, all multiple of 11 ??
  STRC3 = 0x000bc300,  //
  UNK70 = 0x00177000,  //
  UNK72 = 0x00177200,  //
  USAN5E = 0x001b5e00, // USAN string
  USAN5F = 0x001b5f00, // USAN string
  STR40 = 0x001f4000,  // strings ?
  UID41 = 0x001f4100,  // zero + UID
  STR43 = 0x001f4300,  // multi string stored ?
  STR44 = 0x001f4400,  // multi strings stored ?
  STR46 = 0x001f4600,  // Str64 x 5;
  BOOL2 = 0xff000400,  // Another bool stored as int32 ?
  FLOAT8 =
      0xff000800, // 0x55f9 patient weight / 55f8 Patient height * 100 (in cm)
  USAN20 = 0xff002000, // USAN string ???
  UNK21 = 0xff002100,  // 3a5e ??
  UINT16 = 0xff002200, // 1bc3 contains a 64x64x 16bits icon image (most likely
                       // either bytes or ushort)
  CHARACTER_SET = 0xff002300, // 17f2 seems to store the character set used / to
                              // use ? Stored as UTF-16 ?
  INT32 = 0xff002400,         //
  UNK25 = 0xff002500,         //
  FLOAT28 = 0xff002800,       // float single precision. afea ?? VM1_n
  DOUBLE = 0xff002900,        // 0x13ec is Imaging Frequency
  BOOL = 0xff002a00,          // BOOL stored as INT32 ?
  STRING = 0xff002c00,        // SHIFT-JIS string
  UNK31 = 0xff003100,         //
  UNK32 = 0xff003200,         //
  UNKF2 = 0xfff00200,         //
};

static void print_float(const float *buffer, int len) {
  const int m = sizeof(float);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
#if 0
      const float cur = buffer[i];
#else
    float cur = -1;
    memcpy(&cur, buffer + i, sizeof cur);
#endif
    assert(isfinite(cur) && !isnan(cur));
    printf("%f", cur);
  }
  printf("] #%d", len);
}
static void print_double(const double *buffer, int len) {
  const int m = sizeof(double);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const double cur = buffer[i];
    assert(isfinite(cur) && !isnan(cur));
    printf("%g", cur);
  }
  printf("] #%d", len);
}
static void print_uint8(const uint8_t *buffer, int len) {
  const int m = sizeof(uint8_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const uint16_t cur = buffer[i];
    printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_uint16(const uint16_t *buffer, int len) {
  const int m = sizeof(uint16_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const uint16_t cur = buffer[i];
    printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_int16(const int16_t *buffer, int len) {
  const int m = sizeof(int16_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const int16_t cur = buffer[i];
    printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_int32(const int32_t *buffer, int len) {
  const int m = sizeof(int32_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const int32_t cur = buffer[i];
    printf("%d", cur);
  }
  printf("] #%d", len);
}
static void print_int64(const int64_t *buffer, int len) {
  const int m = sizeof(int64_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const int64_t cur = buffer[i];
    printf("%lld", cur);
  }
  printf("] #%d", len);
}
static void print_uint64(const uint64_t *buffer, int len) {
  const int m = sizeof(uint64_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const uint64_t cur = buffer[i];
    printf("%llu", cur);
  }
  printf("] #%d", len);
}
static void print_uint32(const uint32_t *buffer, int len) {
  const int m = sizeof(uint32_t);
  assert(len % m == 0);
  int i;
  printf(" [");
  for (i = 0; i < len / m; i++) {
    if (i)
      printf(",");
    const uint32_t cur = buffer[i];
    printf("%u", cur);
  }
  printf("] #%d", len);
}
static void print_hex(const unsigned char *buffer, int len) {
  int i;
  printf(" [");
  for (i = 0; i < len; i++) {
    const unsigned char cur = buffer[i];
    if (i)
      printf("\\");
    printf("%02x", cur);
  }
  printf("] #%d", len);
}
static void print_usan(const char *buffer, int len) {
  const char sig[] = {0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00};
  int b = memcmp(buffer, sig, sizeof sig);
  assert(b == 0);
  printf(" [<?USAN:");
  if (len == 48) {
    // USAN5E
    //  00000000  55 53 41 4e 00 50 03 00  00 00 00 00 00 00 00 00
    //  |USAN.P..........| 00000010  00 00 00 00 00 00 00 00  00 00 00 00 00 00
    //  59 40  |..............Y@| 00000020  00 00 00 00 00 00 00 00  00 00 00 00
    //  00 00 00 00  |................|
    // print_uint32( buffer + 8, len - 8 );
    // print_float( buffer + 8, len - 8 );
    unsigned char sig5e[] = {0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00,
                             0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    b = memcmp(buffer, sig5e, sizeof sig5e);
    assert(b == 0);

    print_double(buffer + sizeof sig5e, len - sizeof sig5e);

  } else if (len == 60) {
    // USAN5F
    unsigned char sig5f[] = {0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00,
                             0x01, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 0x00};
    b = memcmp(buffer, sig5f, sizeof sig5f);
    assert(b == 0);
    // 00000000  55 53 41 4e 00 50 03 00  01 00 00 00 28 00 00 00
    // |USAN.P......(...| 00000010  4e 4c 54 4c 61 8e 1e bf  b7 09 4c 40 aa 62
    // 2a fd  |NLTLa.....L@.b*.| 00000020  84 f3 44 40 00 00 00 00  00 00 00 00
    // 00 00 00 00  |..D@............| 00000030  00 00 00 00 00 00 00 00  00 00
    // 00 00              |............|

    print_double(buffer + sizeof sig5f + 4 /* NLTL */, len - sizeof sig5f - 4);

  } else if (len == 68) {
    // USAN20
    unsigned char sig20[] = {0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00,
                             0x01, 0x00, 0x00, 0x00, 0x4e, 0x4b, 0x4e, 0x55};
    b = memcmp(buffer, sig20, sizeof sig20);
    assert(b == 0);
    //  00000000  55 53 41 4e 00 50 03 00  01 00 00 00 4e 4b 4e 55
    //  |USAN.P......NKNU| 00000010  00 00 00 00 00 00 00 00  02 00 00 00 00 00
    //  00 00  |................| 00000020  00 00 00 00 00 00 00 00  00 00 00 00
    //  00 00 00 00  |................| 00000030  00 00 00 00 01 00 00 00  00 00
    //  59 40 00 00 00 00  |..........Y@....| 00000040  00 00 00 00 |....|
    // print_double( buffer + sizeof sig20 + 4 , len - sizeof sig20 - 4 );
    // print_float( buffer + sizeof sig20 + 4 , len - sizeof sig20 - 4 );
    // print_uint32( buffer + sizeof sig20 + 4 , len - sizeof sig20 - 4 );
    // print_int64( buffer + sizeof sig20 + 4 , len - sizeof sig20 - 4 );
    print_int32(buffer + sizeof sig20 + 4, len - sizeof sig20 - 4);
  }

  printf("FIXME?>] #%d", len);
}

static void print_string(const char *buffer, int len) {
  printf(" [%.*s] #%d (%d)", len, buffer, len, strnlen(buffer, len));
}
static void print_wstring(const char *buffer, int len) {
  static const char magic[] = {0xdf, 0xff, 0x79};
  int b = memcmp(buffer, magic, sizeof(magic));
  if (b == 0) {
    /*
    $ hexdump -C out0000
    00000000  df ff 79 17 01 09 00 49  53 4f 38 38 35 39 2d 31
    |..y....ISO8859-1| 00000010  02 08 00 30 30 30 30 30  30 30 30 |...00000000|
    0000001b
    */
    assert(buffer[4] == 0x1);
    assert(buffer[5] == 0x9);
    assert(buffer[6] == 0x0);
    assert(buffer[16] == 0x2);
    assert(buffer[18] == 0x0);
    const char *iso = (char *)buffer + 7;
    const int diff = len - (7 + 9 + 3);
    const char *next = buffer + 7 + 9 + 3;
    int len2 = (unsigned char)buffer[3];
    int len3 = (unsigned char)buffer[5];
    int len4 = (unsigned char)buffer[17];
    assert(len2 + 4 == len);
    assert(len3 == 9);
    assert(len4 == diff);
    assert(strncmp(iso, "ISO8859-1", 9) == 0);
    printf(" [%.*s : %.*s] #%d", len3, iso, len4, next, len);
  } else {
    printf(" [%.*s] #%d (%d)", len, buffer, len, strnlen(buffer, len));
  }
}

// DICOM reference oftne a string with 16/64 bytes (store the trailing 0)
typedef char str16[16 + 1];
typedef char str64[64 + 1];

// [0,TM_MR_DCM_V1.0,Abcdefg Hijklm   ,MRI LOWER EXTREMITY JOINT WITHOUT
// CONTRAST - KNEE,1234567,MR,1] [0,TM_MR_DCM_V2.0,,,TM_MR_DCM_V2.0,MR,3]
typedef struct info43 {
  uint32_t zero;
  char iver[0x45]; // implementation version
  char buf3[0x100];
  str64 buf4;
  str16 buf5;
  char modality[0x15];
  uint32_t val;
} info43;
static void print_string43(const char *buffer, int len) {
  assert(len == 436);
  info43 i;
  // printf( "debug: %d\n", sizeof i );
  // printf( "debug: 0x%x\n", offsetof(info43, buf2) );
  // printf( "debug: 0x%x\n", offsetof(info43, buf3) );
  // printf( "debug: 0x%x\n", offsetof(info43, buf4) );
  // printf( "debug: 0x%x\n", offsetof(info43, buf5) );
  // printf( "debug: 0x%x\n", offsetof(info43, buf6) );
  assert(sizeof i == 436);
  assert(offsetof(info43, iver) == 0x4);
  assert(offsetof(info43, buf3) == 0x49);
  assert(offsetof(info43, buf4) == 0x149);
  assert(offsetof(info43, buf5) == 0x18A);
  assert(offsetof(info43, modality) == 0x19b);
  memcpy(&i, buffer, sizeof i);
  printf(" [");
  printf("%u", i.zero);
  static const char vers1[] = "TM_MR_DCM_V1.0";
  static const char vers2[] = "TM_MR_DCM_V2.0";
  static const char vers3[] = "TM_MR_DCM_V1.0_3";
  static const char vers4[] = "TM_MR1_DCM_V1.0";
  assert(strcmp(i.iver, vers1) == 0 || strcmp(i.iver, vers2) == 0 ||
         strcmp(i.iver, vers3) == 0 || strcmp(i.iver, vers4) == 0);
  assert(strcmp(i.modality, "MR") == 0);
  printf(",%s,%s,%s,%s,%s,", i.iver, i.buf3, i.buf4, i.buf5, i.modality);
  assert(i.val == 1 || i.val == 3);
  printf("%u", i.val);
  printf("] #%d", len);
  // remaining stuff should all be 0
}

// [12345678,Abcdefg Hijklm
// ,12345678,1.2.840.113745.100000.1000000.10000.1000.10000000,MRI LOWER
// EXTREMITY JOINT WITHOUT CONTRAST - HIP,0,1,0,1,0,1]
typedef struct info {
  str64 zero; // all zero
  char buf2[0x15];
  char buf3[0x100];
  str16 buf4;
  str64 buf5;
  str64 buf6;
  uint32_t bools[6];
} info;
static void print_string44(const char *buffer, int len) {
  assert(len == 516);
  info i;
  assert(sizeof i == 516);
  // printf( "debug: %d\n", sizeof i );
  // printf( "debug: 0x%x\n", offsetof(info, buf2) );
  // printf( "debug: 0x%x\n", offsetof(info, buf3) );
  // printf( "debug: 0x%x\n", offsetof(info, buf4) );
  // printf( "debug: 0x%x\n", offsetof(info, buf5) );
  // printf( "debug: 0x%x\n", offsetof(info, buf6) );
  // printf( "debug: 0x%x\n", offsetof(info, buf7) );
  assert(offsetof(info, buf2) == 0x41);
  assert(offsetof(info, buf3) == 0x56);
  assert(offsetof(info, buf4) == 0x156);
  assert(offsetof(info, buf5) == 0x167);
  assert(offsetof(info, buf6) == 0x1A8);
  assert(offsetof(info, bools) == 0x1EC);
  memcpy(&i, buffer, sizeof i);
  int c;
  printf(" [");
  printf("%s,%s,%s,%s,%s", i.buf2, i.buf3, i.buf4, i.buf5, i.buf6);
  for (c = 0; c < 6; ++c) {
    assert(i.bools[c] == c % 2);
    printf(",");
    printf("%d", i.bools[c]);
  }
  printf("] #%d", len);
  // remaining stuff should all be 0
}

typedef struct uid41 {
  uint32_t zero;
  str64 uid1; // Detached Study Management SOP Class (1.2.840.10008.3.1.2.3.1) ?
  str64 uid2; // 1.2.840.113745.101000.1098000.X.Y.Z
  uint16_t zero2;
} uid41;
static void print_uid41(const char *buffer, int len) {
  assert(len == 136);
  uid41 i;
  assert(sizeof i == 136);
  memcpy(&i, buffer, sizeof i);
  assert(i.zero == 0);
  assert(strnlen(i.uid1, sizeof i.uid1) <= 64);
  assert(strnlen(i.uid2, sizeof i.uid2) <= 64);
  assert(i.zero2 == 0);
  printf(" [%u,%.*s,%.*s] #%d", i.zero, sizeof i.uid1, i.uid1, sizeof i.uid2,
         i.uid2, len);
}
typedef struct str40 {
  uint32_t zero;
  char str[7][0x30];
} str40;
static void print_string40(const char *buffer, int len) {
  assert(len % 340 == 0);
  printf(" [");
  int j;
  for (j = 0; j < len / 340; ++j) {
    str40 a;
    assert(sizeof a == 340);
    memcpy(&a, buffer + j * 340, sizeof a);
    if (j)
      printf(",");
    printf("%u:{", a.zero);
    int i;
    for (i = 0; i < 7; ++i) {
      if (i)
        printf(",");
      printf("%s", a.str[i]);
    }
    printf("}");
  }
  printf("] #%d", len);
}

static void print_string46(const char *buffer, int len) {
  assert(len == 325); // 65 * 5
  str64 array5[5];
  assert(sizeof array5 == 325);
  memcpy(array5, buffer, sizeof array5);
  int i;
  printf(" [");
  for (i = 0; i < 5; ++i) {
    if (i)
      printf(",");
    printf("%.*s", sizeof array5[i], array5[i]);
  }
  printf("] #%d", len);
}

static void print_stringbc3(const char *buffer, int len) {
  assert(len == 100);
  uint32_t n;
  int i;
  memcpy(&n, buffer, sizeof n);
  assert(n < 6);
  printf(" [");
  for (i = 0; i < n; ++i) {
    if (i)
      printf(",");
    const char *str = (buffer + 4) + i * 8;
    assert(str[3] == 0x0);
    assert(str[4] == 0x41 || str[4] == 0x43 /* C */ || str[4] == 0x45 /* E */);
    const int c = str[5];
    assert(c <= 10 && c >= 0);
    printf("%.*s#%c%d", 3, str, str[4], c);
  }
  printf("] #%d", len);
  // remaining stuff should all be 0
}

static void print_stringC1(const char *buffer, int len) {
  assert(len % 6 == 0);
  const int n = len / 6;
  int i;
  printf(" [");
  for (i = 0; i < n; ++i) {
    if (i)
      printf(",");
    const char *str = buffer + i * 6;
    assert(str[3] == 0x0);
    assert(str[5] == 0x41 || str[5] == 0x43 /* C */ || str[5] == 0x45 /* E */);
    const int c = str[4];
    assert(c <= 5 && c >= 0);
    printf("%.*s#%d%c", 3, str, c, str[5]);
  }
  printf("] #%d", len);
}

static const unsigned char usan[] = {
    0x55, 0x53, 0x41, 0x4e, 0x00, 0x50, 0x03, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x4e, 0x4b, 0x4e, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static void print(uint32_t type, char *buffer, int len) {
  switch (type) {
  case UNK1:
    assert(len == 4);
    // print_float( (float*)buffer, len);
    print_uint32((uint32_t *)buffer, len);
    // print_uint16( (uint16_t*)buffer, len);
    // print_hex( buffer, len);
    break;
  case STR40:
    assert(len == 1020 || len == 340);
    print_string40(buffer, len);
    break;
  case UID41:
    assert(len == 136);
    print_uid41(buffer, len);
    break;
  case UNK2:
    assert(len == 36);
    // print_float( (float*)buffer, len);
    print_int32((uint32_t *)buffer, len);
    // print_uint16( (uint16_t*)buffer, len);
    // print_hex( buffer, len);
    break;
  case UNKB9:
    assert(len == 24);
    // only zero ?
    print_uint64((uint16_t *)buffer, len);
    break;
  case UNKB8:
    assert(len == 36);
    {
      unsigned char out0000[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00, 0xcd,
                                 0xcc, 0x4c, 0x3f, // 0.8 in float
                                 0xcd, 0xcc, 0x4c, 0x3f, 0x00, 0x00, 0x00,
                                 0x00, 0x00, 0x00, 0x00, 0x00};
    }
    // dump2file(buffer, len);
    print_int16((uint32_t *)buffer, len);
    // print_hex( buffer, len);
    break;
  case UNK72:
    assert(len == 12);
    {
      unsigned char out0000[] = {0x01, 0x00, 0x00, 0x00, 0x22, 0x00,
                                 0xff, 0x00, 0x40, 0x00, 0x00, 0x00};
    }
    // dump2file(buffer, len);
    // print_int32( (uint32_t*)buffer, len);
    print_uint16((uint32_t *)buffer, len);
    // print_float( (uint32_t*)buffer, len);
    // print_hex( buffer, len);
    break;
  case STR43:
    assert(len == 436);
    print_string43(buffer, len);
    break;
  case CHARACTER_SET:
    dump2file(buffer, len);
    printf(" [?CHARACTER_SET?] #%d", len);
    break;
  case STR44:
    assert(len == 516);
    print_string44(buffer, len);
    break;
  case STRC3:
    assert(len == 100);
    print_stringbc3(buffer, len);
    break;
  case UNKBB:
    assert(len == 68);
    // dump2file(buffer, len);
    // print_uint32( (uint32_t*)buffer, len);
    //      print_int16( (uint32_t*)buffer, len);
    print_uint8((uint32_t *)buffer, len);
    //      print_uint16( (uint32_t*)buffer, len);
    // print_uint64( (uint32_t*)buffer+4, len - 4);
    // print_float( (uint32_t*)buffer, len);
    // print_hex( buffer, len);
    break;
  case UNK70:
    assert(len == 24); // vect of bool
    print_uint32((uint32_t *)buffer, len);
    break;
  case STR46:
    assert(len == 325);
    print_string46(buffer, len);
    break;
  case UNKF2:
    assert(len % 4 == 0);
    print_int32((int32_t *)buffer, len);
    break;
  case VECT2FLOAT:
    assert(len == 8 || len == 40);
    print_float((float *)buffer, len);
    break;
  case VECT3FLOAT:
    assert(len % 12 == 0); // 12 or 36
    print_float((float *)buffer, len);
    break;
  case UNK4:
    assert(len % 4 == 0); // int32 ?
    print_int32((int32_t *)buffer, len);
    break;
  case UNKB:
    assert(len == 12); // int32 x 3 ?
    print_int32((int32_t *)buffer, len);
    break;
  case UNKF:
    assert(len == 156); //
    print_int32(buffer, len);
    break;
  case DATETIME:
    assert(len == 19 || len == 20);
    print_string(buffer, len);
    break;
  case FLOAT8:
    assert(len == 4);
    print_float((float *)buffer, len);
    break;
  case USAN20:
  case USAN5E:
  case USAN5F:
    assert(len == 48 || len == 60 || len == 68);
    print_usan(buffer, len);
    break;
  case UNK21:
    assert(len == 20 || len == 16 || len == 24 || len == 28 || len == 88);
    print_int32((int32_t *)buffer, len);
    break;
  case UINT16:
    print_uint16((uint16_t *)buffer, len);
    // print_hex( buffer, len);
    break;
  case INT32:
    assert(len % 4 == 0);
    print_int32((int32_t *)buffer, len);
    break;
  case UNK25:
    assert(len == 4 || len == 512);
    print_uint32((uint32_t *)buffer, len);
    break;
  case UNKBA:
    assert(len == 8); // pair of uint32 ?
    print_uint32((uint32_t *)buffer, len);
    break;
  case UNK31:
    assert(len == 8 || len == 16);
    print_uint64((uint64_t *)buffer, len);
    break;
  case UNK32:
    assert(len % 4 == 0); // all uint are lower than uin16_max
    print_uint32((uint32_t *)buffer, len);
    break;
  case BOOL2:
  case BOOL:
    assert(len == 4); // bool ?
    print_uint32((uint32_t *)buffer, len);
    break;
  case DOUBLE:
    assert(len == 8);
    print_double((double *)buffer, len);
    break;
  case UNKD0:
    print_uint32((uint32_t *)buffer, len);
    // print_uint64( (uint64_t*)buffer, len);
    // print_hex( buffer, len);
    break;
  case FLOAT28:
    assert(len % 4 == 0);
    print_float((float *)buffer, len);
    break;
  case WSTRING:
    print_wstring(buffer, len);
    break;
  case STRC1:
    print_stringC1(buffer, len);
    break;
  case UNKC2:
    assert(len % 11 == 0 && len % 6 == 0); // 264    330    396    462    528 66
    print_uint16((uint16_t *)buffer, len);
    break;
  case STRING:
    // if( len % 2 == 0 ) assert( buffer[len-1] == 0 );
    // b3d5 does not seems to contains a trailing NULL
    print_string(buffer, len);
    dump2file(buffer, len);
    break;
  default:
    assert(0);
    print_hex(buffer, len);
  }
}

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  check_mec_mr3_dict();
  const char *filename = argv[1];
  FILE *in = fopen(filename, "rb");
  int i;
  uint8_t r = 0;
  char buffer[5000 * 2];
  long sz;
  size_t nread;

  fseek(in, 0L, SEEK_END);
  sz = ftell(in);
  fseek(in, 0L, SEEK_SET);
  printf("Size: %ld\n", sz);

  I si;
  assert(sizeof(si) == 32);
  assert(sizeof(si.separator) == 20);
  assert(sizeof(magic2) == 20);

  /* TODO what to do with hypo tag + type because of:
    #k1: 0x000017e3 #k2: 0xff002400
    #k1: 0x000017e3 #k2: 0xff002a00
    */

  int remain = 0;
  bool last_element = false;
  while (--remain != 0) {
    uint32_t nitems;
    fread(&nitems, 1, sizeof nitems, in);
    if (nitems <= 3) {
      assert(!last_element);
      // special case to handle last element ?
      //      printf("<#last element coming: %08x>\n", nitems);
      assert(nitems > 0);
      remain = nitems;
      fread(&nitems, 1, sizeof nitems, in);
      last_element = true;
    }
    ++r;
    //    printf("Group %d #Items: %u\n", r, nitems);
    for (i = 0; i < nitems; ++i) {
      // long pos = ftell(in);
      // printf("Offset 0x%x \n", pos );
      fread(&si, 1, sizeof si, in);
      /*
       * #tag2: 0x0000
       * #tag2: 0x0007
       * #tag2: 0x000b
       * #tag2: 0x0017
       * #tag2: 0x001b
       * #tag2: 0x001f
       * #tag2: 0xff00
       * #tag2: 0xfff0
       */
      bool found = check_mec_mr3_info(r, si.key, si.type);
      assert(found);
      const uint32_t sign = si.type >> 24;
      const char symb = sign ? '_' : ' ';
      printf("(%01x,%05x) %c%04x", r, si.key, symb,
             (si.type & 0x00ffff00) >> 8);
      assert(si.len <= 9509 /* 9216 */ /*9184*/ /* 8192 */);
      // printf("  #k1: 0x%08lx #k2: 0x%08x", si.k1, si.k2 );
      //      printf("  #Pos: %7ld 0x%08lx #Len:%08u 0x%08x\n", pos, pos,
      //      si.len, si.len );
      int b = memcmp(si.separator, magic2, sizeof(magic2));
      assert(b == 0);
      assert(si.len <= sizeof buffer);
      nread = fread(buffer, 1, si.len, in);
      assert(nread == (size_t)si.len);
      print(si.type, buffer, nread);
      //      if( si.k2 == 0xff002c00 )
      // printf("  buffer: [%.*s]", nread,  buffer );
      printf("\n");
    }
  }
  // printf("ngroups = %d\n", r);
  assert(r >= 6 && r <= 8);

  long pos = ftell(in);
  // printf("pos: 0x%08x 0x%08x\n", pos, sz);
  assert(pos == sz || pos + 1 == sz);
  // int ret = feof(in);
  // printf("feof: %d\n", ret);
  fclose(in);
  return 0;
}
