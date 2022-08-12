#include "mec_mr3_io.h"

#include <stdio.h>
#include <stdlib.h>

static size_t file_size(const char *filename) {
  FILE *f = fopen(filename, "rb");
  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fclose(f);
  return fsize;
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
