#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(ft_itos(port)),
      serv_name(SERVER_NAME),
      serv_version(SERVER_VERSION),
      created_time(std::time(NULL)),
      serv_info("Server Info Text"),
      password(ft_strip(_password)),
      enable_ident_protocol(false) {
  serv_socket = socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw socket_create_error();
  }
  if (fcntl(serv_socket, F_SETFL, O_NONBLOCK) == -1) {
    throw socket_setting_error();
  }
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  int retry_cnt = 10;
  std::stringstream port_tmp;
  while (bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
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

  if (listen(serv_socket, 64) == -1) {
    throw socket_listening_error();
  }

  observe_fd[0].fd = serv_socket;
  observe_fd[0].events = POLLIN;

  for (int i = 1; i < MAX_USER; i++) {
    observe_fd[i].fd = -1;
  }

  created_time_str = ctime(&created_time);
  created_time_str.erase(created_time_str.length() - 1);

  std::cout << "Server created at " << created_time_str << '\n'
            << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":"
            << port << std::endl;
}

Server::~Server() {
  std::map<int, User>::iterator head = user_list.begin();
  std::map<int, User>::iterator tail = user_list.end();

  Message notice;

  notice.set_source(serv_name);
  notice.set_cmd_type(NOTICE);
  notice.push_back("");
  notice.push_back(":Connection statistics: client - kb, server - kb.");

  for (; head != tail; head++) {
    User& tmp = head->second;

    notice[0] = tmp.get_nick_name();
    tmp.push_back_msg(notice.to_raw_msg());
    ft_send(tmp.get_pfd());
    tmp.push_back_msg("ERROR :Server going down\r\n");
    ft_send(tmp.get_pfd());
    close(head->first);
  }

  head = tmp_user_list.begin();
  tail = tmp_user_list.end();

  for (; head != tail; head++) {
    User& tmp = head->second;

    notice[0] = tmp.get_nick_name();
    tmp.push_back_msg(notice.to_raw_msg());
    ft_send(tmp.get_pfd());
    tmp.push_back_msg("ERROR :Server going down\r\n");
    ft_send(tmp.get_pfd());
    close(head->first);
  }
  close(serv_socket);
}

void Server::server_quit(void) {
  std::map<int, User>::iterator head = user_list.begin();
  std::map<int, User>::iterator tail = user_list.end();

  Message notice;

  notice.set_source(serv_name);
  notice.set_cmd_type(NOTICE);
  notice.push_back("");
  notice.push_back(":Connection statistics: client - kb, server - kb.");

  for (; head != tail; head++) {
    User& tmp = head->second;

    notice[0] = tmp.get_nick_name();
    tmp.push_back_msg(notice.to_raw_msg());
    ft_send(tmp.get_pfd());
    tmp.push_back_msg("ERROR :Server going down\r\n");
    ft_send(tmp.get_pfd());
    close(head->first);
  }

  head = tmp_user_list.begin();
  tail = tmp_user_list.end();

  for (; head != tail; head++) {
    User& tmp = head->second;

    notice[0] = tmp.get_nick_name();
    tmp.push_back_msg(notice.to_raw_msg());
    ft_send(tmp.get_pfd());
    tmp.push_back_msg("ERROR :Server going down\r\n");
    ft_send(tmp.get_pfd());
    close(head->first);
  }
  close(serv_socket);
}

