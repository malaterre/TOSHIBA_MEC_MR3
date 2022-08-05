/*=========================================================================

  Program: GDCM (Grassroots DICOM). A DICOM library

  Copyright (c) 2006-2011 Mathieu Malaterre
  All rights reserved.
  See Copyright.txt or http://gdcm.sourceforge.net/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h> /* realloc */
#include <string.h> /* memcpy */

struct stream {
  const void *start;
  const void *end;
  void *cur;
  size_t (*read)(void *ptr, size_t size, size_t nmemb, struct stream *in);
  size_t (*write)(const void *ptr, size_t size, size_t nmemb,
                  struct stream *outstream);
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

static size_t stream_write(const void *ptr, size_t size, size_t nmemb,
                           struct stream *out) {
  char *cur = (char *)out->cur;
  const char *end = (const char *)out->end;
  const size_t len = size * nmemb;
  if (cur + len <= end) {
    memcpy(cur, ptr, len);
    out->cur = cur + len;
  } else {
    out->cur = NULL;
    return 0;
  }
  return nmemb;
}

enum CSA_TYPE { INVALID = 0, NOMAGIC = 1, SV10 = 2 };

struct app {
  struct stream *in;
  struct stream *out;
};

static struct app *create_app(struct app *self, struct stream *in,
                              struct stream *out) {
  self->in = in;
  self->out = out;

  return self;
}

static void setup_buffer(struct app *self, void *output, const void *input,
                         size_t len) {
  self->in->cur = (char *)input;
  self->in->start = input;
  self->in->end = (char *)input + len;
  self->in->read = stream_read;
  self->out->cur = output;
  self->out->start = output;
  self->out->end = (char *)output + len;
  self->out->write = stream_write;
}

#define ERROR_RETURN(X, Y)                                                     \
  if ((X) != (Y))                                                              \
  return false

static size_t fread_mirror(void *ptr, size_t size, size_t nmemb,
                           struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb)
      return nmemb;
  }
  return 0;
}

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

static size_t fread_mirror_clean_iso(void *ptr, size_t size, size_t nmemb,
                                     struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    static const char magic[] = {0xdf, 0xff, 0x79};
    if (nmemb >= sizeof magic && memcmp(ptr, magic, sizeof(magic)) == 0) {
      // iso
      struct buffer19 b19;
      assert(sizeof b19 == 19);
      assert(nmemb > sizeof b19);
      memcpy(&b19, ptr, sizeof b19);
      assert(b19.sig2 == 0x1);
      assert(b19.sig3 == 0x0);
      assert(b19.sig4 == 0x2);
      assert(b19.sig5 == 0x0);
      assert(b19.len2 + 4 == nmemb);
      assert(b19.len3 == 9);
      const size_t diff = nmemb - (7 + 9 + 3);
      assert(b19.len4 == diff);
      assert(strncmp(b19.iso, "ISO8859-1", 9) == 0);
      char *str = (char *)ptr + 7 + 9 + 3;
      size_t i;
      for (i = 0; i < b19.len4; ++i) {
        str[i] = ' ';
      }
    } else {
      // raw string buffer
      assert(0);
    }
    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb)
      return nmemb;
  }
  return 0;
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
struct buffer516 {
  str64 zero;
  char buf2[0x15];
  char buf3[0x100]; // phi
  str16 buf4;
  str64 buf5;
  str64 buf6;
  uint32_t bools[6];
};
struct buffer325 {
  str64 array[5];
};

static size_t fread_mirror_clean_buffer(void *ptr, size_t size, size_t nmemb,
                                        struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    if (s == 436) {
      struct buffer436 b436;
      assert(sizeof b436 == 436);
      memcpy(&b436, ptr, nmemb);

      char *str = b436.buf3;
      const size_t len = strnlen(str, sizeof b436.buf3);
      size_t i;
      for (i = 0; i < len; ++i) {
        str[i] = ' ';
      }
      memcpy(ptr, &b436, nmemb);
    } else if (s == 516) {
      struct buffer516 b516;
      assert(sizeof b516 == 516);
      memcpy(&b516, ptr, nmemb);

      char *str = b516.buf3;
      const size_t len = strnlen(str, sizeof b516.buf3);
      size_t i;
      for (i = 0; i < len; ++i) {
        str[i] = ' ';
      }
      memcpy(ptr, &b516, nmemb);
    } else if (s == 325) {
      struct buffer325 b325;
      assert(sizeof b325 == 325);
      memcpy(&b325, ptr, nmemb);
      int a;
      for (a = 0; a < 5; ++a) {
        char *str = b325.array[a];
        const size_t len = strnlen(str, sizeof b325.array[a]);
        size_t i;
        for (i = 0; i < len; ++i) {
          str[i] = ' ';
        }
      }
      memcpy(ptr, &b325, nmemb);
    } else {
      assert(0);
      return 0;
    }
    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb)
      return nmemb;
  }
  return 0;
}

