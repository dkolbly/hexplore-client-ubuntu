#include "ssl.h"
#include <sys/socket.h>
#include <signal.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <poll.h>
#include <vector>
#include "sslserver.h"

struct MyBigDataFrame : DataFrame {
  char bigfoot[1000000];
};

void do_pack(Frame *frame)
{
}

void do_unpack(Frame *frame)
{
  frame->unpacked = new MyBigDataFrame();
}


void *run_server(void *_s)
{
  SSLConnection *s = (SSLConnection *)_s;
  printf("connection %p\n", s);
  //MyServerCallbacks cb;
  //cb.cnx = s;
  s->run();
  return NULL;
}

void *run_client(void *_s)
{
  SSLConnection *s = (SSLConnection *)_s;
  printf("connection %p\n", s);
  //MyClientCallbacks cb;
  //cb.cnx = s;
  s->run();
  return NULL;
}

void print_frame(FramePtr p)
{
  printf("=========== received %s frame ===========\n",
         ((p->type == Frame::CONTROL_FRAME) ? "control" : "data"));
  printf("   code=%u  length=%zu\n", p->code, p->data.size());
  if (p->type == Frame::CONTROL_FRAME) {
    enum SSLControlCode code = (enum SSLControlCode)p->code;
    switch (code) {
    case SSL_CONNECT:
      printf("   did connect\n");
      break;
    case SSL_DISCONNECT:
      printf("   did disconnect (%s)\n", p->data.c_str());
      break;
    case SSL_CLIENT_CERTIFICATE:
      printf("   got client cert:\n  <%s>\n", p->data.c_str());
      break;
    }
  }
}

static bool global_verbose = false;

SSLConnection *run(int fd)
{
  return make_server(fd, do_pack, do_unpack);
}

int be_a_client(sockaddr_in *sin, int sock)
{
  int rc = connect(sock, (const sockaddr *)sin, sizeof(sockaddr_in));
  if (rc < 0) {
    perror("bind");
    return 1;
  }

  SSLConnection *cnx = make_client(sock, do_pack, do_unpack);
  
  NotifyQueue<FramePtr> *nq = cnx->getNotificationQueue();

  //=== read the connect msg ===
  FramePtr a(nq->pop());
  print_frame(a);

  // we are here to serve...  this is the main thread
  for (int i=0; i<100; i++) {
    FramePtr b(nq->pop());
    //print_frame(b);

    FramePtr x(new Frame(Frame::DATA_FRAME));
    x->code = 66;
    x->data = "This is some funny business";
    printf("sending message %d...\n", i);
    cnx->send(x);
  }
  cnx->teardown();
  delete cnx;
  return 0;
}

struct ServerTestConnection {
  ServerTestConnection(SSLConnection *c)
    : cnx(c) {
  }
  SSLConnection *cnx;
  /* which FD should we watch for this connection? */
  int watch_filedes() {
    return cnx->getNotificationQueue()->watch_filedes();
  }
  /* this connection is showing READY... process as needed */
  int process();
};

int ServerTestConnection::process()
{
  NotifyQueue<FramePtr> *nq = cnx->getNotificationQueue();
  FramePtr p(nq->pop());
  if (global_verbose) {
    print_frame(p);
  }

  if ((p->type == Frame::CONTROL_FRAME) 
      && (p->code == SSL_DISCONNECT)) {
    return -1;
  }
  if ((p->type == Frame::DATA_FRAME) && (p->data.size() < 1000)) {
    // bounce something back right away
    FramePtr x(new Frame(Frame::DATA_FRAME));
    x->code = 55;
    x->data = p->data + ".";
    cnx->send(x);
  }
  return 0;
}    

int be_a_server(sockaddr_in *sin, int sock)
{
  int rc = bind(sock, (const sockaddr *)sin, sizeof(sockaddr_in));
  if (rc < 0) {
    perror("bind");
    return 1;
  }
  rc = listen(sock, 5);
  if (rc < 0) {
    perror("listen");
    return 1;
  }

  // we are here to serve...  this is the main thread

  SSLServer<ServerTestConnection*>(sock, [](int sock) -> ServerTestConnection* {
      SSLConnection *c = make_server(sock, do_pack, do_unpack);
      pthread_t exec;
      pthread_create(&exec, NULL, run_server, c);
      return new ServerTestConnection(c);
    });
  return 0;
}


int main(int argc, char *argv[])
{
  signal(SIGPIPE, SIG_IGN);

  int sock = socket(AF_INET, SOCK_STREAM, 0);
  int port = 1777;
  int flag = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

  struct sockaddr_in sa;
  memset(&sa, 0, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_port = htons(port);
  sa.sin_addr.s_addr = 0;
  bool is_server = true;
  if ((argc > 1) && (strcmp(argv[1], "-c") == 0)) {
    is_server = false;
  }
  if (is_server) {
    be_a_server(&sa, sock);
  } else {
    be_a_client(&sa, sock);
  }
}

