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
    assert(len == 24);
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

typedef struct D {
  uint32_t group;
  uint32_t key;
  uint32_t type;
} D;

static const D dict[] = {
    {0x00000001, 0x00000001, 0xfff00200}, //
    {0x00000001, 0x0000006d, 0xff002400}, //
    {0x00000001, 0x00001004, 0xff002400}, //
    {0x00000001, 0x00001005, 0xff002400}, //
    {0x00000001, 0x000013ec, 0xff002900}, //
    {0x00000001, 0x00004e23, 0xff002c00}, //
    {0x00000001, 0x000055f0, 0x0007d000}, //
    {0x00000001, 0x000055f1, 0x00000300}, //
    {0x00000001, 0x000055f2, 0x00000300}, //
    {0x00000001, 0x000055f3, 0x00000300}, //
    {0x00000001, 0x000055f6, 0x00000200}, //
    {0x00000001, 0x000055f7, 0xff000400}, //
    {0x00000001, 0x000055f8, 0xff000800}, //
    {0x00000001, 0x000055f9, 0xff000800}, //
    {0x00000001, 0x000055fa, 0x00000300}, //
    {0x00000001, 0x000055fb, 0x00000300}, //
    {0x00000001, 0x000055fc, 0x00000300}, //
    {0x00000001, 0x000055fd, 0x00000300}, //
    {0x00000001, 0x000055fe, 0x00000300}, //
    {0x00000001, 0x000055ff, 0x00000100}, //
    {0x00000001, 0x00005601, 0x00000300}, //
    {0x00000001, 0x00005604, 0xff000400}, //
    {0x00000001, 0x00005606, 0x00000300}, //
    {0x00000001, 0x0000560a, 0xff002c00}, //
    {0x00000001, 0x0000560b, 0xff002c00}, //
    {0x00000001, 0x0000560c, 0xff002c00}, //
    {0x00000001, 0x0000560d, 0xff002c00}, //
    {0x00000001, 0x00005610, 0xff002c00}, //
    {0x00000001, 0x00005611, 0xff002c00}, //
    {0x00000001, 0x00005612, 0xff002c00}, //
    {0x00000001, 0x00005613, 0xff002c00}, //
    {0x00000001, 0x00005614, 0xff002c00}, //
    {0x00000001, 0x00005616, 0xff002c00}, //
    {0x00000001, 0x00005618, 0xff002c00}, //
    {0x00000001, 0x00005619, 0xff002c00}, //
    {0x00000001, 0x0000561a, 0x00000e00}, //
    {0x00000001, 0x000059d8, 0xff002400}, //
    {0x00000001, 0x00006d61, 0xff002c00}, //
    {0x00000001, 0x00006d62, 0xff002c00}, //
    {0x00000001, 0x00006d63, 0xff002c00}, //
    {0x00000001, 0x00006d64, 0xff002c00}, //
    {0x00000001, 0x00006d65, 0xff002c00}, //
    {0x00000001, 0x00006d66, 0xff002c00}, //
    {0x00000001, 0x00006d67, 0xff002c00}, //
    {0x00000001, 0x00006d68, 0xff002c00}, //
    {0x00000001, 0x00006d69, 0xff002c00}, //
    {0x00000001, 0x00006d71, 0xff002c00}, //
    {0x00000001, 0x00006d72, 0xff002c00}, //
    {0x00000001, 0x00006d73, 0xff002c00}, //
    {0x00000001, 0x00006d74, 0xff002c00}, //
    {0x00000001, 0x00006d75, 0xff002c00}, //
    {0x00000001, 0x00006d76, 0xff002c00}, //
    {0x00000001, 0x00006d77, 0xff002c00}, //
    {0x00000001, 0x00006d78, 0xff002c00}, //
    {0x00000001, 0x00006d79, 0xff002c00}, //
    {0x00000001, 0x00006d80, 0x001f4400}, //
    {0x00000001, 0x00006d81, 0x001f4000}, //
    {0x00000001, 0x00006d82, 0x001f4100}, //
    {0x00000001, 0x00006d83, 0x001f4300}, //
    {0x00000001, 0x00006d84, 0x001f4000}, //
    {0x00000001, 0x00006d87, 0xff002400}, //
    {0x00000001, 0x00006d8a, 0x001f4600}, //
    {0x00000001, 0x00006d8b, 0xff002400}, //
    {0x00000001, 0x0000ac09, 0xff002400}, //
    {0x00000002, 0x00000002, 0xfff00200}, //
    {0x00000002, 0x00000007, 0xfff00200}, //
    {0x00000002, 0x0000000b, 0xfff00200}, //
    {0x00000002, 0x0000006e, 0xff002400}, //
    {0x00000002, 0x00001006, 0xff002800}, //
    {0x00000002, 0x00001007, 0xff002800}, //
    {0x00000002, 0x000017d4, 0xff002a00}, //
    {0x00000002, 0x000017d5, 0xff002a00}, //
    {0x00000002, 0x000017d6, 0xff002a00}, //
    {0x00000002, 0x000017d7, 0xff002a00}, //
    {0x00000002, 0x000017d8, 0xff002a00}, //
    {0x00000002, 0x000017d9, 0xff002a00}, //
    {0x00000002, 0x000017da, 0xff002a00}, //
    {0x00000002, 0x000017db, 0xff002a00}, //
    {0x00000002, 0x000017dc, 0xff002a00}, //
    {0x00000002, 0x000017dd, 0xff002a00}, //
    {0x00000002, 0x000017de, 0xff002400}, //
    {0x00000002, 0x000017df, 0x00177000}, //
    {0x00000002, 0x000017e0, 0x00177000}, //
    {0x00000002, 0x000017e1, 0x00177000}, //
    {0x00000002, 0x000017e2, 0xff002a00}, //
    {0x00000002, 0x000017e3, 0xff002a00}, //
    {0x00000002, 0x000017e4, 0x00177200}, //
    {0x00000002, 0x000017e5, 0xff002400}, //
    {0x00000002, 0x000017e6, 0xff002400}, //
    {0x00000002, 0x000017e7, 0xff002400}, //
    {0x00000002, 0x000017e8, 0xff002400}, //
    {0x00000002, 0x000017e9, 0xff002400}, //
    {0x00000002, 0x000017ea, 0xff002400}, //
    {0x00000002, 0x000017eb, 0xff002400}, //
    {0x00000002, 0x000017ec, 0xff002400}, //
    {0x00000002, 0x000017ed, 0xff002400}, //
    {0x00000002, 0x000017ee, 0xff002400}, //
    {0x00000002, 0x000017f0, 0xff002400}, //
    {0x00000002, 0x000017f1, 0xff002a00}, //
    {0x00000002, 0x000017f2, 0xff002300}, //
    {0x00000002, 0x000017f4, 0xff002400}, //
    {0x00000002, 0x000017f5, 0xff002400}, //
    {0x00000002, 0x000017f6, 0xff002400}, //
    {0x00000002, 0x000017f7, 0xff003200}, //
    {0x00000002, 0x000017f8, 0xff002800}, //
    {0x00000002, 0x000017f9, 0xff002800}, //
    {0x00000002, 0x000017fa, 0xff003200}, //
    {0x00000002, 0x000017fc, 0xff003200}, //
    {0x00000002, 0x000017fd, 0xff002800}, //
    {0x00000002, 0x000017fe, 0xff002800}, //
    {0x00000002, 0x000017ff, 0xff002800}, //
    {0x00000002, 0x00001800, 0xff002800}, //
    {0x00000002, 0x00001816, 0xff002400}, //
    {0x00000002, 0x00001817, 0xff002400}, //
    {0x00000002, 0x00001818, 0xff002400}, //
    {0x00000002, 0x00001838, 0xff002400}, //
    {0x00000002, 0x00001839, 0xff002400}, //
    {0x00000002, 0x0000183a, 0xff002800}, //
    {0x00000002, 0x0000183b, 0xff002400}, //
    {0x00000002, 0x0000183c, 0xff002800}, //
    {0x00000002, 0x0000183d, 0xff002800}, //
    {0x00000002, 0x0000183e, 0xff002800}, //
    {0x00000002, 0x0000183f, 0xff002800}, //
    {0x00000002, 0x00001840, 0xff002800}, //
    {0x00000002, 0x00001841, 0xff002800}, //
    {0x00000002, 0x00001842, 0xff002800}, //
    {0x00000002, 0x00001843, 0xff002a00}, //
    {0x00000002, 0x00001844, 0xff002400}, //
    {0x00000002, 0x00001845, 0xff002400}, //
    {0x00000002, 0x00001848, 0xff002a00}, //
    {0x00000002, 0x00001849, 0xff002a00}, //
    {0x00000002, 0x0000184a, 0xff002800}, //
    {0x00000002, 0x0000185f, 0xff002400}, //
    {0x00000002, 0x00001860, 0xff002400}, //
    {0x00000002, 0x00001861, 0xff003200}, //
    {0x00000002, 0x00001862, 0xff002400}, //
    {0x00000002, 0x00001863, 0xff002400}, //
    {0x00000002, 0x00001864, 0xff002400}, //
    {0x00000002, 0x00001865, 0xff002400}, //
    {0x00000002, 0x00001866, 0xff002400}, //
    {0x00000002, 0x00001867, 0xff002400}, //
    {0x00000002, 0x00005600, 0x00000300}, //
    {0x00000002, 0x00005615, 0xff002c00}, //
    {0x00000002, 0x00009c41, 0xff002c00}, //
    {0x00000002, 0x0000a7f8, 0xff002c00}, //
    {0x00000002, 0x0000a7f9, 0xff002c00}, //
    {0x00000002, 0x0000a7fb, 0xff002400}, //
    {0x00000002, 0x0000a7fd, 0xff003200}, //
    {0x00000002, 0x0000a7fe, 0xff002400}, //
    {0x00000002, 0x0000a7ff, 0xff002800}, //
    {0x00000002, 0x0000a800, 0xff002400}, //
    {0x00000002, 0x0000a801, 0xff002800}, //
    {0x00000002, 0x0000a802, 0xff002800}, //
    {0x00000002, 0x0000a803, 0xff002400}, //
    {0x00000002, 0x0000a804, 0xff002400}, //
    {0x00000002, 0x0000a805, 0xff003200}, //
    {0x00000002, 0x0000a806, 0x00000500}, //
    {0x00000002, 0x0000a807, 0x00000500}, //
    {0x00000002, 0x0000a808, 0xff002400}, //
    {0x00000002, 0x0000a809, 0xff003200}, //
    {0x00000002, 0x0000a80a, 0x000bb800}, //
    {0x00000002, 0x0000a80b, 0x000bb900}, //
    {0x00000002, 0x0000a80c, 0xff002a00}, //
    {0x00000002, 0x0000a80d, 0xff002800}, //
    {0x00000002, 0x0000a80e, 0xff002800}, //
    {0x00000002, 0x0000a80f, 0xff002400}, //
    {0x00000002, 0x0000a810, 0xff002400}, //
    {0x00000002, 0x0000a813, 0xff002400}, //
    {0x00000002, 0x0000a814, 0xff002800}, //
    {0x00000002, 0x0000a815, 0xff002400}, //
    {0x00000002, 0x0000a816, 0xff002400}, //
    {0x00000002, 0x0000a817, 0xff002400}, //
    {0x00000002, 0x0000a819, 0xff002400}, //
    {0x00000002, 0x0000a81b, 0xff002400}, //
    {0x00000002, 0x0000a81c, 0xff003200}, //
    {0x00000002, 0x0000a81d, 0xff002a00}, //
    {0x00000002, 0x0000a822, 0x00000500}, //
    {0x00000002, 0x0000a823, 0xff002c00}, //
    {0x00000002, 0x0000a824, 0xff002400}, //
    {0x00000002, 0x0000a825, 0xff002400}, //
    {0x00000002, 0x0000a826, 0xff003200}, //
    {0x00000002, 0x0000a827, 0xff002400}, //
    {0x00000002, 0x0000a828, 0xff002400}, //
    {0x00000002, 0x0000a82c, 0xff002400}, //
    {0x00000002, 0x0000a82d, 0x000bbb00}, //
    {0x00000002, 0x0000a82e, 0xff002400}, //
    {0x00000002, 0x0000a82f, 0xff002a00}, //
    {0x00000002, 0x0000a830, 0xff002800}, //
    {0x00000002, 0x0000a832, 0xff002a00}, //
    {0x00000002, 0x0000a834, 0xff002800}, //
    {0x00000002, 0x0000a835, 0xff003200}, //
    {0x00000002, 0x0000a836, 0xff002400}, //
    {0x00000002, 0x0000a837, 0xff002400}, //
    {0x00000002, 0x0000a838, 0xff002400}, //
    {0x00000002, 0x0000a839, 0xff002400}, //
    {0x00000002, 0x0000a83a, 0xff002400}, //
    {0x00000002, 0x0000a83b, 0xff002400}, //
    {0x00000002, 0x0000a83c, 0xff002400}, //
    {0x00000002, 0x0000a83d, 0xff002400}, //
    {0x00000002, 0x0000a844, 0xff002c00}, //
    {0x00000002, 0x0000a846, 0xff002400}, //
    {0x00000002, 0x0000a847, 0xff002400}, //
    {0x00000002, 0x0000a848, 0xff002800}, //
    {0x00000002, 0x0000a849, 0xff002800}, //
    {0x00000002, 0x0000a84a, 0xff002a00}, //
    {0x00000002, 0x0000a84b, 0xff002400}, //
    {0x00000002, 0x0000a84c, 0xff002400}, //
    {0x00000002, 0x0000a84d, 0xff002400}, //
    {0x00000002, 0x0000a84e, 0xff002400}, //
    {0x00000002, 0x0000a84f, 0xff002800}, //
    {0x00000002, 0x0000a850, 0xff002800}, //
    {0x00000002, 0x0000a851, 0xff002800}, //
    {0x00000002, 0x0000a852, 0xff002400}, //
    {0x00000002, 0x0000a853, 0xff002400}, //
    {0x00000002, 0x0000a854, 0xff002400}, //
    {0x00000002, 0x0000a85d, 0xff002a00}, //
    {0x00000002, 0x0000a85e, 0xff003200}, //
    {0x00000002, 0x0000a864, 0xff002400}, //
    {0x00000002, 0x0000a865, 0xff002400}, //
    {0x00000002, 0x0000a866, 0xff002800}, //
    {0x00000002, 0x0000a867, 0xff002800}, //
    {0x00000002, 0x0000a868, 0xff002800}, //
    {0x00000002, 0x0000a869, 0xff002400}, //
    {0x00000002, 0x0000a86a, 0xff002400}, //
    {0x00000002, 0x0000a86b, 0xff002400}, //
    {0x00000002, 0x0000a86c, 0xff002400}, //
    {0x00000002, 0x0000a86d, 0xff002400}, //
    {0x00000002, 0x0000a86e, 0xff002400}, //
    {0x00000002, 0x0000a86f, 0xff002800}, //
    {0x00000002, 0x0000a870, 0xff002400}, //
    {0x00000002, 0x0000a8c1, 0xff002c00}, //
    {0x00000002, 0x0000a8c2, 0xff002a00}, //
    {0x00000002, 0x0000a8c3, 0xff003200}, //
    {0x00000002, 0x0000a8c4, 0xff002400}, //
    {0x00000002, 0x0000a8c5, 0xff002400}, //
    {0x00000002, 0x0000a8c6, 0xff002400}, //
    {0x00000002, 0x0000a8c7, 0x000bba00}, //
    {0x00000002, 0x0000a8c8, 0x00000400}, //
    {0x00000002, 0x0000a8c9, 0xff002400}, //
    {0x00000002, 0x0000a8ca, 0xff002400}, //
    {0x00000002, 0x0000a8cb, 0x00000400}, //
    {0x00000002, 0x0000a8cc, 0x00000400}, //
    {0x00000002, 0x0000a8cd, 0x00000400}, //
    {0x00000002, 0x0000a8ce, 0x00000400}, //
    {0x00000002, 0x0000a8cf, 0x00000400}, //
    {0x00000002, 0x0000a8d0, 0xff002400}, //
    {0x00000002, 0x0000a8d1, 0xff002400}, //
    {0x00000002, 0x0000a8d6, 0xff002400}, //
    {0x00000002, 0x0000a8d7, 0xff002400}, //
    {0x00000002, 0x0000a8d8, 0xff002a00}, //
    {0x00000002, 0x0000a8d9, 0xff002400}, //
    {0x00000002, 0x0000a8da, 0xff002c00}, //
    {0x00000002, 0x0000a8db, 0xff002400}, //
    {0x00000002, 0x0000a8dc, 0xff002400}, //
    {0x00000002, 0x0000a8dd, 0xff002800}, //
    {0x00000002, 0x0000a8de, 0xff002400}, //
    {0x00000002, 0x0000a8df, 0xff002800}, //
    {0x00000002, 0x0000a8e0, 0xff002400}, //
    {0x00000002, 0x0000a8e1, 0xff002400}, //
    {0x00000002, 0x0000a8e2, 0xff002800}, //
    {0x00000002, 0x0000a8e3, 0xff002800}, //
    {0x00000002, 0x0000a8e4, 0xff002800}, //
    {0x00000002, 0x0000a8e6, 0xff002400}, //
    {0x00000002, 0x0000a8e7, 0xff002400}, //
    {0x00000002, 0x0000a8e8, 0xff002400}, //
    {0x00000002, 0x0000a8e9, 0xff003200}, //
    {0x00000002, 0x0000a8ea, 0xff003200}, //
    {0x00000002, 0x0000a8eb, 0xff002400}, //
    {0x00000002, 0x0000a8ef, 0xff002800}, //
    {0x00000002, 0x0000a8f2, 0xff002400}, //
    {0x00000002, 0x0000a8f3, 0xff002400}, //
    {0x00000002, 0x0000a8f5, 0xff003200}, //
    {0x00000002, 0x0000a8f6, 0xff002400}, //
    {0x00000002, 0x0000a8f7, 0xff002400}, //
    {0x00000002, 0x0000a8f8, 0xff002400}, //
    {0x00000002, 0x0000a8fa, 0xff002400}, //
    {0x00000002, 0x0000a8fb, 0xff002400}, //
    {0x00000002, 0x0000a8fd, 0x00000600}, //
    {0x00000002, 0x0000a8fe, 0x00000600}, //
    {0x00000002, 0x0000a8ff, 0xff003200}, //
    {0x00000002, 0x0000a900, 0xff002400}, //
    {0x00000002, 0x0000a901, 0xff002400}, //
    {0x00000002, 0x0000a902, 0xff002400}, //
    {0x00000002, 0x0000a903, 0xff002400}, //
    {0x00000002, 0x0000a904, 0xff002400}, //
    {0x00000002, 0x0000a905, 0xff002800}, //
    {0x00000002, 0x0000a906, 0xff002400}, //
    {0x00000002, 0x0000a907, 0xff002400}, //
    {0x00000002, 0x0000a908, 0xff002400}, //
    {0x00000002, 0x0000a909, 0xff002400}, //
    {0x00000002, 0x0000a90a, 0xff002400}, //
    {0x00000002, 0x0000a90b, 0xff002400}, //
    {0x00000002, 0x0000a90c, 0xff002400}, //
    {0x00000002, 0x0000a90e, 0xff002a00}, //
    {0x00000002, 0x0000a915, 0xff002400}, //
    {0x00000002, 0x0000a916, 0xff002400}, //
    {0x00000002, 0x0000a917, 0xff002400}, //
    {0x00000002, 0x0000a918, 0xff003200}, //
    {0x00000002, 0x0000a919, 0xff003200}, //
    {0x00000002, 0x0000a91a, 0xff002400}, //
    {0x00000002, 0x0000a91b, 0xff002400}, //
    {0x00000002, 0x0000a91c, 0x000bbb00}, //
    {0x00000002, 0x0000a91d, 0xff002400}, //
    {0x00000002, 0x0000a920, 0xff002400}, //
    {0x00000002, 0x0000a921, 0xff002400}, //
    {0x00000002, 0x0000a922, 0xff002400}, //
    {0x00000002, 0x0000a923, 0xff002400}, //
    {0x00000002, 0x0000a924, 0xff002400}, //
    {0x00000002, 0x0000a925, 0xff002400}, //
    {0x00000002, 0x0000a926, 0xff002400}, //
    {0x00000002, 0x0000a927, 0xff002400}, //
    {0x00000002, 0x0000a928, 0xff002400}, //
    {0x00000002, 0x0000a929, 0xff002400}, //
    {0x00000002, 0x0000a92a, 0xff002400}, //
    {0x00000002, 0x0000a92b, 0xff002400}, //
    {0x00000002, 0x0000a92c, 0x000bbb00}, //
    {0x00000002, 0x0000a92d, 0x00000600}, //
    {0x00000002, 0x0000a92f, 0xff002400}, //
    {0x00000002, 0x0000a930, 0xff002400}, //
    {0x00000002, 0x0000a931, 0xff002400}, //
    {0x00000002, 0x0000a932, 0xff002400}, //
    {0x00000002, 0x0000a934, 0xff002a00}, //
    {0x00000002, 0x0000a935, 0xff002400}, //
    {0x00000002, 0x0000a936, 0xff002400}, //
    {0x00000002, 0x0000a937, 0xff002800}, //
    {0x00000002, 0x0000a938, 0x00000600}, //
    {0x00000002, 0x0000a93a, 0xff002400}, //
    {0x00000002, 0x0000a93c, 0xff002400}, //
    {0x00000002, 0x0000a93d, 0xff002400}, //
    {0x00000002, 0x0000a93e, 0xff002800}, //
    {0x00000002, 0x0000a940, 0xff002800}, //
    {0x00000002, 0x0000a941, 0xff002400}, //
    {0x00000002, 0x0000a942, 0xff002400}, //
    {0x00000002, 0x0000a943, 0xff002400}, //
    {0x00000002, 0x0000a944, 0xff002400}, //
    {0x00000002, 0x0000a945, 0xff002400}, //
    {0x00000002, 0x0000a946, 0xff002400}, //
    {0x00000002, 0x0000a947, 0xff002800}, //
    {0x00000002, 0x0000a948, 0xff002400}, //
    {0x00000002, 0x0000a949, 0xff002400}, //
    {0x00000002, 0x0000a94a, 0xff002400}, //
    {0x00000002, 0x0000a94b, 0xff002800}, //
    {0x00000002, 0x0000a94c, 0xff002400}, //
    {0x00000002, 0x0000a94d, 0xff002400}, //
    {0x00000002, 0x0000a94e, 0xff002800}, //
    {0x00000002, 0x0000a94f, 0xff002400}, //
    {0x00000002, 0x0000a95b, 0xff002800}, //
    {0x00000002, 0x0000a95c, 0xff002400}, //
    {0x00000002, 0x0000a95d, 0xff002400}, //
    {0x00000002, 0x0000a95e, 0xff002400}, //
    {0x00000002, 0x0000a95f, 0xff002400}, //
    {0x00000002, 0x0000a960, 0xff002800}, //
    {0x00000002, 0x0000a961, 0xff002400}, //
    {0x00000002, 0x0000a962, 0xff002400}, //
    {0x00000002, 0x0000a964, 0xff002400}, //
    {0x00000002, 0x0000a965, 0x000bc100}, //
    {0x00000002, 0x0000a966, 0x000bc200}, //
    {0x00000002, 0x0000a96a, 0x000bc300}, //
    {0x00000002, 0x0000a96b, 0xff002400}, //
    {0x00000002, 0x0000a96c, 0xff002400}, //
    {0x00000002, 0x0000a96d, 0xff002a00}, //
    {0x00000002, 0x0000a96e, 0x000bc200}, //
    {0x00000002, 0x0000a96f, 0xff002400}, //
    {0x00000002, 0x0000a970, 0xff002400}, //
    {0x00000002, 0x0000a971, 0xff002400}, //
    {0x00000002, 0x0000a972, 0xff002400}, //
    {0x00000002, 0x0000a973, 0xff002800}, //
    {0x00000002, 0x0000a974, 0xff002c00}, //
    {0x00000002, 0x0000a975, 0x00000600}, //
    {0x00000002, 0x0000a976, 0xff002400}, //
    {0x00000002, 0x0000a977, 0xff002400}, //
    {0x00000002, 0x0000a979, 0xff002400}, //
    {0x00000002, 0x0000a980, 0xff002800}, //
    {0x00000002, 0x0000a987, 0xff002400}, //
    {0x00000002, 0x0000a988, 0xff002400}, //
    {0x00000002, 0x0000a989, 0xff002400}, //
    {0x00000002, 0x0000a98a, 0x00000f00}, //
    {0x00000002, 0x0000a98b, 0xff002400}, //
    {0x00000002, 0x0000a98c, 0xff002400}, //
    {0x00000002, 0x0000a98d, 0x000bc100}, //
    {0x00000002, 0x0000a98e, 0x000bc200}, //
    {0x00000002, 0x0000a98f, 0xff002400}, //
    {0x00000002, 0x0000a990, 0xff002400}, //
    {0x00000002, 0x0000a991, 0xff002400}, //
    {0x00000002, 0x0000a992, 0xff002400}, //
    {0x00000002, 0x0000a993, 0xff002400}, //
    {0x00000002, 0x0000abe0, 0xff002c00}, //
    {0x00000002, 0x0000abe1, 0xff002c00}, //
    {0x00000002, 0x0000abe2, 0xff002c00}, //
    {0x00000002, 0x0000abe3, 0xff002400}, //
    {0x00000002, 0x0000abe4, 0xff002800}, //
    {0x00000002, 0x0000abe5, 0xff002c00}, //
    {0x00000002, 0x0000abe6, 0xff002c00}, //
    {0x00000002, 0x0000abe8, 0xff002c00}, //
    {0x00000002, 0x0000abe9, 0xff002c00}, //
    {0x00000002, 0x0000abeb, 0xff002800}, //
    {0x00000002, 0x0000abec, 0xff002800}, //
    {0x00000002, 0x0000abed, 0xff002800}, //
    {0x00000002, 0x0000abee, 0xff002400}, //
    {0x00000002, 0x0000abf2, 0xff002400}, //
    {0x00000002, 0x0000abf3, 0xff002400}, //
    {0x00000002, 0x0000abf5, 0xff002a00}, //
    {0x00000002, 0x0000abf6, 0xff002400}, //
    {0x00000002, 0x0000abf7, 0xff002400}, //
    {0x00000002, 0x0000abf8, 0xff002400}, //
    {0x00000002, 0x0000abf9, 0xff002400}, //
    {0x00000002, 0x0000abfa, 0xff002400}, //
    {0x00000002, 0x0000abfb, 0xff002400}, //
    {0x00000002, 0x0000abfc, 0xff002400}, //
    {0x00000002, 0x0000abfd, 0xff002400}, //
    {0x00000002, 0x0000abfe, 0xff002400}, //
    {0x00000002, 0x0000ac00, 0xff002800}, //
    {0x00000002, 0x0000ac01, 0xff002800}, //
    {0x00000002, 0x0000ac09, 0xff002400}, //
    {0x00000002, 0x0000ac0a, 0xff002a00}, //
    {0x00000002, 0x0000ac0b, 0x00000500}, //
    {0x00000002, 0x0000ac0c, 0x00000400}, //
    {0x00000002, 0x0000ac0d, 0xff002a00}, //
    {0x00000002, 0x0000ac0e, 0xff002800}, //
    {0x00000002, 0x0000ac0f, 0xff002400}, //
    {0x00000002, 0x0000ac10, 0xff002400}, //
    {0x00000002, 0x0000ac11, 0xff002400}, //
    {0x00000002, 0x0000ac12, 0xff002400}, //
    {0x00000002, 0x0000ac13, 0xff002400}, //
    {0x00000002, 0x0000ac14, 0xff002400}, //
    {0x00000002, 0x0000ac15, 0xff002400}, //
    {0x00000002, 0x0000ac16, 0xff002400}, //
    {0x00000002, 0x0000ac17, 0xff002400}, //
    {0x00000002, 0x0000ac18, 0xff002400}, //
    {0x00000002, 0x0000ac19, 0x00000400}, //
    {0x00000002, 0x0000ac1a, 0xff002400}, //
    {0x00000002, 0x0000ac1b, 0xff002400}, //
    {0x00000002, 0x0000ac1c, 0xff002400}, //
    {0x00000002, 0x0000ac1d, 0xff002400}, //
    {0x00000002, 0x0000ac1e, 0xff002400}, //
    {0x00000002, 0x0000ac1f, 0xff002400}, //
    {0x00000002, 0x0000ac20, 0xff003100}, //
    {0x00000002, 0x0000ac21, 0xff002800}, //
    {0x00000002, 0x0000ac22, 0xff002400}, //
    {0x00000002, 0x0000ac23, 0xff002800}, //
    {0x00000002, 0x0000ac24, 0xff002800}, //
    {0x00000002, 0x0000ac25, 0xff002400}, //
    {0x00000002, 0x0000ac26, 0xff003100}, //
    {0x00000002, 0x0000ac27, 0xff002400}, //
    {0x00000002, 0x0000ac28, 0xff003100}, //
    {0x00000002, 0x0000ac29, 0xff002800}, //
    {0x00000002, 0x0000ac2a, 0xff002400}, //
    {0x00000002, 0x0000ac2b, 0xff002400}, //
    {0x00000002, 0x0000ac2d, 0xff002800}, //
    {0x00000002, 0x0000ac2e, 0xff002400}, //
    {0x00000002, 0x0000ac2f, 0x00000600}, //
    {0x00000002, 0x0000ac30, 0xff002400}, //
    {0x00000002, 0x0000ac31, 0xff002400}, //
    {0x00000002, 0x0000ac32, 0xff002400}, //
    {0x00000002, 0x0000ac33, 0xff002400}, //
    {0x00000002, 0x0000ac34, 0xff002400}, //
    {0x00000002, 0x0000ac35, 0xff002800}, //
    {0x00000002, 0x0000ac36, 0xff002400}, //
    {0x00000002, 0x0000ac37, 0xff002400}, //
    {0x00000002, 0x0000ac38, 0xff002800}, //
    {0x00000002, 0x0000ac39, 0xff002400}, //
    {0x00000002, 0x0000ac3a, 0xff002400}, //
    {0x00000002, 0x0000ac3c, 0xff002800}, //
    {0x00000002, 0x0000ac3d, 0xff002400}, //
    {0x00000002, 0x0000ac3e, 0xff002800}, //
    {0x00000002, 0x0000ac3f, 0xff002400}, //
    {0x00000002, 0x0000ac40, 0xff002400}, //
    {0x00000002, 0x0000ac41, 0xff002400}, //
    {0x00000002, 0x0000ac42, 0xff002800}, //
    {0x00000002, 0x0000ac43, 0xff002400}, //
    {0x00000002, 0x0000ac44, 0xff002c00}, //
    {0x00000002, 0x0000afc8, 0xff002400}, //
    {0x00000002, 0x0000afc9, 0xff002900}, //
    {0x00000002, 0x0000afca, 0xff002400}, //
    {0x00000002, 0x0000afcc, 0xff002800}, //
    {0x00000002, 0x0000afce, 0xff002800}, //
    {0x00000002, 0x0000afcf, 0xff002400}, //
    {0x00000002, 0x0000afd0, 0xff002c00}, //
    {0x00000002, 0x0000afd1, 0xff002800}, //
    {0x00000002, 0x0000afd2, 0xff002800}, //
    {0x00000002, 0x0000afd5, 0xff002400}, //
    {0x00000002, 0x0000afd6, 0xff002400}, //
    {0x00000002, 0x0000afd8, 0xff002a00}, //
    {0x00000002, 0x0000afd9, 0xff002200}, //
    {0x00000002, 0x0000afda, 0xff002800}, //
    {0x00000002, 0x0000afde, 0xff002400}, //
    {0x00000002, 0x0000afdf, 0xff002800}, //
    {0x00000002, 0x0000afe0, 0xff002c00}, //
    {0x00000002, 0x0000afe1, 0xff002400}, //
    {0x00000002, 0x0000afe2, 0xff002800}, //
    {0x00000002, 0x0000afe4, 0xff002400}, //
    {0x00000002, 0x0000afe5, 0xff002800}, //
    {0x00000002, 0x0000afe6, 0xff002800}, //
    {0x00000002, 0x0000afe7, 0xff002800}, //
    {0x00000002, 0x0000afe8, 0xff002800}, //
    {0x00000002, 0x0000afe9, 0xff002400}, //
    {0x00000002, 0x0000afea, 0xff002800}, //
    {0x00000002, 0x0000afeb, 0xff002800}, //
    {0x00000002, 0x0000aff0, 0xff002400}, //
    {0x00000002, 0x0000aff5, 0xff002800}, //
    {0x00000002, 0x0000aff6, 0xff002800}, //
    {0x00000002, 0x0000aff7, 0xff002400}, //
    {0x00000002, 0x0000affa, 0xff002500}, //
    {0x00000002, 0x0000affc, 0xff002400}, //
    {0x00000002, 0x0000affd, 0xff002400}, //
    {0x00000002, 0x0000afff, 0xff002200}, //
    {0x00000002, 0x0000b004, 0xff002400}, //
    {0x00000002, 0x0000b005, 0xff002400}, //
    {0x00000002, 0x0000b006, 0x00000500}, //
    {0x00000002, 0x0000b007, 0xff002800}, //
    {0x00000002, 0x0000b008, 0xff002800}, //
    {0x00000002, 0x0000b009, 0xff002800}, //
    {0x00000002, 0x0000b00a, 0xff002800}, //
    {0x00000002, 0x0000b010, 0x00000f00}, //
    {0x00000002, 0x0000b011, 0xff002800}, //
    {0x00000002, 0x0000b012, 0xff002800}, //
    {0x00000002, 0x0000b3b0, 0xff002800}, //
    {0x00000002, 0x0000b3b1, 0xff002400}, //
    {0x00000002, 0x0000b3b2, 0xff002400}, //
    {0x00000002, 0x0000b3b3, 0xff002300}, //
    {0x00000002, 0x0000b3b4, 0x00000b00}, //
    {0x00000002, 0x0000b3b5, 0xff002400}, //
    {0x00000002, 0x0000b3b7, 0xff002400}, //
    {0x00000002, 0x0000b3b9, 0xff002500}, //
    {0x00000002, 0x0000b3ba, 0xff002800}, //
    {0x00000002, 0x0000b3bb, 0xff002400}, //
    {0x00000002, 0x0000b3c2, 0xff002400}, //
    {0x00000002, 0x0000b3c3, 0xff002400}, //
    {0x00000002, 0x0000b3c4, 0xff002800}, //
    {0x00000002, 0x0000b3c5, 0xff002400}, //
    {0x00000002, 0x0000b3c6, 0xff002400}, //
    {0x00000002, 0x0000b3c7, 0xff002800}, //
    {0x00000002, 0x0000b3c8, 0xff002400}, //
    {0x00000002, 0x0000b3c9, 0xff002400}, //
    {0x00000002, 0x0000b3ca, 0xff003200}, //
    {0x00000002, 0x0000b3cb, 0xff003200}, //
    {0x00000002, 0x0000b3cc, 0xff003200}, //
    {0x00000002, 0x0000b3cd, 0xff002400}, //
    {0x00000002, 0x0000b3ce, 0xff002400}, //
    {0x00000002, 0x0000b3d0, 0xff002500}, //
    {0x00000002, 0x0000b3d1, 0xff002400}, //
    {0x00000002, 0x0000b3d2, 0xff002800}, //
    {0x00000002, 0x0000b3d3, 0xff002800}, //
    {0x00000002, 0x0000b3d4, 0xff002800}, //
    {0x00000002, 0x0000b3d5, 0xff002c00}, //
    {0x00000002, 0x0000b3d6, 0xff002800}, //
    {0x00000002, 0x0000b3d7, 0xff002400}, //
    {0x00000002, 0x0000b3d9, 0xff002400}, //
    {0x00000002, 0x0000b3da, 0xff002400}, //
    {0x00000002, 0x0000b3db, 0xff003200}, //
    {0x00000002, 0x0000b3dd, 0xff002400}, //
    {0x00000002, 0x0000b3de, 0x00000600}, //
    {0x00000002, 0x0000b3df, 0xff002400}, //
    {0x00000002, 0x0000b3e0, 0xff002800}, //
    {0x00000002, 0x0000b3e1, 0xff003200}, //
    {0x00000002, 0x0000b3e2, 0xff002400}, //
    {0x00000002, 0x0000b3e3, 0xff002800}, //
    {0x00000002, 0x0000b3e4, 0xff002400}, //
    {0x00000002, 0x0000b3e5, 0xff002400}, //
    {0x00000002, 0x0000b3e6, 0xff002400}, //
    {0x00000002, 0x0000b3e7, 0xff002400}, //
    {0x00000002, 0x0000b3e8, 0xff002400}, //
    {0x00000002, 0x0000b3e9, 0xff002400}, //
    {0x00000002, 0x0000b3ea, 0xff002400}, //
    {0x00000002, 0x0000b3eb, 0xff002400}, //
    {0x00000002, 0x0000b3ec, 0xff002400}, //
    {0x00000002, 0x0000b3ee, 0xff002400}, //
    {0x00000002, 0x0000b3ef, 0xff002400}, //
    {0x00000002, 0x0000b3f0, 0xff002400}, //
    {0x00000002, 0x0000b3f1, 0xff002400}, //
    {0x00000002, 0x0000b3f2, 0xff002800}, //
    {0x00000002, 0x0000b3f3, 0xff002800}, //
    {0x00000002, 0x0000b3f4, 0xff002800}, //
    {0x00000002, 0x0000b3f5, 0xff003200}, //
    {0x00000002, 0x0000b3f6, 0xff002400}, //
    {0x00000002, 0x0000b3f7, 0xff002400}, //
    {0x00000002, 0x0000b3f8, 0xff002400}, //
    {0x00000002, 0x0000b3f9, 0xff002400}, //
    {0x00000002, 0x0000b3fa, 0xff002400}, //
    {0x00000002, 0x0000b3fd, 0xff002400}, //
    {0x00000002, 0x0000b3fe, 0xff002400}, //
    {0x00000002, 0x0000b3ff, 0xff002400}, //
    {0x00000002, 0x0000b400, 0xff002800}, //
    {0x00000002, 0x0000b401, 0xff002400}, //
    {0x00000002, 0x0000b404, 0xff002400}, //
    {0x00000002, 0x0000b405, 0xff002400}, //
    {0x00000002, 0x0000b406, 0xff002400}, //
    {0x00000002, 0x0000b407, 0xff002400}, //
    {0x00000002, 0x0000b798, 0xff002c00}, //
    {0x00000002, 0x0000b799, 0xff002c00}, //
    {0x00000002, 0x0000b79a, 0xff002c00}, //
    {0x00000002, 0x0000fa09, 0xff002800}, //
    {0x00000002, 0x0000fa0b, 0xff002800}, //
    {0x00000002, 0x0000fa0d, 0xff002800}, //
    {0x00000002, 0x0000fa13, 0xff002800}, //
    {0x00000002, 0x0000fa14, 0xff003100}, //
    {0x00000002, 0x0000fa15, 0xff003100}, //
    {0x00000002, 0x0000fa16, 0xff003100}, //
    {0x00000002, 0x0000fa17, 0xff003100}, //
    {0x00000002, 0x0000fa1a, 0xff002800}, //
    {0x00000002, 0x0000fa1b, 0xff003100}, //
    {0x00000002, 0x0000fa22, 0xff002800}, //
    {0x00000002, 0x0000fa23, 0xff003100}, //
    {0x00000002, 0x0000fa25, 0xff003100}, //
    {0x00000002, 0x0000fa26, 0xff002800}, //
    {0x00000002, 0x0000fa29, 0xff003100}, //
    {0x00000002, 0x0000fa2a, 0xff002800}, //
    {0x00000002, 0x0000fa2d, 0xff003100}, //
    {0x00000002, 0x0000fa2e, 0xff002800}, //
    {0x00000002, 0x0000fa37, 0xff002800}, //
    {0x00000002, 0x0000fa38, 0xff003100}, //
    {0x00000002, 0x0000fa3c, 0xff002800}, //
    {0x00000002, 0x0000fa3d, 0xff002800}, //
    {0x00000002, 0x0000fa3e, 0xff003100}, //
    {0x00000002, 0x0000fa3f, 0xff003100}, //
    {0x00000002, 0x0000fa4b, 0xff002800}, //
    {0x00000002, 0x0000fa4c, 0xff002800}, //
    {0x00000002, 0x0000fa4d, 0xff003100}, //
    {0x00000002, 0x0000fa4e, 0xff003100}, //
    {0x00000002, 0x00023283, 0x00000400}, //
    {0x00000003, 0x00000008, 0xfff00200}, //
    {0x00000003, 0x00000065, 0xff002400}, //
    {0x00000003, 0x00000066, 0xff002400}, //
    {0x00000003, 0x0000006f, 0xff002400}, //
    {0x00000003, 0x000017d5, 0xff002a00}, //
    {0x00000003, 0x000017d7, 0xff002a00}, //
    {0x00000003, 0x000017d8, 0xff002a00}, //
    {0x00000003, 0x000017da, 0xff002a00}, //
    {0x00000003, 0x000017db, 0xff002a00}, //
    {0x00000003, 0x000017dd, 0xff002a00}, //
    {0x00000003, 0x000017de, 0xff002400}, //
    {0x00000003, 0x000017df, 0x00177000}, //
    {0x00000003, 0x000017e0, 0x00177000}, //
    {0x00000003, 0x000017e1, 0x00177000}, //
    {0x00000003, 0x000017e2, 0xff002a00}, //
    {0x00000003, 0x000017ef, 0xff002c00}, //
    {0x00000003, 0x000017f0, 0xff002400}, //
    {0x00000003, 0x000017f4, 0xff002400}, //
    {0x00000003, 0x000017f5, 0xff002400}, //
    {0x00000003, 0x0000180a, 0xff002100}, //
    {0x00000003, 0x00001838, 0xff002400}, //
    {0x00000003, 0x00001839, 0xff002400}, //
    {0x00000003, 0x0000183b, 0xff002400}, //
    {0x00000003, 0x0000183d, 0xff002800}, //
    {0x00000003, 0x00001843, 0xff002a00}, //
    {0x00000003, 0x00001844, 0xff002400}, //
    {0x00000003, 0x00001845, 0xff002400}, //
    {0x00000003, 0x00001862, 0xff002400}, //
    {0x00000003, 0x00001863, 0xff002400}, //
    {0x00000003, 0x00001864, 0xff002400}, //
    {0x00000003, 0x00001865, 0xff002400}, //
    {0x00000003, 0x0000a832, 0xff002a00}, //
    {0x00000003, 0x0000afd1, 0xff002800}, //
    {0x00000003, 0x0000afea, 0xff002800}, //
    {0x00000003, 0x0000afeb, 0xff002800}, //
    {0x00000003, 0x0000aff5, 0xff002800}, //
    {0x00000003, 0x0000aff6, 0xff002800}, //
    {0x00000003, 0x0000b3c1, 0xff002400}, //
    {0x00000003, 0x0000b3cf, 0xff002400}, //
    {0x00000003, 0x0000b3e4, 0xff002400}, //
    {0x00000003, 0x0000b3ee, 0xff002400}, //
    {0x00000003, 0x0000b3ef, 0xff002400}, //
    {0x00000003, 0x0000b3f0, 0xff002400}, //
    {0x00000003, 0x0000b3f1, 0xff002400}, //
    {0x00000003, 0x0000fde8, 0x00000100}, //
    {0x00000003, 0x0000fe02, 0x00000e00}, //
    {0x00000003, 0x00023a50, 0xff002400}, //
    {0x00000003, 0x00023a5c, 0xff002400}, //
    {0x00000003, 0x00023a5d, 0xff002400}, //
    {0x00000003, 0x00023a5e, 0xff002100}, //
    {0x00000003, 0x00023a60, 0xff002c00}, //
    {0x00000003, 0x00023a61, 0xff002400}, //
    {0x00000003, 0x00023a62, 0xff002c00}, //
    {0x00000003, 0x00023a79, 0xff002400}, //
    {0x00000003, 0x00023a7a, 0xff002400}, //
    {0x00000003, 0x00023a7b, 0xff002400}, //
    {0x00000003, 0x00023a7c, 0xff002400}, //
    {0x00000004, 0x00000009, 0xfff00200}, //
    {0x00000004, 0x00000067, 0xff002400}, //
    {0x00000004, 0x00000068, 0xff002400}, //
    {0x00000004, 0x0000006b, 0xff002800}, //
    {0x00000004, 0x0000006c, 0xff002800}, //
    {0x00000004, 0x000017e3, 0xff002400}, //
    {0x00000004, 0x00001808, 0xff002400}, //
    {0x00000004, 0x00001809, 0xff002400}, //
    {0x00000004, 0x00001bc4, 0xff002400}, //
    {0x00000004, 0x00001bc5, 0xff002400}, //
    {0x00000004, 0x00001bcc, 0xff002c00}, //
    {0x00000004, 0x0000a806, 0x00000500}, //
    {0x00000004, 0x0000b3b4, 0x00000b00}, //
    {0x00000004, 0x0000fa06, 0xff002400}, //
    {0x00000004, 0x00014438, 0x00000600}, //
    {0x00000004, 0x00014439, 0x00000600}, //
    {0x00000004, 0x0001443a, 0x00000600}, //
    {0x00000004, 0x0001443b, 0xff002800}, //
    {0x00000004, 0x0001e078, 0xff002800}, //
    {0x00000004, 0x0001e464, 0xff002400}, //
    {0x00000004, 0x0001e465, 0xff002400}, //
    {0x00000004, 0x0001e466, 0xff002400}, //
    {0x00000004, 0x00023a5a, 0xff002400}, //
    {0x00000004, 0x00028873, 0xff002800}, //
    {0x00000004, 0x00028876, 0x00000400}, //
    {0x00000004, 0x00028877, 0xff002800}, //
    {0x00000004, 0x00028878, 0xff002400}, //
    {0x00000004, 0x00028879, 0xff002400}, //
    {0x00000004, 0x0002887a, 0xff002a00}, //
    {0x00000004, 0x0002887b, 0xff002400}, //
    {0x00000004, 0x0002887c, 0xff002400}, //
    {0x00000005, 0x00000069, 0xff002800}, //
    {0x00000005, 0x0000006b, 0xff002800}, //
    {0x00000005, 0x0000006c, 0xff002800}, //
    {0x00000005, 0x00001bc3, 0xff002200}, //
    {0x00000005, 0x0000a819, 0xff002400}, //
    {0x00000005, 0x0000fdea, 0xff002400}, //
    {0x00000005, 0x0002c6f0, 0x00000100}, //
    {0x00000005, 0x0002d691, 0xff002800}, //
    {0x00000005, 0x0002d695, 0xff002400}, //
    {0x00000005, 0x0002d696, 0xff002400}, //
    {0x00000005, 0x0002da78, 0x001b5e00}, //
    {0x00000005, 0x0002da79, 0x001b5f00}, //
    {0x00000005, 0x0002da7a, 0xff002000}, //
    {0x00000006, 0x00000c1d, 0xff002400}, //
    {0x00000006, 0x00036718, 0xff002400}, //
    {0x00000006, 0x00036719, 0x00000600}, //
    {0x00000006, 0x0003671a, 0x00000600}, //
    {0x00000006, 0x0003671b, 0x00000600}, //
    {0x00000006, 0x00036721, 0xff002400}, //
    {0x00000006, 0x00036722, 0xff002a00}, //
    {0x00000006, 0x00036725, 0xff002400}, //
    {0x00000006, 0x0003672a, 0xff002400}, //
    {0x00000007, 0x00000c1d, 0xff002400}, //
    {0x00000007, 0x00036718, 0xff002400}, //
    {0x00000007, 0x00036719, 0x00000600}, //
    {0x00000007, 0x0003671a, 0x00000600}, //
    {0x00000007, 0x0003671b, 0x00000600}, //
    {0x00000007, 0x0003671c, 0xff002800}, //
    {0x00000007, 0x0003671e, 0xff002a00}, //
    {0x00000007, 0x00036721, 0xff002400}, //
    {0x00000007, 0x00036722, 0xff002a00}, //
    {0x00000007, 0x00036723, 0x00000600}, //
    {0x00000007, 0x00036724, 0x00000600}, //
    {0x00000007, 0x00036725, 0xff002400}, //
    {0x00000007, 0x00036727, 0xff002400}, //
    {0x00000007, 0x00036728, 0xff002400}, //
    {0x00000007, 0x0003672a, 0xff002400}, //
    {0x00000007, 0x0003672b, 0x00000600}, //
    {0x00000008, 0x00000c1d, 0xff002400}, //
    {0x00000008, 0x00036718, 0xff002400}, //
    {0x00000008, 0x00036719, 0x00000600}, //
    {0x00000008, 0x0003671a, 0x00000600}, //
    {0x00000008, 0x0003671b, 0x00000600}, //
    {0x00000008, 0x0003671c, 0xff002800}, //
    {0x00000008, 0x0003671e, 0xff002a00}, //
    {0x00000008, 0x00036721, 0xff002400}, //
    {0x00000008, 0x00036722, 0xff002a00}, //
    {0x00000008, 0x00036723, 0x00000600}, //
    {0x00000008, 0x00036724, 0x00000600}, //
    {0x00000008, 0x00036725, 0xff002400}, //
    {0x00000008, 0x00036727, 0xff002400}, //
    {0x00000008, 0x00036728, 0xff002400}, //
    {0x00000008, 0x0003672a, 0xff002400}, //
    {0x00000008, 0x0003672b, 0x00000600}, //
};

