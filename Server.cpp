#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(_port),
      serv_name(SERVER_NAME),
      serv_version(SERVER_VERSION),
      chantypes(CHANTYPES),
      created_time(std::time(NULL)),
      password(_password),
      enable_ident_protocol(false) {
  serv_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }
  if (fcntl(serv_socket, F_SETFL, O_NONBLOCK) == -1) {
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

  created_time_str = ctime(&created_time);

  std::clog << "Server created at " << created_time_str << '\n'
            << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":"
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
    tmp_user_timeout_chk();
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
      // error_handling
      // accept 함수 에러나는 경우 찾아보자
      perror("accept() error");
      return -1;
    }

    std::map<unsigned int, int>::iterator ip_it =
        ip_list.find(user_addr.sin_addr.s_addr);
    if (ip_it != ip_list.end()) {
      if (ip_it->second > 9) {
        std::cerr << "To many connection at " << inet_ntoa(user_addr.sin_addr)
                  << '\n';
        send(user_socket, "ERROR :Too many connection at your ipv4",
             std::strlen("ERROR :Too many connection at your ipv4"),
             MSG_DONTWAIT);
        close(user_socket);
        return -1;
      } else {
        ip_it->second++;
      }
    } else {
      ip_list.insert(std::pair<in_addr_t, int>(user_addr.sin_addr.s_addr, 0));
    }

    if (::fcntl(user_socket, F_SETFL, O_NONBLOCK) == -1) {
      // error_handling
      perror("fcntl() error");
      send(user_socket, "ERROR :socket setting error",
           std::strlen("ERROR :socket setting error"), MSG_DONTWAIT);
      close(user_socket);
      return -1;
    }

    int bufSize = SOCKET_BUFFER_SIZE;
    socklen_t len = sizeof(bufSize);
    if (setsockopt(user_socket, SOL_SOCKET, SO_SNDBUF, &bufSize,
                   sizeof(bufSize)) == -1) {
      send(user_socket, "ERROR :socket setting error",
           std::strlen("ERROR :socket setting error"), MSG_DONTWAIT);
      perror("setsockopt() error");
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
      return -1;
    }

    int i = 1;
    for (; i < MAX_USER; i++) {
      if (observe_fd[i].fd == -1) {
        observe_fd[i].fd = user_socket;
        observe_fd[i].events = POLLIN;
        break;
      }
    }
    (*this).add_tmp_user(observe_fd[i], user_addr);

    std::clog << "Connection established at " << user_socket << '\n';
    connection_limit--;
  }
  return 0;
}

void Server::connection_fin(pollfd& p_val) {
  std::map<unsigned int, int>::iterator ip_it =
      ip_list.find((*this)[p_val.fd].get_user_addr().sin_addr.s_addr);

  ip_it->second--;
  if (ip_it->second == 0) {
    ip_list.erase(ip_it);
  }
  (*this).remove_user(p_val.fd);
  p_val.fd = -1;
}

void Server::ft_send(pollfd& p_val) {
  if (send_msg_at_queue(p_val.fd) == -1) {
    p_val.events = POLLIN | POLLOUT;
  } else {
    p_val.events = POLLIN;
  }
}

void Server::ft_sendd(pollfd& p_val) {
  if (send_msg_at_queue(p_val.fd) == -1) {
    p_val.events = POLLOUT;
  } else {
    connection_fin(p_val);
  }
}

int Server::send_msg_at_queue(int socket_fd) {
  User& user_tmp = (*this)[socket_fd];
  std::size_t to_send_num = user_tmp.get_to_send_size();
  size_t msg_len;
  size_t idx;
  ssize_t bytes_sent;
  bool error_flag;

  while (to_send_num > 0) {
    const String& msg = user_tmp.get_front_msg();
    msg_len = msg.length();
    idx = 0;
    error_flag = false;

    while (idx < msg_len) {
      String msg_blk = msg.substr(idx, SOCKET_BUFFER_SIZE);
      bytes_sent = send_msg_block(socket_fd, msg_blk);
      if (bytes_sent == msg_blk.length()) {
        idx += msg_blk.length();
      } else {
        error_flag = true;
        break;
      }
    }
    user_tmp.pop_front_msg();
    if (error_flag == true) {
      user_tmp.push_front_msg(msg.substr(idx + bytes_sent));
      return -1;
    }
    to_send_num--;
  }
  return 0;
}

int Server::send_msg_block(int socket_fd, const String& blk) {
  const char* c_blk = blk.c_str();
  size_t blk_len = blk.length();
  ssize_t bytes_sent = 0;
  size_t total_bytes_sent = 0;

  while (total_bytes_sent < blk_len) {
    bytes_sent = send(socket_fd, c_blk + total_bytes_sent,
                      blk_len - total_bytes_sent, MSG_DONTWAIT);
    if (bytes_sent < 0) {
      break;
    }
    total_bytes_sent += bytes_sent;
  }
  return total_bytes_sent;
}

