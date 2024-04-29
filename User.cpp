#include "User.hpp"

User::User(const int _user_socket, const sockaddr_in& _user_addr)
    : user_socket(_user_socket),
      user_addr(_user_addr),
      created_time(std::time(NULL)),
      nick_name(make_random_string(20)),
      nick_init_chk(NOT_YET),
      user_name(""),
      real_name(""),
      user_init_chk(NOT_YET),
      password_chk(NOT_YET),
      is_authenticated(NOT_YET),
      have_to_disconnect(false) {}

User::User(const User& origin)
    : user_socket(origin.user_socket),
      user_addr(origin.user_addr),
      created_time(origin.created_time),
      nick_name(origin.nick_name),
      nick_init_chk(origin.nick_init_chk),
      user_name(origin.user_name),
      real_name(origin.real_name),
      user_init_chk(origin.user_init_chk),
      password_chk(origin.password_chk),
      is_authenticated(origin.is_authenticated),
      have_to_disconnect(origin.have_to_disconnect),
      to_send(origin.to_send) {}

User::~User() {}

void User::set_nick_name(const std::string& input) { nick_name = input; }

void User::set_nick_init_chk(const chk_status input) { nick_init_chk = input; }

void User::set_user_name(const std::string& input) { user_name = input; }

void User::set_real_name(const std::string& input) { real_name = input; }

void User::set_user_init_chk(const chk_status input) { user_init_chk = input; }

void User::set_password_chk(const chk_status input) { password_chk = input; }

void User::set_is_authenticated(const chk_status input) {
  is_authenticated = input;
}

void User::set_have_to_disconnect(const bool input) {
  have_to_disconnect = input;
}

const std::string User::get_nick_name(void) const {
  if (nick_init_chk != NOT_YET) {
    return nick_name;
  } else {
    return "*";
  }
}

const chk_status User::get_nick_init_chk(void) const { return nick_init_chk; }

const std::string& User::get_user_name(void) const { return user_name; }

const std::string& User::get_real_name(void) const { return real_name; }

const chk_status User::get_user_init_chk(void) const { return user_init_chk; }

const int User::get_user_socket(void) const { return user_socket; }

const sockaddr_in& User::get_user_addr(void) const { return user_addr; }

const chk_status User::get_password_chk(void) const { return password_chk; }

const chk_status User::get_is_authenticated(void) const {
  return is_authenticated;
}

const bool User::get_have_to_disconnect(void) const {
  return have_to_disconnect;
}

const time_t User::get_created_time(void) const { return created_time; }

void User::push_msg(const std::string& msg) { to_send.push(msg); }

const std::string& User::front_msg(void) { return to_send.front(); }

void User::pop_msg(void) { to_send.pop(); }

std::size_t User::number_of_to_send(void) { return to_send.size(); }
