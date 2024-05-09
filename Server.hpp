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
#include <sstream>
#include <string>

#include "Message.hpp"
#include "User.hpp"
#include "custom_exception.hpp"
#include "util.hpp"

#define MAX_USER 256
#define POLL_TIMEOUT 5
#define AUTHENTICATE_TIMEOUT 20
#define BUFFER_SIZE 65536

#define SERVER_NAME "ft_irc"
#define CHAN_TYPE "#&"
#define INIT_MAX_NICKNAME_LEN 9
#define INIT_MAX_USERNAME_LEN 12

class Server {
 private:
  int port;
  std::string str_port;
  std::string serv_name;
  std::string chantype;

  std::string password;
  int serv_socket;
  sockaddr_in serv_addr;

  pollfd observe_fd[MAX_USER];
  std::map<int, User> tmp_user_list;
  std::map<std::string, int> tmp_nick_to_soc;
  std::map<int, User> user_list;
  std::map<std::string, int> nick_to_soc;

  bool enable_ident_protocol;
  std::size_t max_nickname_len;
  std::size_t max_username_len;

  int client_socket_init(void);
  void revent_pollout(pollfd& p_val);
  void revent_pollin(pollfd& p_val);
  void auth_user(pollfd& p_val, std::vector<std::string>& msg_list);
  void not_auth_user(pollfd& p_val, std::vector<std::string>& msg_list);

  // not use
  Server();
  Server(const Server& origin);
  Server& operator=(const Server& origin);

 public:
  Server(const char* _port, const char* _password);
  ~Server();

  void listen(void);

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
  void move_tmp_user_to_user_list(int socket_fd);
  void remove_user(const int socket_fd);
  void remove_user(const std::string& nickname);
  void change_nickname(const std::string& old_nick,
                       const std::string& new_nick);
  void tmp_user_timeout_chk(void);

  int send_msg_at_queue(int socket_fd);

  User& operator[](const int socket_fd);
  int operator[](const std::string& nickname);

  void cmd_pass(int recv_fd, const Message& msg);
  void cmd_nick(int recv_fd, const Message& msg);
  void cmd_user(int recv_fd, const Message& msg);
  void cmd_mode(int recv_fd, const Message& msg);
  void cmd_pong(int recv_fd, const Message& msg);
  void cmd_quit(pollfd& p_val, const Message& msg);
};

#endif