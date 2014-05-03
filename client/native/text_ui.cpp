#include <unistd.h>
#include <SDL.h>
#include <hexcom/misc.h>
#include "text_ui.h"
#include "connection.h"
#include "wirehandler.h"
#include "wire/hello.pb.h"
#include "wire/terrain.pb.h"
#include "wire/entity.pb.h"

struct TextUIWireHandler : public WireHandler {
  Connection *cnx;
  ClientWorld *world;

  virtual void dispatch(wire::terrain::Terrain *);
  virtual void dispatch(wire::hello::ServerGreeting *);
  virtual void dispatch(wire::entity::PlayerStatus *);
  virtual void dispatch(wire::entity::EntityInfo *);
  virtual void dispatch(wire::entity::EntityType *);
  virtual void dispatch(wire::entity::Tell *);
};

void TextUIWireHandler::dispatch(wire::entity::Tell *msg)
{
  printf("Told #%u ==> \"%s\"\n", msg->target(), msg->message().c_str());
}

void TextUIWireHandler::dispatch(wire::entity::EntityType *etype)
{
  printf("Got entity type \"%s\" parent=\"%s\"\n", 
         etype->type().c_str(),
         etype->has_parent() ? etype->parent().c_str() : "--");
}

void TextUIWireHandler::dispatch(wire::entity::PlayerStatus *msg)
{
  printf("received player status\n");
}

void TextUIWireHandler::dispatch(wire::entity::EntityInfo *msg)
{
  printf("received entity info\n");
}


void TextUIWireHandler::dispatch(wire::terrain::Terrain *msg)
{
  wire::terrain::Rect const& area(msg->area());
  printf("received terrain update(%d,%d)\n", area.x(), area.y());
}

void TextUIWireHandler::dispatch(wire::hello::ServerGreeting *msg)
{
  printf("%.3f: Received ServerGreeting\n", real_ftime());
  printf("    Server name \"%s\"\n", msg->servername().c_str());
  printf("    Users %d/%d\n", msg->current_users(), msg->max_users());
  
  wire::entity::BecomePlayer p;
  p.set_playername("donovan");
}



int tui_run(Connection *cnx, ClientWorld *w)
{
  TextUIWireHandler *h = new TextUIWireHandler();
  h->cnx = cnx;
  h->world = w;

  while (true) {
    h->flush_incoming(cnx);
    usleep(10000);
  }
  return 0;
}
