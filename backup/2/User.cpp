#include "User.hpp"

User::User(const int _user_socket, const sockaddr_in& _user_addr)
    : user_socket(_user_socket),
      user_addr(_user_addr),
      created_time(std::time(NULL)),
      nick_name(""),
      user_name(""),
      real_name(""),
      password_chk(false),
      is_authenticated(false) {}

User::User(const User& origin)
    : user_socket(origin.user_socket),
      user_addr(origin.user_addr),
      created_time(origin.created_time),
      nick_name(origin.nick_name),
      user_name(origin.user_name),
      real_name(origin.real_name),
      password_chk(origin.password_chk),
      is_authenticated(origin.is_authenticated) {}

User::~User() { ::close(user_socket); }

void User::set_nick_name(const std::string& input) { nick_name = input; }

void User::set_user_name(const std::string& input) { user_name = input; }

void User::set_real_name(const std::string& input) { real_name = input; }

void User::set_password_chk(const bool input) { password_chk = input; }

void User::set_is_authenticated(const bool input) { is_authenticated = input; }

const std::string& User::get_nick_name(void) const { return nick_name; }

const std::string& User::get_user_name(void) const { return user_name; }

const std::string& User::get_real_name(void) const { return real_name; }

const int User::get_user_socket(void) const { return user_socket; }

const sockaddr_in& User::get_user_addr(void) const { return user_addr; }

const bool User::get_password_chk(void) const { return password_chk; }

const bool User::get_is_authenticated(void) const { return is_authenticated; }

const time_t User::get_created_time(void) const { return created_time; }
