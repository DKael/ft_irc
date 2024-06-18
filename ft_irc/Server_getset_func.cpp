#include "Server.hpp"

int Server::get_port(void) const { return port; }

int Server::get_serv_socket(void) const { return serv_socket; }

int Server::get_tmp_user_cnt(void) const { return tmp_user_list.size(); }

int Server::get_user_cnt(void) const { return user_list.size(); }

int Server::get_channel_num(void) const { return channel_list.size(); };

bool Server::get_enable_ident_protocol(void) const {
  return enable_ident_protocol;
}

const String& Server::get_str_port(void) const { return str_port; }

const String& Server::get_serv_name(void) const { return serv_name; }

const String& Server::get_serv_version(void) const { return serv_version; }

const String& Server::get_password(void) const { return password; }

const String& Server::get_serv_info(void) const { return serv_info; }

const String& Server::get_created_time_str(void) const {
  return created_time_str;
}

const sockaddr_in& Server::get_serv_addr(void) const { return serv_addr; }

const std::time_t& Server::get_created_time(void) const { return created_time; }

void Server::set_enable_ident_protocol(bool input) {
  enable_ident_protocol = input;
}

void Server::set_serv_info(const String& input) { serv_info = input; }
