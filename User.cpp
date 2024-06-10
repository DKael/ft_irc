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
      invited_channels(origin.invited_channels),
      // join_channels(origin.join_channels),
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

// getter functions

const int User::get_user_socket(void) const { return user_socket; }

const sockaddr_in& User::get_user_addr(void) const { return user_addr; }

const time_t User::get_created_time(void) const { return created_time; }

const std::string& User::get_nick_name(void) const {
  if (nick_init_chk != NOT_YET) {
    return nick_name;
  } else {
    return dummy;
  }
}

const std::string& User::get_nick_name_no_chk(void) const { return nick_name; }

const chk_status User::get_nick_init_chk(void) const { return nick_init_chk; }

const std::string& User::get_user_name(void) const { return user_name; }

const std::string& User::get_real_name(void) const { return real_name; }

const chk_status User::get_user_init_chk(void) const { return user_init_chk; }

const chk_status User::get_password_chk(void) const { return password_chk; }

const chk_status User::get_is_authenticated(void) const {
  return is_authenticated;
}

const bool User::get_have_to_disconnect(void) const {
  return have_to_disconnect;
}

const std::map<std::string, int>& User::get_invited_channels(void) const {
  return invited_channels;
}

// const std::map<std::string, int>& User::get_join_channels(void) const {
//   return join_channels;
// }

void User::push_invited_channel(std::string& channelName) {
  std::map<std::string, int>::iterator it = invited_channels.find(channelName);

  if (it == invited_channels.end()) {
    invited_channels.insert(std::pair<std::string, int>(channelName, 0));
  }
}

const bool User::is_invited(std::string& channelName) const {
  std::map<std::string, int>::const_iterator cit =
      invited_channels.find(channelName);

  if (cit != invited_channels.end()) {
    return true;
  } else {
    return false;
  }
}

void User::remove_all_invitations(void) { invited_channels.clear(); }

void User::remove_invitation(std::string& channelName) {
  std::map<std::string, int>::iterator it = invited_channels.find(channelName);

  if (it != invited_channels.end()) {
    invited_channels.erase(it);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
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
  std::string channelName;
  for (cit = user.get_invited_channels().begin();
       cit != user.get_invited_channels().end(); ++cit) {
    channelName = (*cit).first;
    out << channelName << ", ";
  }
  std::cout << "\n\n";
  return out;
}
