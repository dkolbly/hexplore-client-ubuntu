#include "pick.h"
#include "hex.h"

const frect frect::empty = {.x0 = 0, .y0 = 0, .z0 = 0,
                            .x1 = 0, .y1 = 0, .z1 = 0};


struct SimplePicker : Picker {
  SimplePicker(frect const& box)
    : Picker(box)
  {
  }
  virtual int pick(glm::vec3 const& origin,
                   glm::vec3 const& direction,
                   double t_min,
                   double t_max,
                   PickPoint *p);
};

int SimplePicker::pick(glm::vec3 const& origin,
                       glm::vec3 const& direction,
                       double t_min,
                       double t_max,
                       PickPoint *p)
{
  p->owner = this;
  p->index = 0;
  p->range = t_min;
  return 0;
}


struct GroupPicker : Picker {
  GroupPicker(std::vector<PickerPtr> const& sub);
  std::vector<PickerPtr>        members;
  virtual int pick(glm::vec3 const& origin,
                   glm::vec3 const& direction,
                   double t_min,
                   double t_max,
                   PickPoint *p);
};

PickerPtr make_group_picker(std::vector<PickerPtr> const& vec)
{
  return PickerPtr(new GroupPicker(vec));
}

#define MAX_GROUP_FANOUT        (16)

GroupPicker::GroupPicker(std::vector<PickerPtr> const& sub)
  : Picker(frect::empty)
{
  assert(sub.size() <= MAX_GROUP_FANOUT);
  members = sub;
  bool first = true;

  for (std::vector<PickerPtr>::const_iterator i=members.begin();
       i != members.end();
       ++i) {
    if (first) {
      bbox = (*i)->bbox;
      first = false;
    } else {
      bbox.union_box((*i)->bbox);
    }
  }
}

int GroupPicker::pick(glm::vec3 const& origin,
                      glm::vec3 const& direction,
                      double t_min,
                      double t_max,
                      PickPoint *p)
{
  struct {
    Picker     *ptr;
    double      min_t;
    double      max_t;
  } candidates[MAX_GROUP_FANOUT];
  unsigned num_candidates = 0;

  for (std::vector<PickerPtr>::iterator i=members.begin();
       i != members.end();
       ++i) {
    double t0, t1;
    Picker *p = i->get();
    int rc = p->closest(origin, direction, &t0, &t1);
    if (rc == 0) {
      candidates[num_candidates].ptr = p;
      candidates[num_candidates].min_t = t0;
      candidates[num_candidates].max_t = t1;
      num_candidates++;
    }
  }
  printf("group picker: %u candidates\n", num_candidates);
  for (unsigned i=0; i<num_candidates; i++) {
    printf("   %p  %.3f - %.3f\n",
           candidates[i].ptr,
           candidates[i].min_t,
           candidates[i].max_t);
  }
  return -1;
}


// Return the x coordinate of where the given vector
// interscets the given constant-y plane

double intersect_y_plane(glm::vec3 const& origin,
                         glm::vec3 const& direction,
                         double y)
{
  if (fabs(direction[1]) < 1e-6) {
    return -1;
  }
  double t = (y - origin[1]) / direction[1];
  if (t < 0) {
    // intersection is before our origin
    return -1;
  }
  return t;
}

double intersect_x_plane(glm::vec3 const& origin,
                         glm::vec3 const& direction,
                         double x)
{
  if (fabs(direction[0]) < 1e-6) {
    return -1;
  }
  double t = (x - origin[0]) / direction[0];
  if (t < 0) {
    // intersection is before our origin
    return -1;
  }
  return t;
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


int Picker::closest(glm::vec3 const& origin,
                    glm::vec3 const& direction,
                    double *min,
                    double *max)
{
  static const bool verbose = false;
  frect r = bbox;
  double rc;
  bool x0_hit = false, x1_hit = false;
  bool y0_hit = false, y1_hit = false;
  bool z0_hit = false, z1_hit = false;
  double best_rc = 1e100;
  double worst_rc = 0;

  rc = intersect_y_plane(origin, direction, r.y0);
  if (rc >= 0) {
    double x = origin[0] + rc * direction[0];
    double z = origin[2] + rc * direction[2];
    if (verbose) {
      printf("  y0 @ %.3f (z %.3f)\n", x, z);
    }
    if ((x >= r.x0) && (x <= r.x1) && (z >= r.z0) && (z <= r.z1)) {
      y0_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }

  rc = intersect_y_plane(origin, direction, r.y1);
  if (rc >= 0) {
    double x = origin[0] + rc * direction[0];
    double z = origin[2] + rc * direction[2];
    if (verbose) {
      printf("  y1 @ %.3f (z %.3f)\n", x, z);
    }
    if ((x >= r.x0) && (x <= r.x1) && (z >= r.z0) && (z <= r.z1)) {
      y1_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }

  rc = intersect_x_plane(origin, direction, r.x0);
  if (rc >= 0) {
    double y = origin[1] + rc * direction[1];
    double z = origin[2] + rc * direction[2];
    if (verbose) {
      printf("  x0 @ %.3f (z %.3f)\n", y, z);
    }
    if ((y >= r.y0) && (y <= r.y1) && (z >= r.z0) && (z <= r.z1)) {
      x0_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }
  rc = intersect_x_plane(origin, direction, r.x1);
  if (rc >= 0) {
    double y = origin[1] + rc * direction[1];
    double z = origin[2] + rc * direction[2];
    if (verbose) {
      printf("  x1 @ %.3f (z %.3f)\n", y, z);
    }
    if ((y >= r.y0) && (y <= r.y1) && (z >= r.z0) && (z <= r.z1)) {
      x1_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }

  rc = intersect_z_plane(origin, direction, r.z0);
  if (rc >= 0) {
    double x = origin[0] + rc * direction[0];
    double y = origin[1] + rc * direction[1];
    if (verbose) {
      printf("  z0 @ ( %.3f, %.3f, z0)\n", x, y);
    }
    if ((y >= r.y0) && (y <= r.y1) && (x >= r.x0) && (x <= r.x1)) {
      z0_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }
  rc = intersect_z_plane(origin, direction, r.z1);
  if (rc >= 0) {
    double x = origin[0] + rc * direction[0];
    double y = origin[1] + rc * direction[1];
    if (verbose) {
      printf("  z1 @ ( %.3f, %.3f, z1)\n", x, y);
    }
    if ((y >= r.y0) && (y <= r.y1) && (x >= r.x0) && (x <= r.x1)) {
      z1_hit = true;
      if (rc < best_rc) {
        best_rc = rc;
      }
      if (rc > worst_rc) {
        worst_rc = rc;
      }
    }
  }

  if (verbose) {
    printf("   summary: %c %c  %c %c  %c %c\n",
           y0_hit ? 't' : 'f',
           y1_hit ? 't' : 'f',
           x0_hit ? 't' : 'f',
           x1_hit ? 't' : 'f',
           z0_hit ? 't' : 'f',
           z1_hit ? 't' : 'f');
  }
  if (y0_hit || y1_hit || x0_hit || x1_hit || z0_hit || z1_hit) {
    *min = best_rc;
    *max = worst_rc;
    return 0;
  }
  return -1;
}


PickerPtr make_simple_picker(frect const& box)
{
  Picker *p = new SimplePicker(box);
  return PickerPtr(p);
}
