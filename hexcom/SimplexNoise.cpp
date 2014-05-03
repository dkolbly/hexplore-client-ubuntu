/*
 *  Based on the Java implementation of SimplexNoise
 *  by Stefan Gustavson
 *
 *  obtained from
 *  http://webstaff.itn.liu.se/~stegu/simplexnoise/SimplexNoise.java
 *  on 2014-02-16
 *
 *  and described in
 *  http://staffwww.itn.liu.se/~stegu/simplexnoise/simplexnoise.pdf
 */

#include <glm/glm.hpp>
#include <string.h>
#include "SimplexNoise.h"

/*
 * A speed-improved simplex noise algorithm for 2D, 3D and 4D in Java.
 *
 * Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * Better rank ordering method by Stefan Gustavson in 2012.
 *
 * This could be speeded up even further, but it's useful as it is.
 *
 * Version 2012-03-09
 *
 * This code was placed in the public domain by its original author,
 * Stefan Gustavson. You may use it as you see fit, but
 * attribution is appreciated.
 *
 */

struct Grad2 {
  glm::dvec2 g;

  Grad2(double dx, double dy)
    : g(dx, dy) {
  }
};

struct Grad3 {
  glm::dvec3 g;

  Grad3(double dx, double dy, double dz)
    : g(dx, dy, dz) {
  }
};

struct Grad4 {
  glm::dvec4 g;

  Grad4(double dx, double dy, double dz, double dw)
    : g(dx, dy, dz, dw) {
  }
};
    

static Grad3 grad3[] = {Grad3(1,1,0),
                        Grad3(-1,1,0),
                        Grad3(1,-1,0),
                        Grad3(-1,-1,0),

                        Grad3(1,0,1),
                        Grad3(-1,0,1),
                        Grad3(1,0,-1),
                        Grad3(-1,0,-1),

                        Grad3(0,1,1),
                        Grad3(0,-1,1),
                        Grad3(0,1,-1),
                        Grad3(0,-1,-1)};



static Grad4 grad4[]= {Grad4(0,1,1,1),Grad4(0,1,1,-1),Grad4(0,1,-1,1),Grad4(0,1,-1,-1),
                   Grad4(0,-1,1,1),Grad4(0,-1,1,-1),Grad4(0,-1,-1,1),Grad4(0,-1,-1,-1),
                   Grad4(1,0,1,1),Grad4(1,0,1,-1),Grad4(1,0,-1,1),Grad4(1,0,-1,-1),
                   Grad4(-1,0,1,1),Grad4(-1,0,1,-1),Grad4(-1,0,-1,1),Grad4(-1,0,-1,-1),
                   Grad4(1,1,0,1),Grad4(1,1,0,-1),Grad4(1,-1,0,1),Grad4(1,-1,0,-1),
                   Grad4(-1,1,0,1),Grad4(-1,1,0,-1),Grad4(-1,-1,0,1),Grad4(-1,-1,0,-1),
                   Grad4(1,1,1,0),Grad4(1,1,-1,0),Grad4(1,-1,1,0),Grad4(1,-1,-1,0),
                   Grad4(-1,1,1,0),Grad4(-1,1,-1,0),Grad4(-1,-1,1,0),Grad4(-1,-1,-1,0)};


short SimplexNoise::get_perm(int k)
{
  unsigned r = mix[(k & 0xFF)];
  r += mix[((k >> 8) + r) & 0xFF];
  r += mix[((k >> 16) + r) & 0xFF];
  r += mix[((k >> 24) + r) & 0xFF];
  return (r + (r >> 8) + (r >> 16) + (r >> 24)) & 0xFF;
  /*  srandom(k^seed);
      return random() & 255;*/
}

short SimplexNoise::get_permMod12(int k)
{
  return get_perm(k) % 12;
}

SimplexNoise::SimplexNoise(int _seed)
  : seed(_seed)
{
  srandom(seed);
  for (int i=0; i<256; i++) {
    mix[i] = random();
  }
}

// Skewing and unskewing factors for 2, 3, and 4 dimensions

