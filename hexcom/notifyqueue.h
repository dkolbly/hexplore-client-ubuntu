#ifndef _H_HEXCOM_NOTIFYQUEUE
#define _H_HEXCOM_NOTIFYQUEUE

#include <deque>
#include <pthread.h>
#include <unistd.h>

template<class T>
struct NotifyQueue {

private:
  struct Locked {
    Locked(pthread_mutex_t *lock)
      : the_lock(lock) { 
      pthread_mutex_lock(the_lock);
    }
    ~Locked() {
      pthread_mutex_unlock(the_lock);
    }
  private:
    pthread_mutex_t *the_lock;
  };

public:
  NotifyQueue();
  ~NotifyQueue();

  void push(T const& item) {
    {
      Locked scope_lock(&lock);
      pending.push_back(item);
    }
    write(write_side_fd, "q", 1);
  }

  T pop() {
    char ch;
    read(read_side_fd, &ch, 1);

    Locked scope_lock(&lock);
    if (pending.empty()) {
      return T();
    }
    T tmp(pending.front());
    pending.pop_front();
    return tmp;
  }

  int watch_filedes() {
    return read_side_fd;
  }

private:
  int                   read_side_fd;
  int                   write_side_fd;
  pthread_mutex_t       lock;
  std::deque<T>         pending;
};

#endif /* _H_HEXCOM_NOTIFYQUEUE */
