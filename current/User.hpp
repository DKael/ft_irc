#ifndef USER_HPP
#define USER_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <queue>
#include <string>

#include "string_func.hpp"

#define BLACK       "\033[0;30m"
#define RED         "\033[0;31m"
#define GREEN       "\033[0;32m"
#define GREEN_BOLD  "\033[1;32m"
#define YELLOW      "\033[0;33m"
#define BLUE        "\033[0;34m"
#define PURPLE      "\033[0;35m"
#define CYAN        "\033[0;36m"
#define WHITE       "\033[0;37m"
#define DEF_COLOR   "\033[0;39m"
#define LF          "\e[1K\r"

enum chk_status {
  NOT_YET = 0,
  FAIL,
  OK,
};

class User
{
 private:
  const int               user_socket;
  const sockaddr_in       user_addr;
  std::time_t             created_time;

  std::string             nick_name;
  chk_status              nick_init_chk;
  std::string             user_name;
  std::string             real_name;
  chk_status              user_init_chk;
  chk_status              password_chk;
  chk_status              is_authenticated;
  bool                    have_to_disconnect;

  std::queue<std::string> to_send;

  // not use
  User();
  User& operator=(const User& origin);

 public:
  User(const int _user_socket, const sockaddr_in& _user_addr);
  User(const User& origin);
  ~User();

  void set_nick_name(const std::string& input);
  void set_nick_init_chk(const chk_status input);
  void set_user_name(const std::string& input);
  void set_real_name(const std::string& input);
  void set_user_init_chk(const chk_status input);
  void set_password_chk(const chk_status input);
  void set_is_authenticated(const chk_status input);
  void set_have_to_disconnect(const bool input);

  const int get_user_socket(void) const;
  const sockaddr_in& get_user_addr(void) const;
  const time_t get_created_time(void) const;
  const std::string& get_nick_name(void) const;
  const chk_status get_nick_init_chk(void) const;
  const std::string& get_user_name(void) const;
  const std::string& get_real_name(void) const;
  const chk_status get_user_init_chk(void) const;
  const chk_status get_password_chk(void) const;
  const chk_status get_is_authenticated(void) const;
  const bool get_have_to_disconnect(void) const;

  void push_msg(const std::string& msg);
  const std::string& front_msg(void);
  void pop_msg(void);
  std::size_t number_of_to_send(void);
};

inline std::ostream& operator<<(std::ostream& out, User user) 
{
  out << GREEN << "[Client Information]" << WHITE << std::endl
      << "NICKNAME :: " << user.get_nick_name() << std::endl
      << "USERNAME :: " << user.get_user_name() << std::endl
      << "REALNAME :: " << user.get_real_name() << std::endl
      << "Client Socket(fd) :: " << user.get_user_socket() << std::endl
      << "Client address(sockaddr_in) :: " << &user.get_user_addr() << std::endl
      << "Client created time :: " << user.get_created_time() << std::endl;
      if (user.get_password_chk() == OK)
        out << "STATUS PASSWORD :: OK" << std::endl;
      else if (user.get_password_chk() == FAIL)
        out << "STATUS PASSWORD :: FAILED" << std::endl;
      if (user.get_is_authenticated() == OK)
        out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
      else if (user.get_is_authenticated() == FAIL)
        out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
  return (out);
}

#endif