void Server::revent_pollout(pollfd& p_val) {
  if ((*this)[p_val.fd].get_have_to_disconnect() == false) {
    ft_send(p_val);
  } else {
    ft_sendd(p_val);
  }
}

void Server::revent_pollin(pollfd& p_val) {
  User& event_user = (*this)[p_val.fd];
  std::vector<String> msg_list;

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
    std::cerr << "Maybe index out of range error or String is too "
                 "long to store\n";
  } catch (const std::exception& e) {
    // error handling
    std::cerr << e.what() << '\n';
    std::cerr << "unexpected exception occur! Program terminated!\n";
    exit(1);
  }
}

void Server::auth_user(pollfd& p_val, std::vector<String>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (int j = 0; j < msg_list.size(); j++) {
    if (msg_list[j] == String("connection finish")) {
      std::clog << "Connection close at " << p_val.fd << '\n';
      connection_fin(p_val);
      msg_list.clear();
      break;
    }

    Message msg(p_val.fd, msg_list[j]);

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

    std::cout << YELLOW << msg << WHITE << std::endl;
#endif
    ////////////////////////////////////////////////////////////////////////////////////////////

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
      cmd_ping(p_val.fd, msg);
    } else if (cmd_type == PONG) {
      cmd_pong(p_val.fd, msg);
    } else if (cmd_type == ERROR) {
      event_user.push_back_msg(msg.to_raw_msg());
    } else if (cmd_type == CAP) {
      continue;
    } else if (cmd_type == QUIT) {
      cmd_quit(p_val.fd, msg);
      msg_list.clear();
      break;
    } else if (cmd_type == PRIVMSG) {
      cmd_privmsg(p_val.fd, msg);
    } else if (cmd_type == JOIN) {
      cmd_join(p_val.fd, msg);
    } else if (cmd_type == WHO) {
      cmd_who(p_val.fd, msg);
    } else if (cmd_type == KICK) {
      cmd_kick(p_val.fd, msg);
    } else if (cmd_type == INVITE) {
      cmd_invite(p_val.fd, msg);
    } else if (cmd_type == TOPIC) {
      cmd_topic(p_val.fd, msg);
    }
    ft_send(p_val);
  }
  event_user.set_last_ping(std::time(NULL));
}

void Server::not_auth_user(pollfd& p_val, std::vector<String>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (int j = 0; j < msg_list.size(); j++) {
    if (msg_list[j] == String("connection finish")) {
      std::clog << "Connection close at " << p_val.fd << '\n';
      connection_fin(p_val);
      msg_list.clear();
      break;
    }

    Message msg(p_val.fd, msg_list[j]);

    //////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    std::cout << YELLOW << "msg :: " << msg_list[j] << WHITE << std::endl;
    std::cout << YELLOW << msg << WHITE << std::endl;
#endif
    //////////////////////////////////////////////////////////////////////

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
      event_user.push_back_msg(msg.to_raw_msg());
    } else if (cmd_type == QUIT) {
      cmd_quit(p_val.fd, msg);
      msg_list.clear();
      break;
    } else {
      Message rpl = Message::rpl_451(serv_name, event_user.get_nick_name());
      event_user.push_back_msg(rpl.to_raw_msg());
    }
    ft_send(p_val);

    if (event_user.get_nick_init_chk() == OK &&
        event_user.get_user_init_chk() == OK) {
      if (event_user.get_password_chk() != OK) {
        event_user.set_have_to_disconnect(true);
        event_user.push_back_msg(
            Message::rpl_464(serv_name, event_user.get_nick_name())
                .to_raw_msg());
        ft_sendd(p_val);
      } else {
        auth_complete(p_val);
      }
    }
  }
}

