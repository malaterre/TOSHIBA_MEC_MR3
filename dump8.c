#include "mec_mr3_dict.h"

#include <assert.h>
#include <iconv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t file_size(const char *filename) {
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fclose(f);
  return fsize;
}

struct stream {
  const void *start;
  const void *end;
  void *cur;
  size_t (*read)(void *ptr, size_t size, size_t nmemb, struct stream *in);
};

static size_t stream_read(void *ptr, size_t size, size_t nmemb,
                          struct stream *in) {
  char *cur = (char *)in->cur;
  const char *end = (const char *)in->end;
  const size_t len = size * nmemb;
  if (cur + len <= end) {
    memcpy(ptr, cur, len);
    in->cur = cur + len;
  } else {
    in->cur = NULL;
    return 0;
  }
  return nmemb;
}

struct app {
  struct stream *in;
  iconv_t conv;
};

static struct app *create_app(struct app *self, struct stream *in) {
  self->in = in;
  self->conv = iconv_open("utf-8", "shift-jis");
  assert(self->conv != (iconv_t)-1);

  return self;
}

static void setup_buffer(struct app *self, const void *input, size_t len) {
  self->in->cur = (char *)input;
  self->in->start = input;
  self->in->end = (char *)input + len;
  self->in->read = stream_read;
}

#define ERROR_RETURN(X, Y)                                                     \
  if ((X) != (Y))                                                              \
  return false

static size_t fread_mirror(void *ptr, size_t size, size_t nmemb,
                           struct app *self) {
  struct stream *instream = self->in;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    return nmemb;
  }
  assert(0);
  return s;
}

static bool write_trailer(struct app *self) {
  assert(self->in->cur <= self->in->end);
  if (self->in->cur == self->in->end)
    return true;
  // else it is missing one byte (nul byte):
  char padding;
  size_t s = fread_mirror(&padding, sizeof padding, 1, self);
  ERROR_RETURN(s, 1);
  ERROR_RETURN(padding, 0);

  return true;
}

struct mec_mr3_info {
  uint32_t key;
  uint32_t type;
};

struct mec_mr3_item_data {
  uint32_t len;
  char *buffer;
};

static const unsigned char magic2[] = {0, 0, 0, 0, 0, 0, 0, 0, 0xc, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0, 0,   0};

static bool read_info(struct app *self, const uint8_t group,
                      struct mec_mr3_info *info) {
  // read key and type at once:
  size_t s = fread_mirror(info, sizeof *info, 1, self);
  ERROR_RETURN(s, 1);
  bool found = check_mec_mr3_info(group, info->key, info->type);
  ERROR_RETURN(found, true);

  return true;
}

static bool read_data(struct app *self, const uint8_t group,
                      const struct mec_mr3_info *info,
                      struct mec_mr3_item_data *data) {
  size_t s = fread_mirror(&data->len, sizeof data->len, 1, self);
  ERROR_RETURN(s, 1);
  // in the wild we have: data->len <= 9509
  unsigned char separator[20];
  s = fread_mirror(separator, sizeof *separator,
                   sizeof separator / sizeof *separator, self);
  ERROR_RETURN(s, sizeof separator / sizeof *separator);
  int b = memcmp(separator, magic2, sizeof(magic2));
  ERROR_RETURN(b, 0);
  data->buffer = (char *)realloc(data->buffer, data->len);
  if (data->len != 0 && data->buffer == NULL) {
    return false;
  }

  s = fread_mirror(data->buffer, 1, data->len, self);
  ERROR_RETURN(s, data->len);

  return true;
}

enum Type {
  ISO_8859_1_STRING =
      0x00000300,          // ASCII string / or struct with 'ISO-8859-1' marker
  STRUCT_436 = 0x001f4300, // Fixed struct 436 bytes (struct with ASCII strings)
  STRUCT_516 = 0x001f4400, // Fixed struct 516 bytes (struct with ASCII strings)
  STRUCT_325 = 0x001f4600, // Fixed struct 325 bytes (struct with ASCII strings)
  SHIFT_JIS_STRING = 0xff002c00, // SHIFT-JIS string
};

struct buffer19 {
  char sig1[0x3];
  unsigned char len2;
  char sig2;
  unsigned char len3;
  char sig3;
  char iso[0x9];
  char sig4;
  unsigned char len4;
  char sig5;
};

