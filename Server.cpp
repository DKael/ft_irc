#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(_port),
      serv_name(SERVER_NAME),
      chantype(CHAN_TYPE),
      password(_password),
      enable_ident_protocol(false),
      max_nick_len(9) {
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

  std::cout << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":"
            << port << '\n';
}

Server::~Server() {
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
  ::close(serv_socket);
}

const int Server::get_port(void) const { return port; }

const std::string& Server::get_str_port(void) const { return str_port; }

const std::string& Server::get_serv_name(void) const { return serv_name; }

const std::string& Server::get_password(void) const { return password; }

const int Server::get_serv_socket(void) const { return serv_socket; }

const sockaddr_in& Server::get_serv_addr(void) const { return serv_addr; }

const int Server::get_tmp_user_cnt(void) const { return tmp_user_list.size(); }

const int Server::get_user_cnt(void) const { return user_list.size(); }

const bool Server::get_enable_ident_protocol(void) const {
  return enable_ident_protocol;
}

void Server::add_tmp_user(const int user_socker, const sockaddr_in& user_addr) {
  User tmp(user_socker, user_addr);

  while (tmp_nick_to_soc.find(tmp.get_nick_name()) != tmp_nick_to_soc.end()) {
    tmp.set_nick_name(make_random_string(20));
  }
  tmp_nick_to_soc.insert(std::make_pair(tmp.get_nick_name(), user_socker));
  tmp_user_list.insert(std::make_pair(user_socker, tmp));
}

void Server::move_tmp_user_to_user_list(int socket_fd) {
  User& user_tmp = (*this)[socket_fd];

  nick_to_soc.insert(std::make_pair(user_tmp.get_nick_name(), socket_fd));
  user_list.insert(std::make_pair(socket_fd, user_tmp));
  tmp_nick_to_soc.erase(user_tmp.get_nick_name());
  tmp_user_list.erase(socket_fd);
}

void Server::add_user(const User& input) {
  nick_to_soc.insert(
      std::make_pair(input.get_nick_name(), input.get_user_socket()));
  user_list.insert(std::make_pair(input.get_user_socket(), input));
}

