#include "mec_mr3_dict.h"

#include <assert.h>
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
};

static struct app *create_app(struct app *self, struct stream *in) {
  self->in = in;

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

static bool read_info(struct app *self, struct mec_mr3_info *info) {
  // read key and type at once:
  size_t s = fread_mirror(info, sizeof *info, 1, self);
  ERROR_RETURN(s, 1);
  ERROR_RETURN(info->key & 0xfff00000, 0x0);
  ERROR_RETURN(info->type & 0x00ff, 0x0);
  const uint32_t sign = info->type >> 24;
  ERROR_RETURN(sign == 0x0 || sign == 0xff, true);

  return true;
}

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
  if (data->len != 0 && data->buffer == NULL) {
    return false;
  }

  s = fread_mirror(data->buffer, 1, data->len, self);
  ERROR_RETURN(s, data->len);

  return true;
}

static bool read_group(struct app *self, uint32_t nitems,
                       struct mec_mr3_info *info,
                       struct mec_mr3_item_data *data) {
  bool good = true;
  uint32_t i;
  for (i = 0; i < nitems && good; ++i) {
    good = good && read_info(self, info);
    // lazy evaluation:
    good = good && read_data(self, info, data);
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
    good = good && read_group(self, nitems, &info, &data);
  }
  // read remaining groups:
  while (good && --remain != 0) {
    uint32_t nitems;
    s = fread_mirror(&nitems, sizeof nitems, 1, self);
    if (s != 1 || nitems <= 3) {
      good = false;
    }
    good = good && read_group(self, nitems, &info, &data);
  }
  // release memory:
  free(data.buffer);
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
  free(inbuffer);

  return 0;
}