void Server::server_listen(void) {
  int event_cnt = 0;

  while (true) {
    tmp_user_timeout_chk();
    event_cnt = poll(observe_fd, MAX_USER, POLL_TIMEOUT * 1000);
    user_ping_chk();
    if (event_cnt == 0) {
      continue;
    } else if (event_cnt < 0) {
      perror("poll() error!");
      return;
    } else {
      if (observe_fd[0].revents & POLLIN) {
        user_socket_init();
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

int Server::user_socket_init(void) {
  int connection_limit = 10;
  socklen_t user_addr_len;
  sockaddr_in user_addr;
  int user_socket;

  while (connection_limit != 0) {
    user_addr_len = sizeof(user_addr);
    memset(&user_addr, 0, user_addr_len);
    user_socket = ::accept(serv_socket, (sockaddr*)&user_addr, &user_addr_len);
    if (user_socket == -1) {
      return 0;
    }

    std::map<unsigned int, int>::iterator ip_it =
        ip_list.find(user_addr.sin_addr.s_addr);
    if (ip_it != ip_list.end()) {
      if (ip_it->second > MAXCONNECTIONSIP) {
        std::cout << "To many connection at " << inet_ntoa(user_addr.sin_addr)
                  << '\n';
        send(user_socket,
             "ERROR :Connection refused, too many connections from your IP "
             "address\r\n",
             strlen("ERROR :Connection refused, too many connections from "
                    "your IP address\r\n"),
             MSG_DONTWAIT);
        close(user_socket);
        return -1;
      } else {
        ip_it->second++;
      }
    } else {
      ip_list.insert(std::pair<in_addr_t, int>(user_addr.sin_addr.s_addr, 1));
    }
#ifdef DEBUG
    std::cout << YELLOW << "ip : " << GREEN << inet_ntoa(user_addr.sin_addr)
              << "(" << user_addr.sin_addr.s_addr << "), connected count : "
              << ip_list.find(user_addr.sin_addr.s_addr)->second << WHITE
              << "\n\n";
#endif

    if (fcntl(user_socket, F_SETFL, O_NONBLOCK) == -1) {
      // error_handling
      perror("fcntl() error");
      send(user_socket, "ERROR :socket setting error\r\n",
           strlen("ERROR :socket setting error\r\n"), MSG_DONTWAIT);
      close(user_socket);
      return -1;
    }

    int bufSize = SOCKET_BUFFER_SIZE;
    if (setsockopt(user_socket, SOL_SOCKET, SO_SNDBUF, &bufSize,
                   sizeof(bufSize)) == -1) {
      send(user_socket, "ERROR :socket setting error\r\n",
           strlen("ERROR :socket setting error\r\n"), MSG_DONTWAIT);
      perror("setsockopt() error");
      close(user_socket);
      return -1;
    }

    if ((tmp_user_list.size() + user_list.size()) > MAX_USER) {
      // todo : send some error message to user
      // 근데 이거 클라이언트한테 메세지 보내려면 소켓을 연결해야 하는데
      // 에러 메세지 답장 안 보내고 연결요청 무시하려면 되려나
      // 아니면 예비소켓 하나 남겨두고 잠시 연결했다 바로 연결 종료하는
      // 용도로 사용하는 것도 방법일 듯
      std::cerr << "User connection limit exceed!\n";
      send(user_socket, "ERROR :Too many users on server\r\n",
           strlen("ERROR :Too many users on server\r\n"), MSG_DONTWAIT);
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

    std::cout << "Connection established at " << user_socket << '\n';
    connection_limit--;
  }
  return 0;
}

void Server::connection_fin(pollfd& p_val) {
  std::map<unsigned int, int>::iterator ip_it =
      ip_list.find((*this)[p_val.fd].get_user_addr().sin_addr.s_addr);

  ip_it->second--;
#ifdef DEBUG
  std::cout << GREEN << "ip : " << GREEN
            << inet_ntoa((*this)[p_val.fd].get_user_addr().sin_addr) << "("
            << ip_it->first << "), connected count : " << ip_it->second << WHITE
            << "\n\n";
#endif
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
  std::map<int, User>::iterator user_it = user_list.find(socket_fd);
  if (user_it == user_list.end()) {
    user_it = tmp_user_list.find(socket_fd);
    if (user_it == tmp_user_list.end()) {
      return 0;
    }
  }

  User& tmp_user = user_it->second;

  if (tmp_user.get_already_disconnected() == true) {
    return 0;
  }

  size_t to_send_num = tmp_user.get_to_send_size();
  size_t msg_len;
  size_t idx;
  ssize_t bytes_sent;
  bool error_flag;

  while (to_send_num > 0) {
    const String& msg = tmp_user.get_front_msg();
    msg_len = msg.length();
    idx = 0;
    error_flag = false;

    while (idx < msg_len) {
      String msg_blk = msg.substr(idx, SOCKET_BUFFER_SIZE);
      bytes_sent = send_msg_block(socket_fd, msg_blk);
      if (bytes_sent == static_cast<ssize_t>(msg_blk.length())) {
        idx += msg_blk.length();
      } else {
        error_flag = true;
        break;
      }
    }
    tmp_user.pop_front_msg();
    if (error_flag == true) {
      tmp_user.push_front_msg(msg.substr(idx + bytes_sent));
      return -1;
    }
    to_send_num--;
  }
  return 0;
}

ssize_t Server::send_msg_block(int socket_fd, const String& blk) {
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

    if (msg_list.size() == 0) {
      if (event_user.get_already_disconnected() == true) {
        std::cout << "Connection close at " << p_val.fd << '\n';
        connection_fin(p_val);
      }
      return;
    }

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
  } catch (const std::invalid_argument& e) {
    std::cerr << e.what() << '\n';
    std::cerr << "Maybe cmd_nick throw this\n";
  } catch (const std::exception& e) {
    // error handling
    std::cerr << e.what() << '\n';
    std::cerr << "unexpected exception occur! Program terminated!\n";
    exit(1);
  }
}

void Server::auth_user(pollfd& p_val, std::vector<String>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (size_t j = 0; j < msg_list.size(); j++) {
    Message msg(p_val.fd, msg_list[j]);

    ////////////////////////////////////////////////////////////////////////////////////////////
#ifdef DEBUG
    std::cout << RED << "\n[IRSSI REQUEST] :: " << YELLOW << msg_list[j]
              << WHITE << std::endl;

    // SHOW THE LIST OF userS
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
      return;
    } else if (cmd_type == WHO) {
      cmd_who(p_val.fd, msg);
    } else if (cmd_type == KICK) {
      cmd_kick(p_val.fd, msg);
    } else if (cmd_type == INVITE) {
      cmd_invite(p_val.fd, msg);
    } else if (cmd_type == TOPIC) {
      cmd_topic(p_val.fd, msg);
    } else if (cmd_type == NAMES) {
      cmd_names(p_val.fd, msg);
    } else if (cmd_type == PRIVMSG) {
      cmd_privmsg(p_val.fd, msg);
    } else if (cmd_type == JOIN) {
      cmd_join(p_val.fd, msg);
    } else if (cmd_type == PART) {
      cmd_part(p_val.fd, msg);
    } else if (cmd_type == LIST) {
      cmd_list(p_val.fd, msg);
    } else if (cmd_type == WHOIS) {
      cmd_whois(p_val.fd, msg);
    } else {
      event_user.push_back_msg(
          rpl_421(serv_name, event_user.get_nick_name(), msg.get_cmd())
              .to_raw_msg());
    }
    ft_send(p_val);
  }
  event_user.set_have_to_ping_chk(false);
  event_user.set_last_ping(std::time(NULL));
}

void Server::not_auth_user(pollfd& p_val, std::vector<String>& msg_list) {
  User& event_user = (*this)[p_val.fd];

  for (size_t j = 0; j < msg_list.size(); j++) {
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
      Message rpl = rpl_451(serv_name, event_user.get_nick_name());
      event_user.push_back_msg(rpl.to_raw_msg());
    }
    ft_send(p_val);

    if (event_user.get_nick_init_chk() == OK &&
        event_user.get_user_init_chk() == OK) {
      if (event_user.get_password_chk() != OK) {
        event_user.push_back_msg(
            rpl_464(serv_name, event_user.get_nick_name()).to_raw_msg());
        event_user.set_have_to_disconnect(true);
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
  const String& user = event_user.get_nick_name();

  event_user.push_back_msg(
      rpl_001(serv_name, user, event_user.make_source(1)).to_raw_msg());
  event_user.push_back_msg(
      rpl_002(serv_name, user, serv_name, serv_version).to_raw_msg());
  event_user.push_back_msg(
      rpl_003(serv_name, user, created_time_str).to_raw_msg());
  event_user.push_back_msg(rpl_004(serv_name, user, serv_name, serv_version,
                                   AVAILABLE_USER_MODES,
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
    specs2.push_back("KICKLEN=" + ft_itos(KICKLEN));
  }
  event_user.push_back_msg(rpl_005(serv_name, user, specs1).to_raw_msg());
  event_user.push_back_msg(rpl_005(serv_name, user, specs2).to_raw_msg());

  ft_send(p_val);
}

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
  User& tmp_user = (*this)[socket_fd];

  nick_to_soc.insert(std::make_pair(tmp_user.get_nick_name(), socket_fd));
  user_list.insert(std::make_pair(socket_fd, tmp_user));
  (*this)[socket_fd].set_last_ping(std::time(NULL));

  tmp_nick_to_soc.erase(tmp_user.get_nick_name());
  tmp_user_list.erase(socket_fd);
}

void Server::remove_user(int socket_fd) {
  std::map<int, User>::iterator it1;
  std::map<String, int>::iterator it2;
  String tmp;

  it1 = user_list.find(socket_fd);
  if (it1 != user_list.end()) {
    User& event_user = (*this)[socket_fd];
    const String& event_user_nick = event_user.get_nick_name();

    std::map<String, int>::const_iterator user_chan_it =
        event_user.get_channels().begin();
    for (; user_chan_it != event_user.get_channels().end(); ++user_chan_it) {
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(user_chan_it->first);
      if (chan_it != channel_list.end()) {
        chan_it->second.remove_user(event_user_nick);
        if (chan_it->second.get_user_num() == 0) {
          channel_list.erase(chan_it->first);
        }
      }
    }

    tmp = (it1->second).get_nick_name();
    user_list.erase(it1);
    it2 = nick_to_soc.find(tmp);
    nick_to_soc.erase(it2);
    close(socket_fd);
    return;
  }

  it1 = tmp_user_list.find(socket_fd);
  if (it1 != tmp_user_list.end()) {
    tmp = (it1->second).get_nick_name_no_chk();
    tmp_user_list.erase(it1);
    it2 = tmp_nick_to_soc.find(tmp);
    tmp_nick_to_soc.erase(it2);
    close(socket_fd);
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
    User& event_user = (*this)[it1->second];
    const String& event_user_nick = event_user.get_nick_name();

    std::map<String, int>::const_iterator user_chan_it =
        event_user.get_channels().begin();
    for (; user_chan_it != event_user.get_channels().end(); ++user_chan_it) {
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(user_chan_it->first);
      if (chan_it != channel_list.end()) {
        chan_it->second.remove_user(event_user_nick);
        if (chan_it->second.get_user_num() == 0) {
          channel_list.erase(chan_it->first);
        }
      }
    }

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

void Server::change_nickname(const String& old_nick, const String& new_nick) {
  std::map<String, int>::iterator it;
  int tmp_fd;

  it = nick_to_soc.find(old_nick);
  if (it != nick_to_soc.end()) {
    tmp_fd = it->second;
    nick_to_soc.erase(it);
    nick_to_soc.insert(std::make_pair(new_nick, tmp_fd));
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

void Server::user_ping_chk(void) {
  std::map<int, User>::iterator user_it = user_list.begin();
  std::map<int, User>::iterator user_it2;
  std::time_t now = std::time(NULL);

  while (user_it != user_list.end()) {
    User& tmp_user = user_it->second;
    const String& tmp_user_nick = tmp_user.get_nick_name();

    if (tmp_user.get_have_to_ping_chk() == true) {
      if (now > tmp_user.get_last_ping() + PINGTIMEOUT + PONGTIMEOUT) {
        user_it2 = user_it;
        ++user_it2;

        Message rpl;

        rpl.set_source(tmp_user.make_source(1));
        rpl.set_cmd_type(QUIT);
        rpl.push_back(":Ping timeout: 20 seconds");
        send_msg_to_connected_user(tmp_user, rpl.to_raw_msg());

        rpl.clear();
        rpl.set_source(serv_name);
        rpl.set_cmd_type(NOTICE);
        rpl.push_back(tmp_user_nick);
        rpl.push_back(":Connection statistics: client - kb, server - kb.");
        tmp_user.push_back_msg(rpl.to_raw_msg());
        tmp_user.push_back_msg("ERROR :Ping timeout: 20 seconds\r\n");

        std::cout << "Connection close at " << tmp_user.get_user_socket()
                  << '\n';
        tmp_user.set_have_to_disconnect(true);
        ft_sendd(tmp_user.get_pfd());
        user_it = user_it2;
        continue;
      }
    } else {
      if (now > tmp_user.get_last_ping() + PINGTIMEOUT) {
        tmp_user.set_have_to_ping_chk(true);
        Message ping;

        ping.set_cmd_type(PING);
        ping.push_back(":" + serv_name);

        tmp_user.push_back_msg(ping.to_raw_msg());
        ft_send(tmp_user.get_pfd());
      }
    }
    ++user_it;
  }
}

void Server::read_msg_from_socket(int socket_fd,
                                  std::vector<String>& msg_list) {
  User& event_user = (*this)[socket_fd];

  char read_block[SOCKET_BUFFER_SIZE];
  int repeat_cnt = 5;
  int read_cnt = 0;
  String read_buf;
  size_t end_idx;

  while (--repeat_cnt >= 0) {
    read_cnt =
        recv(socket_fd, read_block, SOCKET_BUFFER_SIZE - 1, MSG_DONTWAIT);
    read_block[read_cnt] = '\0';
    read_buf += read_block;
    if (read_cnt < SOCKET_BUFFER_SIZE - 1) {
      break;
    }
  }
  if (read_cnt == 0) {
    event_user.set_already_disconnected(true);
  }
  if (read_buf.length() == 0) {
    return;
  }

  size_t front_pos = read_buf.find_first_not_of(" \n\t\v\f\r");
  size_t back_pos = read_buf.find_last_not_of(" \n\t\v\f\r");
  if (front_pos == String::npos || back_pos == String::npos) {
    return;
  }

  ft_split(read_buf.substr(front_pos, back_pos - front_pos + 1), "\r\n",
           msg_list);
  if (event_user.remain_input.length() != 0) {
    msg_list[0] = event_user.remain_input + msg_list[0];
    event_user.remain_input = "";
  }
  end_idx = read_buf.find_last_not_of("\r\n");
  if (end_idx == read_buf.length() - 1) {
    event_user.remain_input = *(msg_list.end() - 1);
    msg_list.erase(msg_list.end() - 1);
  }
}

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

bool Server::chk_channel_exist(const String& chan_name) const {
  std::map<String, Channel>::const_iterator chan_it =
      channel_list.find(chan_name);
  if (chan_it != channel_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Server::send_msg_to_channel(Channel& chan, const String& msg) {
  std::map<String, User*>::iterator chan_user_it = chan.get_user_list().begin();
  for (; chan_user_it != chan.get_user_list().end(); ++chan_user_it) {
    chan_user_it->second->push_back_msg(msg);
    ft_send(chan_user_it->second->get_pfd());
  }
}

void Server::send_msg_to_channel_except_sender(Channel& chan,
                                               const String& sender,
                                               const String& msg) {
  std::map<String, User*>::iterator chan_user_it = chan.get_user_list().begin();
  for (; chan_user_it != chan.get_user_list().end(); ++chan_user_it) {
    if (chan_user_it->first != sender) {
      chan_user_it->second->push_back_msg(msg);
      ft_send(chan_user_it->second->get_pfd());
    }
  }
}

void Server::send_msg_to_connected_user(const User& u, const String& msg) {
  std::map<int, User>::iterator user_it = user_list.begin();
  std::map<String, int>::const_iterator user_chan_it;

  for (; user_it != user_list.end(); ++user_it) {
    User& tmp_u = user_it->second;
    const String& tmp_name = tmp_u.get_nick_name();

    if (tmp_name == u.get_nick_name()) {
      continue;
    }

    for (user_chan_it = u.get_channels().begin();
         user_chan_it != u.get_channels().end(); ++user_chan_it) {
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(user_chan_it->first);
      if (chan_it != channel_list.end()) {
        if (chan_it->second.chk_user_join(tmp_name) == true) {
          tmp_u.push_back_msg(msg);
          ft_send(tmp_u.get_pfd());
          break;
        }
      }
    }
  }
}

#ifdef DEBUG
void Server::print_channel_list(void) const {
  std::map<String, Channel>::const_iterator chan_it = channel_list.begin();

  std::cout << RED << "[Channel Lists in the server]\n";
  for (; chan_it != channel_list.end(); ++chan_it) {
    std::cout << YELLOW << "name : " << chan_it->first
              << " , user number in channel : "
              << chan_it->second.get_user_list().size() << '\n';
  }
  std::cout << WHITE;
}

void Server::print_user_list(void) const {
  std::map<int, User>::const_iterator user_it = user_list.begin();

  std::cout << RED << "[User Lists in the server]\n" << YELLOW;
  for (; user_it != user_list.end(); ++user_it) {
    std::cout << user_it->second.get_nick_name() << ", ";
  }
  std::cout << WHITE << '\n';
}

#endif