#ifndef _H_HEXCOM_RANDOMPIXEL
#define _H_HEXCOM_RANDOMPIXEL

#include "picture.h"

struct RandomFromDensityField {
  RandomFromDensityField(Picture const& pic);
  double find(double rand, unsigned *x, unsigned *y);
  uint64_t last;
  uint64_t *cumulator;
  unsigned width, height;
};

#endif /* _H_HEXCOM_RANDOMPIXEL */
