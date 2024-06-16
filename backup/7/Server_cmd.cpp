#include "Server.hpp"

void Server::cmd_topic(int recv_fd, const Message& msg) {
  std::string targetChannelStr = msg.get_params()[0];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  channel_iterator = get_channel_iterator(targetChannelStr);
  if (channel_iterator == channel_list.end()) {
    // ERR_NOSUCHCHANNEL (403)
    User& event_user = (*this)[recv_fd];
    Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return;
  }

  // OPERATOR가 아닐경우 482 RESPONSE
  // checkPrivilege() 라는 함수로 나중에 refactoring 할것
  User& event_user = (*this)[recv_fd];

  // if (get_channel(channel_iterator).get_channel_mode() & FLAG_T
  // == 0 ) { if
  // (get_channel(channel_iterator).isOperator(event_user) ==
  // false) {
  //   Message rpl = Message::rpl_482(serv_name, event_user.get_nick_name(),
  //   msg); event_user.push_msg(rpl.to_raw_msg()); return;
  // }
  // }  else {
  // TOPIC 변경하기
  // }
  if (get_channel(channel_iterator).isOperator(event_user) ==
      false) {
    Message rpl = Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return;
  }

  /*
    < 2024/05/11 15:09:25.000532736  length=45 from=4305 to=4349
    :dy!~memememe@localhost TOPIC #test :welfkn\r
  */
  std::map<int, User>::iterator it;
  for (it = user_list.begin(); it != user_list.end(); ++it) {
    std::string clientNickName = it->second.get_nick_name();
    Message rpl;

    rpl.set_source(event_user.get_nick_name() + "!~" +
                   (*this)[it->first].get_user_name() + "@localhost");
    rpl.set_cmd_type(TOPIC);
    rpl.push_back(msg.get_params()[0]);
    ////////////////////////////////////
    /* topic 내용 적어주기 */
    std::string trailing;
    if (msg.get_params_size() > 1) {
      for (int i = 1; i < msg.get_params_size(); ++i) {
        trailing += msg.get_params()[i];
      }
    }
    rpl.push_back(":" + trailing);

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

void Server::cmd_invite(int recv_fd, const Message& msg) {
  // 초대하려는 유저가 권한이 있는지 확인
  User& event_user = (*this)[recv_fd];
  std::cout << msg;
  /*
    > 2024/05/09 21:54:32.000536657  length=15 from=537 to=551
    INVITE dy2 #a\r
    < 2024/05/09 21:54:32.000536960  length=40 from=2359 to=2398
    :dy2!~memememe@localhost 341 dy dy2 #a\r
  */
  std::string incomingClientNickName = msg.get_params()[0];
  std::string targetChannelStr = msg.get_params()[1];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  //////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////
  try {
    //     < 2024/05/13 21:25:18.000445451  length=62 from=1598 to=1659
    // :irc.example.net 401 dy lkkllk :No such nick or channel name\r

    std::map<std::string, User&>::iterator it;
    it = get_channel(get_channel_iterator(targetChannelStr))
             .get_channel_client_list()
             .begin();
    int fd = (*this)[incomingClientNickName];

    Message response341 = Message::rpl_341(serv_name, event_user, msg);
    event_user.push_msg(response341.to_raw_msg());

    User& invitedClient = (*this)[fd];
    // 초대받은 클라이언트에 초대받은채널 벡터 업데이트
    invitedClient.push_invited_channel(targetChannelStr);
    Message rpl;
    rpl.set_source(invitedClient.get_nick_name() + std::string("!~") +
                   invitedClient.get_user_name() + std::string("@localhost"));
    rpl.set_cmd_type(INVITE);
    rpl.push_back(invitedClient.get_nick_name());
    rpl.push_back(msg.get_params()[1]);
    invitedClient.push_msg(rpl.to_raw_msg());
    pollfd* tmp;
    for (int i = 0; i < MAX_USER; i++) {
      if (observe_fd[i].fd == invitedClient.get_user_socket()) {
        tmp = &(observe_fd[i]);
      }
    }
    if ((*this).send_msg_at_queue(invitedClient.get_user_socket()) == -1) {
      tmp->events = POLLIN | POLLOUT;
    } else {
      tmp->events = POLLIN;
    }
  } catch (std::invalid_argument) {
    Message rpl = Message::rpl_401_invitation(
        serv_name, event_user.get_nick_name(), incomingClientNickName);
    event_user.push_msg(rpl.to_raw_msg());
  }
}

void Server::cmd_kick(int recv_fd, const Message& msg) {
  std::string targetChannelStr = msg.get_params()[0];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  // 강퇴할 클라이언트가 속할 채널
  channel_iterator = get_channel_iterator(targetChannelStr);
  if (channel_iterator == channel_list.end()) {
    // ERR_NOSUCHCHANNEL (403)
    User& event_user = (*this)[recv_fd];
    Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return;
  }

  // OPERATOR가 아닐경우 482 RESPONSE
  // checkPrivilege() 라는 함수로 나중에 refactoring 할것
  User& event_user = (*this)[recv_fd];
  if (get_channel(channel_iterator).isOperator(event_user) ==
      false) {
    Message rpl = Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return;
  }

  try {
    int targetFileDescriptor = (*this)[msg.get_params()[1]];
    User& opUser = (*this)[recv_fd];
    User& outUser = (*this)[targetFileDescriptor];
    (*this).kickClient(opUser, outUser,
                       get_channel(channel_iterator), msg);
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
  channel_iterator = channel_list.begin();
  if (channel_iterator == channel_list.end()) return;
  std::cout << CYAN << "=>> Server Channel List :: [";
  std::map<std::string, Channel>::const_iterator cit;
  bool found = false;
  for (cit = channel_list.begin(); cit != channel_list.end();
       cit++) {
    std::string channelName = cit->first;
    std::cout << channelName << ", ";
    found = true;
  }
  if (found == false) return;
  std::cout << "]" << std::endl << std::endl;

  // get_channel(get_channel_iterator(targetChannelStr)).visualizeClientList();
  std::cout << get_channel(
      get_channel_iterator(targetChannelStr));
}

/////////////////////////////////////////////////////////////////////
void Server::cmd_names(int recv_fd, const Message& msg) {
  std::map<std::string, Channel>::const_iterator it;
  std::cout << "[FT_IRC Server] <Channel Status> :: [";

  for (it = channel_list.begin(); it != channel_list.end();
       ++it) {
    const std::string& channelName = it->first;
    std::cout << channelName << "=> ";
  }
  std::cout << "]" << std::endl << std::endl;
}

void Server::cmd_join(int recv_fd, const Message& msg) {
  // [TO DO] :: channel 목록 capacity를 넘으면 더이상 받지 않기 => RFC 에서
  // 어떻게 리스폰스를 주는지 확인해볼것

  // < 2024/06/13 15:29:35.000364884  length=91 from=4905 to=4995
  // :irc.example.net 471 dy____ #test :Cannot join channel (+l) -- Channel is full, try later\r

  try {
    // mode +l 이 켜져있다면의 조건을 추가해야함
    // if (get_current_channel_num() > get_max_channel_num())  // && channel이
    // 새로운 채널인지 확인하고  // 맞다면 에러를 뱉어야함
    //   throw(channel_list_capacity_error());
    std::string targetChannelStr = msg.get_params()[0];
    std::string::size_type pos = targetChannelStr.find('#');
    if (pos != std::string::npos) {
      targetChannelStr.erase(pos, 1);
    }
    channel_iterator = get_channel_iterator(targetChannelStr);

    Message response;
    User& incomingClient = (*this)[recv_fd];

    // 서버에 채널은 등록이 되어있고
    if (channel_iterator != channel_list.end()) {
      // 들어가려는 채널이 초대받은 사람만 갈수 있는 경우
      if (get_channel(get_channel_iterator(targetChannelStr))
              .isMode(FLAG_I)) {
        if ((*this)[recv_fd].isInvited(targetChannelStr)) {
          incomingClient.push_msg(response.to_raw_msg());
          try {
            get_channel(get_channel_iterator(targetChannelStr))
                .addClient((*this)[recv_fd]);
          } catch (const channel_client_capacity_error &e) {
            incomingClient.push_msg(
              Message::rpl_471(
                    serv_name,
                    incomingClient.get_nick_name(),
                    msg.get_params()[0])
                  .to_raw_msg());
            return ;
          }

          // [STEP 1]
          response.set_source(incomingClient.get_nick_name() + std::string("!") +
                              std::string("@localhost"));
          response.set_cmd_type(JOIN);
          response.push_back(msg.get_params()[0]);

          // [STEP 2]
          incomingClient.push_msg(
              Message::rpl_353(
                  serv_name,
                  get_channel(
                      get_channel_iterator(targetChannelStr)),
                  incomingClient.get_nick_name(), msg.get_params()[0])
                  .to_raw_msg());
          incomingClient.push_msg(
              Message::rpl_366(serv_name, incomingClient.get_nick_name(),
                               msg.get_params()[0])
                  .to_raw_msg());

          std::map<std::string, User&> clients =
              get_channel(get_channel_iterator(targetChannelStr))
                  .get_channel_client_list();
          std::map<std::string, User&>::iterator it;

          for (it = clients.begin(); it != clients.end(); ++it) {
            std::string clientNickName = it->second.get_nick_name();
            int fd = it->second.get_user_socket();
            if (clientNickName == (*this)[recv_fd].get_nick_name()) continue;
            (*this)[fd].push_msg(response.to_raw_msg());
            // event기록하기
            pollfd* tmp;
            for (int i = 0; i < MAX_USER; i++) {
              if (observe_fd[i].fd == ((*this)[fd]).get_user_socket()) {
                tmp = &(observe_fd[i]);
              }
            }
            if ((*this).send_msg_at_queue(((*this)[fd]).get_user_socket()) ==
                -1) {
              tmp->events = POLLIN | POLLOUT;
            } else {
              tmp->events = POLLIN;
            }
          }

          // 들어간 이후에 초대장은 소멸
          incomingClient.removeInvitation(targetChannelStr);
          return;
        } else {
          incomingClient.push_msg(
              Message::rpl_473(serv_name, incomingClient.get_nick_name(), msg)
                  .to_raw_msg());
          return;
        }
      } else {
        // 들어가려는 채널이 초대 제한이 없는 경우
        User& incomingClient = (*this)[recv_fd];

        try {
          get_channel(get_channel_iterator(targetChannelStr))
              .addClient((*this)[recv_fd]);
        } catch (const channel_client_capacity_error &e) {
          incomingClient.push_msg(
            Message::rpl_471(
                  serv_name,
                  incomingClient.get_nick_name(),
                  msg.get_params()[0])
                .to_raw_msg());
          return ;
        }

        response.set_source(incomingClient.get_nick_name() + std::string("!") +
                            std::string("@localhost"));
        response.set_cmd_type(JOIN);
        response.push_back(msg.get_params()[0]);
        incomingClient.push_msg(response.to_raw_msg());
        std::map<std::string, User&> clients =
            get_channel(get_channel_iterator(targetChannelStr))
                .get_channel_client_list();
        std::map<std::string, User&>::iterator it;
        for (it = clients.begin(); it != clients.end(); ++it) {
          std::string clientNickName = it->second.get_nick_name();
          int fd = it->second.get_user_socket();
          if (clientNickName == (*this)[recv_fd].get_nick_name()) continue;
          (*this)[fd].push_msg(response.to_raw_msg());
          // event기록하기
          pollfd* tmp;
          for (int i = 0; i < MAX_USER; i++) {
            if (observe_fd[i].fd == ((*this)[fd]).get_user_socket()) {
              tmp = &(observe_fd[i]);
            }
          }
          if ((*this).send_msg_at_queue(((*this)[fd]).get_user_socket()) ==
              -1) {
            tmp->events = POLLIN | POLLOUT;
          } else {
            tmp->events = POLLIN;
          }
        }
        incomingClient.push_msg(
            Message::rpl_353(serv_name,
                             get_channel(
                                 get_channel_iterator(targetChannelStr)),
                             incomingClient.get_nick_name(),
                             msg.get_params()[0])
                .to_raw_msg());
        incomingClient.push_msg(Message::rpl_366(serv_name,
                                                 incomingClient.get_nick_name(),
                                                 msg.get_params()[0])
                                    .to_raw_msg());
        return;
      }
    } else {
      Channel newChannel(targetChannelStr);
      addChannel(newChannel);
      get_channel(get_channel_iterator(targetChannelStr))
          .addClient((*this)[recv_fd]);
      get_channel(get_channel_iterator(targetChannelStr))
          .addOperator((*this)[recv_fd]);
      incomingClient.push_msg(
          Message::rpl_353(
              serv_name,
              get_channel(get_channel_iterator(targetChannelStr)),
              incomingClient.get_nick_name(), msg.get_params()[0])
              .to_raw_msg());
      incomingClient.push_msg(Message::rpl_366(serv_name,
                                               incomingClient.get_nick_name(),
                                               msg.get_params()[0])
                                  .to_raw_msg());
    }
    // [STEP 1] :: JOIN 요청을 수신 후 => 클라이언트와 닉네임 사용자 정보를
    // 나타내줌

    // [STEP 2] :: 이 채널에 몇명의 어떤 클라이언트들이 있는지 반응을 보내줌
    // example => :irc.example.net 353 lfkn___ = #b :lfkn___ lfkn__ lfkn_
    // @lfkn\r for 문으로 map을 순회하면서 닉네임을 만들어줄것
    // incomingClient.push_msg(Message::rpl_353(serv_name,
    // get_channel(get_channel_iterator(targetChannelStr)),
    // incomingClient.get_nick_name(), msg.get_params()[0]) .to_raw_msg());

    // [STEP 3] ::
    // :irc.example.net 366 lfkn___ #b :End of NAMES list\r
    // incomingClient.push_msg(Message::rpl_366(serv_name,
    // incomingClient.get_nick_name(), msg.get_params()[0]).to_raw_msg());
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
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
    if (('0' <= nick_tmp[0] && nick_tmp[0] <= '9') || nick_tmp[0] == ':' ||
        nick_tmp.find_first_of(chantypes + std::string(": \n\t\v\f\r")) !=
            std::string::npos ||
        nick_tmp.length() > NICKLEN) {
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
        (*this).change_nickname(event_user.get_nick_name(), nick_tmp);
        event_user.push_msg(rpl.to_raw_msg());
        // 이제 이 CLIENT는 nickName을 바꾼 사람이므로 초대장은 유효하지 않게
        // 됨. 다시 들어가고 싶으면 다시 초대장을 받아서 해당 채널의 INVITATION
        // 리스트에 이름을 올려야 됨.
        event_user.removeAllInvitations();
        return;
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
    if (user_tmp.length() > USERLEN) {
      user_tmp = user_tmp.substr(0, USERLEN);
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
  // mode +i k l m t 등 서브젝트에서 요구한 구현 요청은 이 단계에서 처리후
  // return ;
  if (msg.get_params()[0][0] == '#') {
    // #################################################################################################
    // #################################################################################################
    // #################################################################################################
    // 전처리 과정
    // 서버에 요청된 채널이 있다면..
    std::string targetChannelStr = msg.get_params()[0];
    std::string::size_type pos = targetChannelStr.find('#');
    if (pos != std::string::npos) {
      targetChannelStr.erase(pos, 1);
    }
    // 해당 채널 찾고
    channel_iterator = get_channel_iterator(targetChannelStr);

    // 없으면 에러 RESPONSE 뱉어주기
    if (channel_iterator == channel_list.end()) {
      // ERR_NOSUCHCHANNEL (403)
      User& event_user = (*this)[recv_fd];
      Message rpl =
          Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
      event_user.push_msg(rpl.to_raw_msg());
      return;
    }

    // ##################################################################################################
    // ##################################### [ /mode +i ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 && msg.get_params()[1] == INVITE_MODE_ON) {
      // [STEP 1]
      // 일단 MODE 설정한 유저에게 응답 보내주기

      // 그런데 그 전에 operator가 아니면 권한이 없다고 말해주기
      if (get_channel(channel_iterator)
              .isOperator((*this)[recv_fd]) == false) {
        User& event_user = (*this)[recv_fd];
        Message rpl =
            Message::rpl_482(serv_name, (*this)[recv_fd].get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      std::string targetChannelStr = msg.get_params()[0];

      // 채널명에서 #없애주기 => 순회하면서 채널명을 찾기위함 (채널리스트에서)
      std::string::size_type pos = targetChannelStr.find('#');
      if (pos != std::string::npos) {
        targetChannelStr.erase(pos, 1);
      }
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (channel_iterator == channel_list.end()) {
        // ERR_NOSUCHCHANNEL (403)
        User& event_user = (*this)[recv_fd];
        Message rpl =
            Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (get_channel(get_channel_iterator(targetChannelStr))
              .isMode(FLAG_I))
        return;

      // 여기서 어떤 모드인지에 따라 RESPONSE 메세지를 처리해줌
      // 즉, 채널 속성값을 여기서 변경함

      get_channel(get_channel_iterator(targetChannelStr))
          .setMode(FLAG_I);

      User& event_user = (*this)[recv_fd];

      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr))
          .setMode(FLAG_I);
      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it =
               get_channel(get_channel_iterator(targetChannelStr))
                   .get_channel_client_list()
                   .begin();
           it !=
           get_channel(get_channel_iterator(targetChannelStr))
               .get_channel_client_list()
               .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
        it->second.push_msg(rpl.to_raw_msg());
        pollfd* tmp;
        for (int i = 0; i < MAX_USER; i++) {
          if (observe_fd[i].fd == (it->second).get_user_socket()) {
            tmp = &(observe_fd[i]);
          }
        }

        // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
        // 여기서는 continue를 해줌
        if (clientNickName == event_user.get_nick_name()) continue;
        if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
          tmp->events = POLLIN | POLLOUT;
        } else {
          tmp->events = POLLIN;
        }
      }
    }

    // ##################################################################################################
    // ##################################### [ /mode -i ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 && msg.get_params()[1] == INVITE_MODE_OFF) {
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (!get_channel(get_channel_iterator(targetChannelStr))
               .isMode(FLAG_I))
        return;

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr))
          .unsetMode(FLAG_I);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it =
               get_channel(get_channel_iterator(targetChannelStr))
                   .get_channel_client_list()
                   .begin();
           it !=
           get_channel(get_channel_iterator(targetChannelStr))
               .get_channel_client_list()
               .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
        it->second.push_msg(rpl.to_raw_msg());
        pollfd* tmp;
        for (int i = 0; i < MAX_USER; i++) {
          if (observe_fd[i].fd == (it->second).get_user_socket()) {
            tmp = &(observe_fd[i]);
          }
        }
        // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
        // 여기서는 continue를 해줌
        if (clientNickName == event_user.get_nick_name()) continue;
        if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
          tmp->events = POLLIN | POLLOUT;
        } else {
          tmp->events = POLLIN;
        }
      }
    }

    // ##################################################################################################
    // ##################################### [ /mode +o ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 && msg.get_params()[1] == OPERATING_MODE_ON) {
      // check for privilege
      if (get_channel(channel_iterator).isOperator(event_user) ==
          false) {
        Message rpl = Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      // check for invalid channel
      std::string targetChannelStr = msg.get_params()[0];
      std::string::size_type pos = targetChannelStr.find('#');
      if (pos != std::string::npos) {
        targetChannelStr.erase(pos, 1);
      }
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (channel_iterator == channel_list.end()) {
        // ERR_NOSUCHCHANNEL (403)
        User& event_user = (*this)[recv_fd];
        Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      // 서버에 등록되어 있지 않은 경우
      try {
        (*this)[msg.get_params()[2]];
      } catch (std::invalid_argument& e) {
        event_user.push_msg(
            Message::rpl_401_mode_operator(serv_name, event_user.get_nick_name(), msg.get_params()[2])
                .to_raw_msg());
        return ;
      }

      // check for invalid nickname => 이 경우는 서버에는 등록되어 있는 유저이지만 채널에 없는 경우임
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (get_channel(channel_iterator).foundClient(msg.get_params()[2]) == false) {
        // < 2024/06/12 16:19:38.000655856  length=65 from=19794 to=19858
        // :irc.example.net 441 dy_ dy__ #zzz :They aren't on that channel\r
        event_user.push_msg(
            Message::rpl_441(serv_name, msg)
                .to_raw_msg());
        return ;
      }

      // assign the target client into the OP list

      if (get_channel(channel_iterator).isOperator(msg.get_params()[2])) {
        return ;
      } else {
        int fd = (*this)[msg.get_params()[2]];
        User& targetClient = (*this)[fd];
        get_channel(channel_iterator).addOperator(targetClient);
      }

              // RESPONSE
              Message rpl;
              rpl.set_source(event_user.get_nick_name() + std::string("!~") + event_user.get_user_name() +
                            std::string("@localhost"));
              rpl.set_cmd_type(MODE);
              rpl.push_back(msg.get_params()[0]);
              rpl.push_back(OPERATING_MODE_ON);
              rpl.push_back(msg.get_params()[2]);

              // BROADCASTING
              std::map<std::string, User&>::iterator it;
              for (it =
                      get_channel(get_channel_iterator(targetChannelStr))
                          .get_channel_client_list()
                          .begin();
                  it !=
                  get_channel(get_channel_iterator(targetChannelStr))
                      .get_channel_client_list()
                      .end();
                  ++it) {
                std::string clientNickName = it->second.get_nick_name();
                it->second.push_msg(rpl.to_raw_msg());
                pollfd* tmp;
                for (int i = 0; i < MAX_USER; i++) {
                  if (observe_fd[i].fd == (it->second).get_user_socket()) {
                    tmp = &(observe_fd[i]);
                  }
                }

                // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
                // 여기서는 continue를 해줌
                if (clientNickName == event_user.get_nick_name())
                  continue;
                if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
                  tmp->events = POLLIN | POLLOUT;
                } else {
                  tmp->events = POLLIN;
                }
              }
    }

    // ##################################################################################################
    // ##################################### [ /mode -o ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 && msg.get_params()[1] == OPERATING_MODE_OFF) {
      // check for privilege
      if (get_channel(channel_iterator).isOperator(event_user) ==
          false) {
        Message rpl = Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      // check for invalid channel
      std::string targetChannelStr = msg.get_params()[0];
      std::string::size_type pos = targetChannelStr.find('#');
      if (pos != std::string::npos) {
        targetChannelStr.erase(pos, 1);
      }
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (channel_iterator == channel_list.end()) {
        // ERR_NOSUCHCHANNEL (403)
        User& event_user = (*this)[recv_fd];
        Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_msg(rpl.to_raw_msg());
        return;
      }

      // 서버에 등록되어 있지 않은 경우
      try {
        (*this)[msg.get_params()[2]];
      } catch (std::invalid_argument& e) {
        event_user.push_msg(
            Message::rpl_401_mode_operator(serv_name, event_user.get_nick_name(), msg.get_params()[2])
                .to_raw_msg());
        return ;
      }

      // check for invalid nickname => 이 경우는 서버에는 등록되어 있는 유저이지만 채널에 그 유저가 없는 경우
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (get_channel(channel_iterator).foundClient(msg.get_params()[2]) == false) {
        // < 2024/06/12 16:19:38.000655856  length=65 from=19794 to=19858
        // :irc.example.net 441 dy_ dy__ #zzz :They aren't on that channel\r
        event_user.push_msg(
            Message::rpl_441(serv_name, msg)
                .to_raw_msg());
        return ;
      }

      // remove the target client from the OP list
      if (get_channel(channel_iterator).isOperator(msg.get_params()[2]) == false) {
        return ;
      } else {
        int fd = (*this)[msg.get_params()[2]];
        User& targetClient = (*this)[fd];
        get_channel(channel_iterator).removeOperator(targetClient);
      }

              // RESPONSE
              Message rpl;
              rpl.set_source(event_user.get_nick_name() + std::string("!~") + event_user.get_user_name() +
                            std::string("@localhost"));
              rpl.set_cmd_type(MODE);
              rpl.push_back(msg.get_params()[0]);
              rpl.push_back(OPERATING_MODE_OFF);
              rpl.push_back(msg.get_params()[2]);

              // BROADCASTING
              std::map<std::string, User&>::iterator it;
              for (it =
                      get_channel(get_channel_iterator(targetChannelStr))
                          .get_channel_client_list()
                          .begin();
                  it !=
                  get_channel(get_channel_iterator(targetChannelStr))
                      .get_channel_client_list()
                      .end();
                  ++it) {
                std::string clientNickName = it->second.get_nick_name();
                it->second.push_msg(rpl.to_raw_msg());
                pollfd* tmp;
                for (int i = 0; i < MAX_USER; i++) {
                  if (observe_fd[i].fd == (it->second).get_user_socket()) {
                    tmp = &(observe_fd[i]);
                  }
                }

                // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
                // 여기서는 continue를 해줌
                if (clientNickName == event_user.get_nick_name())
                  continue;
                if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
                  tmp->events = POLLIN | POLLOUT;
                } else {
                  tmp->events = POLLIN;
                }
              }
    }

    // ##################################################################################################
    // ##################################### [ /mode +l ]
    // ###############################################
    // ##################################################################################################
    // default -> limit number 65535
    if (msg.get_params_size() > 2 && msg.get_params()[1] == LIMIT_ON) {
      /*
        > 2024/06/12 21:25:16.000268737  length=21 from=416 to=436
        MODE #test +l 12111\r
        < 2024/06/12 21:25:16.000269041  length=45 from=2307 to=2351
        :dy!~memememe@localhost MODE #test +l 12111\r 
      */

      if (atoi(msg.get_params()[2].c_str()) == 0) {
        return ;
      }
      get_channel(get_channel_iterator(targetChannelStr)).setMode(FLAG_L);
      get_channel(get_channel_iterator(targetChannelStr)).setLimit(msg.get_params()[2]);

              // > 2024/06/12 22:44:00.000649604  length=18 from=3035 to=3052
              // MODE #test +l 22\r
              // < 2024/06/12 22:44:00.000649893  length=42 from=18211 to=18252
              // :dy!~memememe@localhost MODE #test +l 22\r

              // RESPONSE
              Message rpl;
              rpl.set_source(event_user.get_nick_name() + std::string("!~") + event_user.get_user_name() +
                            std::string("@localhost"));
              rpl.set_cmd_type(MODE);
              rpl.push_back(msg.get_params()[0]);
              rpl.push_back(LIMIT_ON);
              rpl.push_back(msg.get_params()[2]);

              // BROADCASTING
              std::map<std::string, User&>::iterator it;
              for (it =
                      get_channel(get_channel_iterator(targetChannelStr))
                          .get_channel_client_list()
                          .begin();
                  it !=
                  get_channel(get_channel_iterator(targetChannelStr))
                      .get_channel_client_list()
                      .end();
                  ++it) {
                std::string clientNickName = it->second.get_nick_name();
                it->second.push_msg(rpl.to_raw_msg());
                pollfd* tmp;
                for (int i = 0; i < MAX_USER; i++) {
                  if (observe_fd[i].fd == (it->second).get_user_socket()) {
                    tmp = &(observe_fd[i]);
                  }
                }

                // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
                // 여기서는 continue를 해줌
                if (clientNickName == event_user.get_nick_name())
                  continue;
                if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
                  tmp->events = POLLIN | POLLOUT;
                } else {
                  tmp->events = POLLIN;
                }
              }

    } else if (msg.get_params_size() > 1 && msg.starts_with(msg.get_params()[1], LIMIT_OFF) == true) {
      get_channel(get_channel_iterator(targetChannelStr)).unsetMode(FLAG_L);
      get_channel(get_channel_iterator(targetChannelStr)).setLimit(INIT_CLIENT_LIMIT);

      // > 2024/06/12 22:44:04.000925650  length=15 from=3053 to=3067
      // MODE #test -l\r
      // < 2024/06/12 22:44:04.000925974  length=39 from=18253 to=18291
      // :dy!~memememe@localhost MODE #test -l\r

      /* 

        -l123123 이런 경우 k l i s t 가 아닌것들은 
        :irc.example.net 472 dy 1 :is unknown mode char for #test\r
        :irc.example.net 472 dy 2 :is unknown mode char for #test\r
        :irc.example.net 472 dy 3 :is unknown mode char for #test\r
      */

      std::string::const_iterator iterator = msg.get_params()[1].begin() + 2;
      for (; iterator != msg.get_params()[1].end(); ++iterator) {
        std::cout << *iterator << std::endl;
        char chanmodes[5] = { 'k', 'l', 'i', 's', 't' };
        if (std::find(chanmodes, chanmodes + 5, *iterator) == chanmodes + 5) {
          // :irc.example.net 472 dy 1 :is unknown mode char for #test\r
          Message rpl;
          event_user.push_msg(Message::rpl_472(serv_name, event_user.get_nick_name(), *iterator, msg)
                                    .to_raw_msg());
        }
      }

      // RESPONSE
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") + event_user.get_user_name() +
                    std::string("@localhost"));
      rpl.set_cmd_type(MODE);
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(LIMIT_OFF);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it =
              get_channel(get_channel_iterator(targetChannelStr))
                  .get_channel_client_list()
                  .begin();
          it !=
          get_channel(get_channel_iterator(targetChannelStr))
              .get_channel_client_list()
              .end();
          ++it) {
        std::string clientNickName = it->second.get_nick_name();
        it->second.push_msg(rpl.to_raw_msg());
        pollfd* tmp;
        for (int i = 0; i < MAX_USER; i++) {
          if (observe_fd[i].fd == (it->second).get_user_socket()) {
            tmp = &(observe_fd[i]);
          }
        }

        // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
        // 여기서는 continue를 해줌
        if (clientNickName == event_user.get_nick_name())
          continue;
        if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
          tmp->events = POLLIN | POLLOUT;
        } else {
          tmp->events = POLLIN;
        }
      }
    }
  // rpl.set_source(event_user.get_nick_name() + std::string("!") +
  //                event_user.get_user_name() + std::string("@localhost"));
  // rpl.set_cmd_type(MODE);
  // rpl.push_back(event_user.get_nick_name());
  // rpl.push_back(":" + msg[0]);
  // event_user.push_msg(rpl.to_raw_msg());
 }
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

    if (get_channel(get_channel_iterator(targetChannelStr))
            .foundClient(sourceNickName) == false) {
      return;
    }
    std::map<std::string, User&> map =
        get_channel(get_channel_iterator(targetChannelStr))
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
        Message::rpl_401(serv_name, source_user.get_nick_name(), msg)
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

