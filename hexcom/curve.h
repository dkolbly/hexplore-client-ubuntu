#ifndef _H_HEXCOM_CURVE
#define _H_HEXCOM_CURVE

static inline float interpolate(double dx, float y0, float y1)
{
  return y0 * (1-dx) + y1 * dx;
}

struct Curve {
  /**
   *   len - the number of samples; the last sample (i.e., at y[len-1])
   *         is the y value when x==limit.  The first sample (at y[0])
   *         is the y value when x==origin.  Notice the fencepost effect;
   *         the size of each interval is (limit-origin)/(len-1).
   */
  static Curve *build(double origin, double limit, size_t len, float *y);

  inline double get(double x) {
    x = (x - origin) * scale;
    int ix = x;
    if (ix < 0) {
      return data[0];
    }
    if (ix >= len) {
      return data[len];
    }

    double dx = x - ix;
    if (dx < 0) {
      return data[0];
    }
    //printf("x=%.4f ix=%d  dx=%.4f\n", x, ix, dx);
    return data[ix] + dx * (data[ix+1] - data[ix]);
  }

private:
  Curve();

  double        origin;
  double        scale;
  int           len;
  float         data[];
};

#endif /* _H_HEXCOM_CURVE */
