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

#include "Message.hpp"
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
  std::map<std::string, int> tmp_nick_to_soc;
  std::map<int, User> user_list;
  std::map<std::string, int> nick_to_soc;

  bool enable_ident_protocol;

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
  const bool get_enable_ident_protocol(void) const;

  void add_tmp_user(const int socket_fd, const sockaddr_in& addr);
  void add_user(const User& input);
  void move_tmp_user_to_user_list(int socket_fd);
  void remove_user(const int socket_fd);
  void remove_user(const std::string& nickname);
  void change_nickname(const std::string& old_nick,
                       const std::string& new_nick);
  void tmp_user_timeout_chk(void);

  int send_msg(int socket_fd);

  User& operator[](const int socket_fd);
  int operator[](const std::string& nickname);
};

#endif