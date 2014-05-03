#include <SDL.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include "connection.h"
#include "wirehandler.h"
#include "wire/hello.pb.h"
#include "wire/terrain.pb.h"
#include "wire/entity.pb.h"
#include "wire/major.pb.h"
#include "ui.h"


int greet(Connection *cnx)
{
  extern const char *build_date;
  extern const char *build_source_rev;
  extern const char *build_version;

  wire::hello::ClientGreeting c;
  c.set_version(1);
  std::string useragent = "hexplore/native";
  useragent.append(" ");
  useragent.append(build_version);
  useragent.append(" ");
  useragent.append(build_date);
  useragent.append(" ");
  useragent.append(build_source_rev);

  c.set_useragent(useragent);
  c.set_username(cnx->world->username);
  std::string buf;
  c.SerializeToString(&buf);
  cnx->send(wire::major::Major::HELLO_CLIENT_GREETING, buf);
  return 0;
}

Connection *connection_make(ClientWorld *w, ClientOptions const& opt)
{
  int sock = socket(AF_INET, SOCK_STREAM, 0);

  int flag = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(opt.server_port);
  sa.sin_addr.s_addr = htonl(0x7f000001);

  if (!opt.server_host.empty()) {
    const char *hostname = opt.server_host.c_str();
    struct hostent *ent = gethostbyname(hostname);
    if (!ent) {
      perror(hostname);
      return NULL;
    }
    if (!ent->h_addr_list[0]) {
      fprintf(stderr, "%s: no addresses\n", hostname);
      return NULL;
    }
    if (ent->h_addrtype != AF_INET) {
      fprintf(stderr, "%s: not an IPv4 address\n", hostname);
      return NULL;
    }
    memcpy(&sa.sin_addr, ent->h_addr_list[0], ent->h_length);
  }

  int rc = connect(sock, (const sockaddr *)&sa, sizeof(sa));
  if (rc < 0) {
    perror("connect");
    return NULL;
  }

  Connection *cnx = new Connection();
  cnx->queue_lock = SDL_CreateMutex();
  cnx->sock = sock;
  cnx->world = w;
  greet(cnx);

  SDL_CreateThread(Connection::run, "cnxn", cnx);
  return cnx;
}

int Connection::run(void *data)
{
  Connection *self = (Connection *)data;
  return self->run();
}

int Connection::run()
{
  static const bool verbose = false;
  // BLECH, this is copied code from the server side
  int fd = sock;
  while (true) {
    uint8_t header[8];
    int rc = read(fd, &header, sizeof(header));
    if (rc != sizeof(header)) {
      printf("Received %d, terminating\n", rc);
      return -1;
    }
    int len = (((unsigned)header[4]) << 24)
      + (((unsigned)header[5]) << 16)
      + (((unsigned)header[6]) << 8)
      + (((unsigned)header[7]) << 0);
    if (verbose) {
      printf("Received %02x %02x %02x %02x len=%d\n",
             header[0], header[1], header[2], header[3],
             len);
    }
    if ((header[0] != 'H') || (header[1] != 'x')) {
      printf("Bad magic, terminating\n");
      return -1;
    }
    if ((len < 1) || (len > (1<<20))) {
      printf("Bad length %d, terminating\n", len);
      return -1;
    }
    std::string payload(len, '\0');
    rc = read(fd, &payload[0], len);
    if (rc != len) {
      printf("Data received %d, terminating\n", rc);
      return -1;
    }
    unsigned m = (((unsigned)header[2]) << 8) + header[3];
    if (wire::major::Major_IsValid(m)) {
      receive((wire::major::Major)m, payload);
    } else {
      printf("Invalid major %u\n", m);
      return -1;
    }
  }
}

