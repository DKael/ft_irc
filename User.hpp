#ifndef USER_HPP
#define USER_HPP

#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <list>
#include <map>
#include <string>

#include "string_func.hpp"

typedef std::string String;

#define NICKLEN 9
#define USERLEN 12

#define USER_FLAG_I (1 << 0)

#define INIT_PING_OFFSET 60

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

enum chk_status {
  NOT_YET = 0,
  FAIL,
  OK,
};

class User {
 private:
  const String dummy;
  pollfd& pfd;
  const int user_socket;
  const sockaddr_in user_addr;
  const std::time_t created_time;

  String nick_name;
  chk_status nick_init_chk;
  String user_name;
  String real_name;
  chk_status user_init_chk;
  chk_status password_chk;
  chk_status is_authenticated;
  bool have_to_disconnect;
  bool have_to_ping_chk;
  std::time_t last_ping;
  std::list<String> to_send;
  std::map<String, int> invited_channels;
  std::map<String, int> channels;

  int mode;

  // not use
  User();
  User& operator=(const User& origin);

 public:
  // constructors & desturctor
  User(pollfd& _pfd, const sockaddr_in& _user_addr);
  User(const User& origin);
  ~User();

  // setter functions
  void set_nick_name(const String& input);
  void set_nick_init_chk(const chk_status input);
  void set_user_name(const String& input);
  void set_real_name(const String& input);
  void set_user_init_chk(const chk_status input);
  void set_password_chk(const chk_status input);
  void set_is_authenticated(const chk_status input);
  void set_have_to_disconnect(bool input);
  void set_have_to_ping_chk(bool input);
  void set_last_ping(std::time_t input);
  void change_nickname(const String& new_nick);

  // getter functions
  pollfd& get_pfd(void) const;
  int get_user_socket(void) const;
  const sockaddr_in& get_user_addr(void) const;
  const String get_host_ip(void) const;
  time_t get_created_time(void) const;
  const String& get_nick_name(void) const;
  const String& get_nick_name_no_chk(void) const;
  chk_status get_nick_init_chk(void) const;
  const String& get_user_name(void) const;
  const String& get_real_name(void) const;
  chk_status get_user_init_chk(void) const;
  chk_status get_password_chk(void) const;
  chk_status get_is_authenticated(void) const;
  bool get_have_to_disconnect(void) const;
  bool get_have_to_ping_chk(void) const;
  std::time_t get_last_ping(void) const;
  const std::map<String, int>& get_invited_channels(void) const;
  const std::map<String, int>& get_channels(void) const;
  String make_source(int mode);

  void push_front_msg(const String& msg);
  void push_back_msg(const String& msg);
  const String& get_front_msg(void) const;
  void pop_front_msg(void);
  std::size_t get_to_send_size(void);

  void push_invitation(const String& chan_name);
  void remove_invitation(const String& chan_name);
  void remove_all_invitations(void);
  bool is_invited(const String& chan_name) const;
  void join_channel(const String& chan_name);
  void part_channel(const String& chan_name);

  void set_mode(int flag);
  void unset_mode(int flag);
  bool chk_mode(int flag) const;
};

#ifdef DEBUG

#include <iostream>
std::ostream& operator<<(std::ostream& out, const User& user);

#endif

#endif