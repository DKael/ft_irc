#include <iostream>
#include <string>

#include "Message.hpp"
#include "Server.hpp"
#include "util.hpp"

Server* g_server_ptr;

void on_sigint(int sig) {
  signal(sig, SIG_IGN);

  g_server_ptr->server_quit();
  exit(130);
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << " <port> <password to connect>\n";
    return 1;
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return 1;
  } else if (ft_strip(String(argv[2])).length() == 0) {
    std::cerr << "Empty password!";
    std::cerr << "Usage : " << argv[0] << " <port> <password to connect>\n";
    return 1;
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return 1;
  } else if (ft_strip(String(argv[2])).length() == 0) {
    std::cerr << "Empty password!";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);
    Message::map_init();
    g_server_ptr = &serv;
    signal(SIGINT, on_sigint);

    serv.server_listen();

  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
