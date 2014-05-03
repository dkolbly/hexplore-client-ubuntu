#ifndef _H_HEXCOM_HEX
#define _H_HEXCOM_HEX

#include <math.h>
#include "hextypes.h"

#define PI (M_PI)       // deprecated... use M_PI

#define DEG_TO_RAD(x)  (x * (M_PI/180.0))
#define RAD_TO_DEG(x)  (x * (180.0/M_PI))

static const double z_scale = 13.0/128;

static const double x_stride = 1.0;
static const double a = x_stride/sqrt(12.0);
static const double edge = 2*a;
static const double y_stride = 1.5 * edge;

static const double uv_scale = 1.0 / (4*a);

void convert_xy_to_hex(double x, double y, int *ixp, int *iyp);
void convert_xyz_to_hex(glm::vec3 const &loc, int *ixp, int *iyp, int *izp);

/* these are the origin functions */
static inline double hex_x(int x, int y)
{
  double wx = x * x_stride;
  return (y & 1) ? (wx + x_stride/2) : wx;
}

static inline double hex_y(int x, int y)
{
  return y * y_stride;
}

/* location of hex center, relative to what's returned
   by hex_x() and hex_y()
*/

static inline double hex_center_x(int x, int y)
{
  double wx = x * x_stride;
  return (y & 1) ? (wx + x_stride) : (wx + x_stride/2);
}

static inline double hex_center_y(int x, int y)
{
  return y * y_stride + a;
}

static inline void hex_se(int *x, int *y)
{
  if (*y & 1) { (*x)++; }
  (*y)--;
}

static inline void hex_sw(int *x, int *y)
{
  if (!(*y & 1)) { (*x)--; }
  (*y)--;
}

static inline void hex_ne(int *x, int *y)
{
  if (*y & 1) { (*x)++; }
  (*y)++;
}

static inline void hex_nw(int *x, int *y)
{
  if (!(*y & 1)) { (*x)--; }
  (*y)++;
}

static inline void hex_w(int *x, int *y)
{
  (*x)--;
}

static inline void hex_e(int *x, int *y)
{
  (*x)++;
}

/**
 *  Grand unified neighbor-finder
 */
static inline void hex_neighbor(int n, int *x, int *y)
{
  switch (n) {
  case 0: hex_sw(x, y); break;
  case 1: hex_se(x, y); break;
  case 2: hex_e(x, y); break;
  case 3: hex_ne(x, y); break;
  case 4: hex_nw(x, y); break;
  case 5: hex_w(x, y); break;
  }
}

struct HexEdge {
  dvec2   online;
  dvec2   dir;
};

extern struct HexEdge hex_edges[6];

/**
 *   These coefficients determine how big the world "appears". 
 *
 *   The world is infinite and flat, but as you walk East/West your
 *   longitude changes.  This determines how fast your longitude, and
 *   is therefore essentially determines the size of a time zone.
 *
 *   2*PI is one day's worth of time zone, so if this is set to
 *   2*PI/1000 then it takes 1000 units in world space (recall the
 *   x_stride is defined as 1.0, so 1000 units in world space
 *   corresponds to 1000 hexes in the E/W direction) to go all the
 *   way around the world.
 *
 *   Given the current settings for the length of a day and running
 *   speed, the value of 30,000 is not quite enough to run as fast
 *   as the world "turns".
 *
 *   Similarly, although the sun is always directly overhead, solar
 *   flux, nominal weather, and moon positions are determined by your
 *   latitude.
 */

const float longitude_radians_per_x = 2*M_PI / 30000.0;

const float latitude_radians_per_y = 2*M_PI / 3000.0;

struct Calendar {
  double        cal_year;       // year
  float         cal_toy;        // time of year (0=Jan 1, ~1=Dec 31)
  float         cal_tod;        // time of day (0=midnight, 0.5=noon)
};


#endif /* _H_HEXCOM_HEX */
