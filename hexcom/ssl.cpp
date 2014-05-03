#include "ssl.h"
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <fcntl.h>
#include "notifyqueue.h"

#define WIRE_MAGIC      (0x4878) 

template<class T> NotifyQueue<T>::NotifyQueue()
{
  int fd[2];
  if (pipe(fd) < 0) {
    abort();
  }
  read_side_fd = fd[0];
  write_side_fd = fd[1];
  pthread_mutex_init(&lock, NULL);
}

template<class T> NotifyQueue<T>::~NotifyQueue()
{
  close(read_side_fd);
  close(write_side_fd);
}

struct WireFrameHeader {
  uint16_t      magic;
  uint16_t      code;
  uint32_t      len;
};

struct OpenSSLConnection : SSLConnection {
  OpenSSLConnection(int sock, 
                    void (*packer)(Frame *frame),
                    void (*unpacker)(Frame *frame),
                    bool clientq);

  pthread_t ssl_thread;
  // these are called in the i/o thread to pack and unpack data frames
  void (*packer)(Frame *frame);
  void (*unpacker)(Frame *frame);

  pthread_mutex_t destruction;  // shutdown lock
  int ssl_filedes;
  unsigned ssl_modes;
  bool did_issue_connect;
  static const bool verbose = false;

  NotifyQueue<FramePtr> inbound;        // SSL -> main
  NotifyQueue<FramePtr> outbound;       // main -> SSL

  virtual void run();
  virtual void send(FramePtr frame);
  virtual NotifyQueue<FramePtr> *getNotificationQueue() {
    return &inbound;
  }
  virtual void teardown();

  std::string peer_cn;
  std::string exit_reason;

  SSL *ssl_handle;

  virtual ~OpenSSLConnection();

  // ssl_writeLen is how much we've written so far
  // ssl_writeBufferLen is how much we're trying to write
  char *ssl_writeBuffer;
  FramePtr ssl_writePending;
  WireFrameHeader ssl_writeFrameHeader;
  size_t ssl_writeLen;
  size_t ssl_writeBufferLen;

  char *read_buffer;
  // read_len is how much we've read so far
  // read_goal is how much we're trying to read
  size_t read_len, read_goal;
  WireFrameHeader ssl_wireFrameHeader;
  std::string pending;

  /* A terminal SSL error occurred */
  void terminal(const char *msg);

  static bool ssl_clientMode;
  static SSL_CTX *ssl_ctx;
  static int ssl_cnxnIndex;
  static int sslVerifyCallback(int preverify_ok, X509_STORE_CTX *ctx);
  static void sslSubsystemInitialize(bool clientq);

private:
  enum ModeStep {
    CLEAN_SHUTDOWN = 3,
    KEEP_RUNNING = 4,
    ERROR_CONDITION = 5
  };

  int attach();
  void semi_internal_run();
  int internal_run();

  void tell_main_control(enum SSLControlCode code, std::string const& data) {
    Frame *f = new Frame(Frame::CONTROL_FRAME);
    f->code = code;
    f->data = data;
    inbound.push(FramePtr(f));
  }

  void tell_main_data(uint16_t code) {
    Frame *f = new Frame(Frame::DATA_FRAME);
    f->code = code;
    f->data.swap(pending);
    if (unpacker) {
      unpacker(f);
    }
    inbound.push(FramePtr(f));
  }

  ModeStep step();
};

bool OpenSSLConnection::ssl_clientMode;
SSL_CTX *OpenSSLConnection::ssl_ctx;
int OpenSSLConnection::ssl_cnxnIndex;

void OpenSSLConnection::sslSubsystemInitialize(bool clientq)
{
  SSL_library_init();

  ssl_clientMode = clientq;

  if (clientq) {
    ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
  } else {
    ssl_ctx = SSL_CTX_new(TLSv1_2_server_method());
  }
  if (!ssl_ctx) {
    ERR_print_errors_fp(stderr);
    abort();
  }

  printf("ssl_ctx %p\n", ssl_ctx);

  ssl_cnxnIndex = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);
  SSL_CTX_set_verify(ssl_ctx, SSL_VERIFY_PEER, sslVerifyCallback);
  SSL_CTX_set_verify_depth(ssl_ctx, 3);

  int rc;

  rc = SSL_CTX_load_verify_locations(ssl_ctx, "root_ca.crt", NULL);
  if (rc != 1) {
    ERR_print_errors_fp(stderr);
    abort();
  }

  rc = SSL_CTX_use_PrivateKey_file(ssl_ctx, 
                                   clientq ? "user.key" : "server.key", 
                                   SSL_FILETYPE_PEM);
  if (rc != 1) {
    ERR_print_errors_fp(stderr);
    abort();
  }
  
  rc = SSL_CTX_use_certificate_file(ssl_ctx, 
                                    clientq ? "user.crt" : "server.crt", 
                                    SSL_FILETYPE_PEM);
  if (rc != 1) {
    ERR_print_errors_fp(stderr);
    abort();
  }
}


