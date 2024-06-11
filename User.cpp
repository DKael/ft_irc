#include "User.hpp"

User::User(int _user_socket, const sockaddr_in& _user_addr)
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
      have_to_disconnect(false),
      dummy("*") {}

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
      to_send(origin.to_send),
      invited_channels(origin.invited_channels),
      channels(origin.channels),
      dummy("*") {}

User::~User() {}

// setter functions

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

void User::change_nickname(const std::string& new_nick) {
  nick_name = new_nick;
  invited_channels.clear();
}

// getter functions

int User::get_user_socket(void) const { return user_socket; }

const sockaddr_in& User::get_user_addr(void) const { return user_addr; }

time_t User::get_created_time(void) const { return created_time; }

const std::string& User::get_nick_name(void) const {
  if (nick_init_chk != NOT_YET) {
    return nick_name;
  } else {
    return dummy;
  }
}

const std::string& User::get_nick_name_no_chk(void) const { return nick_name; }

chk_status User::get_nick_init_chk(void) const { return nick_init_chk; }

const std::string& User::get_user_name(void) const { return user_name; }

const std::string& User::get_real_name(void) const { return real_name; }

chk_status User::get_user_init_chk(void) const { return user_init_chk; }

chk_status User::get_password_chk(void) const { return password_chk; }

chk_status User::get_is_authenticated(void) const { return is_authenticated; }

bool User::get_have_to_disconnect(void) const { return have_to_disconnect; }

const std::map<std::string, int>& User::get_invited_channels(void) const {
  return invited_channels;
}

const std::map<std::string, int>& User::get_channels(void) const {
  return channels;
}

/*
mode 1 : <nickname>!<user>@<host>
mode 2 : <nickname>!<user>
mode 3 : <nickname>
*/
std::string User::make_source(int mode = 1) {
  std::string source = nick_name;
  std::string ip;

  if (mode <= 2) {
    source += "!";
    source += user_name;
  }
  if (mode <= 1) {
    source += "@";
    ip = inet_ntoa(user_addr.sin_addr);
    if (ip == "127.0.0.1") {
      source += "localhost";
    } else {
      source += ip;
    }
  }
  return source;
}

void User::push_front_msg(const std::string& msg) { to_send.push_front(msg); }

void User::push_back_msg(const std::string& msg) { to_send.push_back(msg); }

const std::string& User::get_front_msg(void) const { return to_send.front(); }

void User::pop_front_msg(void) { to_send.pop_front(); }

std::size_t User::get_to_send_size(void) { return to_send.size(); }

void User::push_invitation(std::string& chan_name) {
  std::map<std::string, int>::iterator it = invited_channels.find(chan_name);

  if (it == invited_channels.end()) {
    invited_channels.insert(std::pair<std::string, int>(chan_name, 0));
  }
}

void User::remove_invitation(std::string& chan_name) {
  std::map<std::string, int>::iterator it = invited_channels.find(chan_name);

  if (it != invited_channels.end()) {
    invited_channels.erase(it);
  }
}

void User::remove_all_invitations(void) { invited_channels.clear(); }

bool User::is_invited(std::string& chan_name) const {
  std::map<std::string, int>::const_iterator cit =
      invited_channels.find(chan_name);

  if (cit != invited_channels.end()) {
    return true;
  } else {
    return false;
  }
}

void User::join_channel(std::string& chan_name) {
  std::map<std::string, int>::iterator it = channels.find(chan_name);

  if (it == channels.end()) {
    channels.insert(std::pair<std::string, int>(chan_name, 0));
    if (is_invited(nick_name) == true) {
      invited_channels.erase(chan_name);
    }
  }
}

void User::part_channel(std::string& chan_name) {
  std::map<std::string, int>::iterator it = channels.find(chan_name);

  if (it != channels.end()) {
    channels.erase(chan_name);
  }
}

#ifdef DEBUG

std::ostream& operator<<(std::ostream& out, const User& user) {
  out << GREEN << "\n\t[Client Information]" << WHITE << std::endl
      << "\tNICKNAME :: " << user.get_nick_name() << std::endl
      << "\tUSERNAME :: " << user.get_user_name() << std::endl
      << "\tREALNAME :: " << user.get_real_name()
      << std::endl
      // << "Client Socket(fd) :: " << user.get_user_socket() << std::endl
      // << "Client address(sockaddr_in) :: " << &user.get_user_addr() <<
      // std::endl
      // << "Client created time :: " << user.get_created_time() << std::endl;
      // if (user.get_password_chk() == OK)
      //   out << "STATUS PASSWORD :: OK" << std::endl;
      // else if (user.get_password_chk() == FAIL)
      //   out << "STATUS PASSWORD :: FAILED" << std::endl;
      // if (user.get_is_authenticated() == OK)
      //   out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
      // else if (user.get_is_authenticated() == FAIL)
      //   out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
      << "\n\tInvited Channel Lists :: ";
  std::map<std::string, int>::const_iterator cit;
  std::string chan_name;
  for (cit = user.get_invited_channels().begin();
       cit != user.get_invited_channels().end(); ++cit) {
    chan_name = (*cit).first;
    out << chan_name << ", ";
  }
  std::cout << "\n\n";
  return out;
}

#endif