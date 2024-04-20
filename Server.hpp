#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>

#include "User.hpp"
#include "custom_exception.hpp"

#define MAX_USER 1024

class Server {
 private:
  int port;
  std::string s_port;
  std::string password;
  int serv_socket;
  sockaddr_in serv_addr;
  std::string serv_name;
  int user_cnt;
  std::map<int, std::string> fd_to_nickname;
  std::map<std::string, User> user_list;  // key is user's nickname

  // not use
  Server();
  Server(const Server& origin);
  Server& operator=(const Server& origin);

 public:
  Server(const char* _port, const char* _password);
  // Server(const std::string& _port, const std::string& _password);
  ~Server();
  const int get_port(void);
  const std::string& get_s_port(void);
  const std::string& get_password(void);
  const int get_serv_socket(void);
  const int get_user_cnt(void);
  std::string& operator[](int socket_fd);
  User& operator[](const std::string& nickname);
};

#endif