static void check_dict() {
  static const uint32_t size = sizeof(dict) / sizeof(*dict);
  const D *prev = dict + 0;
  for (uint32_t i = 1; i < size; ++i) {
    const D *next = dict + i;
    if (next->group == prev->group)
      assert(next->key > prev->key);
    else
      assert(next->group > prev->group);
    prev = next;
  }
}

static void check_type(uint32_t group, I *si) {
  assert((si->key & 0xfff00000) == 0x0);
  assert((si->type & 0x00ff) == 0x0);
  const uint32_t sign = si->type >> 24;
  assert(sign == 0x0 || sign == 0xff);
  const uint32_t type = si->type;
  static const uint32_t size = sizeof(dict) / sizeof(*dict);
  bool found = false;
#if 0
  if (si->key == 0x000017e3 && si->type == 0xff002a00) {
    found = true;
  } else {
#endif
  for (uint32_t i = 0; i < size; ++i) {
    const D *d = dict + i;
    if (group == d->group && si->key == d->key) {
      assert(d->type == type);
      found = true;
    }
  }
  //  }
  assert(found);
}

int main(int argc, char *argv[]) {
  if (argc < 2)
    return 1;
  check_dict();
  const char *filename = argv[1];
  FILE *in = fopen(filename, "rb");
  int i, r = 0;
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
      printf("<#last element coming: %08x>\n", nitems);
      assert(nitems > 0);
      remain = nitems;
      fread(&nitems, 1, sizeof nitems, in);
      last_element = true;
    }
    ++r;
    printf("Group %d #Items: %u\n", r, nitems);
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
      check_type(r, &si);
      printf("  #group: 0x%08x #key: 0x%08x #type: 0x%08x ", r, si.key,
             si.type);
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