OpenSSLConnection::OpenSSLConnection(int sock, 
                                     void (*_packer)(Frame *frame),
                                     void (*_unpacker)(Frame *frame),
                                     bool clientq)
  : packer(_packer),
    unpacker(_unpacker)
{
  pthread_mutex_init(&destruction, NULL);
  pthread_mutex_lock(&destruction);

  ssl_filedes = sock;
  did_issue_connect = false;
  if (!ssl_ctx) {
    sslSubsystemInitialize(clientq);
  }
  if (clientq != ssl_clientMode) {
    // we are changing our role... that is just wrong!
    abort();
  }
}

int OpenSSLConnection::attach()
{
  ssl_handle = SSL_new(ssl_ctx);

  if (!ssl_handle) {
    terminal("new");
    return -1;
  }

  if (!SSL_set_fd(ssl_handle, ssl_filedes)) {
    terminal("set_fd");
    return -1;
  }
  fcntl(ssl_filedes, F_SETFL, O_NONBLOCK);
  BIO_set_nbio(SSL_get_rbio(ssl_handle), 1);
  BIO_set_nbio(SSL_get_wbio(ssl_handle), 1);

  SSL_set_ex_data(ssl_handle, ssl_cnxnIndex, this);
  return 0;
}

void OpenSSLConnection::terminal(const char *msg)
{
  char buf[200];

  exit_reason = "03-827E SSL error in ";
  exit_reason += msg;
  unsigned long err = ERR_get_error();
  ERR_error_string_n(err, &buf[0], sizeof(buf));
  exit_reason += ": ";
  exit_reason += buf;

  fprintf(stderr, "Fatal SSL Error in %s:\n", msg);
  ERR_print_errors_fp(stderr);
}

#define MODE_HANDSHAKE  (1<<0)
#define MODE_READ       (1<<1)
#define MODE_WRITE      (1<<2)
#define MODE_SHUTDOWN   (1<<3)

void OpenSSLConnection::semi_internal_run()
{
  int rc = internal_run();
  std::string msg;

  if (rc < 0) {
    msg = "error:";
    msg += exit_reason;
  } else {
    msg = "clean";
  }
  tell_main_control(SSL_DISCONNECT, msg);
  if (verbose) {
    printf("(%p) done, rc=%d... \"%s\"\n", this, rc, msg.c_str());
  }
}

int OpenSSLConnection::internal_run()
{
  ssl_modes = MODE_HANDSHAKE;

  while (1) {
    enum ModeStep rc = step();
    switch (rc) {
    case CLEAN_SHUTDOWN:
      return 0;
    case ERROR_CONDITION:
      return -1;
    case KEEP_RUNNING:
      break;
    default:
      assert(0);
    }
  }
}

