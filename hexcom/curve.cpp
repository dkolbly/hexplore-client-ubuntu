#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "curve.h"

Curve *Curve::build(double origin, double limit, size_t len, float *y)
{
  Curve *c = (Curve *)malloc(sizeof(Curve) + sizeof(float) * len);
  c->origin = origin;
  // the scale is what we multiply (x-origin) by in order
  // to get the offset
  double dx = (limit - origin)/(double)(len-1);
  c->scale = 1.0/dx;
  
  size_t n = len-1;
  c->len = n;

  for (size_t i=0; i<=n; i++) {
    c->data[i] = y[i];
  }
  return c;
}
