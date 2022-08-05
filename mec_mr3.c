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

#define ERROR_RETURN(X, Y) \
  if ((X) != (Y)) return false

static size_t fread_mirror(void *ptr, size_t size, size_t nmemb,
                           struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb) return nmemb;
  }
  return 0;
}

static size_t fread_mirror_clean(void *ptr, size_t size, size_t nmemb,
                                 struct app *self) {
  struct stream *instream = self->in;
  struct stream *outstream = self->out;

  if (nmemb != 64) return 0;
  size_t s = instream->read(ptr, size, nmemb, instream);
  if (s == nmemb) {
    char *str = (char *)ptr;
    const size_t len = strnlen(str, nmemb);
    assert(len < nmemb);
    size_t i;
    for (i = len; i < nmemb; ++i) {
      str[i] = 0;
    }
    s = outstream->write(ptr, size, nmemb, outstream);
    if (s == nmemb) return nmemb;
  }
  return 0;
}

static bool read_magic(struct app *self) { return true; }

static bool write_trailer(struct app *self) {
  assert(self->in->cur <= self->in->end);
  if (self->in->cur == self->in->end) return true;
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

static bool read_data(struct app *self, struct mec_mr3_item_data *data) {
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
  s = fread_mirror(data->buffer, 1, data->len, self);
  ERROR_RETURN(s, data->len);

  return true;
}

static bool read_group(struct app *self, uint32_t nitems,
                       struct mec_mr3_info *info,
                       struct mec_mr3_item_data *data) {
  uint32_t i;
  size_t s;
  for (i = 0; i < nitems; ++i) {
    read_info(self, info);
    read_data(self, data);
  }
  return true;
}

#undef ERROR_RETURN

static bool mec_mr3_scrub(void *output, const void *input, size_t len) {
  if (!input || !output) return false;
  struct stream sin;
  struct stream sout;
  struct app a;
  struct app *self = create_app(&a, &sin, &sout);
  setup_buffer(self, output, input, len);
  if (!read_magic(self)) return false;

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
  if (!good) return false;

  // write trailer:
  if (!write_trailer(self)) {
    return false;
  }

  // make sure the whole input was processed:
  assert(self->in->cur <= self->in->end);
  if (self->in->cur < self->in->end) {
    return false;
  }
  assert(self->out->cur == self->out->end);  // programmer error
  return true;
}

void *mec_mr3_memcpy(void *dest, const void *src, size_t n) {
  const bool b = mec_mr3_scrub(dest, src, n);
  return b ? dest : NULL;
}