OpenSSLConnection::ModeStep OpenSSLConnection::step()
{
  int rc;

  struct pollfd poll_list[2];

  poll_list[0].fd = ssl_filedes;
  poll_list[0].events = 0;
  poll_list[0].revents = 0;

  poll_list[1].fd = outbound.watch_filedes();
  poll_list[1].events = 0;
  poll_list[1].revents = 0;

  // figure out what to poll for based on our mode

  if (ssl_modes & MODE_SHUTDOWN) {
    rc = SSL_shutdown(ssl_handle);
    if (rc < 0) {
      terminal("shutdown");
      return ERROR_CONDITION;
    }
    return CLEAN_SHUTDOWN;
  }
  
 more_writing:
  if (ssl_modes & MODE_WRITE) {
    if (ssl_writeLen < ssl_writeBufferLen) {
      rc = SSL_write(ssl_handle, 
                     &ssl_writeBuffer[ssl_writeLen], 
                     ssl_writeBufferLen - ssl_writeLen);
      if (rc > 0) {
        ssl_writeLen += rc;
        goto more_writing;
      } else {
        rc = SSL_get_error(ssl_handle, rc);
        if (rc == SSL_ERROR_WANT_READ) {
          poll_list[0].events |= POLLIN;
        } else if (rc == SSL_ERROR_WANT_WRITE) {
          poll_list[0].events |= POLLOUT;
        } else {
          terminal("SSL_write");
          return ERROR_CONDITION;
        }
      }
    } else {
      if (ssl_writeBuffer == (char *)&ssl_writeFrameHeader) {
        // done writing the header, now write the body
        ssl_writeBuffer = &ssl_writePending->data[0];
        ssl_writeBufferLen = ssl_writePending->data.size();
        ssl_writeLen = 0;
        goto more_writing;
      } else {
        // done writing the body, we're done writing
        ssl_modes &= ~MODE_WRITE;
        goto more_writing;
      }
    }
  } else {
    // keep an eye out for more stuff that might be
    // intended to got outbound
    poll_list[1].events |= POLLIN;
  }

  if (ssl_modes & MODE_HANDSHAKE) {
    if (ssl_clientMode) {
      rc = SSL_connect(ssl_handle);
    } else {
      rc = SSL_accept(ssl_handle);
    }
    if (rc != 1) {
      int err = SSL_get_error(ssl_handle, rc);
      if (err == SSL_ERROR_WANT_READ) {
        poll_list[0].events |= POLLIN;
      } else if (err == SSL_ERROR_WANT_WRITE) {
        poll_list[0].events |= POLLOUT;
      } else {
        terminal("connect/accept");
        return ERROR_CONDITION;
      }
      if (!did_issue_connect) {
        tell_main_control(SSL_CONNECT, "");
        did_issue_connect = true;
      }
    } else {
      if (verbose) {
        printf("handshake ok\n");
        printf("<%s>\n", peer_cn.c_str());
      }
      if (!did_issue_connect) {
        tell_main_control(SSL_CONNECT, "");
        did_issue_connect = true;
      }
      tell_main_control(SSL_CLIENT_CERTIFICATE, peer_cn);
      ssl_modes = MODE_READ;
      read_len = 0;
      read_goal = sizeof(WireFrameHeader);
      read_buffer = (char *)&ssl_wireFrameHeader;
    }
  }
  
  if (ssl_modes & MODE_READ) {
  more_reading:
    if (read_len < read_goal) {
      rc = SSL_read(ssl_handle, 
                    &read_buffer[read_len], 
                    read_goal - read_len);
      if (verbose) {
        printf("did ssl_read %d\n", rc);
      }

      if (rc > 0) {
        read_len += rc;
        goto more_reading;
      } else {
        rc = SSL_get_error(ssl_handle, rc);
        if (rc == SSL_ERROR_WANT_READ) {
          poll_list[0].events |= POLLIN;
        } else if (rc == SSL_ERROR_WANT_WRITE) {
          poll_list[0].events |= POLLOUT;
        } else if (rc == SSL_ERROR_ZERO_RETURN) {
          // the other side shut down cleanly
          return CLEAN_SHUTDOWN;
        } else {
          // some other error
          exit_reason = "03-823E connection terminated";
          return ERROR_CONDITION;
        }
      }
    } else {
      if (read_buffer == (char *)&ssl_wireFrameHeader) {
        if (verbose) {
          printf("Received Header %02x %02x %02x %02x  %02x %02x %02x %02x\n",
                 ((uint8_t *)&ssl_wireFrameHeader)[0],
                 ((uint8_t *)&ssl_wireFrameHeader)[1],
                 ((uint8_t *)&ssl_wireFrameHeader)[2],
                 ((uint8_t *)&ssl_wireFrameHeader)[3],
                 ((uint8_t *)&ssl_wireFrameHeader)[4],
                 ((uint8_t *)&ssl_wireFrameHeader)[5],
                 ((uint8_t *)&ssl_wireFrameHeader)[6],
                 ((uint8_t *)&ssl_wireFrameHeader)[7]);
        }

        // we only got the header... that tells us how to read the rest
        uint16_t magic = ntohs(ssl_wireFrameHeader.magic);

        if (magic != WIRE_MAGIC) {
          // not talking our language...
          exit_reason = "03-820E protocol error (bad magic)";
          return ERROR_CONDITION;
        }
        uint16_t code = ntohs(ssl_wireFrameHeader.code);
        if (code > 100) {
          exit_reason = "03-821E protocol error (bad code)";
          return ERROR_CONDITION;
        }

        uint32_t len = ntohl(ssl_wireFrameHeader.len);
        if (len > 10000000) {
          exit_reason = "03-822E protocol error (insane length)";
          return ERROR_CONDITION;
        }
        pending.resize(len);
        // reset our read state to read the payload
        read_buffer = &pending[0];
        read_goal = len;
        read_len = 0;
        goto more_reading;
      } else {
        // we've read some real, useful data
        tell_main_data(ntohs(ssl_wireFrameHeader.code));
        // go back and start reading another header
        ssl_modes = MODE_READ;
        read_len = 0;
        read_goal = sizeof(WireFrameHeader);
        read_buffer = (char *)&ssl_wireFrameHeader;
      }
    }
  }

  rc = poll(&poll_list[0], 2, 1000);
  if (rc == 0) {
    if (verbose) {
      printf("(%p) timeout\n", this);
    }
    return KEEP_RUNNING;
  }

  // check our notification queue
  if (poll_list[1].revents & (POLLIN|POLLHUP)) {
    FramePtr fp(outbound.pop());
    if (fp->type == Frame::CONTROL_FRAME) {
      if (fp->code == SSL_DISCONNECT) {
        // we might wind up writing a partial frame, but who cares?
        return CLEAN_SHUTDOWN;
      }
    } else if (fp->type == Frame::DATA_FRAME) {
      ssl_writePending = fp;
      if (unpacker) {
        unpacker(&*ssl_writePending);
      }
      ssl_writeBuffer = (char *)&ssl_writeFrameHeader;
      ssl_writeBufferLen = sizeof(ssl_writeFrameHeader);
      ssl_writeLen = 0;
      ssl_modes |= MODE_WRITE;
      ssl_writeFrameHeader.magic = htons(WIRE_MAGIC);
      ssl_writeFrameHeader.code = htons(ssl_writePending->code);
      ssl_writeFrameHeader.len = htonl(ssl_writePending->data.size());
    }
  }

  return KEEP_RUNNING;
}

