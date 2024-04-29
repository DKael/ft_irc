#include "Server.hpp"

void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list);

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(_port),
      serv_name(SERVER_NAME),
      chantype(CHAN_TYPE),
      password(_password),
      enable_ident_protocol(false),
      max_nickname_len(INIT_MAX_NICKNAME_LEN),
      max_username_len(INIT_MAX_USERNAME_LEN) {
  serv_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }
  if (::fcntl(serv_socket, F_SETFL, O_NONBLOCK) == -1) {
    throw std::exception();
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  int retry_cnt = 10;
  std::stringstream port_tmp;
  while (::bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    if (retry_cnt == 0) {
      throw std::exception();
    } else if (errno == EADDRINUSE) {
      port++;
      if (port > 65335) {
        port = 1024;
      }
      port_tmp << port;
      port_tmp >> str_port;
      serv_addr.sin_port = htons(port);
      std::cerr << "Port " << str_port << " already in use. Try port " << port
                << '\n';
      retry_cnt--;
    } else {
      throw socket_bind_error();
    }
  }

  if (::listen(serv_socket, 64) == -1) {
    throw socket_listening_error();
  }

  observe_fd[0].fd = serv_socket;
  observe_fd[0].events = POLLIN;

  for (int i = 1; i < MAX_USER; i++) {
    observe_fd[i].fd = -1;
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
}

void Server::listen(void) {
  int event_cnt = 0;

  while (true) {
    event_cnt = poll(observe_fd, MAX_USER, POLL_TIMEOUT * 1000);
    if (event_cnt == 0) {
      continue;
    } else if (event_cnt < 0) {
      perror("poll() error!");
      return;
    } else {
      if (observe_fd[0].revents & POLLIN) {
        client_socket_init();
        event_cnt--;
      }
      for (int i = 1; i < MAX_USER && event_cnt > 0; i++) {
        if (observe_fd[i].fd > 0) {
          if (observe_fd[i].revents & POLLOUT) {
            revent_pollout(observe_fd[i]);
          }
          if (observe_fd[i].revents & POLLIN) {
            revent_pollin(observe_fd[i]);
          }
          if (observe_fd[i].revents & POLLHUP) {
            observe_fd[i].fd = -1;
          }
          if (observe_fd[i].revents) {
            event_cnt--;
          }
        }
      }
    }
  }
}

int Server::client_socket_init(void) {
  int connection_limit = 10;
  socklen_t user_addr_len;
  sockaddr_in user_addr;
  int user_socket;

  while (connection_limit != 0) {
    user_addr_len = sizeof(user_addr);
    std::memset(&user_addr, 0, user_addr_len);
    user_socket = ::accept(serv_socket, (sockaddr*)&user_addr, &user_addr_len);
    if (user_socket == -1) {
      if (errno == EWOULDBLOCK) {
        return 0;
      } else {
        // error_handling
        // accept 함수 에러나는 경우 찾아보자
        perror("accept() error");
        return -1;
      }
    }

    if (::fcntl(user_socket, F_SETFL, O_NONBLOCK) == -1) {
      // error_handling
      perror("fcntl() error");
      send(user_socket, "ERROR :socket setting error",
           std::strlen("ERROR :socket setting error"), MSG_DONTWAIT);
      close(user_socket);
      return -1;
    }

    if ((tmp_user_list.size() + user_list.size()) > MAX_USER) {
      // todo : send some error message to client
      // 근데 이거 클라이언트한테 메세지 보내려면 소켓을 연결해야 하는데
      // 에러 메세지 답장 안 보내고 연결요청 무시하려면 되려나
      // 아니면 예비소켓 하나 남겨두고 잠시 연결했다 바로 연결 종료하는
      // 용도로 사용하는 것도 방법일 듯
      std::cerr << "User connection limit exceed!\n";
      send(user_socket, "ERROR :Too many users on server",
           std::strlen("ERROR :Too many users on server"), MSG_DONTWAIT);
      close(user_socket);
      return 0;
    }

    (*this).add_tmp_user(user_socket, user_addr);
    for (int i = 1; i < MAX_USER; i++) {
      if (observe_fd[i].fd == -1) {
        observe_fd[i].fd = user_socket;
        observe_fd[i].events = POLLIN;
        break;
      }
    }
    std::cerr << "Connection established at " << user_socket << '\n';
    connection_limit--;
  }
  return 0;
}

void Server::revent_pollout(pollfd& p_val) {
  if ((*this)[p_val.fd].get_have_to_disconnect() == false) {
    if ((*this).send_msg_at_queue(p_val.fd) == -1) {
      p_val.events = POLLIN | POLLOUT;
    } else {
      p_val.events = POLLIN;
    }
  } else {
    if ((*this).send_msg_at_queue(p_val.fd) != -1) {
      (*this).remove_user(p_val.fd);
      p_val.fd = -1;
    }
  }
}

void Server::revent_pollin(pollfd& p_val) {
  User& event_user = (*this)[p_val.fd];
  std::vector<std::string> msg_list;

  try {
    read_msg_from_socket(p_val.fd, msg_list);

    if (event_user.get_is_authenticated() == OK) {
      auth_user(p_val, msg_list);
    } else {
      not_auth_user(p_val, msg_list);
    }
  } catch (const std::bad_alloc& e) {
    std::cerr << e.what() << '\n';
    std::cerr << "Not enough memory so can't excute vector.push_back "
                 "or another things require additional memory\n";
  } catch (const std::length_error& e) {
    std::cerr << e.what() << '\n';
    std::cerr << "Maybe index out of range error or std::string is too "
                 "long to store\n";
  } catch (const std::exception& e) {
    // error handling
    std::cerr << e.what() << '\n';
    std::cerr << "unexpected exception occur! Program terminated!\n";
    exit(1);
  }
}

void Server::auth_user(pollfd& p_val, std::vector<std::string>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (int j = 0; j < msg_list.size(); j++) {
    if (msg_list[j] == std::string("connection finish")) {
      (*this).remove_user(p_val.fd);
      std::cerr << "Connection close at " << p_val.fd << '\n';
      p_val.fd = -1;
      msg_list.clear();
      break;
    }

    Message msg(p_val.fd, msg_list[j]);
    int cmd_type = msg.get_cmd_type();
    if (cmd_type == PASS) {
      cmd_pass(p_val.fd, msg);
    } else if (cmd_type == NICK) {
      cmd_nick(p_val.fd, msg);
    } else if (cmd_type == USER) {
      cmd_user(p_val.fd, msg);
    } else if (cmd_type == MODE) {
      cmd_mode(p_val.fd, msg);
    } else if (cmd_type == PING) {
      cmd_pong(p_val.fd, msg);
    } else if (cmd_type == ERROR) {
      event_user.push_msg(msg.to_raw_msg());
    } else if (cmd_type == CAP) {
      continue;
    } else if (cmd_type == QUIT) {
      cmd_quit(p_val, msg);
      msg_list.clear();
      break;
    }
    if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
      p_val.events = POLLIN | POLLOUT;
    } else {
      p_val.events = POLLIN;
    }
  }
}

