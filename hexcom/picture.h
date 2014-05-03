#ifndef _H_HEXCOM_PICTURE
#define _H_HEXCOM_PICTURE

#include <stdint.h>

struct Pixel {
  uint16_t      r, g, b, a;
};

/**
 *  Represents an image loaded into memory
 */
struct Picture {
  unsigned width;
  unsigned height;
  uint16_t *data;

  uint16_t *red_plane() const { return &data[0]; }
  uint16_t *green_plane() const { return &data[width*height]; }
  uint16_t *blue_plane() const { return &data[2*width*height]; }
  uint16_t *alpha_plane() const { return &data[3*width*height]; }

  static Picture *load_png(const char *path);
  Pixel get_pixel(int x, int y);
};

#endif /* _H_HEXCOM_PICTURE */
