#ifndef _H_HEXCOM_SIMPLEXNOISE
#define _H_HEXCOM_SIMPLEXNOISE

struct SimplexNoise {  // Simplex noise in 2D

  SimplexNoise(int seed);
  double noise2(double x, double y);
  double noise3(double x, double y, double z);
  double noise4(double x, double y, double z, double w);

private:
  int seed;
  unsigned mix[256];
  short get_perm(int k);
  short get_permMod12(int k);
};


#endif /* _H_HEXCOM_SIMPLEXNOISE */

