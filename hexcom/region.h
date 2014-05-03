#ifndef _H_HEXCOM_REGION
#define _H_HEXCOM_REGION

#include <unordered_map>
#include <vector>
#include "pick.h"

/* flags */

#define DEEP_SHADOW             (0x3)
#define MEDIUM_SHADOW           (0x2)
#define LIGHT_SHADOW            (0x1)
#define NO_SHADOW               (0x0)

#define MAX_DEPTH              (5000)


struct Span {
  unsigned short height;
  unsigned char type;
  unsigned char flags;
};

/**
 *   The size of a region; the terrain is divided into "squares"
 *   this much on a side (I say "squares" because it's a tiling of
 *   hexes, but the hexes are organized in a grid and the terrain
 *   is divided into regions based on that grid)
 */

#define REGION_SIZE_BITS        (5)
#define REGION_SIZE             (1<<REGION_SIZE_BITS)

#define WITHINREGION_MASK       (REGION_SIZE-1)                 // get just the within-region part
#define REGION_MASK             (~(REGION_SIZE-1))              // get just the region part

struct Posn {
  Posn() { };
  Posn(int _x, int _y) : x(_x), y(_y) { };

  int x;
  int y;
  struct hash {
    unsigned operator()(Posn const& a) const {
      return (a.x>>REGION_SIZE_BITS)
        + (a.y>>REGION_SIZE_BITS)
        + (0xFACE ^ (a.x >> 16))
        + (0xCAFE ^ (((a.x|1) * (a.y|1)) >> 16));
    }
  };
  struct cmp {
    bool operator()(Posn const& a, Posn const& b) const {
      return (a.x == b.x) && (a.y == b.y);
    }
  };

  bool operator==(Posn const& b) {
    return (x == b.x) && (y == b.y);
  }
};

// return the region position which contains the
// given hex position
static inline Posn regionPosn(Posn const& hexPosn)
{
  return Posn(hexPosn.x & REGION_MASK,
              hexPosn.y & REGION_MASK);
}

struct Region;
typedef std::vector<Span> SpanVector;

/* compute the total height of the first N (numSpans) spans */
static inline int spanHeightTo(SpanVector const& vec, unsigned numSpans)
{
  int z = 0;
  for (unsigned i=0; i<numSpans; i++) {
    z += vec[i].height;
  }
  return z;
}

struct Region {
  Posn origin;          // coordinate of lower left (from top) column
  short basement;       // z0 for start of spans
  // columns[Y][X]
  SpanVector columns[REGION_SIZE][REGION_SIZE];
  // the x,y,z given here are in world [integer] coordinates;
  // i.e., z=0 is sea level, and (x,y) better start within the region's origin
  // returns -1 on error
  int set(int x, int y, int z, int height, int type, int flags);
  // pack and unpack an encoded representation for wire transport
  void unpack(std::string const& encoded);
  void pack(std::string *encoded);
};

/**
 *   Expand a SpanVector in a given vertical region, starting at z_base
 *   and for distance h, into a flat vector of types and flags.  The flags
 *   may be NULL if that information is not sought.
 *   z_base is in absolute coordinates (i.e., 0=sea level)
 */

void expandSpan(Region *rgn, SpanVector const& vec, int z_base, int h,
                unsigned char *types,
                unsigned char *flags);

PickerPtr makeRegionPicker(Region *rgn);

/**
 *       <-----12----->      <-----12----->      <-----12----->  <--5--> <--5--> 3 2 1 0
 *    +-------------------+-------------------+-----------------+-------+-------+-------+
 *    |     hit z         |   span  index     |        z0       |   y   |   x   |  face |
 *    +-------------------+-------------------+-----------------+-------+-------+-------+
 *
 *    Note that the hitz is the z at which the pick ray enters the column; in the case
 *    of TOP and BOTTOM face hits, this may be outside the span itself.
 *
 *
 *             pick     :
 *               ray \_ :
 *                     \_  <------ hit z
 *                      :\_
 *                      :  \
 *                      +---*---------------+  <--- top of span
 *                      |   face=TOP        |
 *                      |                   |
 *                      |                   |
 *                      |                   |
 *                     //                  //
 *
 */

struct RegionPickIndex {
  RegionPickIndex(uint64_t i)
  : value(i) {
  }
  uint64_t value;

  int face() { return (value & 15); }
  int x() { return ((value >> 4) & (REGION_SIZE-1)); }
  int y() { return ((value >> (4+REGION_SIZE_BITS)) & (REGION_SIZE-1)); }
  // relative to the basement; i.e., 0=basement, typ. 1000=ground floor
  int z0() { return ((value >> (4+2*REGION_SIZE_BITS)) & ((1<<12)-1)); }
  int span() { return ((value >> (4+2*REGION_SIZE_BITS+12)) & ((1<<12)-1)); }
  int hitz() { return ((value >> (4+2*REGION_SIZE_BITS+12+12)) & ((1<<12)-1)); }
};


#define PICK_INDEX_FACE_TOP     (6)
#define PICK_INDEX_FACE_BOTTOM  (7)

#endif /* _H_HEXCOM_REGION */