static void dump2file(const char *in, int len) {
  static int debug = 0;
  char buffer[512];
  sprintf(buffer, "out%04d", debug);
  ++debug;
  FILE *f = fopen(buffer, "wb");
  fwrite(in, 1, len, f);
  fclose(f);
}

static bool print_iso(void *ptr, size_t size, size_t nmemb, struct app *self) {
  static const char magic[] = {0xdf, 0xff, 0x79};
  if (nmemb >= sizeof magic && memcmp(ptr, magic, sizeof(magic)) == 0) {
    // iso
    struct buffer19 b19;
    if (nmemb < sizeof b19)
      return 0;
    memcpy(&b19, ptr, sizeof b19);
    if (b19.sig2 != 0x1 || b19.sig3 != 0x0 || b19.sig4 != 0x2 ||
        b19.sig5 != 0x0)
      return 0;
    const size_t diff = nmemb - sizeof b19;
    if (b19.len2 != nmemb - 4 || b19.len3 != 9 || b19.len4 != diff)
      return 0;
    if (strncmp(b19.iso, "ISO8859-1", 9) != 0)
      return 0;
    char *str = (char *)ptr + sizeof b19;
    {
      char *gbk_str = str;
      char dest_str[100];
      char *out = dest_str;
      size_t inbytes = b19.len4;
      size_t outbytes = sizeof dest_str;
      if (iconv(self->conv, &gbk_str, &inbytes, &out, &outbytes) ==
          (size_t)-1) {
        dump2file(gbk_str, inbytes);
        printf("{%.*s : %.*s}", 9, b19.iso, b19.len4, str);
        fflush(stdout);
        assert(0);
      }
      dest_str[sizeof dest_str - outbytes] = 0;
      printf("{%.*s : %.*s}", 9, b19.iso, outbytes, dest_str);
    }
  } else {
    // raw string buffer
    printf("[%.*s]", (int)nmemb, (char *)ptr);
  }
  return true;
}

typedef char str16[16 + 1];
typedef char str64[64 + 1];

struct buffer436 {
  uint32_t zero;
  char iver[0x45];
  char buf3[0x100]; // phi
  str64 buf4;
  str16 buf5;
  char modality[0x15];
  uint32_t val;
};
struct buffer325 {
  str64 array[5];
};

void print_buffer436(struct buffer436 *b436) {
  static const char vers1[] = "TM_MR_DCM_V1.0";
  static const char vers2[] = "TM_MR_DCM_V2.0";
  static const char vers3[] = "TM_MR_DCM_V1.0_3";
  static const char vers4[] = "TM_MR1_DCM_V1.0";
  assert(b436->zero == 0);
  assert(strcmp(b436->iver, vers1) == 0 || strcmp(b436->iver, vers2) == 0 ||
         strcmp(b436->iver, vers3) == 0 || strcmp(b436->iver, vers4) == 0);
  assert(strcmp(b436->modality, "MR") == 0);
  assert(b436->val == 1 || b436->val == 3);
  printf("{%u; %sr; %s; %s; %s; %s;%u}", b436->zero, b436->iver, b436->buf3,
         b436->buf4, b436->buf5, b436->modality, b436->val);
}

struct buffer516 {
  str64 zero; // aka 'none'
  char buf2[0x15];
  char buf3[0x100]; // phi
  str16 buf4;
  str64 buf5;
  str64 buf6;
  uint32_t bools[6];
};

void print_buffer516(struct buffer516 *b516) {
  printf("{%s; %s; %s; %s; %s; %s; (", b516->zero, b516->buf2, b516->buf3,
         b516->buf4, b516->buf5, b516->buf6);
  int c;
  for (c = 0; c < 6; ++c) {
    assert(b516->bools[c] == c % 2);
    if (c)
      printf(",");
    printf("%d", b516->bools[c]);
  }
  printf(")}");
}

void print_buffer325(struct buffer325 *b325) {}

