#include "picture.h"
#include <png.h>
#include <stdlib.h>

static void hc_png_error_fn(png_structp png, const char *msg)
{
  fprintf(stderr, "png error: %s\n", msg);
  abort();
}

static void hc_png_warning_fn(png_structp png, const char *msg)
{
  fprintf(stderr, "png warning: %s\n", msg);
}

Picture *Picture::load_png(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f) {
    perror(path);
    return NULL;
  }
  unsigned char header[8];
  int n = fread(&header[0], 1, sizeof(header), f);
  if (n < (int)sizeof(header)) {
    fprintf(stderr, "%s: fread() failure\n", path);
    fclose(f);
    return NULL;
  }
  if (png_sig_cmp(&header[0], 0, n) != 0) {
    fprintf(stderr, "%s: png_sig_cmp() failure\n", path);
    fclose(f);
    return NULL;
  }

  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 
                                               (void *)NULL,
                                               hc_png_error_fn, 
                                               hc_png_warning_fn);

  if (!png_ptr) {
    fprintf(stderr, "%s: png_create_read_struct() failure\n", path);
    fclose(f);
    return NULL;
  }
  png_infop info = png_create_info_struct(png_ptr);
  if (!info) {
    fprintf(stderr, "%s: png_create_info_struct() failure\n", path);
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(f);
    return NULL;
  }
  
  /* not in libpng12...
  if (setjmp(png_jmpbuf(png_ptr))) {
    // error path
    fprintf(stderr, "%s: error path\n", path);
    png_destroy_read_struct(&png_ptr, &info, NULL);
    fclose(f);
    return NULL;
  }
  */
  
  png_init_io(png_ptr, f);
  png_set_sig_bytes(png_ptr, n);

  // libpng12 (ubuntu 13.10) does not have PNG_TRANSFORM_{SCALE,EXPAND}_16 
  // so we implement it ourselves
  png_read_png(png_ptr, info, 
               PNG_TRANSFORM_EXPAND
               | PNG_TRANSFORM_BGR
               | PNG_TRANSFORM_GRAY_TO_RGB,
               NULL);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type,
    compression_type, filter_method;
  
  png_get_IHDR(png_ptr, 
               info,
               &width,
               &height,
               &bit_depth,
               &color_type,
               &interlace_type,
               &compression_type,
               &filter_method);

  printf("%s: %u x %u (%d bits), ", path, width, height, bit_depth);
  printf("mode=%d (%s)\n", color_type, (color_type & PNG_COLOR_MASK_ALPHA) ? "has alpha" : "no alpha");

  if (!((bit_depth == 8) || (bit_depth == 16))) {
    abort();
  }

  unsigned char **rows = png_get_rows(png_ptr, info);
  unsigned pixels = width*height;
  uint16_t *data = (uint16_t *)malloc(sizeof(uint16_t)*4*pixels);
  uint16_t *red_plane = &data[0];
  uint16_t *green_plane = &data[pixels];
  uint16_t *blue_plane = &data[2*pixels];
  uint16_t *alpha_plane = &data[3*pixels];

  if (bit_depth == 8) {
    unsigned i = 0;
    for (unsigned y=0; y<height; y++) {
      uint8_t *r = (uint8_t *)rows[y];
      for (unsigned x=0; x<width; x++) {
        if (color_type & PNG_COLOR_MASK_ALPHA) {
          blue_plane[i] = r[4*x+0] * 0x101;
          green_plane[i] = r[4*x+1] * 0x101;
          red_plane[i] = r[4*x+2] * 0x101;
          alpha_plane[i] = r[4*x+3] * 0x101;
        } else {
          blue_plane[i] = r[3*x+0] * 0x101;
          green_plane[i] = r[3*x+1] * 0x101;
          red_plane[i] = r[3*x+2] * 0x101;
          alpha_plane[i] = 0xFFFF;
        }
        i++;
      }
    }
  } else {
    unsigned i = 0;
    for (unsigned y=0; y<height; y++) {
      uint16_t *r = (uint16_t *)rows[y];
      for (unsigned x=0; x<width; x++) {
        if (color_type & PNG_COLOR_MASK_ALPHA) {
          red_plane[i] = r[4*x+0];
          green_plane[i] = r[4*x+1];
          blue_plane[i] = r[4*x+2];
          alpha_plane[i] = r[4*x+3];
        } else {
          red_plane[i] = r[3*x+0];
          green_plane[i] = r[3*x+1];
          blue_plane[i] = r[3*x+2];
          alpha_plane[i] = 0xFFFF;
        }
        i++;
      }
    }
  }

  png_destroy_read_struct(&png_ptr, &info, NULL);
  fclose(f);

  Picture *p = new Picture();
  p->width = width;
  p->height = height;
  p->data = data;
  return p;
}

Pixel Picture::get_pixel(int x, int y)
{
  if ((x < 0) || ((unsigned)x >= width)) {
    return (Pixel){.r=0, .g=0, .b=0, .a=0};
  }
  if ((y < 0) || ((unsigned)y >= height)) {
    return (Pixel){.r=0, .g=0, .b=0, .a=0};
  }

  Pixel p;
  unsigned index = x + y *height;
  p.r = red_plane()[index];
  p.g = green_plane()[index];
  p.b = blue_plane()[index];
  p.a = alpha_plane()[index];
  return p;
}
