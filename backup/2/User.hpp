#ifndef USER_HPP
#define USER_HPP

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <string>

class User {
 private:
  const int user_socket;
  const sockaddr_in user_addr;
  std::time_t created_time;

  std::string nick_name;
  std::string user_name;
  std::string real_name;
  bool password_chk;
  bool is_authenticated;

  // not use
  User();
  User& operator=(const User& origin);

 public:
  User(const int _user_socket, const sockaddr_in& _user_addr);
  User(const User& origin);
  ~User();

  void set_nick_name(const std::string& input);
  void set_user_name(const std::string& input);
  void set_real_name(const std::string& input);
  void set_password_chk(const bool input);
  void set_is_authenticated(const bool input);

  const int get_user_socket(void) const;
  const sockaddr_in& get_user_addr(void) const;
  const time_t get_created_time(void) const;
  const std::string& get_nick_name(void) const;
  const std::string& get_user_name(void) const;
  const std::string& get_real_name(void) const;
  const bool get_password_chk(void) const;
  const bool get_is_authenticated(void) const;
};

#endif