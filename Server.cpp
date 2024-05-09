#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : port(std::atoi(_port)),
      str_port(_port),
      serv_name(SERVER_NAME),
      chantype(CHAN_TYPE),
      password(_password),
      enable_ident_protocol(false),
      max_nickname_len(INIT_MAX_NICKNAME_LEN),
      max_username_len(INIT_MAX_USERNAME_LEN),
      max_channel_num(INIT_MAX_CHANNEL_NUM) {
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
    int retry_cnt = 10;
    std::stringstream port_tmp;
    while (::bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) ==
           -1) {
      if (retry_cnt == 0) {
        throw std::exception();
      } else if (errno == EADDRINUSE) {
        port++;
        if (port > 65335) {
          port = 1024;
        }
        port_tmp << port - 1;
        port_tmp >> str_port;
        serv_addr.sin_port = htons(port);
        std::cerr << "Port " << str_port << " already in use. Try port " << port
                  << '\n';
        retry_cnt--;
      } else {
        throw socket_bind_error();
      }
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
          if (observe_fd[i].revents & POLLOUT) {
            revent_pollout(observe_fd[i]);
          }
          // IRSSI로 부터 데이터가 들어왔을때
          if (observe_fd[i].revents &
              (POLLIN | POLLHUP)) {  // 소켓연결이 끊어졌을 경우 발생
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
    std::cout << "early break\n";
    for (int i = 1; i < MAX_USER; i++) {
      if (observe_fd[i].fd == -1) {
        observe_fd[i].fd = user_socket;
        observe_fd[i].events = POLLIN;
        std::cout << "yeckim babo\n";
        break;
      }
    }
    // std::cerr << "Connection established at " << user_socket << '\n';
    std::cout << "Connection established at " << user_socket << '\n';
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
      std::cout << GREEN << "\t\t\tNOT AUTHENTICATED USER!!!!!!" << std::endl;
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

    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    std::cout << RED << "\n[IRSSI REQUEST] :: " << YELLOW << msg_list[j]
              << WHITE << std::endl;

    // SHOW THE LIST OF CLIENTS
    // [DEBUG]
    if (msg_list[0] == "lusers") {
      std::vector<User> clientsVector = (*this).getUserList();
      for (std::vector<User>::const_iterator it = clientsVector.begin();
           it != clientsVector.end(); ++it) {
        const User& user = *it;
        std::cout << user;
      }
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
    } else if (cmd_type == PRIVMSG) {
      cmd_privmsg(p_val.fd, msg);
    } else if (cmd_type == ERROR) {
      event_user.push_msg(msg.to_raw_msg());
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
    } else if (cmd_type == WHO) {
      cmd_who(p_val.fd, msg);
    } else if (cmd_type == KICK) {
      cmd_kick(p_val.fd, msg);
    }
    // else if (cmd_type == INVITE) {
    //   cmd_invite(p_val.fd, msg);
    // }
    if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
      p_val.events = POLLIN | POLLOUT;
    } else {
      p_val.events = POLLIN;
    }
  }
}

// void Server::cmd_invite(int recv_fd, const Message& msg) {

// }

