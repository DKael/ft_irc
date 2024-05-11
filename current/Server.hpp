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

#include "Channel.hpp"
#include "Message.hpp"
#include "User.hpp"
#include "custom_exception.hpp"
#include "util.hpp"

#define MAX_USER 256
#define POLL_TIMEOUT 5
#define AUTHENTICATE_TIMEOUT 20
#define BLOCK_SIZE 1025
#define BUFFER_SIZE 65536

#define SERVER_NAME "ft_irc"
#define CHAN_TYPE "#&"
#define CHANNEL_MARK '#'
#define INIT_MAX_NICKNAME_LEN 9
#define INIT_MAX_USERNAME_LEN 12

#define INIT_MAX_CHANNEL_NUM 20

// MODE FLAG
#define INVITE_MODE_ON "+i"
#define INVITE_MODE_OFF "-i"

#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define GREEN_BOLD "\033[1;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"
#define DEF_COLOR "\033[0;39m"
#define LF "\e[1K\r"

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

  // CHANNEL
  std::map<std::string, Channel> server_channel_list;
  const int max_channel_num;

  std::map<std::string, Channel>::iterator server_channel_iterator;

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

  const int get_max_channel_num(void) const;
  int get_current_channel_num(void);

  std::map<std::string, Channel>::iterator get_server_channel_iterator(
      std::string targetChannelStr);
  Channel& get_server_channel(
      std::map<std::string, Channel>::iterator iterator);

  void add_tmp_user(const int socket_fd, const sockaddr_in& addr);
  void move_tmp_user_to_user_list(int socket_fd);
  void remove_user(const int socket_fd);
  void remove_user(const std::string& nickname);
  void change_nickname(const std::string& old_nick,
                       const std::string& new_nick);
  void tmp_user_timeout_chk(void);

  int send_msg_at_queue(int socket_fd);

  //////////////////////////////////////////////////////////////////////////////////////////////////////
  /* IMPLEMENTATIONS OF COMMANDS */
  void cmd_pass(int recv_fd, const Message& msg);
  void cmd_nick(int recv_fd, const Message& msg);
  void cmd_user(int recv_fd, const Message& msg);
  void cmd_mode(int recv_fd, const Message& msg);
  void cmd_pong(int recv_fd, const Message& msg);
  void cmd_quit(pollfd& p_val, const Message& msg);

  void cmd_privmsg(int recv_fd, const Message& msg);
  void cmd_join(int recv_fd, const Message& msg);
  void cmd_kick(int recv_fd, const Message& msg);
  void kickClient(User& opUser, User& outUser, Channel& channelName,
                  const Message& msg);
  
  void cmd_invite(int recv_fd, const Message& msg);
  void cmd_topic(int recv_fd, const Message& msg);








  //////////////////////////////////////////////////////////////////////////////////////////////////////

  void addChannel(Channel& newChannel);

  // std::map<std::string, Channel> channelLst;

  User& operator[](const int socket_fd);
  int operator[](const std::string& nickname);

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // [DEBUG] PURPOSE ONLY
  // CLIENT LIST

  std::vector<User> getUserList(void) const {
    std::vector<User> users;
    for (std::map<int, User>::const_iterator it = user_list.begin();
         it != user_list.end(); ++it) {
      users.push_back(it->second);
    }
    return users;
  }

  void visualizeChannelList(void) {
    std::map<std::string, Channel>::const_iterator it;

    std::cout << RED << "[Channel Lists in the server] :: ";
    for (it = server_channel_list.begin(); it != server_channel_list.end();
         ++it) {
      const std::string& channelName = it->first;
      const Channel& channel = it->second;

      std::cout << channelName << " => ";
    }
    std::cout << WHITE << std::endl;
  }

  void cmd_who(int recv_fd, const Message& msg);
  void cmd_names(int recv_fd, const Message& msg);
};

#endif