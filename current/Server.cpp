#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(_port),
      serv_name(SERVER_NAME),
      serv_version(SERVER_VERSION),
      chantypes(CHANTYPES),
      password(_password),
      enable_ident_protocol(false) {
  serv_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }
  if (fcntl(serv_socket, F_SETFL, O_NONBLOCK) == -1) {
    throw std::exception();
  }
  int bufSize = SOCKET_BUFFER_SIZE;
  socklen_t len = sizeof(bufSize);
  if (setsockopt(serv_socket, SOL_SOCKET, SO_SNDBUF, &bufSize,
                 sizeof(bufSize)) == -1) {
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
      std::cerr << "Port " << port << " already in use.";
      port++;
      if (port > 65335) {
        port = 1024;
      }
      port_tmp << port;
      port_tmp >> str_port;
      std::cerr << "Try port " << port << '\n';
      serv_addr.sin_port = htons(port);
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
            << port << std::endl;
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
          if (observe_fd[i].revents & (POLLIN | POLLHUP)) {
            revent_pollin(observe_fd[i]);
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
    std::clog << "Connection established at " << user_socket << '\n';
    connection_limit--;
  }
  return 0;
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
      std::clog << "Connection close at " << p_val.fd << '\n';
      p_val.fd = -1;
      msg_list.clear();
      break;
    }

////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    std::cout << RED << "\n[IRSSI REQUEST] :: " << YELLOW << msg_list[j]
              << WHITE << std::endl;

    // SHOW THE LIST OF CLIENTS
    // [DEBUG]
    if (msg_list[0] == "lusers") {
      std::map<int, User>::iterator it = user_list.begin();
      for (; it != user_list.end(); ++it) {
        std::cout << it->second;
      }
    }
#endif
////////////////////////////////////////////////////////////////////////////////////////////

    Message msg(p_val.fd, msg_list[j]);

#ifdef DEBUG
    std::cout << msg;
#endif

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
    } else if (cmd_type == PRIVMSG) {
      cmd_privmsg(p_val.fd, msg);
    } else if (cmd_type == ERROR) {
      ft_send(p_val.fd, msg);
    } else if (cmd_type == CAP) {
      continue;
    } else if (cmd_type == QUIT) {
      cmd_quit(p_val, msg);
      msg_list.clear();
      break;
      if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
        p_val.events = POLLIN | POLLOUT;
      } else {
        p_val.events = POLLIN;
      }
    } else if (cmd_type == JOIN) {
      cmd_join(p_val.fd, msg);
    } else if (cmd_type == PART) {
      cmd_part(p_val.fd, msg);
    } else if (cmd_type == WHO) {
      cmd_who(p_val.fd, msg);
    } else if (cmd_type == KICK) {
      cmd_kick(p_val.fd, msg);
    } else if (cmd_type == INVITE) {
      cmd_invite(p_val.fd, msg);
    } else if (cmd_type == TOPIC) {
      cmd_topic(p_val.fd, msg);
    }
    if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
      p_val.events = POLLIN | POLLOUT;
    } else {
      p_val.events = POLLIN;
    }
  }
}

void Server::ft_send(int send_fd, Message& msg) {
  std::string str_msg = msg.to_raw_msg();

  send(send_fd, str_msg.c_str(), str_msg.length(), MSG_DONTWAIT);
}


int Server::send_msg_at_queue(int socket_fd) {
  User& user_tmp = (*this)[socket_fd];
  int send_result;
  std::size_t to_send_num = user_tmp.number_of_to_send();

  while (to_send_num > 0) {
    const std::string& msg_tmp = user_tmp.front_msg();
    std::cout << YELLOW << "[SERVER SENDING...] " << GREEN_BOLD << "["
              << msg_tmp << "]" << WHITE;
    send_result =
        send(socket_fd, msg_tmp.c_str(), msg_tmp.length(), MSG_DONTWAIT);
    user_tmp.pop_msg();
    if (send_result == -1) {
      return -1;
    }
    to_send_num--;
  }
  return 0;
}

