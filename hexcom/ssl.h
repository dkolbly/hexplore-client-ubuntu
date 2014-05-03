#include <memory>
#include "notifyqueue.h"

struct DataFrame {
  virtual ~DataFrame() { }
};

struct Frame {
  enum FrameType {
    CONTROL_FRAME,
    DATA_FRAME
  };

  Frame(FrameType t) 
    : type(t),
      unpacked(NULL) {
  }

  ~Frame() {
    if (unpacked) {
      delete unpacked;
    }
  }

  FrameType     type;
  unsigned      code;
  std::string   data;           // data on the wire
  DataFrame    *unpacked;       // unpacked into memory (DATA frames only)
};

enum SSLControlCode {
  SSL_CONNECT,
  SSL_DISCONNECT,
  SSL_CLIENT_CERTIFICATE
};

typedef std::shared_ptr<Frame> FramePtr;

/*struct SSLCallbacks {
  virtual int didReceiveFrame(FramePtr ptr) = 0;
  virtual int didControl(FramePtr ptr) = 0;
  };*/

struct SSLConnection {
  virtual ~SSLConnection() { };
  virtual void run() = 0;
  virtual void send(FramePtr frame) = 0;
  virtual NotifyQueue<FramePtr> *getNotificationQueue() = 0;
  virtual void teardown() = 0;
};
  
SSLConnection *make_client(int fd, 
                           void (*packer)(Frame *frame),
                           void (*unpacker)(Frame *frame));

SSLConnection *make_server(int fd, 
                           void (*packer)(Frame *frame),
                           void (*unpacker)(Frame *frame));
