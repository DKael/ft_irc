#include "User.hpp"

User::User(pollfd& _pfd, const sockaddr_in& _user_addr)
    : dummy("*"),
      pfd(_pfd),
      user_socket(_pfd.fd),
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
      have_to_ping_chk(false),
      last_ping(0),
      mode(0) {}

User::User(const User& origin)
    : dummy("*"),
      pfd(origin.pfd),
      user_socket(origin.user_socket),
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
      have_to_ping_chk(origin.have_to_ping_chk),
      last_ping(origin.last_ping),
      to_send(origin.to_send),
      invited_channels(origin.invited_channels),
      channels(origin.channels),
      mode(origin.mode) {}

User::~User() {}

// setter functions

void User::set_nick_name(const String& input) { nick_name = input; }

void User::set_nick_init_chk(const chk_status input) { nick_init_chk = input; }

void User::set_user_name(const String& input) { user_name = input; }

void User::set_real_name(const String& input) { real_name = input; }

void User::set_user_init_chk(const chk_status input) { user_init_chk = input; }

void User::set_password_chk(const chk_status input) { password_chk = input; }

void User::set_is_authenticated(const chk_status input) {
  is_authenticated = input;
}

void User::set_have_to_disconnect(bool input) { have_to_disconnect = input; }

void User::set_have_to_ping_chk(bool input) { have_to_ping_chk = input; }

void User::set_last_ping(std::time_t input) { last_ping = input; }

void User::change_nickname(const String& new_nick) {
  nick_name = new_nick;
  invited_channels.clear();
}

// getter functions

pollfd& User::get_pfd(void) const { return pfd; }

int User::get_user_socket(void) const { return user_socket; }

const sockaddr_in& User::get_user_addr(void) const { return user_addr; }

const String User::get_host_ip(void) const {
  String ip;
  ip = inet_ntoa(user_addr.sin_addr);
  if (ip == "127.0.0.1") {
    ip = "localhost";
  }
  return ip;
}

time_t User::get_created_time(void) const { return created_time; }

const String& User::get_nick_name(void) const {
  if (nick_init_chk != NOT_YET) {
    return nick_name;
  } else {
    return dummy;
  }
}

const String& User::get_nick_name_no_chk(void) const { return nick_name; }

chk_status User::get_nick_init_chk(void) const { return nick_init_chk; }

const String& User::get_user_name(void) const { return user_name; }

const String& User::get_real_name(void) const { return real_name; }

chk_status User::get_user_init_chk(void) const { return user_init_chk; }

chk_status User::get_password_chk(void) const { return password_chk; }

chk_status User::get_is_authenticated(void) const { return is_authenticated; }

bool User::get_have_to_disconnect(void) const { return have_to_disconnect; }

bool User::get_have_to_ping_chk(void) const { return have_to_ping_chk; }

std::time_t User::get_last_ping(void) const { return last_ping; }

int User::get_mode(void) const { return mode; }

const std::map<String, int>& User::get_invited_channels(void) const {
  return invited_channels;
}

const std::map<String, int>& User::get_channels(void) const { return channels; }

/*
mode 1 : <nickname>!<user>@<host>
mode 2 : <nickname>!<user>
mode 3 : <nickname>
*/
String User::make_source(int mode = 1) {
  String source = nick_name;
  String ip;

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

void User::push_front_msg(const String& msg) { to_send.push_front(msg); }

void User::push_back_msg(const String& msg) { to_send.push_back(msg); }

const String& User::get_front_msg(void) const { return to_send.front(); }

void User::pop_front_msg(void) { to_send.pop_front(); }

std::size_t User::get_to_send_size(void) { return to_send.size(); }

void User::push_invitation(const String& chan_name) {
  std::map<String, int>::iterator it = invited_channels.find(chan_name);

  if (it == invited_channels.end()) {
    invited_channels.insert(std::pair<String, int>(chan_name, 0));
  }
}

void User::remove_invitation(const String& chan_name) {
  std::map<String, int>::iterator it = invited_channels.find(chan_name);

  if (it != invited_channels.end()) {
    invited_channels.erase(it);
  }
}

void User::remove_all_invitations(void) { invited_channels.clear(); }

bool User::is_invited(const String& chan_name) const {
  std::map<String, int>::const_iterator cit = invited_channels.find(chan_name);

  if (cit != invited_channels.end()) {
    return true;
  } else {
    return false;
  }
}

void User::join_channel(const String& chan_name) {
  std::map<String, int>::iterator it = channels.find(chan_name);

  if (it == channels.end()) {
    channels.insert(std::pair<String, int>(chan_name, 0));
    if (is_invited(nick_name) == true) {
      invited_channels.erase(chan_name);
    }
  }
}

void User::part_channel(const String& chan_name) {
  std::map<String, int>::iterator it = channels.find(chan_name);

  if (it != channels.end()) {
    channels.erase(chan_name);
  }
}

void User::set_mode(int flag) { mode |= flag; }

void User::unset_mode(int flag) { mode &= ~flag; }

bool User::chk_mode(int flag) const { return mode & flag; }

void User::set_mode(char flag) {
  if (flag == USER_FLAG_I_CHAR) {
    mode |= USER_FLAG_I;
  }
}
void User::unset_mode(char flag) {
  if (flag == USER_FLAG_I_CHAR) {
    mode &= ~USER_FLAG_I;
  }
}
bool User::chk_mode(char flag) const {
  if (flag == USER_FLAG_I_CHAR) {
    return mode & USER_FLAG_I;
  }
  return false;
}

String User::make_mode_str(void) {
  String mode_str = "";

  if (mode & USER_FLAG_I) {
    mode_str += "i";
  }

  return mode_str;
}

#ifdef DEBUG

std::ostream& operator<<(std::ostream& out, const User& user) {
  out << GREEN << "\n\t[user Information]" << WHITE << std::endl
      << "\tNICKNAME :: " << user.get_nick_name() << std::endl
      << "\tUSERNAME :: " << user.get_user_name() << std::endl
      << "\tREALNAME :: " << user.get_real_name()
      << std::endl
      // << "user Socket(fd) :: " << user.get_user_socket() << std::endl
      // << "user address(sockaddr_in) :: " << &user.get_user_addr() <<
      // std::endl
      // << "user created time :: " << user.get_created_time() << std::endl;
      // if (user.get_password_chk() == OK)
      //   out << "STATUS PASSWORD :: OK" << std::endl;
      // else if (user.get_password_chk() == FAIL)
      //   out << "STATUS PASSWORD :: FAILED" << std::endl;
      // if (user.get_is_authenticated() == OK)
      //   out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
      // else if (user.get_is_authenticated() == FAIL)
      //   out << "AUTHENTICATION :: AUTHENTICATED" << std::endl;
      << "\n\tInvited Channel Lists :: ";
  std::map<String, int>::const_iterator cit;
  String chan_name;
  for (cit = user.get_invited_channels().begin();
       cit != user.get_invited_channels().end(); ++cit) {
    chan_name = (*cit).first;
    out << chan_name << ", ";
  }
  std::cout << "\n\n";
  return out;
}

#endif