#include "pick.h"
#include "hex.h"
#include "region.h"
#include <string.h>
#include "misc.h"

//static const double region_x_stride = x_stride * REGION_SIZE;
//static const double region_y_stride = y_stride * REGION_SIZE;

static const double center_to_edge_distance = x_stride / 2.0;
static const double center_to_vertex_distance = edge;
static const double y_overhang = a;
static const double x_overhang = center_to_edge_distance;

struct RegionPicker : Picker {
  RegionPicker(Region *rgn);
  virtual int pick(glm::vec3 const& origin,
                   glm::vec3 const& direction,
                   double t_min,
                   double t_max,
                   PickPoint *p);
  Region   *rp_region;
};


static int cmp_column_range(const void *_a, const void *_b)
{
  const ColumnPick *a = (const ColumnPick *)_a;
  const ColumnPick *b = (const ColumnPick *)_b;
  if (a->enter_range < b->enter_range) {
    return -1;
  } else {
    return 1;
  }
}


int hexPickColumn(glm::vec3 const& _origin,
                  glm::vec3 const& _direction,
                  int ix, int iy,
                  ColumnPick *cp)
{
  const bool verbose = false;
  dvec2         base(hex_x(ix,iy), hex_y(ix,iy));
  dvec2         direction(_direction[0], _direction[1]);
  dvec2         normdir(-_direction[1], _direction[0]);
  dvec2         origin(_origin[0], _origin[1]);

  int rc;
  unsigned enter_face = 0, exit_face = 0;

  for (int i=0; i<6; i++) {
    dvec2 o = base + hex_edges[i].online;
    if (verbose) {
      printf("  edge[%d] at %.3f , %.3f in direction %.3f , %.3f\n",
             i, o[0], o[1], hex_edges[i].dir[0], hex_edges[i].dir[1]);
    }
    double t;
    rc = line_intersect(o, hex_edges[i].dir, origin, normdir, &t);
    if (rc == 0) {
      if (verbose) {
        printf("  edge[%d] parallel\n", i);
      }
    } else {
      if (verbose) {
        printf("  edge[%d] %s %.3f",
               i,
               (rc == POTENTIALLY_ENTERING) ? "PE" : "PL",
               t);
      }
      if ((t >= 0) && (t <= 1.0)) {
        if (verbose) {
            printf("  *INTERSECT..");
        }
        dvec2 hexnorm(-hex_edges[i].dir[1], hex_edges[i].dir[0]);
        int rc2 = line_intersect(origin, direction, o, hexnorm, &t);
        double zcrossing = _origin[2] + _direction[2] * t;
        if (rc == POTENTIALLY_ENTERING) {
          if (enter_face == 0) {
            // hack to deal with cases where we are exactly on a boundary
            // and we would otherwise show the degenerate case of entering
            // multiple faces at one.  This way we just choose one; it's
            // more-or-less arbitrary so what the heck
            enter_face |= (1<<i);
            assert(rc2);
            cp->enter_z = zcrossing;
            cp->enter_range = t;
            cp->enter_face = i;
          }
        } else {
          if (exit_face == 0) {
            // likewise
            exit_face |= (1<<i);
            assert(rc2);
            cp->exit_z = zcrossing;
            cp->exit_range = t;
            cp->exit_face = i;
          }
        }
      }
      if (verbose) {
        printf("\n");
      }
    }
  }
  if ((enter_face == 0) && (exit_face == 0)) {
    return 0;
  }
  assert(enter_face && exit_face);
  assert( (enter_face & ~((1<<ffs(enter_face))-1)) == 0 );
  assert( (exit_face & ~((1<<ffs(exit_face))-1)) == 0 );
  if (verbose) {
    printf("hexpick ENTER %#x EXIT %#x z from %.3f to %.3f\n", 
           enter_face, exit_face, 
           cp->enter_z, cp->exit_z);
  }
  return 1;
}

static inline double intersect_z_plane(glm::vec3 const& origin,
                                       glm::vec3 const& direction,
                                       double z)
{
  if (fabs(direction[2]) < 1e-6) {
    return -1;
  }
  double t = (z - origin[2]) / direction[2];
  if (t < 0) {
    // intersection is before our origin
    return -1;
  }
  return t;
}

