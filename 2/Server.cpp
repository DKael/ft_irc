#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : s_port(_port), password(_password), user_cnt(0) {
  port = std::atoi(_port);
  if (port < 0 || port > 65535) {
    throw port_range_error();
  }

  serv_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }

  int flag;
  flag = ::fcntl(serv_socket, F_GETFL, 0);
  if (::fcntl(serv_socket, F_SETFL, flag | O_NONBLOCK) == -1) {
    throw std::exception();
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

Server::~Server() {
  ::close(serv_socket);
  std::map<int, std::string>::iterator head = fd_to_nickname.begin();
  std::map<int, std::string>::iterator tail = fd_to_nickname.end();

  for (; head != tail; head++) {
    ::close(head->first);
  }
}

const int Server::get_port(void) { return port; }

const std::string& Server::get_s_port(void) { return s_port; }

const std::string& Server::get_password(void) { return password; }

const int Server::get_serv_socket(void) { return serv_socket; }

const int Server::get_user_cnt(void) { return user_cnt; }

std::string& Server::operator[](int socket_fd) {
  if (fd_to_nickname.find(socket_fd) != fd_to_nickname.end()) {
    return fd_to_nickname[socket_fd];
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

User& Server::operator[](const std::string& nickname) {
  if (user_list.find(nickname) != user_list.end()) {
    return user_list[nickname];
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}