int OpenSSLConnection::sslVerifyCallback(int preverify_ok, 
                                     X509_STORE_CTX *ctx)
{
  if (!preverify_ok) {
    printf("Preverification already failed\n");
    // no need to continue... things are broken already
    return 0;
  }

  char    buf[256];
  X509   *err_cert;
  int     err, depth;
  SSL    *ssl;

  err_cert = X509_STORE_CTX_get_current_cert(ctx);
  err = X509_STORE_CTX_get_error(ctx);
  if (err != X509_V_OK) {
    const char *p = X509_verify_cert_error_string(err);
    printf("ERROR %d = %s\n", err, p);
    return 0;
  }

  depth = X509_STORE_CTX_get_error_depth(ctx);

  /*
   * Get the pointer to the SSL connection that is involved
   * in this certificate verification
   */
  int k = SSL_get_ex_data_X509_STORE_CTX_idx();
  ssl = (SSL *)X509_STORE_CTX_get_ex_data(ctx, k);

  /*
   * Get the application specific data (i.e., us) that is stored
   * in the SSL object
   */
  void *data = SSL_get_ex_data(ssl, ssl_cnxnIndex);
  OpenSSLConnection *me = static_cast<OpenSSLConnection*>(data);

  X509_NAME_oneline(X509_get_subject_name(err_cert), buf, sizeof(buf));

  printf("Peer is <%s>\n", buf);
  printf("error %d depth %d\n", err, depth);
  me->peer_cn = buf;
  return 1;
}

void OpenSSLConnection::run()
{
  // this operates in the i/o thread
  printf("(%p) attaching\n", this);
  attach();
  printf("(%p) running\n", this);
  semi_internal_run();
  printf("(%p) done\n", this);
  pthread_mutex_unlock(&destruction);
}

void OpenSSLConnection::send(FramePtr frame)
{
  outbound.push(frame);
}

void OpenSSLConnection::teardown()
{
  Frame *f = new Frame(Frame::CONTROL_FRAME);
  f->code = SSL_DISCONNECT;
  outbound.push(FramePtr(f));
  pthread_mutex_lock(&destruction);
  void *retval;
  pthread_join(ssl_thread, &retval);
}

OpenSSLConnection::~OpenSSLConnection()
{
  SSL_free(ssl_handle);
}

void *do_run(void *_cnx)
{
  OpenSSLConnection *cnx = (OpenSSLConnection *)_cnx;
  cnx->run();
  return NULL;
}

SSLConnection *make_client(int fd, 
                           void (*packer)(Frame *frame),
                           void (*unpacker)(Frame *frame))
{
  OpenSSLConnection *c = new OpenSSLConnection(fd, packer, unpacker, true);
  pthread_create(&c->ssl_thread, NULL, do_run, c);
  return c;
}

SSLConnection *make_server(int fd, 
                           void (*packer)(Frame *frame),
                           void (*unpacker)(Frame *frame))
{
  OpenSSLConnection *c = new OpenSSLConnection(fd, packer, unpacker, false);
  pthread_create(&c->ssl_thread, NULL, do_run, c);
  return c;
}