static const double F2 = 0.5*(sqrt(3.0)-1.0);
static const double G2 = (3.0-sqrt(3.0))/6.0;
static const double F3 = 1.0/3.0;
static const double G3 = 1.0/6.0;
static const double F4 = (sqrt(5.0)-1.0)/4.0;
static const double G4 = (5.0-sqrt(5.0))/20.0;

  // This method is a *lot* faster than using (int)Math.floor(x)
static inline int fastfloor(double x) {
  int xi = (int)x;
  return (x<xi) ? xi-1 : xi;
}

static inline double dot(Grad2 const& g, double x, double y) {
    return g.g.x*x + g.g.y*y; 
}

static inline double dot(Grad3 const& g, double x, double y, double z) {
  return g.g.x*x + g.g.y*y + g.g.z*z; 
}

static inline double dot(Grad3 const& g, double x, double y) {
  return g.g.x*x + g.g.y*y; 
}

static inline double dot(Grad4 const& g, double x, double y, double z, double w) {
  return g.g.x*x + g.g.y*y + g.g.z*z + g.g.w*w; 
}

double SimplexNoise::noise2(double xin, double yin)
{
  double n0, n1, n2; // Noise contributions from the three corners
  // Skew the input space to determine which simplex cell we're in
  double s = (xin+yin)*F2; // Hairy factor for 2D
  int i = fastfloor(xin+s);
  int j = fastfloor(yin+s);
  double t = (i+j)*G2;
  double X0 = i-t; // Unskew the cell origin back to (x,y) space
  double Y0 = j-t;
  double x0 = xin-X0; // The x,y distances from the cell origin
  double y0 = yin-Y0;
  // For the 2D case, the simplex shape is an equilateral triangle.
  // Determine which simplex we are in.
  int i1, j1; // Offsets for second (middle) corner of simplex in (i,j) coords
  if(x0>y0) {i1=1; j1=0;} // lower triangle, XY order: (0,0)->(1,0)->(1,1)
  else {i1=0; j1=1;}      // upper triangle, YX order: (0,0)->(0,1)->(1,1)
  // A step of (1,0) in (i,j) means a step of (1-c,-c) in (x,y), and
  // a step of (0,1) in (i,j) means a step of (-c,1-c) in (x,y), where
  // c = (3-sqrt(3))/6
  double x1 = x0 - i1 + G2; // Offsets for middle corner in (x,y) unskewed coords
  double y1 = y0 - j1 + G2;
  double x2 = x0 - 1.0 + 2.0 * G2; // Offsets for last corner in (x,y) unskewed coords
  double y2 = y0 - 1.0 + 2.0 * G2;
  // Work out the hashed gradient indices of the three simplex corners
  //int ii = i & 255;
  //int jj = j & 255;
  int gi0 = get_permMod12(i+get_perm(j));
  int gi1 = get_permMod12(i+i1+get_perm(j+j1));
  int gi2 = get_permMod12(i+1+get_perm(j+1));
  // Calculate the contribution from the three corners
  double t0 = 0.5 - x0*x0-y0*y0;
  if(t0<0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad3[gi0], x0, y0);  // (x,y) of grad3 used for 2D gradient
  }
  double t1 = 0.5 - x1*x1-y1*y1;
  if(t1<0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad3[gi1], x1, y1);
  }
  double t2 = 0.5 - x2*x2-y2*y2;
  if(t2<0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad3[gi2], x2, y2);
  }
  // Add contributions from each corner to get the final noise value.
  // The result is scaled to return values in the interval [-1,1].
  return 70.0 * (n0 + n1 + n2);
}

