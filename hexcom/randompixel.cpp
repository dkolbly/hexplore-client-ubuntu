#include <stdlib.h>
#include <stdio.h>
#include "randompixel.h"

RandomFromDensityField::RandomFromDensityField(Picture const& pic)
{
  width = pic.width;
  height = pic.height;
  cumulator = (uint64_t*)malloc(sizeof(uint64_t)*(1 + pic.width * pic.height));

  uint16_t *src = pic.red_plane();
  uint64_t a = 0, *dst = cumulator;

  for (unsigned y=0; y<pic.height; y++) {
    for (unsigned x=0; x<pic.width; x++) {
      *dst++ = a;
      uint16_t red = *src++;
      if (red) {
        a += red;
      } else {
        a += 1;
      }
    }
  }
  *dst++ = a;
  last = a;
  printf("RDF %u x %u ==> %u last value %lu\n", 
         width,
         height,
         width*height,
         a);
}

double RandomFromDensityField::find(double rand, unsigned *x, unsigned *y)
{
  if (rand < 0) {
    return -1;
  }
  uint64_t k = rand * last;
  if (k > last) {
    return -1;
  }
  unsigned low = 0;
  unsigned high = width*height;
  while (high > (low+1)) {
    unsigned mid = (low + high) / 2;
    /*printf("  low %4u   -- mid %4u --  high %u\n", low, mid, high);*/
    if (k < cumulator[mid]) {
      high = mid;
    } else {
      low = mid;
    }
  }
  *x = low % width;
  *y = low / width;
  return (k - cumulator[low]) / (double)(cumulator[low+1] - cumulator[low]);
}


#if 0
int main(int argc, char *argv[])
{
  Picture *p = Picture::load_png(argv[1]);
  RandomFromDensityField f(*p);
  for (int i=2; i<argc; i++) {
    double u = atof(argv[i]);
    unsigned x, y;
    double r = f.find(u, &x, &y);
    printf(" %.3f ==> %u %u  %.3f\n", u, x, y, r);
  }
}
#endif
