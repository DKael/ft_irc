

#include <iostream>

#include "Server.hpp"

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : ./" << argv[0] << "s <port> <password to connect>\n";
  }

  try {
    Server serv(argv[1], argv[2]);
  } catch (port_range_error& e) {
    std::cerr << e.what() << '\n';
  } catch (socket_create_error& e) {
    std::cerr << e.what() << '\n';
  } catch (socket_bind_error& e) {
    std::cerr << e.what() << '\n';
  } catch (socket_listening_error& e) {
    std::cerr << e.what() << '\n';
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}