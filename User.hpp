#ifndef USER_HPP
#define USER_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <list>
#include <map>
#include <string>

#include "string_func.hpp"

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
  const std::string dummy;
  const int user_socket;
  const sockaddr_in user_addr;
  const std::time_t created_time;

  std::string nick_name;
  chk_status nick_init_chk;
  std::string user_name;
  std::string real_name;
  chk_status user_init_chk;
  chk_status password_chk;
  chk_status is_authenticated;
  bool have_to_disconnect;
  std::list<std::string> to_send;
  std::map<std::string, int> invited_channels;
  std::map<std::string, int> channels;

  // not use
  User();
  User& operator=(const User& origin);

 public:
  // constructors & desturctor
  User(int _user_socket, const sockaddr_in& _user_addr);
  User(const User& origin);
  ~User();

  // setter functions
  void set_nick_name(const std::string& input);
  void set_nick_init_chk(const chk_status input);
  void set_user_name(const std::string& input);
  void set_real_name(const std::string& input);
  void set_user_init_chk(const chk_status input);
  void set_password_chk(const chk_status input);
  void set_is_authenticated(const chk_status input);
  void set_have_to_disconnect(const bool input);
  void change_nickname(const std::string& new_nick);

  // getter functions
  int get_user_socket(void) const;
  const sockaddr_in& get_user_addr(void) const;
  time_t get_created_time(void) const;
  const std::string& get_nick_name(void) const;
  const std::string& get_nick_name_no_chk(void) const;
  chk_status get_nick_init_chk(void) const;
  const std::string& get_user_name(void) const;
  const std::string& get_real_name(void) const;
  chk_status get_user_init_chk(void) const;
  chk_status get_password_chk(void) const;
  chk_status get_is_authenticated(void) const;
  bool get_have_to_disconnect(void) const;
  const std::map<std::string, int>& get_invited_channels(void) const;
  const std::map<std::string, int>& get_channels(void) const;
  std::string make_source(int mode);

  void push_front_msg(const std::string& msg);
  void push_back_msg(const std::string& msg);
  const std::string& get_front_msg(void) const;
  void pop_front_msg(void);
  std::size_t get_to_send_size(void);

  void push_invitation(std::string& chan_name);
  void remove_invitation(std::string& chan_name);
  void remove_all_invitations(void);
  bool is_invited(std::string& chan_name) const;
  void join_channel(std::string& chan_name);
  void part_channel(std::string& chan_name);
};

#ifdef DEBUG

#include <iostream>
std::ostream& operator<<(std::ostream& out, const User& user);

#endif

#endif