static size_t fread_mirror_clean_shift_jis(void *ptr, size_t size, size_t nmemb,
                                           struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    char *str = ptr;
    const size_t len = strnlen(str, nmemb);
    size_t i;
    for (i = 0; i < len; ++i) {
      str[i] = ' ';
    }

    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb)
      return nmemb;
  }
  return 0;
}

static bool read_magic(struct app *self) { return true; }

static bool write_trailer(struct app *self) {
  assert(self->in->cur <= self->in->end);
  if (self->in->cur == self->in->end)
    return true;
  // else it is missing one byte (nul byte):
  char padding;
  size_t s = fread_mirror(&padding, 1, sizeof padding, self);
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

static bool read_info(struct app *self, struct mec_mr3_info *info) {
  // read key and type at once:
  size_t s = fread_mirror(info, sizeof *info, 1, self);
  ERROR_RETURN(s, 1);
  ERROR_RETURN(info->type & 0x00ff, 0x0);

  return true;
}

static const uint32_t with_phi[] = {
    0x000055f2, /* !!!charset!!! */
    0x000055f3, /* */
    0x000055fc, /* */
    0x0000560c, /* !!!charset!!! */
    0x0000560d, /* */
    0x00005612, /* */
    0x00006d77, /* */
    0x00006d80, /* buffer */
    0x00006d83, /* buffer */
    0x00006d8a, /* */
};

static inline bool key_is_phi(const uint32_t val) {
  int i;
  for (i = 0; i < sizeof(with_phi) / sizeof(*with_phi); i++) {
    if (with_phi[i] == val)
      return true;
  }
  return false;
}

enum Type {
  ISO_8859_1_STRING =
      0x00000300, // ASCII string / or struct with 'ISO-8859-1 :' identifier
  BUFFER_436 = 0x001f4300, // Fixed buffer 436 bytes (struct with ASCII strings)
  BUFFER_516 = 0x001f4400, // Fixed buffer 516 bytes (struct with ASCII strings)
  BUFFER_325 = 0x001f4600, // Fixed buffer 325 bytes (struct with ASCII strings)
  SHIFT_JIS_STRING = 0xff002c00, // SHIFT-JIS strings
};

static bool read_data(struct app *self, const struct mec_mr3_info *info,
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

  if (key_is_phi(info->key)) {
    // found a key indicating potential phi
    switch (info->type) {
      // clean string depending on its type:
    case ISO_8859_1_STRING:
      s = fread_mirror_clean_iso(data->buffer, 1, data->len, self);
      break;
    case BUFFER_436:
    case BUFFER_516:
    case BUFFER_325:
      s = fread_mirror_clean_buffer(data->buffer, 1, data->len, self);
      break;
    case SHIFT_JIS_STRING:
      s = fread_mirror_clean_shift_jis(data->buffer, 1, data->len, self);
      break;
    default:
      assert(0);
      return false;
    }
  } else {
    s = fread_mirror(data->buffer, 1, data->len, self);
  }
  ERROR_RETURN(s, data->len);

  return true;
}

static bool read_group(struct app *self, uint32_t nitems,
                       struct mec_mr3_info *info,
                       struct mec_mr3_item_data *data) {
  uint32_t i;
  for (i = 0; i < nitems; ++i) {
    read_info(self, info);
    read_data(self, info, data);
  }
  return true;
}

#undef ERROR_RETURN

static bool mec_mr3_scrub(void *output, const void *input, size_t len) {
  if (!input || !output)
    return false;
  struct stream sin;
  struct stream sout;
  struct app a;
  struct app *self = create_app(&a, &sin, &sout);
  setup_buffer(self, output, input, len);
  if (!read_magic(self))
    return false;

  bool good = true;
  struct mec_mr3_info info;
  struct mec_mr3_item_data data;
  data.len = 0;
  data.buffer = NULL;

  uint32_t remain = 0;
  size_t s;
  bool last_element = false;
  while (!last_element) {
    uint32_t nitems;
    s = fread_mirror(&nitems, sizeof nitems, 1, self);
    assert(nitems > 0);
    if (nitems <= 3) {
      // special case to handle last element
      remain = nitems;
      last_element = true;
      fread_mirror(&nitems, 1, sizeof nitems, self);
    }
    read_group(self, nitems, &info, &data);
  }
  while (--remain != 0) {
    uint32_t nitems;
    s = fread_mirror(&nitems, sizeof nitems, 1, self);
    assert(nitems > 3);
    read_group(self, nitems, &info, &data);
  }
  if (!good)
    return false;

  // write trailer:
  if (!write_trailer(self)) {
    return false;
  }

  // make sure the whole input was processed:
  assert(self->in->cur <= self->in->end);
  if (self->in->cur < self->in->end) {
    return false;
  }
  assert(self->out->cur == self->out->end); // programmer error
  return true;
}

void *mec_mr3_memcpy(void *dest, const void *src, size_t n) {
  const bool b = mec_mr3_scrub(dest, src, n);
  return b ? dest : NULL;
}