void Server::remove_user(const int socket_fd) {
  std::map<int, User>::iterator it1;
  std::map<std::string, int>::iterator it2;
  std::string tmp;

  it1 = user_list.find(socket_fd);
  if (it1 != user_list.end()) {
    tmp = (it1->second).get_nick_name();
    user_list.erase(it1);
    it2 = nick_to_soc.find(tmp);
    nick_to_soc.erase(it2);
    ::close(socket_fd);
    return;
  }

  it1 = tmp_user_list.find(socket_fd);
  if (it1 != tmp_user_list.end()) {
    tmp = (it1->second).get_nick_name();
    tmp_user_list.erase(it1);
    it2 = tmp_nick_to_soc.find(tmp);
    tmp_nick_to_soc.erase(it2);
    ::close(socket_fd);
    return;
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

void Server::remove_user(const std::string& nickname) {
  std::map<std::string, int>::iterator it1;
  std::map<int, User>::iterator it2;
  int tmp;

  it1 = nick_to_soc.find(nickname);
  if (it1 != nick_to_soc.end()) {
    tmp = it1->second;
    nick_to_soc.erase(it1);
    it2 = user_list.find(tmp);
    user_list.erase(it2);
    ::close(tmp);
    return;
  }

  it1 = tmp_nick_to_soc.find(nickname);
  if (it1 != tmp_nick_to_soc.end()) {
    tmp = it1->second;
    tmp_nick_to_soc.erase(it1);
    it2 = tmp_user_list.find(tmp);
    tmp_user_list.erase(it2);
    ::close(tmp);
    return;
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

void Server::change_nickname(const std::string& old_nick,
                             const std::string& new_nick) {
  std::map<std::string, int>::iterator it;
  int tmp;

  it = nick_to_soc.find(old_nick);
  if (it != nick_to_soc.end()) {
    tmp = it->second;
    nick_to_soc.erase(it);
    nick_to_soc.insert(std::make_pair(new_nick, tmp));
    (*this)[tmp].set_nick_name(new_nick);
    return;
  }
  it = tmp_nick_to_soc.find(old_nick);
  if (it != tmp_nick_to_soc.end()) {
    tmp = it->second;
    tmp_nick_to_soc.erase(it);
    tmp_nick_to_soc.insert(std::make_pair(new_nick, tmp));
    (*this)[tmp].set_nick_name(new_nick);
    return;
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

void Server::tmp_user_timeout_chk(void) {
  time_t current_time = time(NULL);
  std::map<int, User>::iterator it1 = tmp_user_list.begin();
  std::map<int, User>::iterator it2 = tmp_user_list.end();
  std::string tmp_nick;

  while (it1 != it2) {
    if (current_time >
        (it1->second).get_created_time() + AUTHENTICATE_TIMEOUT) {
      tmp_nick = (it1->second).get_nick_name();
      it1++;
      remove_user(tmp_nick);
    } else {
      it1++;
    }
  }
}

int Server::send_msg(int socket_fd) {
  User& user_tmp = (*this)[socket_fd];
  int send_result;
  std::size_t to_send_num = user_tmp.number_of_to_send();

  while (to_send_num > 0) {
    const std::string& msg_tmp = user_tmp.front_msg();
    send_result =
        send(socket_fd, msg_tmp.c_str(), msg_tmp.length(), MSG_DONTWAIT);
    if (send_result == -1) {
      if (errno == EWOULDBLOCK) {
        return -1;
      } else {
        std::cerr << "send() error\n";
        return -2;
      }
    }
    user_tmp.pop_msg();
    to_send_num--;
  }
  return 0;
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

int Server::operator[](const std::string& nickname) {
  if (nick_to_soc.find(nickname) != nick_to_soc.end()) {
    return nick_to_soc.at(nickname);
  } else if (tmp_nick_to_soc.find(nickname) != tmp_nick_to_soc.end()) {
    return tmp_nick_to_soc.at(nickname);
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

void Server::cmd_pass(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  std::string pass_tmp;

  if (event_user.get_password_chk() == NOT_YET) {
    if (msg.get_params_size() < 1) {
      event_user.push_msg(Message::rpl_461(serv_name,
                                           event_user.get_nick_name(),
                                           msg.get_raw_cmd())
                              .to_raw_msg());
      return;
    }
    if (msg[0].at(0) == ':') {
      pass_tmp = msg[0].substr(1);
    } else {
      pass_tmp = msg[0];
    }
    if (password == pass_tmp) {
      event_user.set_password_chk(OK);
    } else {
      event_user.set_password_chk(FAIL);
    }
    return;
  } else {
    event_user.push_msg(
        Message::rpl_462(serv_name, event_user.get_nick_name()).to_raw_msg());
    return;
  }
}

void Server::cmd_nick(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  std::string nick_tmp;

  if (msg.get_params_size() == 0) {
    event_user.push_msg(Message::rpl_461(serv_name, event_user.get_nick_name(),
                                         msg.get_raw_cmd())
                            .to_raw_msg());
    return;
  } else {
    if (msg[0].at(0) == ':') {
      nick_tmp = msg[0].substr(1);
    } else {
      nick_tmp = msg[0];
    }
    if (('0' <= nick_tmp[0] && nick_tmp[0] <= '9') ||
        nick_tmp.find_first_of(chantype + std::string(": \n\t\v\f\r")) !=
            std::string::npos ||
        nick_tmp.length() > MAX_NICK_LEN) {
      event_user.push_msg(
          Message::rpl_432(serv_name, event_user.get_nick_name(), nick_tmp)
              .to_raw_msg());
      return;
    }
    try {
      (*this)[nick_tmp];
      event_user.push_msg(
          Message::rpl_433(serv_name, event_user.get_nick_name(), nick_tmp)
              .to_raw_msg());
      return;
    } catch (std::invalid_argument& e) {
      if (event_user.get_is_authenticated() == OK) {
        Message rpl;

        rpl.set_source(event_user.get_nick_name() + "!" +
                       event_user.get_user_name() + "@localhost");
        rpl.set_cmd_type(NICK);
        rpl.push_back(":" + nick_tmp);
        event_user.push_msg(rpl.to_raw_msg());
      }
      (*this).change_nickname(event_user.get_nick_name(), nick_tmp);
      event_user.set_nick_init_chk(OK);
    }
  }
}

void Server::cmd_user(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  std::string user_tmp;

  if (msg.get_params_size() < 4) {
    event_user.push_msg(Message::rpl_461(serv_name, event_user.get_nick_name(),
                                         msg.get_raw_cmd())
                            .to_raw_msg());
    return;
  }
  if (event_user.get_user_init_chk() == NOT_YET) {
    if (enable_ident_protocol == true) {
      user_tmp = msg[0];
    } else {
      user_tmp = "~" + msg[0];
    }
    if (user_tmp.length() > MAX_USER_NAME_LEN) {
      user_tmp = user_tmp.substr(0, MAX_USER_NAME_LEN);
    }
    event_user.set_real_name(msg[3]);
    event_user.set_user_init_chk(OK);
  } else {
    if (event_user.get_is_authenticated() == OK) {
      Message rpl = Message::rpl_462(serv_name, event_user.get_nick_name());
      event_user.push_msg(rpl.to_raw_msg());
      return;
    } else {
      Message rpl = Message::rpl_451(serv_name, event_user.get_nick_name());
      event_user.push_msg(rpl.to_raw_msg());
      return;
    }
  }
}
