#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port))
    ,  str_port(_port)
    , serv_name("ft_irc")
    , password(_password) 
{
  serv_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);
  if (::fcntl(serv_socket, F_SETFL, O_NONBLOCK) == -1) {
    throw std::exception();
  }

  if (::bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    throw socket_bind_error();
  }

  if (::listen(serv_socket, 128) == -1) {
    throw socket_listening_error();
  }

  std::cout << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":" << port << std::endl;
}

Server::~Server()
{
  ::close(serv_socket);
  std::map<int, User>::iterator head = user_list.begin();
  std::map<int, User>::iterator tail = user_list.end();

  for (; head != tail; head++) {
    ::close(head->first);
  }

  head = tmp_user_list.begin();
  tail = tmp_user_list.end();

  for (; head != tail; head++) {
    ::close(head->first);
  }
}

const int Server::get_port(void) const { return port; }

const std::string& Server::get_str_port(void) const { return str_port; }

const std::string& Server::get_serv_name(void) const { return serv_name; }

const std::string& Server::get_password(void) const { return password; }

const int Server::get_serv_socket(void) const { return serv_socket; }

const sockaddr_in& Server::get_serv_addr(void) const { return serv_addr; }

const int Server::get_tmp_user_cnt(void) const { return tmp_user_list.size(); }

const int Server::get_user_cnt(void) const { return user_list.size(); }

void Server::add_tmp_user(const int user_socker, const sockaddr_in& user_addr) {
  User tmp(user_socker, user_addr);
  tmp_user_list.insert(std::make_pair(user_socker, tmp));
}

void Server::add_user(const User& input) {
  user_list.insert(std::make_pair(input.get_user_socket(), input));
}

User& Server::operator[](const int socket_fd) {
  if (user_list.find(socket_fd) != user_list.end()) {
    return user_list.at(socket_fd);
  } else if (tmp_user_list.find(socket_fd) != tmp_user_list.end()) {
    return tmp_user_list.at(socket_fd);
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}