// 3D simplex noise
double SimplexNoise::noise3(double xin, double yin, double zin)
{
  double n0, n1, n2, n3; // Noise contributions from the four corners
  // Skew the input space to determine which simplex cell we're in
  double s = (xin+yin+zin)*F3; // Very nice and simple skew factor for 3D
  int i = fastfloor(xin+s);
  int j = fastfloor(yin+s);
  int k = fastfloor(zin+s);
  double t = (i+j+k)*G3;
  double X0 = i-t; // Unskew the cell origin back to (x,y,z) space
  double Y0 = j-t;
  double Z0 = k-t;
  double x0 = xin-X0; // The x,y,z distances from the cell origin
  double y0 = yin-Y0;
  double z0 = zin-Z0;
  // For the 3D case, the simplex shape is a slightly irregular tetrahedron.
  // Determine which simplex we are in.
  int i1, j1, k1; // Offsets for second corner of simplex in (i,j,k) coords
  int i2, j2, k2; // Offsets for third corner of simplex in (i,j,k) coords
  if(x0>=y0) {
    if(y0>=z0)
      { i1=1; j1=0; k1=0; i2=1; j2=1; k2=0; } // X Y Z order
    else if(x0>=z0) { i1=1; j1=0; k1=0; i2=1; j2=0; k2=1; } // X Z Y order
    else { i1=0; j1=0; k1=1; i2=1; j2=0; k2=1; } // Z X Y order
  }
  else { // x0<y0
    if(y0<z0) { i1=0; j1=0; k1=1; i2=0; j2=1; k2=1; } // Z Y X order
    else if(x0<z0) { i1=0; j1=1; k1=0; i2=0; j2=1; k2=1; } // Y Z X order
    else { i1=0; j1=1; k1=0; i2=1; j2=1; k2=0; } // Y X Z order
  }
  // A step of (1,0,0) in (i,j,k) means a step of (1-c,-c,-c) in (x,y,z),
  // a step of (0,1,0) in (i,j,k) means a step of (-c,1-c,-c) in (x,y,z), and
  // a step of (0,0,1) in (i,j,k) means a step of (-c,-c,1-c) in (x,y,z), where
  // c = 1/6.
  double x1 = x0 - i1 + G3; // Offsets for second corner in (x,y,z) coords
  double y1 = y0 - j1 + G3;
  double z1 = z0 - k1 + G3;
  double x2 = x0 - i2 + 2.0*G3; // Offsets for third corner in (x,y,z) coords
  double y2 = y0 - j2 + 2.0*G3;
  double z2 = z0 - k2 + 2.0*G3;
  double x3 = x0 - 1.0 + 3.0*G3; // Offsets for last corner in (x,y,z) coords
  double y3 = y0 - 1.0 + 3.0*G3;
  double z3 = z0 - 1.0 + 3.0*G3;
  // Work out the hashed gradient indices of the four simplex corners
  //int ii = i & 255;
  //int jj = j & 255;
  //int kk = k & 255;
  int gi0 = get_permMod12(i+get_perm(j+get_perm(k)));
  int gi1 = get_permMod12(i+i1+get_perm(j+j1+get_perm(k+k1)));
  int gi2 = get_permMod12(i+i2+get_perm(j+j2+get_perm(k+k2)));
  int gi3 = get_permMod12(i+1+get_perm(j+1+get_perm(k+1)));
  // Calculate the contribution from the four corners
  double t0 = 0.6 - x0*x0 - y0*y0 - z0*z0;
  if(t0<0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad3[gi0], x0, y0, z0);
  }
  double t1 = 0.6 - x1*x1 - y1*y1 - z1*z1;
  if(t1<0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad3[gi1], x1, y1, z1);
  }
  double t2 = 0.6 - x2*x2 - y2*y2 - z2*z2;
  if(t2<0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad3[gi2], x2, y2, z2);
  }
  double t3 = 0.6 - x3*x3 - y3*y3 - z3*z3;
  if(t3<0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3[gi3], x3, y3, z3);
  }
  // Add contributions from each corner to get the final noise value.
  // The result is scaled to stay just inside [-1,1]
  return 32.0*(n0 + n1 + n2 + n3);
}


  // 4D simplex noise, better simplex rank ordering method 2012-03-09
