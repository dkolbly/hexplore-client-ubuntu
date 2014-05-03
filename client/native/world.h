#ifndef _H_HEXPLORE_CLIENT_WORLD
#define _H_HEXPLORE_CLIENT_WORLD

#include "clientoptions.h"
#include <vector>
#include <hexcom/pick.h>

struct Slab {
  int x, y;             // location of column
  short z0;             // bottom
  short z1;             // top
  unsigned char type;
  unsigned char flags;
};

#include <hexcom/region.h>

struct Connection;

struct SpanInfo {
  Region       *region;
  Span         *span;
  int           x, y, z;
};

struct ClientRegion : Region {
  PickerPtr     picker;
};

typedef std::unordered_map<Posn, ClientRegion*, Posn::hash, Posn::cmp> regionCacheType;

struct World {
  regionCacheType regionCache;
};

struct ClientWorld {
  std::string   username;
  std::string   playername;
  World         state;

  // Retrieve the Span object that is below the given location
  SpanInfo getSpanBelow(glm::vec3 const& loc);
  SpanInfo getSpanAdjacent(SpanInfo from, float dir);

  /*
   *  Returns information about the span we are adjacent to when exiting
   *  the given face (0=SW, 1=SE, 2=E, ...)
   */
  SpanInfo getSpanAdjacentByFace(SpanInfo from, int exit_face);
  SpanInfo getSpan(SpanInfo from);
  SpanVector *getColumn(int ix, int iy, ClientRegion **rgnp);

  void requestRegionIfNotPresent(Connection *cnx, Posn const& p);
  std::unordered_map<Posn, bool, Posn::hash, Posn::cmp> pendingRequests;
};


ClientWorld *create_client_world(ClientOptions const& opt);

// build a Picker for a region
PickerPtr makeRegionPicker(Region *rgn);

#endif /* _H_HEXPLORE_CLIENT_WORLD */