void Server::auth_complete(pollfd& p_val) {
  move_tmp_user_to_user_list(p_val.fd);
  User& event_user = (*this)[p_val.fd];
  event_user.set_is_authenticated(OK);
  const String& client = event_user.get_nick_name();

  event_user.push_back_msg(
      Message::rpl_001(serv_name, client, event_user.make_source(1))
          .to_raw_msg());
  event_user.push_back_msg(
      Message::rpl_002(serv_name, client, serv_name, serv_version)
          .to_raw_msg());
  event_user.push_back_msg(
      Message::rpl_003(serv_name, client, created_time_str).to_raw_msg());
  event_user.push_back_msg(Message::rpl_004(serv_name, client, serv_name,
                                            serv_version, AVAILABLE_USER_MODES,
                                            AVAILABLE_CHANNEL_MODES)
                               .to_raw_msg());

  static bool specs_created = false;
  static std::vector<String> specs1;
  static std::vector<String> specs2;

  if (specs_created == false) {
    specs_created = true;

    specs1.push_back(IRC_PROTOCOL);
    specs1.push_back("IRCD=" + String(IRCD));
    specs1.push_back("CHARSET=" + String(CHARSET));
    specs1.push_back("CASEMAPPING=" + String(CASEMAPPING));
    specs1.push_back("PREFIX=" + String(PREFIX));
    specs1.push_back("CHANTYPES=" + String(CHANTYPES));
    specs1.push_back("CHANMODES=" + String(CHANMODES));
    specs1.push_back("CHANLIMIT=" + String(CHANLIMIT));

    specs2.push_back("CHANNELLEN=" + ft_itos(CHANNELLEN));
    specs2.push_back("NICKLEN=" + ft_itos(NICKLEN));
    specs2.push_back("TOPICLEN=" + ft_itos(TOPICLEN));
    specs2.push_back("AWAYLEN=" + ft_itos(AWAYLEN));
    specs2.push_back("KICKLEN=" + ft_itos(KICKLEN));
  }
  event_user.push_back_msg(
      Message::rpl_005(serv_name, client, specs1).to_raw_msg());
  event_user.push_back_msg(
      Message::rpl_005(serv_name, client, specs2).to_raw_msg());

  ft_send(p_val);
}

int Server::get_port(void) const { return port; }

const String& Server::get_str_port(void) const { return str_port; }

const String& Server::get_serv_name(void) const { return serv_name; }

const String& Server::get_password(void) const { return password; }

const std::time_t& Server::get_created_time(void) const { return created_time; }

const String& Server::get_created_time_str(void) const {
  return created_time_str;
}

int Server::get_serv_socket(void) const { return serv_socket; }

const sockaddr_in& Server::get_serv_addr(void) const { return serv_addr; }

int Server::get_tmp_user_cnt(void) const { return tmp_user_list.size(); }

int Server::get_user_cnt(void) const { return user_list.size(); }

bool Server::get_enable_ident_protocol(void) const {
  return enable_ident_protocol;
}
int Server::get_channel_num(void) const { return channel_list.size(); };

void Server::add_tmp_user(pollfd& pfd, const sockaddr_in& user_addr) {
  User tmp(pfd, user_addr);
  String tmp_nick = tmp.get_nick_name_no_chk();

  while (tmp_nick_to_soc.find(tmp_nick) != tmp_nick_to_soc.end()) {
    tmp_nick = make_random_string(20);
    tmp.set_nick_name(tmp_nick);
  }
  tmp_nick_to_soc.insert(std::make_pair(tmp_nick, pfd.fd));
  tmp_user_list.insert(std::make_pair(pfd.fd, tmp));
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
  std::map<String, int>::iterator it2;
  String tmp;

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

void Server::remove_user(const String& nickname) {
  std::map<String, int>::iterator it1;
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

void Server::tmp_user_timeout_chk(void) {
  std::map<int, User>::iterator it1 = tmp_user_list.begin();
  std::map<int, User>::iterator it2 = tmp_user_list.end();
  time_t current_time = time(NULL);

  while (it1 != it2) {
    if (current_time >
        (it1->second).get_created_time() + AUTHENTICATE_TIMEOUT) {
      User& tmp_user = it1->second;
      it1++;
      connection_fin(tmp_user.get_pfd());
    } else {
      it1++;
    }
  }
}

void Server::user_ping_chk(void) {}

User& Server::operator[](int socket_fd) {
  if (user_list.find(socket_fd) != user_list.end()) {
    return user_list.at(socket_fd);
  } else if (tmp_user_list.find(socket_fd) != tmp_user_list.end()) {
    return tmp_user_list.at(socket_fd);
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

int Server::operator[](const String& nickname) {
  if (nick_to_soc.find(nickname) != nick_to_soc.end()) {
    return nick_to_soc.at(nickname);
  } else if (tmp_nick_to_soc.find(nickname) != tmp_nick_to_soc.end()) {
    return tmp_nick_to_soc.at(nickname);
  } else {
    throw std::invalid_argument("Subsription error!");
  }
}

void Server::add_channel(Channel& new_channel) {
  channel_list.insert(
      std::pair<String, Channel>(new_channel.get_channel_name(), new_channel));
}

bool Server::chk_user_in_channel(const String& nickname,
                                 const String& chan_name) const {
  std::map<String, int>::const_iterator user_chan_it =
      event_user_chan.find(tmp_chan_name);
  if (user_chan_it == event_user_chan.end()) {
    event_user_in_chan = false;
  } else {
    event_user_in_chan = true;
  }
}