static bool print_struct(void *ptr, size_t size, size_t nmemb,
                         struct app *self) {
  struct stream *instream = self->in;

  assert(size == 1);
  const size_t s = nmemb;
  if (s == 436) {
    struct buffer436 b436;
    memcpy(&b436, ptr, nmemb);
    print_buffer436(&b436);
  } else if (s == 516) {
    struct buffer516 b516;
    memcpy(&b516, ptr, nmemb);
    print_buffer516(&b516);
  } else if (s == 325) {
    struct buffer325 b325;
    memcpy(&b325, ptr, nmemb);
    print_buffer325(&b325);
  } else {
    assert(0); // programmer error
    return 0;
  }
  return true;
}

static bool print(struct app *self, const uint8_t group,
                  const struct mec_mr3_info *info,
                  struct mec_mr3_item_data *data) {
  const char *name = get_mec_mr3_info_name(group, info->key, info->type);
  const uint32_t sign = info->type >> 24;
  const char symb = sign ? '_' : ' ';

  bool ret = true;
  uint32_t mult = 1;
  // print info
  printf("(%01x,%05x) %c%04x ", group, info->key, symb,
         (info->type & 0x00ffff00) >> 8);
  // print data:
  switch (info->type) {
  case ISO_8859_1_STRING:
    ret = print_iso(data->buffer, 1, data->len, self);
    break;
  case STRUCT_436:
  case STRUCT_516:
  case STRUCT_325:
    ret = print_struct(data->buffer, 1, data->len, self);
    break;
  case SHIFT_JIS_STRING:
    // s = print_shift_jis(data->buffer, 1, data->len, self);
    break;
  default:
    ret = true;
  }
  // print key name
  printf(" # %u,%u %s", data->len, mult, name);

  printf("\n");
  return ret;
}

static bool read_group(struct app *self, uint8_t group, uint32_t nitems,
                       struct mec_mr3_info *info,
                       struct mec_mr3_item_data *data) {
  bool good = true;
  uint32_t i;
  for (i = 0; i < nitems && good; ++i) {
    good = good && read_info(self, group, info);
    // lazy evaluation:
    good = good && read_data(self, group, info, data);
    good = good && print(self, group, info, data);
  }
  return good;
}

static bool mec_mr3_print(const void *input, size_t len) {
  if (!input)
    return false;
  struct stream sin;
  struct app a;
  struct app *self = create_app(&a, &sin);
  setup_buffer(self, input, len);

  bool good = true;
  struct mec_mr3_info info;
  struct mec_mr3_item_data data;
  data.len = 0;
  data.buffer = NULL;

  uint32_t remain = 1;
  size_t s;
  bool last_element = false;
  uint8_t group = 0;
  // read until last set of group found:
  while (!last_element && good) {
    uint32_t nitems;
    s = fread_mirror(&nitems, sizeof nitems, 1, self);
    if (s != 1 || nitems == 0) {
      good = false;
    }
    if (good && nitems <= 3) {
      // special case to handle last element
      remain = nitems;
      last_element = true;
      s = fread_mirror(&nitems, sizeof nitems, 1, self);
      if (s != 1 || nitems == 0) {
        good = false;
      }
    }
    // lazy evaluation
    ++group;
    good = good && read_group(self, group, nitems, &info, &data);
  }
  // read remaining groups:
  while (good && --remain != 0) {
    uint32_t nitems;
    s = fread_mirror(&nitems, sizeof nitems, 1, self);
    if (s != 1 || nitems <= 3) {
      good = false;
    }
    ++group;
    good = good && read_group(self, group, nitems, &info, &data);
  }
  // release memory:
  free(data.buffer);
  iconv_close(self->conv);
  if (!good)
    return false;

  // write trailer:
  if (!write_trailer(self)) {
    return false;
  }

  // make sure the whole input was processed:
  assert(self->in->cur <= self->in->end); // programmer error
  if (self->in->cur < self->in->end) {
    return false;
  }
  return true;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "missing arg\n");
    return 1;
  }
  const char *infilename = argv[1];

  size_t buf_len = file_size(infilename);
  size_t n;

  FILE *in = fopen(infilename, "rb");
  void *inbuffer = malloc(buf_len);
  n = fread(inbuffer, 1, buf_len, in);
  fclose(in);
  int ret = 0;
  if (n == buf_len) {
    if (!mec_mr3_print(inbuffer, buf_len)) {
      ret = 1;
    }
  }
  free(inbuffer);

  return ret;
}
