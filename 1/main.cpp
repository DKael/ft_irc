#include <iostream>
#include "Server.hpp"

int main(int argc, char** argv) {
  try {
    if (argc != 3) {
      std::string Usage = "Usage : ";
      std::string ircserv = argv[0];
      std::string format = " <port> <password>";
      throw (Error(Usage + ircserv + format));
    }
    Server serv(argv[1], argv[2]);
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
