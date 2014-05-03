#ifndef _H_HEXPLORE_CLIENT_CONNECTION
#define _H_HEXPLORE_CLIENT_CONNECTION

#include "clientoptions.h"
#include "world.h"
#include "wire/major.pb.h"
#include <string>

struct IncomingMessage {
  wire::major::Major    major;
  void                 *decoded;
};

typedef std::vector<IncomingMessage> IncomingMessageVector;

struct Connection {
  int sock;
  int send(wire::major::Major major, std::string const& buf);
  int receive(wire::major::Major major, std::string const& buf);

  static int run(void *data);
  int run();
  ClientWorld *world;

  void request_view(int x, int y);
  SDL_mutex *queue_lock;
  IncomingMessageVector queue;
};


Connection *connection_make(ClientWorld *world, ClientOptions const& opt);

#endif /* _H_HEXPLORE_CLIENT_CONNECTION */
