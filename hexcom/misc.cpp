#include "misc.h"

std::vector<uint8_t> *load_file(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f) {
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  size_t n = ftell(f);
  std::vector<uint8_t> *buf = new std::vector<uint8_t>(n+1);
  fseek(f, 0, SEEK_SET);
  fread(buf->data(), 1, n, f);
  fclose(f);
  (*buf)[n] = '\0';
  return buf;
}