void Server::not_auth_user(pollfd& p_val, std::vector<std::string>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (int j = 0; j < msg_list.size(); j++) {
    if (msg_list[j] == std::string("connection finish")) {
      (*this).remove_user(p_val.fd);
      std::cerr << "Connection close at " << p_val.fd << '\n';
      p_val.fd = -1;
      msg_list.clear();
      break;
    }
    Message msg(p_val.fd, msg_list[j]);
    int cmd_type = msg.get_cmd_type();

    if (cmd_type == PASS) {
      cmd_pass(p_val.fd, msg);
    } else if (cmd_type == NICK) {
      if (event_user.get_password_chk() == NOT_YET) {
        event_user.set_password_chk(FAIL);
      }
      cmd_nick(p_val.fd, msg);
    } else if (cmd_type == USER) {
      if (event_user.get_password_chk() == NOT_YET) {
        event_user.set_password_chk(FAIL);
      }
      cmd_user(p_val.fd, msg);
    } else if (cmd_type == CAP) {
      continue;
    } else if (cmd_type == ERROR) {
      event_user.push_msg(msg.to_raw_msg());
    } else if (cmd_type == QUIT) {
      cmd_quit(p_val, msg);
      msg_list.clear();
      break;
    } else {
      Message rpl = Message::rpl_451(serv_name, event_user.get_nick_name());
      event_user.push_msg(rpl.to_raw_msg());
    }
    if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
      p_val.events = POLLIN | POLLOUT;
    } else {
      p_val.events = POLLIN;
    }

    if (event_user.get_nick_init_chk() == OK &&
        event_user.get_user_init_chk() == OK) {
      if (event_user.get_password_chk() != OK) {
        event_user.set_have_to_disconnect(true);
        event_user.push_msg(
            Message::rpl_464(serv_name, event_user.get_nick_name())
                .to_raw_msg());
        if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
          p_val.events = POLLOUT;
        } else {
          (*this).remove_user(p_val.fd);
          p_val.fd = -1;
        }
      } else {
        // authenticate complete
        move_tmp_user_to_user_list(event_user.get_user_socket());
        User& event_user1 = (*this)[p_val.fd];

        Message rpl;
        rpl.set_source(serv_name);
        rpl.push_back(event_user.get_nick_name());

        rpl.set_numeric("001");
        rpl.push_back(":Welcome to the Internet Relay Network");
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("002");
        rpl.push_back(event_user.get_nick_name());
        rpl.push_back(std::string(":Your host is ") + serv_name);
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("003");
        rpl.push_back(event_user.get_nick_name());
        rpl.push_back(":This server has been started ~");
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("004");
        rpl.push_back(event_user.get_nick_name());
        rpl.push_back(":" + serv_name);
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("005");
        rpl.push_back(event_user.get_nick_name());
        rpl.push_back("RFC2812");
        rpl.push_back("IRCD=ngIRCd");
        rpl.push_back("CHARSET=UTF-8");
        rpl.push_back("CASEMAPPING=ascii");
        rpl.push_back("PREFIX=(qaohv)~&@%+");
        rpl.push_back("CHANTYPES=#&+");
        rpl.push_back("CHANMODES=beI,k,l,imMnOPQRstVz");
        rpl.push_back("CHANLIMIT=#&+:10");
        rpl.push_back(":are supported on this server");
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.push_back(event_user.get_nick_name());
        rpl.push_back("CHANNELLEN=50");
        rpl.push_back("NICKLEN=9");
        rpl.push_back("TOPICLEN=490");
        rpl.push_back("AWAYLEN=127");
        rpl.push_back("KICKLEN=400");
        rpl.push_back("MODES=5");
        rpl.push_back("MAXLIST=beI:50");
        rpl.push_back("EXCEPTS=e");
        rpl.push_back("PENALTY");
        rpl.push_back("FNC");
        rpl.push_back(":are supported on this server");
        event_user1.push_msg(rpl.to_raw_msg());
        event_user1.set_is_authenticated(OK);

        if ((*this).send_msg_at_queue(event_user1.get_user_socket()) == -1) {
          p_val.events = POLLIN | POLLOUT;
        } else {
          p_val.events = POLLIN;
        }
      }
    }
  }
}