void Server::cmd_part(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  Message rpl;

  std::string targetChannelStr = msg.get_params()[0];
  std::string::size_type pos = targetChannelStr.find('#');
  if (pos != std::string::npos) {
    targetChannelStr.erase(pos, 1);
  }
  channel_iterator = get_channel_iterator(targetChannelStr);
  if (channel_iterator == channel_list.end()) {
    Message rpl = Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
    event_user.push_msg(rpl.to_raw_msg());
    return ;
  }

  rpl.set_source(event_user.get_nick_name() + std::string("!~") + event_user.get_user_name() + std::string("@localhost"));
  rpl.set_cmd_type(PART);
  rpl.push_back(msg.get_params()[0] + std::string(" :"));
  if (msg.get_params_size() > 1)
    rpl.push_back(msg.get_params().back());
  // BROADCASTING
  get_channel(get_channel_iterator(targetChannelStr)).removeClient(event_user);
  get_channel(get_channel_iterator(targetChannelStr)).removeOperator(event_user);
  if (get_channel(get_channel_iterator(targetChannelStr)).isEmptyChannel() == true) {
    (*this).removeChannel(targetChannelStr);
    event_user.push_msg(rpl.to_raw_msg());
    return ;
  }
  event_user.push_msg(rpl.to_raw_msg());
  std::map<std::string, User&>::iterator it;
  for (it =
            get_channel(get_channel_iterator(targetChannelStr))
                .get_channel_client_list()
                .begin();
        it !=
        get_channel(get_channel_iterator(targetChannelStr))
            .get_channel_client_list()
            .end();
        ++it) {
    std::string clientNickName = it->second.get_nick_name();
    it->second.push_msg(rpl.to_raw_msg());
    pollfd* tmp;
    for (int i = 0; i < MAX_USER; i++) {
      if (observe_fd[i].fd == (it->second).get_user_socket()) {
        tmp = &(observe_fd[i]);
      }
    }
    // broadcasting 하는건데 event_user는 윗단에서 poll처리를 해줌으로
    // 여기서는 continue를 해줌
    if (clientNickName == event_user.get_nick_name()) {
      continue;
    }
    if ((*this).send_msg_at_queue((it->second).get_user_socket()) == -1) {
      tmp->events = POLLIN | POLLOUT;
    } else {
      tmp->events = POLLIN;
    }
  }
}