double SimplexNoise::noise4(double x, double y, double z, double w) 
{

  double n0, n1, n2, n3, n4; // Noise contributions from the five corners
  // Skew the (x,y,z,w) space to determine which cell of 24 simplices we're in
  double s = (x + y + z + w) * F4; // Factor for 4D skewing
  int i = fastfloor(x + s);
  int j = fastfloor(y + s);
  int k = fastfloor(z + s);
  int l = fastfloor(w + s);
  double t = (i + j + k + l) * G4; // Factor for 4D unskewing
  double X0 = i - t; // Unskew the cell origin back to (x,y,z,w) space
  double Y0 = j - t;
  double Z0 = k - t;
  double W0 = l - t;
  double x0 = x - X0;  // The x,y,z,w distances from the cell origin
  double y0 = y - Y0;
  double z0 = z - Z0;
  double w0 = w - W0;
  // For the 4D case, the simplex is a 4D shape I won't even try to describe.
  // To find out which of the 24 possible simplices we're in, we need to
  // determine the magnitude ordering of x0, y0, z0 and w0.
  // Six pair-wise comparisons are performed between each possible pair
  // of the four coordinates, and the results are used to rank the numbers.
  int rankx = 0;
  int ranky = 0;
  int rankz = 0;
  int rankw = 0;
  if(x0 > y0) rankx++; else ranky++;
  if(x0 > z0) rankx++; else rankz++;
  if(x0 > w0) rankx++; else rankw++;
  if(y0 > z0) ranky++; else rankz++;
  if(y0 > w0) ranky++; else rankw++;
  if(z0 > w0) rankz++; else rankw++;
  int i1, j1, k1, l1; // The integer offsets for the second simplex corner
  int i2, j2, k2, l2; // The integer offsets for the third simplex corner
  int i3, j3, k3, l3; // The integer offsets for the fourth simplex corner
  // simplex[c] is a 4-vector with the numbers 0, 1, 2 and 3 in some order.
  // Many values of c will never occur, since e.g. x>y>z>w makes x<z, y<w and x<w
  // impossible. Only the 24 indices which have non-zero entries make any sense.
  // We use a thresholding to set the coordinates in turn from the largest magnitude.
  // Rank 3 denotes the largest coordinate.
  i1 = rankx >= 3 ? 1 : 0;
  j1 = ranky >= 3 ? 1 : 0;
  k1 = rankz >= 3 ? 1 : 0;
  l1 = rankw >= 3 ? 1 : 0;
  // Rank 2 denotes the second largest coordinate.
  i2 = rankx >= 2 ? 1 : 0;
  j2 = ranky >= 2 ? 1 : 0;
  k2 = rankz >= 2 ? 1 : 0;
  l2 = rankw >= 2 ? 1 : 0;
  // Rank 1 denotes the second smallest coordinate.
  i3 = rankx >= 1 ? 1 : 0;
  j3 = ranky >= 1 ? 1 : 0;
  k3 = rankz >= 1 ? 1 : 0;
  l3 = rankw >= 1 ? 1 : 0;
  // The fifth corner has all coordinate offsets = 1, so no need to compute that.
  double x1 = x0 - i1 + G4; // Offsets for second corner in (x,y,z,w) coords
  double y1 = y0 - j1 + G4;
  double z1 = z0 - k1 + G4;
  double w1 = w0 - l1 + G4;
  double x2 = x0 - i2 + 2.0*G4; // Offsets for third corner in (x,y,z,w) coords
  double y2 = y0 - j2 + 2.0*G4;
  double z2 = z0 - k2 + 2.0*G4;
  double w2 = w0 - l2 + 2.0*G4;
  double x3 = x0 - i3 + 3.0*G4; // Offsets for fourth corner in (x,y,z,w) coords
  double y3 = y0 - j3 + 3.0*G4;
  double z3 = z0 - k3 + 3.0*G4;
  double w3 = w0 - l3 + 3.0*G4;
  double x4 = x0 - 1.0 + 4.0*G4; // Offsets for last corner in (x,y,z,w) coords
  double y4 = y0 - 1.0 + 4.0*G4;
  double z4 = z0 - 1.0 + 4.0*G4;
  double w4 = w0 - 1.0 + 4.0*G4;
  // Work out the hashed gradient indices of the five simplex corners
  int ii = i;// & 255;
  int jj = j;// & 255;
  int kk = k;// & 255;
  int ll = l;// & 255;
#define p(n)  get_perm(n)
  int gi0 = p(ii+p(jj+p(kk+p(ll)))) % 32;
  int gi1 = p(ii+i1+p(jj+j1+p(kk+k1+p(ll+l1)))) % 32;
  int gi2 = p(ii+i2+p(jj+j2+p(kk+k2+p(ll+l2)))) % 32;
  int gi3 = p(ii+i3+p(jj+j3+p(kk+k3+p(ll+l3)))) % 32;
  int gi4 = p(ii+1+p(jj+1+p(kk+1+p(ll+1)))) % 32;
#undef p
  // Calculate the contribution from the five corners
  double t0 = 0.6 - x0*x0 - y0*y0 - z0*z0 - w0*w0;
  if(t0<0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad4[gi0], x0, y0, z0, w0);
  }
  double t1 = 0.6 - x1*x1 - y1*y1 - z1*z1 - w1*w1;
  if(t1<0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad4[gi1], x1, y1, z1, w1);
  }
  double t2 = 0.6 - x2*x2 - y2*y2 - z2*z2 - w2*w2;
  if(t2<0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad4[gi2], x2, y2, z2, w2);
  }
  double t3 = 0.6 - x3*x3 - y3*y3 - z3*z3 - w3*w3;
  if(t3<0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad4[gi3], x3, y3, z3, w3);
  }
  double t4 = 0.6 - x4*x4 - y4*y4 - z4*z4 - w4*w4;
  if(t4<0) n4 = 0.0;
  else {
    t4 *= t4;
    n4 = t4 * t4 * dot(grad4[gi4], x4, y4, z4, w4);
  }
  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}

