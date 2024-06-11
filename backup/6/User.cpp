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
      to_send(origin.to_send),
      invited_channels(origin.invited_channels),
      dummy("*") {}

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

const std::vector<std::string>& User::get_invited_channel_vec(void) const { return invited_channels; }


void User::push_msg(const std::string& msg) { to_send.push(msg); }
void User::push_invited_channel(std::string channelName) { invited_channels.push_back(channelName); }
const std::string& User::front_msg(void) { return to_send.front(); }

void User::pop_msg(void) { to_send.pop(); }

std::size_t User::number_of_to_send(void) { return to_send.size(); }

const bool User::isInvited(std::string channelName) {
  for (std::vector<std::string>::const_iterator it = (*this).get_invited_channel_vec().begin(); it != (*this).get_invited_channel_vec().end();++it) {
    std::string channel = *it;
    if (channel == channelName)
      return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
std::ostream& operator<<(std::ostream& out, const User& user) {
  out << GREEN << "\n\t[Client Information]" << WHITE << std::endl
      << "\tNICKNAME :: " << user.get_nick_name() << std::endl
      << "\tUSERNAME :: " << user.get_user_name() << std::endl
      << "\tREALNAME :: " << user.get_real_name() << std::endl
      // << "Client Socket(fd) :: " << user.get_user_socket() << std::endl
      // << "Client address(sockaddr_in) :: " << &user.get_user_addr() << std::endl
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
      std::vector<std::string>::const_iterator cit;
      for (cit = user.get_invited_channel_vec().begin(); cit != user.get_invited_channel_vec().end(); ++cit) {
        std::string channelName = *cit;
        out << channelName << ", ";
      }
      std::cout << std::endl << std::endl;
  return out;
}

void User::removeAllInvitations(void) { 
  invited_channels.clear();
}

void User::removeInvitation(std::string channelName) {
  for (std::vector<std::string>::iterator it = invited_channels.begin(); it != invited_channels.end();) {
      if (*it == channelName) {
          it = invited_channels.erase(it);
      } else {
          ++it;
      }
  }
}



