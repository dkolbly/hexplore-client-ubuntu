#include <math.h>
#include <glm/glm.hpp>
#include "hex.h"

void convert_xy_to_hex(double x, double y, int *ixp, int *iyp)
{
  static const bool verbose = false;

  int iy = floor(y / y_stride);
  double adjusted_x = x;
  if (iy & 1) {
    adjusted_x -= x_stride/2;
  }
  int ix = floor(adjusted_x / x_stride);

  dvec2 estimated_origin(hex_x(ix,iy), hex_y(ix,iy));
  dvec2 p = dvec2(x,y) - estimated_origin;
  if (verbose) {
    printf("estimate for (%.3f, %.3f)\n", x, y);
    printf("          is (%d,%d) whose origin is (%.3f,%.3f) residual (%.3f, %.3f)\n",
           ix, iy,
           estimated_origin[0], estimated_origin[1],
           p[0], p[1]);
  }
  assert( p[1] >= 0 );          // we should never have a residual below y=0
  assert( p[1] <= y_stride );   // and it should be less than the y_stride
  assert( p[0] >= 0 );          // horizontally, we should be spot on
  assert( p[0] <= x_stride );   // horizontally, we should be spot on

  if (p[1] > edge) {
    if (p[0] <= x_stride/2) {
      // check for above the left cut-line
      double cut = edge + p[0] * (a / (x_stride/2));
      if (verbose) {
        printf("   checking for above left cut line of %.3f\n", cut);
      }
      if (p[1] > cut) {
        if (verbose) {
          printf("        ABOVE LEFT CUT\n");
        }
        hex_nw(&ix, &iy);
      }
    } else {
      // check for above the right cut-line
      double cut = edge + (x_stride - p[0]) * (a / (x_stride/2));
      if (verbose) {
        printf("   checking for above right cut line of %.3f\n", cut);
      }
      if (p[1] > cut) {
        if (verbose) {
          printf("        ABOVE RIGHT CUT\n");
        }
        hex_ne(&ix, &iy);
      }
    }
  }
  *ixp = ix;
  *iyp = iy;
}


struct HexEdge hex_edges[6];

struct HexEdgeBuilder {
  HexEdgeBuilder();
};

static inline dvec2 angle(double a)
{
  return dvec2(cos(DEG_TO_RAD(a)),
               sin(DEG_TO_RAD(a)));
}

HexEdgeBuilder::HexEdgeBuilder() {
  double x0 = 0;
  double x1 = x_stride/2;
  double x2 = x_stride;

  double y0 = -a;
  double y1 = 0;
  double y2 = edge;
  double y3 = edge + a;

  // note that we configure the directions such that a unit distance
  // is the length of an edge; hence, if the intersection parameter is
  // in the range 0-1 then it intersects the edge segment
  hex_edges[0].online = dvec2(x0, y1);
  hex_edges[0].dir = angle(-30) * edge;

  hex_edges[1].online = dvec2(x1, y0);
  hex_edges[1].dir = angle(30) * edge;

  hex_edges[2].online = dvec2(x2, y1);
  hex_edges[2].dir = angle(90) * edge;

  hex_edges[3].online = dvec2(x2, y2);
  hex_edges[3].dir = angle(150) * edge;

  hex_edges[4].online = dvec2(x1, y3);
  hex_edges[4].dir = angle(210) * edge;

  hex_edges[5].online = dvec2(x0, y2);
  hex_edges[5].dir = angle(270) * edge;
}

HexEdgeBuilder __heb;

void convert_xyz_to_hex(glm::vec3 const &loc, int *ixp, int *iyp, int *izp)
{
  convert_xy_to_hex(loc[0], loc[1], ixp, iyp);
  *izp = round(loc[2]/z_scale);
}