void Server::not_auth_user(pollfd& p_val, std::vector<std::string>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (int j = 0; j < msg_list.size(); j++) {
    // [DEBUG]
    std::cout << YELLOW << "msg :: " << msg_list[j] << WHITE << std::endl;
    //////////////////////////////////////////////////////////////////////

    if (msg_list[j] == std::string("connection finish")) {
      (*this).remove_user(p_val.fd);
      std::cerr << "Connection close at " << p_val.fd << '\n';
      p_val.fd = -1;
      msg_list.clear();
      break;
    }

    Message msg(p_val.fd, msg_list[j]);
    int cmd_type = msg.get_cmd_type();

    // [DEBUG]
    std::cout << YELLOW << msg << WHITE << std::endl;
    //////////////////////////////////////////////////////////////////////

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
        rpl.push_back(event_user1.get_nick_name());

        rpl.set_numeric("001");
        rpl.push_back(":Welcome to the Internet Relay Network");
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("002");
        rpl.push_back(event_user1.get_nick_name());
        rpl.push_back(std::string(":Your host is ") + serv_name);
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("003");
        rpl.push_back(event_user1.get_nick_name());
        rpl.push_back(":This server has been started ~");
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("004");
        rpl.push_back(event_user1.get_nick_name());
        rpl.push_back(":" + serv_name);
        event_user1.push_msg(rpl.to_raw_msg());

        rpl.clear();
        rpl.set_numeric("005");
        rpl.push_back(event_user1.get_nick_name());
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
        rpl.push_back(event_user1.get_nick_name());
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

int Server::get_port(void) { return port; }

std::string& Server::get_str_port(void) { return str_port; }

std::string& Server::get_serv_name(void) { return serv_name; }

std::string& Server::get_password(void) { return password; }

int Server::get_serv_socket(void) { return serv_socket; }

sockaddr_in& Server::get_serv_addr(void) { return serv_addr; }

int Server::get_tmp_user_cnt(void) { return tmp_user_list.size(); }

int Server::get_user_cnt(void) { return user_list.size(); }

bool Server::get_enable_ident_protocol(void) {
  return enable_ident_protocol;
}

// CHANNEL
int Server::get_channel_num(void) { return CHANNELNUM; };
int Server::get_current_channel_num(void) {
  return channel_list.size();
};

void Server::add_tmp_user(const int user_socket, const sockaddr_in& user_addr) {
  User tmp(user_socket, user_addr);
  while (tmp_nick_to_soc.find(tmp.get_nick_name()) !=
         tmp_nick_to_soc.end())  // 찾아진다면
  {
    std::cout << std::boolalpha
              << (tmp_nick_to_soc.find(tmp.get_nick_name()) !=
                  tmp_nick_to_soc.end())
              << std::endl;
    tmp.set_nick_name(make_random_string(20));
  }
  std::cout << "~~~~~~~~~" << std::endl;
  tmp_nick_to_soc.insert(std::make_pair(tmp.get_nick_name(), user_socket));
  tmp_user_list.insert(std::make_pair(user_socket, tmp));
}

void Server::move_tmp_user_to_user_list(int socket_fd) {
  User& user_tmp = (*this)[socket_fd];
  // User  authenticatedUser(user_tmp);
  // std::cout << &user_tmp << "VS" << &authenticatedUser << std::endl;
  // nick_to_soc.insert(std::make_pair(authenticatedUser.get_nick_name(),
  // socket_fd)); user_list.insert(std::make_pair(socket_fd,
  // authenticatedUser));
  nick_to_soc.insert(std::make_pair(user_tmp.get_nick_name(), socket_fd));
  user_list.insert(std::make_pair(socket_fd, user_tmp));
  tmp_nick_to_soc.erase(user_tmp.get_nick_name());
  tmp_user_list.erase(socket_fd);

  // user_list 순회
  for (std::map<int, User>::iterator it = user_list.begin();
       it != user_list.end(); ++it) {
    int key = it->first;
    User value = it->second;
    // key와 value를 사용하여 필요한 작업 수행
    std::cout << key << std::endl << value << std::endl;
  }
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
  int tmp_fd;
  std::string old_nick_save = old_nick;

  it = nick_to_soc.find(old_nick);
  if (it != nick_to_soc.end()) {
    tmp_fd = it->second;
    nick_to_soc.erase(it);
    nick_to_soc.insert(std::make_pair(new_nick, tmp_fd));
    // 서버 다 돌면서 old_nick인거 다 찾아서 new_nick으로 바꿔주기;; ㅠㅠ
    for (channel_iterator = channel_list.begin();
         channel_iterator != channel_list.end();
         channel_iterator++) {
      get_channel(channel_iterator)
          .changeClientNickName(old_nick, new_nick);
      // if (get_channel(channel_iterator).foundClient(old_nick)
      // == true) {
      //   get_channel(channel_iterator).changeClientNickName(old_nick,
      //   new_nick);
      // }
    }

    // [REFACTORING] :: broadcast();
    std::map<int, User>::iterator it;
    for (it = user_list.begin(); it != user_list.end(); ++it) {
      std::string clientNickName = it->second.get_nick_name();
      if (clientNickName == old_nick) {
        continue;
      } else {
        Message rpl;

        rpl.set_source(old_nick_save + "!~" +
                       (*this)[it->first].get_user_name() + "@localhost");
        rpl.set_cmd_type(NICK);
        rpl.push_back(":" + new_nick);
        (*this)[it->first].push_msg(rpl.to_raw_msg());
        pollfd* tmp;
        for (int i = 0; i < MAX_USER; i++) {
          if (observe_fd[i].fd == ((*this)[it->first]).get_user_socket()) {
            tmp = &(observe_fd[i]);
          }
        }

        if ((*this).send_msg_at_queue(((*this)[it->first]).get_user_socket()) ==
            -1) {
          tmp->events = POLLIN | POLLOUT;
        } else {
          tmp->events = POLLIN;
        }
      }
    }

    // 실제 데이터는 여기서 바꿔주기 (공지 다 완료한 후)
    (*this)[tmp_fd].set_nick_name(new_nick);

    return;
  }

  it = tmp_nick_to_soc.find(old_nick);
  if (it != tmp_nick_to_soc.end()) {
    tmp_fd = it->second;
    tmp_nick_to_soc.erase(it);
    tmp_nick_to_soc.insert(std::make_pair(new_nick, tmp_fd));
    (*this)[tmp_fd].set_nick_name(new_nick);
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

void Server::addChannel(Channel& newChannel) {
  channel_list.insert(std::pair<std::string, Channel>(
      newChannel.get_channel_name(), newChannel));
  // channel_list.insert(std::pair<std::string,
  // Channel&>(newChannel.get_channel_name(), newChannel));
}

void Server::removeChannel(std::string& channelName) {
  std::map<std::string, Channel>::iterator it = channel_list.find(channelName);
  if (it != channel_list.end()) {
    channel_list.erase(it);
  }
}

std::map<std::string, Channel>::iterator Server::get_channel_iterator(
    std::string targetChannelStr) {
  channel_iterator = channel_list.begin();
  return channel_list.find(targetChannelStr);
}

Channel& Server::get_channel(
    std::map<std::string, Channel>::iterator iterator) {
  return iterator->second;
}

// remove 도 추가할것.
void Server::kickClient(User& opUser, User& outUser, Channel& channelName,
                        const Message& msg) {
  std::string clientNickName = outUser.get_nick_name();
  Message rpl;

  // 강퇴할 클라이언트를 찾으면
  if (channelName.get_channel_client_list().find(clientNickName) !=
      channelName.get_channel_client_list().end()) {
    rpl.set_source(opUser.get_nick_name() + std::string("!") +
                   std::string("~") + opUser.get_user_name() +
                   std::string("@localhost"));
    rpl.set_cmd_type(KICK);
    std::string sentence = msg.get_params()[0] + std::string(" ") +
                           msg.get_params()[1] + std::string(" ") + ":";
    if (msg.get_params_size() > 2) {
      for (int i = 2; i < msg.get_params_size(); i++) {
        sentence += msg.get_params()[i];
      }
    }
    rpl.push_back(sentence);  // 사유 적어주기

    // 채널에 속한 모든 클라이언트들에게 RESPONSE 보내주기
    std::map<std::string, User&>::iterator it;
    it = channelName.get_channel_client_list().begin();
    for (; it != channelName.get_channel_client_list().end(); ++it) {
      User& event_user = it->second;
      event_user.push_msg(rpl.to_raw_msg());

      pollfd* tmp;
      for (int i = 0; i < MAX_USER; i++) {
        if (observe_fd[i].fd == event_user.get_user_socket()) {
          tmp = &(observe_fd[i]);
        }
      }
      if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
        tmp->events = POLLIN | POLLOUT;
      } else {
        tmp->events = POLLIN;
      }
    }
    std::cout << YELLOW << rpl.to_raw_msg() << std::endl;
    if (channelName.isOperator(outUser)) {
      channelName.removeOperator(outUser);
    }
    channelName.get_channel_client_list().erase(clientNickName);
  } else {
    /*
      ERR_NOSUCHNICK (401)
        "<client> <nickname> :No such nick/channel"
        Indicates that no client can be found for the supplied nickname. The
      text used in the last param of this message may vary.
    */

    // :irc.example.net 401 lfkn slkfdn :No such nick or channel name\r
  }
}


/* 
      [IRSSI REQUEST] :: MODE #o +l 20
      < Message contents > 
      fd              : 4
      source          : 
      command         : MODE
      params          : #o, +l, 20
      numeric         : 
      [SERVER SENDING...] [:dy!~memememe@localhost MODE #o +l 20
      ]
      [IRSSI REQUEST] :: WHO l
      < Message contents > 
      fd              : 4
      source          : 
      command         : WHO
      params          : l
      numeric         : 
      =>> Server Channel List :: [i, o, test, ]

      [channel name] :: ���� �
      ����������������ZoBP[y
      [client limit] :: -455421048
      [invite mode] :: OFF
      =================================================================
      ==67278==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7ffee4dabad8 at pc 0x00010b04ab5f bp 0x7ffee4daabd0 sp 0x7ffee4daa398
      READ of size 68 at 0x7ffee4dabad8 thread T0
          #0 0x10b04ab5e in wrap_memchr+0x18e (libclang_rt.asan_osx_dynamic.dylib:x86_64h+0x1eb5e)
          #1 0x7fff732a87b4 in __sfvwrite+0x261 (libsystem_c.dylib:x86_64+0x3b7b4)
          #2 0x7fff732a8b49 in fwrite+0x87 (libsystem_c.dylib:x86_64+0x3bb49)
          #3 0x10b04bc22 in wrap_fwrite+0x52 (libclang_rt.asan_osx_dynamic.dylib:x86_64h+0x1fc22)
          #4 0x10ae55516 in std::__1::basic_streambuf<char, std::__1::char_traits<char> >::sputn(char const*, long)+0xa6 (ircserv:x86_64+0x100003516)
          #5 0x10ae54d2a in std::__1::ostreambuf_iterator<char, std::__1::char_traits<char> > std::__1::__pad_and_output<char, std::__1::char_traits<char> >(std::__1::ostreambuf_iterator<char, std::__1::char_traits<char> >, char const*, char const*, char const*, std::__1::ios_base&, char)+0x74a (ircserv:x86_64+0x100002d2a)
          #6 0x10ae541d7 in std::__1::basic_ostream<char, std::__1::char_traits<char> >& std::__1::__put_character_sequence<char, std::__1::char_traits<char> >(std::__1::basic_ostream<char, std::__1::char_traits<char> >&, char const*, unsigned long)+0x447 (ircserv:x86_64+0x1000021d7)
          #7 0x10ae58d30 in std::__1::basic_ostream<char, std::__1::char_traits<char> >& std::__1::operator<<<char, std::__1::char_traits<char>, std::__1::allocator<char> >(std::__1::basic_ostream<char, std::__1::char_traits<char> >&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > const&)+0x40 (ircserv:x86_64+0x100006d30)
          #8 0x10ae595a3 in operator<<(std::__1::basic_ostream<char, std::__1::char_traits<char> >&, Channel&)+0x323 (ircserv:x86_64+0x1000075a3)
          #9 0x10ae9edc5 in Server::cmd_who(int, Message const&)+0x8d5 (ircserv:x86_64+0x10004cdc5)
          #10 0x10aecc7b2 in Server::auth_user(pollfd&, std::__1::vector<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >, std::__1::allocator<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> > > >&)+0xe12 (ircserv:x86_64+0x10007a7b2)
          #11 0x10aec9ff7 in Server::revent_pollin(pollfd&)+0x267 (ircserv:x86_64+0x100077ff7)
          #12 0x10aec962d in Server::listen()+0x23d (ircserv:x86_64+0x10007762d)
          #13 0x10ae538b2 in main+0x732 (ircserv:x86_64+0x1000018b2)
          #14 0x7fff7321dcc8 in start+0x0 (libdyld.dylib:x86_64+0x1acc8)

      Address 0x7ffee4dabad8 is located in stack of thread T0 at offset 56 in frame
          #0 0x10ae5928f in operator<<(std::__1::basic_ostream<char, std::__1::char_traits<char> >&, Channel&)+0xf (ircserv:x86_64+0x10000728f)

        This frame has 7 object(s):
          [32, 56) 'ref.tmp' (line 98)
          [96, 120) 'operators' (line 102) <== Memory access at offset 56 partially underflows this variable
          [160, 168) 'cit' (line 105)
          [192, 200) 'ref.tmp31' (line 109)
          [224, 232) 'ref.tmp34' (line 109)
          [256, 264) 'it' (line 119)
          [288, 296) 'ref.tmp65' (line 120)
      HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
            (longjmp and C++ exceptions *are* supported)
      SUMMARY: AddressSanitizer: stack-buffer-overflow (libclang_rt.asan_osx_dynamic.dylib:x86_64h+0x1eb5e) in wrap_memchr+0x18e
      Shadow bytes around the buggy address:
        0x1fffdc9b5700: 00 00 00 00 00 00 00 00 f1 f1 f1 f1 00 00 f2 f2
        0x1fffdc9b5710: 00 f2 f2 f2 00 f3 f3 f3 00 00 00 00 00 00 00 00
        0x1fffdc9b5720: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        0x1fffdc9b5730: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        0x1fffdc9b5740: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
      =>0x1fffdc9b5750: 00 00 00 00 f1 f1 f1 f1 00 00 00[f2]f2 f2 f2 f2
        0x1fffdc9b5760: f8 f8 f8 f2 f2 f2 f2 f2 f8 f2 f2 f2 f8 f2 f2 f2
        0x1fffdc9b5770: f8 f2 f2 f2 f8 f2 f2 f2 f8 f3 f3 f3 00 00 00 00
        0x1fffdc9b5780: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        0x1fffdc9b5790: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
        0x1fffdc9b57a0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
      Shadow byte legend (one shadow byte represents 8 application bytes):
        Addressable:           00
        Partially addressable: 01 02 03 04 05 06 07 
        Heap left redzone:       fa
        Freed heap region:       fd
        Stack left redzone:      f1
        Stack mid redzone:       f2
        Stack right redzone:     f3
        Stack after return:      f5
        Stack use after scope:   f8
        Global redzone:          f9
        Global init order:       f6
        Poisoned by user:        f7
        Container overflow:      fc
        Array cookie:            ac
        Intra object redzone:    bb
        ASan internal:           fe
        Left alloca redzone:     ca
        Right alloca redzone:    cb
        Shadow gap:              cc
      ==67278==ABORTING
      [1]    67278 abort      ./ircserv 8080 1234
*/