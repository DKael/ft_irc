#include <iostream>

#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << "s <port> <password to connect>\n";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);
    pollfd observe_fd[MAX_USER];
    int event_cnt = 0;

    socklen_t user_addr_len;
    sockaddr_in user_addr;
    int user_socket;

    observe_fd[0].fd = serv.get_serv_socket();
    observe_fd[0].events = POLLIN;

    for (int i = 1; i < MAX_USER; i++) {
      observe_fd[i].fd = -1;
    }

    while (1) {
      event_cnt = poll(observe_fd, MAX_USER, 1000);

      if (event_cnt == 0) {
        continue;
      } else if (event_cnt < 0) {
        std::cerr << "poll() error!\n";
        return 1;
      } else {
        if (observe_fd[0].revents & POLL_IN) {
          if (serv.get_user_cnt() == MAX_USER) {
            std::cerr << "User connection limit exceed!\n";
            // todo : send some error message to client
            continue;
          }
          user_addr_len = sizeof(user_addr);
          user_socket = ::accept(serv.get_serv_socket(), (sockaddr*)&user_addr,
                                 &user_addr_len);
          if (user_socket == -1) {
            // error_handling
          }
          // check PASS, NICK, USER
          // after registration done, make user object
          // and add it to serv's data

          continue;
        }

        for (int i = 1; i < MAX_USER || event_cnt > 0; i++) {
          if (observe_fd[i].fd > 0 && observe_fd[i].revents & POLLIN) {
            // todo : recv message from customer and interpret it
          }
        }
      }
    }

  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}