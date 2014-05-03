#ifndef _H_HEXCOM_PICK
#define _H_HEXCOM_PICK

#include <glm/glm.hpp>
#include <memory>
#include <vector>

typedef glm::highp_vec2 dvec2;

struct frect {
  double         x0, y0, z0, x1, y1, z1;
  void union_box(const frect& rhs) {
    x0 = (rhs.x0 < x0) ? rhs.x0 : x0;
    x1 = (rhs.x1 > x1) ? rhs.x1 : x1;
    y0 = (rhs.y0 < y0) ? rhs.y0 : y0;
    y1 = (rhs.y1 > y1) ? rhs.y1 : y1;
    z0 = (rhs.z0 < z0) ? rhs.z0 : z0;
    z1 = (rhs.z1 > z1) ? rhs.z1 : z1;
  }
  static const frect empty;
};

struct Picker;

struct PickPoint {
  Picker       *owner;
  uint64_t      index;
  double        range;
};

struct ColumnPick {
  double        enter_z;
  double        exit_z;
  double        enter_range;
  double        exit_range;
  short         dx, dy;
  uint8_t       enter_face;
  uint8_t       exit_face;
};

/**
 *   Figure out where, if anywhere, the given ray (starting at `origin'
 *   and with direction `direction') intersects the hexagonal column/prism
 *   identified by {ix,iy}.
 *
 *   Returns 1 if it does, 0 otherwise.
 *   Does not fill in the dx, dy parts of *cp
 */
int hexPickColumn(glm::vec3 const& origin,
                  glm::vec3 const& direction,
                  int ix, int iy,
                  ColumnPick *cp);


struct Picker {
  // determine the closest possible approach to the line represented
  // by the origin and direction.  Returns 0 if approach is possible,
  // -1 if not.  This uses a fast calculation based on the axis-aligned
  // bounding box.
  int closest(glm::vec3 const& origin,
              glm::vec3 const& direction,
              double *min,
              double *max);
  // A slower calculation to return a pick point (>= 0) and a proximity
  // measure.  Returns -1 if no actual pick point was found
  virtual int pick(glm::vec3 const& origin,
                   glm::vec3 const& direction,
                   double t_min,
                   double t_max,
                   PickPoint *picked) = 0;
  void         *info;
  frect         bbox;
  Picker(frect const& box) : info(NULL), bbox(box) { }
  
};

typedef std::shared_ptr<Picker> PickerPtr;

PickerPtr make_simple_picker(frect const& box);

PickerPtr make_group_picker(std::vector<PickerPtr> const& vec);

/*************************************************************
;;;  Find where the infinite line a-->b intersects
;;;  the line that contains c and has normal n.
;;;  Returns the intersection point on a-->b,
;;;  the symbol pe for a potentially entering
;;;  line and pl for a potentially leaving line,
;;;  and the intersection parameter on a-->b
;;;  (0=a, 1=b).
;;;

(define (clip-to-segment (a <point>) (b <point>) (c <point>) (n <size>))
  (let ((den (inner-product n (point- a b)))
        (num (inner-product n (point- a c))))
    (if (= den 0)
        #f
        (let ((t (/ num den)))
          (values
           (point+ a (size* (point- b a) t))
           (if (< den 0) 'pl 'pe)
           t)))))
*************************************************************/

/*
 *  Returns 0 for no intersection (direction and normal are perpendicular)
 *          1 for potentially entering
 *          2 for potentially leaving
 */
#define POTENTIALLY_ENTERING    (1)
#define POTENTIALLY_LEAVING     (2)

static inline int line_intersect(dvec2 const& origin,      // a
                                 dvec2 const& direction,   // b
                                 dvec2 const& online,      // c
                                 dvec2 const& normal,      // n
                                 double *t)
{
  double den = glm::dot(normal, direction);
  if (fabs(den) < 1.0e-9) {
    // parallel, or as near as we can tell
    return -1;
  }
  double num = glm::dot(normal, online-origin);
  *t = num/den;
  // Leaving and Entering is interpreted in the context of a clockwise
  // circumnavigation; that is, if the the `direction' proceeds
  // clockwise around the figure, and the `normal' points to the left
  // of the direction of the line (i.e., a +90 rotation of the direction
  // of the line), then potentially entering means the line crosses
  // INTO the figure, and potentially leaving means the line crosses
  // OUT OF the figure.
  return (den > 0) ? POTENTIALLY_LEAVING : POTENTIALLY_ENTERING;
}

#endif /* _H_HEXCOM_PICK */
