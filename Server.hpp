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

typedef std::string String;

#define MAX_USER 256
#define POLL_TIMEOUT 5
#define AUTHENTICATE_TIMEOUT 20
#define PING_INTERVAL 60
#define PING_TIMEOUT 20
#define SOCKET_BUFFER_SIZE 8192

#define SERVER_NAME "ft_irc.net"
#define SERVER_VERSION "ft_irc-1.0"
#define AVAILABLE_USER_MODES "i"
#define AVAILABLE_CHANNEL_MODES "iklost"

#define IRC_PROTOCOL "RFC2812"
#define IRCD "ngIRCd"
#define CHARSET "UTF-8"
#define CASEMAPPING "ascii"
#define PREFIX "@%+"
#define CHANNELNUM 20

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
  String str_port;
  String serv_name;
  String serv_version;
  String chantypes;
  std::time_t created_time;
  String created_time_str;

  String password;
  int serv_socket;
  sockaddr_in serv_addr;
  bool enable_ident_protocol;

  pollfd observe_fd[MAX_USER];
  std::map<in_addr_t, int> ip_list;
  std::map<int, User> tmp_user_list;
  std::map<String, int> tmp_nick_to_soc;
  std::map<int, User> user_list;
  std::map<String, int> nick_to_soc;

  // CHANNEL
  std::map<String, Channel> channel_list;

  // private functions
  int user_socket_init(void);
  void connection_fin(pollfd& p_val);
  void ft_send(pollfd& p_val);
  void ft_sendd(pollfd& p_val);
  int send_msg_at_queue(int socket_fd);
  int send_msg_block(int socket_fd, const String& blk);
  void revent_pollout(pollfd& p_val);
  void revent_pollin(pollfd& p_val);
  void auth_user(pollfd& p_val, std::vector<String>& msg_list);
  void not_auth_user(pollfd& p_val, std::vector<String>& msg_list);
  void auth_complete(pollfd& p_val);

  // not use
  Server();
  Server(const Server& origin);
  Server& operator=(const Server& origin);

 public:
  Server(const char* _port, const char* _password);
  ~Server();

  void listen(void);

  int get_port(void) const;
  const String& get_str_port(void) const;
  const String& get_serv_name(void) const;
  const String& get_serv_version(void) const;
  const String& get_password(void) const;
  const std::time_t& get_created_time(void) const;
  const String& get_created_time_str(void) const;
  int get_serv_socket(void) const;
  const sockaddr_in& get_serv_addr(void) const;
  int get_tmp_user_cnt(void) const;
  int get_user_cnt(void) const;
  bool get_enable_ident_protocol(void) const;
  int get_channel_num(void) const;

  void add_tmp_user(pollfd& pfd, const sockaddr_in& addr);
  void move_tmp_user_to_user_list(int socket_fd);
  void remove_user(const int socket_fd);
  void remove_user(const String& nickname);
  void change_nickname(const String& old_nick, const String& new_nick);
  void tmp_user_timeout_chk(void);
  void user_ping_chk(void);

  User& operator[](int socket_fd);
  int operator[](const String& nickname);

  void add_channel(Channel& new_chan);
  bool chk_channel_exist(const String& chan_name) const;
  void send_msg_to_channel(Channel& chan, const String& msg);
  void send_msg_to_channel_except_sender(Channel& chan, const String& sender,
                                         const String& msg);
  void send_msg_to_connected_user(const User& u, const String& msg);

  //////////////////////////////////////////////////////////////////////////////////////////////////////
  /* IMPLEMENTATIONS OF COMMANDS */
  void cmd_pass(int recv_fd, const Message& msg);
  void cmd_nick(int recv_fd, const Message& msg);
  void cmd_user(int recv_fd, const Message& msg);
  void cmd_mode(int recv_fd, const Message& msg);
  void cmd_ping(int recv_fd, const Message& msg);
  void cmd_pong(int recv_fd, const Message& msg);
  void cmd_quit(int recv_fd, const Message& msg);

  void cmd_privmsg(int recv_fd, const Message& msg);
  void cmd_join(int recv_fd, const Message& msg);
  void cmd_kick(int recv_fd, const Message& msg);

  void cmd_invite(int recv_fd, const Message& msg);
  void cmd_topic(int recv_fd, const Message& msg);
  void cmd_who(int recv_fd, const Message& msg);
  void cmd_names(int recv_fd, const Message& msg);

  //////////////////////////////////////////////////////////////////////////////////////////////////////

  ////////////////////////////////////////////////////////////////////////////////////////////////////////
  // [DEBUG] PURPOSE ONLY
  // user LIST

  std::vector<User> getUserList(void) const {
    std::vector<User> users;
    for (std::map<int, User>::const_iterator it = user_list.begin();
         it != user_list.end(); ++it) {
      users.push_back(it->second);
    }
    return users;
  }

  void visualizeChannelList(void) {
    std::map<String, Channel>::const_iterator it;

    std::cout << RED << "[Channel Lists in the server] :: ";
    for (it = channel_list.begin(); it != channel_list.end(); ++it) {
      const String& chan_name = it->first;
      const Channel& channel = it->second;

      std::cout << chan_name << " => ";
    }
    std::cout << WHITE << std::endl;
  }
};

#endif