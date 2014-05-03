#include <SDL.h>
#include "world.h"
#include "connection.h"
#include "wire/terrain.pb.h"
#include <hexcom/hex.h>

void Connection::request_view(int x, int y)
{
  wire::terrain::ViewChange msg;
  wire::terrain::Rect *r = msg.add_visible();
  r->set_x(x);
  r->set_y(y);
  r->set_w(REGION_SIZE);
  r->set_h(REGION_SIZE);

  std::string buf;
  msg.SerializeToString(&buf);
  send(wire::major::Major::TERRAIN_VIEW_CHANGE, buf);
}

ClientWorld *create_client_world(ClientOptions const& opt)
{
  ClientWorld *w = new ClientWorld();
  w->username = opt.username;
  return w;
}

SpanInfo ClientWorld::getSpanAdjacentByFace(SpanInfo from, int exit_face)
{
  SpanInfo si;
  switch (exit_face) {
  case 2:
    si.x = from.x + 1;
    si.y = from.y;
    break;
  case 3:
    si.x = from.x + (from.y & 1);
    si.y = from.y + 1;
    break;
  case 4:
    si.x = from.x - ((from.y & 1) ? 0 : 1);
    si.y = from.y + 1;
    break;
  case 5:
    si.x = from.x - 1;
    si.y = from.y;
    break;
  case 0:
    si.x = from.x - ((from.y & 1) ? 0 : 1);
    si.y = from.y - 1;
    break;
  case 1:
    si.x = from.x + (from.y & 1);
    si.y = from.y - 1;
  }

  si.z = from.z + from.span->height;
  si.span = NULL;
  return getSpan(si);
}

SpanInfo ClientWorld::getSpanAdjacent(SpanInfo from, float dir)
{
  SpanInfo si;
  if (dir <= 30) {
  right:
    si.x = from.x + 1;
    si.y = from.y;
  } else if (dir <= 90) { 
    si.x = from.x + (from.y & 1);
    si.y = from.y + 1;
  } else if (dir <= 150) { 
    si.x = from.x - ((from.y & 1) ? 0 : 1);
    si.y = from.y + 1;
  } else if (dir <= 210) {
    si.x = from.x - 1;
    si.y = from.y;
  } else if (dir <= 270) {
    si.x = from.x - ((from.y & 1) ? 0 : 1);
    si.y = from.y - 1;
  } else if (dir <= 330) {
    si.x = from.x + (from.y & 1);
    si.y = from.y - 1;
  } else {
    goto right;
  }

  // we are coming from ON TOP of that span,
  // so contemplate our z as being at that level
  si.z = from.z + from.span->height;
  si.span = NULL;
  return getSpan(si);
}

SpanInfo ClientWorld::getSpan(SpanInfo si)
{
  // now find the *top* of that column, starting at the current
  // Z.  Note that Z should be the TOP of the column we're
  // coming from
  Posn p(si.x & ~(REGION_SIZE-1),
         si.y & ~(REGION_SIZE-1));

  regionCacheType::iterator i = state.regionCache.find(p);
  if (i == state.regionCache.end()) {
    printf("  no region cached for %d,%d\n", p.x, p.y);
    return si;
  }
  ClientRegion *rgn = i->second;
  si.region = rgn;

  int dy = si.y - p.y;
  int dx = si.x - p.x;
  assert((dx >= 0) && (dx < REGION_SIZE));
  assert((dy >= 0) && (dy < REGION_SIZE));
  std::vector<Span>& col = rgn->columns[dy][dx];
  int iz = si.z - rgn->basement;
  int atz = rgn->basement;

  bool nextnonspace = false;
  for (std::vector<Span>::iterator s=col.begin(); s<col.end(); ++s) {
    if (iz <= 0) {
      if (s->type == 0) {
        return si;
      }
      nextnonspace = true;
    }
    int h = s->height;
    if (s->type != 0) {
      si.span = &(*s);
      si.z = atz;
      if (nextnonspace) {
        return si;
      }
    }
    iz -= h;
    atz += h;
  }
  return si;
}

SpanInfo ClientWorld::getSpanBelow(glm::vec3 const& loc)
{
  int ix, iy, iz;
  // the reverse transformation...
  convert_xyz_to_hex(loc, &ix, &iy, &iz);
  
  Posn p(ix & ~(REGION_SIZE-1),
         iy & ~(REGION_SIZE-1));

  int dy = iy - p.y;
  int dx = ix - p.x;
  SpanInfo si;
  si.region = NULL;
  si.span = NULL;
  si.x = ix;
  si.y = iy;
  si.z = iz;

  regionCacheType::iterator i = state.regionCache.find(p);
  if (i == state.regionCache.end()) {
    printf("  no region cached for %d,%d\n", p.x, p.y);
    return si;
  }
  ClientRegion *rgn = i->second;
  assert((dx >= 0) && (dx < REGION_SIZE));
  assert((dy >= 0) && (dy < REGION_SIZE));

  std::vector<Span>& col = rgn->columns[dy][dx];
  iz -= rgn->basement;
  int atz = rgn->basement;
  si.region = rgn;

  for (std::vector<Span>::iterator s=col.begin(); s<col.end(); ++s) {
    if (iz <= 0) {
      return si;
    }
    int h = s->height;
    if (s->type != 0) {
      si.span = &(*s);
      si.z = atz;
    }
    iz -= h;
    atz += h;
  }
  return si;
}

SpanVector *ClientWorld::getColumn(int ix, int iy, ClientRegion **p)
{
  Posn posn(ix & ~(REGION_SIZE-1),
            iy & ~(REGION_SIZE-1));

  int dy = iy - posn.y;
  int dx = ix - posn.x;
  regionCacheType::iterator i = state.regionCache.find(posn);

  if (i == state.regionCache.end()) {
    return NULL;
  }
  *p = i->second;
  return &i->second->columns[dy][dx];
}


void ClientWorld::requestRegionIfNotPresent(Connection *cnx, Posn const& p)
{
  if (state.regionCache.find(p) != state.regionCache.end()) {
    return;
  }
  if (pendingRequests.find(p) != pendingRequests.end()) {
    return;
  }
  cnx->request_view(p.x, p.y);
  pendingRequests.insert(std::unordered_map<Posn, bool, Posn::hash, Posn::cmp>::value_type(p,true));
}
