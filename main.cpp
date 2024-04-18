

#include <iostream>

#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << "s <port> <password to connect>\n";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);

  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}