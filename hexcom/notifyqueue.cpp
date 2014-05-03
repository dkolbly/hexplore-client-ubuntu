#include "notifyqueue.h"


void NotifyQueue::push(T const& item)
{
  write(write_side_fd, "q", 1);
}

T NotifyQueue::pop()
{
}