void Server::cmd_kick(int recv_fd, const Message& msg) {
  std::string targetChannelStr = msg.get_params()[0];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  // 강퇴할 클라이언트가 속할 채널
  server_channel_iterator = get_server_channel_iterator(targetChannelStr);
  if (server_channel_iterator == server_channel_list.end()) {
    // ERR_NOSUCHCHANNEL (403)
    User& event_user = (*this)[recv_fd];
    Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return;
  }

  try {
    int targetFileDescriptor = (*this)[msg.get_params()[1]];
    User& opUser = (*this)[recv_fd];
    User& outUser = (*this)[targetFileDescriptor];
    (*this).kickClient(opUser, outUser,
                       get_server_channel(server_channel_iterator), msg);
  } catch (std::invalid_argument& e) {
    User& event_user = (*this)[recv_fd];
    Message rpl = Message::rpl_401(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
  }
}

// [DEBUG]
void Server::cmd_who(int recv_fd, const Message& msg) {
  // 채널 안의 클라이언트 목록을 보여주는 기능
  std::string targetChannelStr = msg.get_params()[0];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  if (server_channel_iterator == server_channel_list.end()) return;

  std::cout << CYAN << "=>> Server Channel List :: [";
  std::map<std::string, Channel>::const_iterator cit;
  bool found = false;
  for (cit = server_channel_list.begin(); cit != server_channel_list.end();
       cit++) {
    std::string channelName = cit->first;
    std::cout << channelName << ", ";
    found = true;
  }
  if (found == false) return;
  std::cout << "]" << std::endl << std::endl;

  // get_server_channel(get_server_channel_iterator(targetChannelStr)).visualizeClientList();
  std::cout << get_server_channel(
      get_server_channel_iterator(targetChannelStr));
}

/////////////////////////////////////////////////////////////////////
void Server::cmd_names(int recv_fd, const Message& msg) {
  std::map<std::string, Channel>::const_iterator it;
  std::cout << "[FT_IRC Server] <Channel Status> :: [";

  for (it = server_channel_list.begin(); it != server_channel_list.end();
       ++it) {
    const std::string& channelName = it->first;
    std::cout << channelName << "=> ";
  }
  std::cout << "]" << std::endl << std::endl;
}

void Server::cmd_join(int recv_fd, const Message& msg) {
  // [TO DO] :: channel 목록 capacity를 넘으면 더이상 받지 않기 => RFC 에서
  // 어떻게 리스폰스를 주는지 확인해볼것

  try {
    if (get_current_channel_num() >
        get_max_channel_num())  // && channel이 새로운 채널인지 확인하고
                                // 맞다면 에러를 뱉어야함
      throw(server_channel_list_capacity_error());
    std::string targetChannelStr = msg.get_params()[0];
    std::string::size_type pos = targetChannelStr.find('#');
    if (pos != std::string::npos) {
      targetChannelStr.erase(pos, 1);
    }
    server_channel_iterator = get_server_channel_iterator(targetChannelStr);
    if (server_channel_iterator != server_channel_list.end()) {
      std::cout << "@@@@@@@@@\n";
      get_server_channel(get_server_channel_iterator(targetChannelStr))
          .addClient((*this)[recv_fd]);
    } else {
      Channel newChannel(targetChannelStr);
      addChannel(newChannel);
      get_server_channel(get_server_channel_iterator(targetChannelStr))
          .addClient((*this)[recv_fd]);
      get_server_channel(get_server_channel_iterator(targetChannelStr))
          .addOperator((*this)[recv_fd]);
    }
    // server_channel_iterator =
    // get_server_channel_iterator(targetChannelStr);
    // server_channel_iterator->second.addClient((*this)[recv_fd]);

    User& incomingClient = (*this)[recv_fd];

    // [STEP 1] :: JOIN 요청을 수신 후 => 클라이언트와 닉네임 사용자 정보를
    // 나타내줌
    Message rpl1;
    rpl1.set_source(incomingClient.get_nick_name() + std::string("!") +
                    std::string("@localhost"));
    rpl1.set_cmd_type(JOIN);
    rpl1.push_back(msg.get_params()[0]);
    std::cout << YELLOW << rpl1.to_raw_msg() << std::endl;
    incomingClient.push_msg(rpl1.to_raw_msg());

    // [STEP 2] :: 이 채널에 몇명의 어떤 클라이언트들이 있는지 반응을 보내줌
    // example => :irc.example.net 353 lfkn___ = #b :lfkn___ lfkn__ lfkn_
    // @lfkn\r for 문으로 map을 순회하면서 닉네임을 만들어줄것

    incomingClient.push_msg(
        Message::rpl_353(
            serv_name,
            get_server_channel(get_server_channel_iterator(targetChannelStr)),
            incomingClient.get_nick_name(), msg.get_params()[0])
            .to_raw_msg());
    std::cout << YELLOW
              << Message::rpl_353(
                     serv_name,
                     get_server_channel(
                         get_server_channel_iterator(targetChannelStr)),
                     incomingClient.get_nick_name(), msg.get_params()[0])
                     .to_raw_msg()
              << std::endl;

    // [STEP 3] ::
    // :irc.example.net 366 lfkn___ #b :End of NAMES list\r
    incomingClient.push_msg(Message::rpl_366(serv_name,
                                             incomingClient.get_nick_name(),
                                             msg.get_params()[0])
                                .to_raw_msg());
    std::cout << YELLOW
              << Message::rpl_366(serv_name, incomingClient.get_nick_name(),
                                  msg.get_params()[0])
                     .to_raw_msg()
              << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
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
        std::cout << "--------------->> \t\tuser being deleted :: ["
                  << event_user.get_nick_name() << "]" << std::endl;
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

// CHANNEL
const int Server::get_max_channel_num(void) const { return max_channel_num; };
int Server::get_current_channel_num(void) {
  return server_channel_list.size();
};

void Server::add_tmp_user(const int user_socket, const sockaddr_in& user_addr) {
  User tmp(user_socket, user_addr);
  std::cout << "---------------ㅁ-----------------" << std::endl;
  std::cout << tmp.get_user_socket() << " :: " << tmp.get_nick_name()
            << std::endl;

  // // tmp_user_list 순회 및 출력
  // std::cout << "tmp_user_list contents:" << std::endl;
  // for (const auto& pair : tmp_user_list) {
  //     int key = pair.first;
  //     const User& user = pair.second;
  //     std::cout << "Key: " << key << ", User: " << user << std::endl;
  // }

  // // tmp_nick_to_soc 순회 및 출력
  // std::cout << "tmp_nick_to_soc contents:" << std::endl;
  // for (const auto& pair : tmp_nick_to_soc) {
  //     const std::string& nick = pair.first;
  //     int soc = pair.second;
  //     std::cout << "Nick: " << nick << ", Socket: " << soc << std::endl;
  // }

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
  std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
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
    std::cout << YELLOW << "[SERVER SENDING...] " << GREEN_BOLD << "["
              << msg_tmp << "]" << WHITE;
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
    // if (msg[0].at(0) == ':')
    // {
    //   nick_tmp = msg[0].substr(1);
    //   std::cout << YELLOW << "===================>> " << nick_tmp << WHITE
    //   << std::endl;
    // }
    // else
    // {
    //   nick_tmp = msg[0];
    // }
    nick_tmp = msg[0];
    std::cout << YELLOW << "===================>> " << nick_tmp << WHITE
              << std::endl;

    if (('0' <= nick_tmp[0] && nick_tmp[0] <= '9') || nick_tmp[0] == ':' ||
        nick_tmp.find_first_of(chantype + std::string(": \n\t\v\f\r")) !=
            std::string::npos ||
        nick_tmp.length() > max_nickname_len) {
      // ERR_ERRONEUSNICKNAME (432)
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
        // [DEBUG]
        std::cout << CYAN << rpl << rpl.to_raw_msg();
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
    // user name 여기다?
    event_user.set_user_name(msg.get_params()[0]);
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

void Server::cmd_privmsg(int recv_fd, const Message& msg) {
  User& source_user = (*this)[recv_fd];

  std::string sourceNickName = source_user.get_nick_name();
  std::string sourceUserName = source_user.get_user_name();
  std::string targetNickName = msg.get_params().front();
  // targetNickName 의 첫번째 글자가 '#'일 경우 => 채널간 client 들과의 소통
  if (targetNickName[0] == '#') {
    // 채널에 속한 유저들 fd에 message다 적어서 쏴주기
    std::string targetChannelStr = msg.get_params()[0];
    std::string::size_type pos = targetChannelStr.find('#');
    if (pos != std::string::npos) {
      targetChannelStr.erase(pos, 1);
    }

    if (get_server_channel(get_server_channel_iterator(targetChannelStr))
            .foundClient(sourceNickName) == false) {
      return;
    }
    std::map<std::string, User&> map =
        get_server_channel(get_server_channel_iterator(targetChannelStr))
            .get_channel_client_list();
    std::map<std::string, User&>::iterator it;
    for (it = map.begin(); it != map.end(); ++it) {
      Message rpl;
      int target_fd = (*this)[it->first];
      User& target_user = (*this)[target_fd];
      if (sourceNickName == target_user.get_nick_name()) continue;
      // if (!foundClient(target_user.get_nick_name()))
      //   continue ;

      /*
        :lfkn_!~memememe@localhost PRIVMSG #test :d\r
      */

      rpl.set_source(sourceNickName + std::string("!~") + sourceUserName +
                     std::string("@localhost"));
      rpl.set_cmd_type(PRIVMSG);
      rpl.push_back(msg.get_params()[0]);
      std::string string;
      for (int i = 1; i < msg.get_params_size(); ++i) {
        string += msg.get_params()[i];
        // rpl.push_back(msg.get_params()[i]);
      }
      rpl.push_back(string);
      target_user.push_msg(rpl.to_raw_msg());

      pollfd tmp;
      for (int i = 0; i < MAX_USER; i++) {
        if (observe_fd[i].fd == target_user.get_user_socket()) {
          tmp = observe_fd[i];
        }
      }

      if ((*this).send_msg_at_queue(target_user.get_user_socket()) == -1) {
        tmp.events = POLLIN | POLLOUT;
      } else {
        tmp.events = POLLIN;
      }
    }
    return;
  }

  int targetFileDescriptor;

  try {
    targetFileDescriptor = (*this)[targetNickName];
  } catch (const std::invalid_argument& e) {
    // rpl ERR_NOSUCHNICK (401) 날리기
    source_user.push_msg(
        Message::rpl_401(serv_name, source_user.get_nick_name(), targetNickName)
            .to_raw_msg());
    return;
  }

  std::cout << "===>> " << targetNickName << std::endl;
  std::cout << "===>> " << targetFileDescriptor << std::endl;

  // 만약 찾았으면 거기다 적어주기
  // 기본적인 1:1 대화 기능 구현 성공
  User& target_user = (*this)[targetFileDescriptor];

  // :lfkn!~memememe@localhost JOIN :#owqenflweanfwe\r
  Message rpl;
  rpl.set_source(sourceNickName + std::string("!") + std::string("~") +
                 sourceUserName + std::string("@localhost"));
  rpl.set_cmd_type(PRIVMSG);

  int i = 0;
  for (i = 0; i < msg.get_params_size() - 1; ++i) {
    rpl.push_back(msg.get_params()[i]);
  }
  rpl.push_back(std::string(":") + msg[i]);
  std::cout << YELLOW << rpl.to_raw_msg() << std::endl;
  target_user.push_msg(rpl.to_raw_msg());
  // (*this).send_msg_at_queue(target_user.get_user_socket());

  pollfd* tmp;
  for (int i = 0; i < MAX_USER; i++) {
    if (observe_fd[i].fd == target_user.get_user_socket()) {
      tmp = &(observe_fd[i]);
    }
  }

  if ((*this).send_msg_at_queue(target_user.get_user_socket()) == -1) {
    tmp->events = POLLIN | POLLOUT;
  } else {
    tmp->events = POLLIN;
  }
}

void Server::addChannel(Channel& newChannel) {
  server_channel_list.insert(std::pair<std::string, Channel>(
      newChannel.get_channel_name(), newChannel));
  // server_channel_list.insert(std::pair<std::string,
  // Channel&>(newChannel.get_channel_name(), newChannel));
}

std::map<std::string, Channel>::iterator Server::get_server_channel_iterator(
    std::string targetChannelStr) {
  server_channel_iterator = server_channel_list.begin();
  return server_channel_list.find(targetChannelStr);
}

Channel& Server::get_server_channel(
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
    std::cout << "?!@#?!@#?!@?#!@?#!?@#?!@#?!@?#!@?#\n";
    /*
      ERR_NOSUCHNICK (401)
        "<client> <nickname> :No such nick/channel"
        Indicates that no client can be found for the supplied nickname. The
      text used in the last param of this message may vary.
    */

    // :irc.example.net 401 lfkn slkfdn :No such nick or channel name\r
  }
}