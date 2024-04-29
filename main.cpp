#include <cstring>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "Message.hpp"
#include "Server.hpp"
#include "string_func.hpp"

bool port_chk(const char* input_port) {
  std::stringstream port_chk;
  int port;

  port_chk << std::string(input_port);
  port_chk >> port;
  if (port_chk.fail()) {
    return false;
  } else if (port < 0 || 65335 < port) {
    return false;
  }
  return true;
}

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