int RegionPicker::pick(glm::vec3 const& origin,
                       glm::vec3 const& direction,
                       double t_min,
                       double t_max,
                       PickPoint *p)
{
  bool verbose = false;
  
  // super-crude
  ColumnPick cp[REGION_SIZE*REGION_SIZE];
  unsigned num_cp = 0;

  for (int y=0; y<REGION_SIZE; y++) {
    for (int x=0; x<REGION_SIZE; x++) {
      //printf("------------------------------ in(%d,%d)\n", x, y);
      if (hexPickColumn(origin, direction, 
                        x+rp_region->origin.x, 
                        y+rp_region->origin.y,
                        &cp[num_cp])) {
        if ((cp[num_cp].enter_range >= 0)
            || (cp[num_cp].exit_range >= 0)) {
          cp[num_cp].dx = x;
          cp[num_cp].dy = y;
          num_cp++;
        }
      }
    }
  }
  qsort(&cp[0], num_cp, sizeof(ColumnPick), cmp_column_range);

  // check the stacks, in distance order
  for (unsigned i=0; i<num_cp; i++) {
    if (verbose) {
      printf("   ColumnPick[%u]\n", i);
      printf("   %6.3f - %6.3f   (+%d,+%d)\n", 
             cp[i].enter_range, cp[i].exit_range, 
             cp[i].dx, cp[i].dy);
    }
    SpanVector const& sv(rp_region->columns[cp[i].dy][cp[i].dx]);
    float z0, z1;
    int ze = cp[i].enter_z / z_scale - rp_region->basement;
    if (cp[i].enter_z > cp[i].exit_z) {
      // we enter above the exit point, so z0 is the exit point
      z1 = cp[i].enter_z / z_scale - rp_region->basement;
      z0 = cp[i].exit_z / z_scale - rp_region->basement;
    } else {
      z0 = cp[i].enter_z / z_scale - rp_region->basement;
      z1 = cp[i].exit_z / z_scale - rp_region->basement;
    }
    if (verbose) {
      printf("   checking %.3f - %.3f   ze=%d\n", z0, z1, ze);
    }

    int span_bottom = 0;
    unsigned jx = 0;
    double best_range = 1e9;
    uint64_t best_index = 0;
    bool any_hit = false;

    for (SpanVector::const_iterator j=sv.begin(); j!=sv.end(); ++j, ++jx) {
      int span_top = span_bottom + j->height;
      if (verbose) {
        printf("   span (%d - %d) type=%d\n", 
               span_bottom, span_top, j->type);
      }
      
      if ((z0 <= span_top) && (z1 >= span_bottom) && (j->type != 0)) {
        // found a hit
        double the_range;
        uint64_t index = (cp[i].dx + cp[i].dy * REGION_SIZE) << 4;
        index += span_bottom << (2*REGION_SIZE_BITS + 4);
        index += (uint64_t)jx << (2*REGION_SIZE_BITS + 4 + 12);
        index += (uint64_t)ze << (2*REGION_SIZE_BITS + 4 + 12 + 12);
        // figure out the range to the top or bottom face so we can
        // determine which face is hit
        if (direction[2] < 0) {
          // facing downward; entry point must be on the top face or the side
          double r0 = intersect_z_plane(origin, 
                                        direction, 
                                        (span_top + rp_region->basement) 
                                        * z_scale);
          if (verbose) {
            printf("   range to side is %.3f range to top is %.3f\n", 
                   cp[i].enter_range, r0);
            glm::vec3 w = origin + direction * (float)cp[i].enter_range;
            printf("    side hit => %.3f %.3f %.3f\n", w[0], w[1], w[2]);
            glm::vec3 v = origin + direction * (float)r0;
            printf("    top hit => %.3f %.3f %.3f\n", v[0], v[1], v[2]);
          }
          if (r0 > cp[i].enter_range) {
            index += 6;                      // 6=top
            the_range = r0;
          } else {
            index += cp[i].enter_face;       // 0,1,2,3,4,5 => hex faces
            the_range = cp[i].enter_range;
          }
        } else if (direction[2] > 0) {
          double r0 = intersect_z_plane(origin, 
                                        direction, 
                                        (span_bottom + rp_region->basement) 
                                        * z_scale);
          if (verbose) {
            printf("   range to side is %.3f range to bottom is %.3f\n", 
                   cp[i].enter_range, r0);
            glm::vec3 w = origin + direction * (float)cp[i].enter_range;
            printf("    side hit => %.3f %.3f %.3f\n", w[0], w[1], w[2]);
            glm::vec3 v = origin + direction * (float)r0;
            printf("    top hit => %.3f %.3f %.3f\n", v[0], v[1], v[2]);
          }
          if (r0 > cp[i].enter_range) {
            index += 7;                      // 7=top
            the_range = r0;
          } else {
            index += cp[i].enter_face;       // 0,1,2,3,4,5 => hex faces
            the_range = cp[i].enter_range;
          }
        } else {
          // parallel to horizon; cannot enter the top or bottom faces
          index += cp[i].enter_face;         // 0,1,2,3,4,5 => hex faces
          the_range = cp[i].enter_range;
        }
        if (!any_hit || (the_range < best_range)) {
          best_range = the_range;
          best_index = index;
        }
        any_hit = true;
      }
      span_bottom = span_top;
    }
    if (any_hit) {
      p->owner = this;
      p->range = best_range;
      p->index = best_index;
      return 0;
    }
  }
  return -1;
}


RegionPicker::RegionPicker(Region *rgn)
  : Picker(frect::empty),
    rp_region(rgn)
{
  double x0 = rgn->origin.x * x_stride;
  double y0 = rgn->origin.y * y_stride;
  bbox.x0 = x0;
  bbox.y0 = y0 - y_overhang;
  bbox.x1 = x0 + REGION_SIZE * x_stride + x_overhang;
  bbox.y1 = y0 + REGION_SIZE * y_stride;
  bbox.z0 = rgn->basement * z_scale;
  int max_z = 0;
  for (int y=0; y<REGION_SIZE; y++) {
    for (int x=0; x<REGION_SIZE; x++) {
      SpanVector const& vec(rgn->columns[y][x]);
      int h = 0;
      for (SpanVector::const_iterator j=vec.begin(); j!=vec.end(); ++j) {
        h += j->height;
      }
      if (h > max_z) {
        max_z = h;
      }
    }
  }
  bbox.z1 = (max_z + rgn->basement) * z_scale;
}

// build a Picker for a region

PickerPtr makeRegionPicker(Region *rgn)
{
  RegionPicker *p = new RegionPicker(rgn);
  return PickerPtr(p);
}


