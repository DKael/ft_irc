#include <iostream>
#include <sstream>
#include <string>

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
  if (argc != 2) {
    std::cerr << "Usage : " << argv[0] << " <port>\n";
    return 1;
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return 1;
  }
  std::cout << "Port check complete!\nPort is " << std::atoi(argv[1]) << '\n';
}
