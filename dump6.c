#include "mec_mr3.h"

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
  if (argc < 3) {
    fprintf(stderr, "missing arg\n");
    return 1;
  }
  const char *infilename = argv[1];
  const char *outfilename = argv[2];

  size_t buf_len = file_size(infilename);
  size_t n;

  FILE *in = fopen(infilename, "rb");
  void *inbuffer = malloc(buf_len);
  void *outbuffer = malloc(buf_len);
  n = fread(inbuffer, 1, buf_len, in);
  fclose(in);
  int ret = 1;
  if (n == buf_len) {
    outbuffer = mec_mr3_memcpy(outbuffer, inbuffer, buf_len);
    if (outbuffer) {
      FILE *out = fopen(outfilename, "wb");
      n = fwrite(outbuffer, 1, buf_len, out);
      fclose(out);
      ret = n == buf_len ? 0 : 1;
    }
  }
  free(inbuffer);
  free(outbuffer);

  return ret;
}
