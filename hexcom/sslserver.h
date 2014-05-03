#ifndef _H_HEXPLORE_SSLSERVER
#define _H_HEXPLORE_SSLSERVER

#include <vector>

template <class ConnectionType>
void SSLServer(int sock, ConnectionType newConnectionFunc(int fd))
{
  std::vector<ConnectionType> active;

  while (1) {
    struct pollfd poll_list[100];
    int num;

    poll_list[0].fd = sock;
    poll_list[0].events = POLLIN;
    poll_list[0].revents = 0;
    num = 1;

    for (typename std::vector<ConnectionType>::iterator i=active.begin();
         i != active.end();
         ++i) {
      /*ConnectionType *cnx = *i;
        NotifyQueue<FramePtr> *nq = cnx->getNotificationQueue();*/
      poll_list[num].fd = (*i)->watch_filedes();
      poll_list[num].events = POLLIN;
      poll_list[num].revents = 0;
      num++;
    }
    printf("num=%d\n", num);
    int rc = poll(&poll_list[0], num, 1000);
    if (rc < 0) {
      perror("poll");
    }
    if (rc == 0) {
      printf("main timeout\n");
    } else {
      std::vector<int> kill;
      for (int i=0; (rc > 0) && (i<num); i++) {
        if (poll_list[i].revents) {
          if (i == 0) {
            struct sockaddr_in peer;
            socklen_t peer_len;
            peer_len = sizeof(peer);
            int cnx_fd = accept(sock, (struct sockaddr *)&peer, &peer_len);
            active.push_back(newConnectionFunc(cnx_fd));
          } else {
            if (active[i-1]->process() < 0) {
              kill.push_back(i-1);
            }
          }
        }
      }
      // clean up the kill list
      if (kill.size() > 0) {
        printf("%zu to kill\n", kill.size());
        while (!kill.empty()) {
          int i = kill.back();
          printf("  kill %d\n", i);
          active.erase(active.begin()+i);
          kill.pop_back();
        }
      }
    }
  }
}
#endif /* _H_HEXPLORE_SSLSERVER */