void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list) {
  static std::string remains = "";
  static bool incomplete = false;

  char read_block[BLOCK_SIZE] = {
      0,
  };
  int read_cnt = 0;
  int idx;
  std::vector<std::string> box;

  msg_list.clear();
  while (true) {
    read_cnt = ::recv(socket_fd, read_block, BLOCK_SIZE - 1, MSG_DONTWAIT);
    if (0 < read_cnt && read_cnt <= BLOCK_SIZE - 1) {
      read_block[read_cnt] = '\0';
      box.clear();
      ft_split(std::string(read_block), "\r\n", box);
      idx = 0;
      if (incomplete == true) {
        remains += box[0];
        msg_list.push_back(remains);
        remains = "";
        idx = 1;
      }
      if (read_block[read_cnt - 2] == '\r' &&
          read_block[read_cnt - 1] == '\n') {
        for (; idx < box.size(); idx++) {
          if (box[idx].length() != 0) {
            msg_list.push_back(box[idx]);
          }
        }
        incomplete = false;
      } else {
        for (; idx < box.size() - 1; idx++) {
          if (box[idx].length() != 0) {
            msg_list.push_back(box[idx]);
          }
        }
        remains = box[idx];
        incomplete = true;
      }
      if (read_cnt == BLOCK_SIZE - 1) {
        continue;
      } else {
        break;
      }
    } else if (read_cnt == -1) {
      if (errno == EWOULDBLOCK) {
        // * non-blocking case *
        break;
      } else {
        // * recv() function error *
        // todo : error_handling
        std::cerr << "recv() error\n";
      }
    } else if (read_cnt == 0) {
      // socket connection finish
      // 이 함수는 소켓으로부터 메세지만 읽어들이는 함수이니 여기서 close()
      // 함수를 부르지 말고 외부에서 부를 수 있게 메세지를 남기자
      msg_list.push_back(std::string("connection finish"));
      break;
    }
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

int Server::send_msg_at_queue(int socket_fd) {
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
        nick_tmp.length() > max_nickname_len) {
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
    if (user_tmp.length() > max_username_len) {
      user_tmp = user_tmp.substr(0, max_username_len);
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

void Server::cmd_mode(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  Message rpl;

  rpl.set_source(event_user.get_nick_name() + std::string("!") +
                 event_user.get_user_name() + std::string("@localhost"));
  rpl.set_cmd_type(MODE);
  rpl.push_back(event_user.get_nick_name());
  rpl.push_back(":" + msg[0]);
  event_user.push_msg(rpl.to_raw_msg());
}

void Server::cmd_pong(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  Message rpl;

  rpl.set_source(serv_name);
  rpl.set_cmd_type(PONG);
  rpl.push_back(serv_name);
  rpl.push_back(":" + serv_name);
  event_user.push_msg(rpl.to_raw_msg());
}

void Server::cmd_quit(pollfd& p_val, const Message& msg) {
  User& event_user = (*this)[p_val.fd];
  Message rpl;

  rpl.set_cmd_type(ERROR);
  rpl.push_back(":Closing connection");
  event_user.set_have_to_disconnect(true);
  event_user.push_msg(rpl.to_raw_msg());
  std::cerr << "Connection close at " << p_val.fd << '\n';
  if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
    p_val.events = POLLOUT;
  } else {
    (*this).remove_user(p_val.fd);
    p_val.fd = -1;
  }
}