int Connection::receive(wire::major::Major major, std::string const& data)
{
  IncomingMessage im;
  im.major = major;

  switch (major) {
  case wire::major::Major::HELLO_CLIENT_GREETING:
  case wire::major::Major::TERRAIN_VIEW_CHANGE:
  case wire::major::Major::ENTITY_BECOME_PLAYER:
  case wire::major::Major::TERRAIN_EDIT:
  case wire::major::Major::ENTITY_SCRIPT:
    printf("unexpected major %u\n", major);
    return -1;

#define RCV(MAJ,TYPE) case wire::major::Major::MAJ: {   \
      TYPE *msg = new TYPE();                           \
      if (!msg->ParseFromString(data)) {                \
        delete msg;                                     \
        return -1;                                      \
      }                                                 \
      im.decoded = msg;                                 \
      SDL_LockMutex(queue_lock);                        \
      queue.push_back(im);                              \
      SDL_UnlockMutex(queue_lock);                      \
      return 0;                                         \
                      }
    
    RCV(ENTITY_TELL, wire::entity::Tell);
    RCV(ENTITY_ENTITY_INFO, wire::entity::EntityInfo);
    RCV(ENTITY_ENTITY_TYPE, wire::entity::EntityType);
    RCV(ENTITY_PLAYER_STATUS, wire::entity::PlayerStatus);
    RCV(HELLO_SERVER_GREETING, wire::hello::ServerGreeting);
    RCV(TERRAIN, wire::terrain::Terrain);
  }
  printf("Bad major: %u\n", major);
  return -1;
}

void WireHandler::flush_incoming(Connection *cnx)
{
  static const bool verbose = true;

  if (cnx->queue.size() == 0) {
    return;
  }

  if (verbose) {
    printf("Flushing %zd messages from server\n", cnx->queue.size());
  }

  // while holding the lock, copy it into a temporary vector
  IncomingMessageVector tmp;

  SDL_LockMutex(cnx->queue_lock);
  tmp.swap(cnx->queue);
  SDL_UnlockMutex(cnx->queue_lock);

  // this runs in the main thread
  for (IncomingMessageVector::iterator i=tmp.begin();
       i<tmp.end();
       ++i) {
    switch (i->major) {
      /*
       * Things we don't handle
       */
    case wire::major::Major::HELLO_CLIENT_GREETING:
    case wire::major::Major::TERRAIN_VIEW_CHANGE:
    case wire::major::Major::TERRAIN_EDIT:
    case wire::major::Major::ENTITY_SCRIPT:
    case wire::major::Major::ENTITY_BECOME_PLAYER:
      printf("weird that the client is getting a major %u\n", i->major);
      break;

      /*
       * Things we do
       */
#define DISPATCH(MAJ,TYPE,DEL) case wire::major::Major::MAJ:       \
      {                                                         \
        TYPE *msg = (TYPE *)i->decoded;                         \
        dispatch(msg);                                          \
        if (DEL) {delete msg;}                                  \
        break;                                                  \
      }

      DISPATCH(ENTITY_ENTITY_INFO, wire::entity::EntityInfo, 1);
      DISPATCH(ENTITY_ENTITY_TYPE, wire::entity::EntityType, 0);
      DISPATCH(ENTITY_PLAYER_STATUS, wire::entity::PlayerStatus, 1);
      DISPATCH(ENTITY_TELL, wire::entity::Tell, 1);
      DISPATCH(HELLO_SERVER_GREETING, wire::hello::ServerGreeting, 1);
      DISPATCH(TERRAIN, wire::terrain::Terrain, 1);
    }
  }
}


int Connection::send(wire::major::Major major, std::string const& buf)
{
  uint8_t header[8];
  header[0] = 'H';
  header[1] = 'x';
  header[2] = ((unsigned)major) >> 8;
  header[3] = (unsigned)major;
  size_t len = buf.size();
  header[4] = len >> 24;
  header[5] = len >> 16;
  header[6] = len >> 8;
  header[7] = len;
  struct iovec vec[2];
  vec[0].iov_base = &header[0];
  vec[0].iov_len = sizeof(header);
  vec[1].iov_base = (void*)buf.data();
  vec[1].iov_len = len;
  ssize_t rc = writev(sock, &vec[0], 2);
  if (rc != (ssize_t)(len + sizeof(header))) {
    printf("Write failed, tried %zd got %zd\n", 
           len + sizeof(header),
           rc);
    return -1;
  }
  return 0;
}
