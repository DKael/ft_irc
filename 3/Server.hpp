#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <map>
#include <string>

#include "User.hpp"
#include "custom_exception.hpp"

#define MAX_USER 1024
#define AUTHENTICATE_TIMEOUT 20

class Server {
 private:
  const int port;
  const std::string str_port;
  const std::string serv_name;

  std::string password;
  int serv_socket;
  sockaddr_in serv_addr;

  std::map<int, User> tmp_user_list;
  std::map<int, User> user_list;

  // not use
  Server();
  Server(const Server& origin);
  Server& operator=(const Server& origin);

 public:
  Server(const char* _port, const char* _password);
  ~Server();
  const int get_port(void) const;
  const std::string& get_str_port(void) const;
  const std::string& get_serv_name(void) const;
  const std::string& get_password(void) const;
  const int get_serv_socket(void) const;
  const sockaddr_in& get_serv_addr(void) const;
  const int get_tmp_user_cnt(void) const;
  const int get_user_cnt(void) const;

  void add_tmp_user(const int socket_fd, const sockaddr_in& addr);
  void add_user(const User& input);
  User& operator[](const int socket_fd);
};

#endif