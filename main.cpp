#include <iostream>
#include <string>

#include "Message.hpp"
#include "Server.hpp"
#include "util.h"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << " <port> <password to connect>\n";
    return 1;
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return 1;
  } else if (ft_strip(std::string(argv[2])).length() == 0) {
    std::cerr << "Empty password!";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);
    Message::map_init();

    serv.listen();

  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