#if UNIT_TEST

// from http://www.cs.rit.edu/~ncs/color/t_convert.html

void HSVtoRGB( float *r, float *g, float *b, float h, float s, float v )
{
  int i;
  float f, p, q, t;
  if( s == 0 ) {
    // achromatic (grey)
    *r = *g = *b = v;
    return;
  }
  h /= 60;			// sector 0 to 5
  i = floor( h );
  f = h - i;			// factorial part of h
  p = v * ( 1 - s );
  q = v * ( 1 - s * f );
  t = v * ( 1 - s * ( 1 - f ) );
  switch( i ) {
  case 0:
    *r = v;
    *g = t;
    *b = p;
    break;
  case 1:
    *r = q;
    *g = v;
    *b = p;
    break;
  case 2:
    *r = p;
    *g = v;
    *b = t;
    break;
  case 3:
    *r = p;
    *g = q;
    *b = v;
    break;
  case 4:
    *r = t;
    *g = p;
    *b = v;
    break;
  default:		// case 5:
    *r = v;
    *g = p;
    *b = q;
    break;
  }
}

int main(int argc, char *argv[])
{
  int seed = (argc == 1) ? 1 : atoi(argv[1]);
  SimplexNoise noise(seed);
  SimplexNoise noise_b(seed^0xFEEDFACE);
  int dx = 0;
  int dy = 0;
  if (argc > 2) {
    dx = atoi(argv[2]);
    dy = atoi(argv[3]);
  }

  // generate a PGM

  printf("P3\n%d %d\n255\n", 500, 500);
  for (int y=0; y<500; y++) {
    for (int x=0; x<500; x++) {
      double spatial_scale = 100;
      double height_scale = 1;
      double sum_height_scale = 0;
      double accum = 0;
      for (int i=0; i<4; i++) {
        double n = noise.noise2((x+dx)/spatial_scale, 
                                (y+dy)/spatial_scale);
        accum += n * height_scale;
        sum_height_scale += height_scale;
        height_scale /= 2.0;
        spatial_scale /= 2.0;
      }

      double h = accum / sum_height_scale;
      double n_b = noise_b.noise2((x+dx)/100.0, (y+dy)/100.0);
      h *= (1-n_b*n_b);
      int z;
      z = 128 + 127*h;
      //int z = 128 + 127 * (n_a + n_b * 0.5 + n_c * 0.25) / (1+0.5+0.25);
      if (z < 0) {
        z = 0;
      } else if (z > 255) {
        z = 255;
      }
      
      {
        float hue = ((h+1)/2.0) * 240;
        float sat = 1;
        float val = 1;
        float r, g, b;
        HSVtoRGB(&r, &g, &b, hue, sat, val);
        printf("  %d %d %d", (int)(r*255), (int)(g*255), (int)(b*255));
      }
      //printf(" %d", z);
    }
    printf("\n");
  }
}
#endif
