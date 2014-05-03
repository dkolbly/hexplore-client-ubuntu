#ifndef _H_HEXPLORE_WIREHANDLER
#define _H_HEXPLORE_WIREHANDLER

namespace wire {
  namespace hello {
    struct ServerGreeting;
  };
  namespace terrain {
    struct Terrain;
  };
  namespace model {
    struct Mesh;
  };
  namespace entity {
    struct PlayerStatus;
    struct EntityInfo;
    struct EntityType;
    struct Tell;
  };
};

/*
 *  An abstract class for receiving traffic over the wire
 */
struct WireHandler {
  virtual void dispatch(wire::terrain::Terrain *) = 0;
  virtual void dispatch(wire::hello::ServerGreeting *) = 0;
  virtual void dispatch(wire::entity::PlayerStatus *) = 0;
  virtual void dispatch(wire::entity::EntityType *) = 0;
  virtual void dispatch(wire::entity::EntityInfo *) = 0;
  virtual void dispatch(wire::entity::Tell *) = 0;
  void flush_incoming(Connection *cnx);
};

#endif /* _H_HEXPLORE_WIREHANDLER */
