#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : s_port(_port), password(_password) {
  port = std::atoi(_port);
  if (port < 0 || port > 65535) {
    throw port_range_error();
  }

  serv_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  if (::bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    throw socket_bind_error();
  }

  if (::listen(serv_socket, 128) == -1) {
    throw socket_listening_error();
  }

  std::cout << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":"
            << port << '\n';
}

// Server::Server(const std::string& _port, const std::string& _password) {}

Server::~Server() { ::close(serv_socket); }
