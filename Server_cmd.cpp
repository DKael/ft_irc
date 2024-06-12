#include "Server.hpp"

void Server::cmd_topic(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk
  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  // channel exist chk
  const String& chan_name = msg[0];
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        Message::rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // invitor on channel chk
  Channel& tmp_chan = chan_it->second;
  std::map<String, User&>::iterator chan_client_it =
      tmp_chan.get_client_list().find(event_user_nick);
  if (chan_client_it == tmp_chan.get_client_list().end()) {
    event_user.push_back_msg(
        Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  const String& topic = tmp_chan.get_topic();
  if (msg.get_params_size() == 1) {
    //check the topic

    // topic set chk
    if (topic.length() == 0) {
      event_user.push_back_msg(
          Message::rpl_331(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    } else {
      event_user.push_back_msg(
          Message::rpl_332(serv_name, event_user_nick, chan_name, topic)
              .to_raw_msg());
      event_user.push_back_msg(
          Message::rpl_333(serv_name, event_user_nick, chan_name,
                           tmp_chan.get_topic_set_nick(),
                           ft_ltos(tmp_chan.get_topic_set_time()))
              .to_raw_msg());
      return;
    }
  } else {
    // set the topic

    // if channel mode set to +t
    // client privilege chk
    if (tmp_chan.chk_mode(CHAN_FLAG_T) == true &&
        tmp_chan.is_operator(event_user_nick) == false) {
      event_user.push_back_msg(
          Message::rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    /*
    < 2024/05/11 15:09:25.000532736  length=45 from=4305 to=4349
    :dy!~memememe@localhost TOPIC #test :welfkn\r
    */
    const String& new_topic = msg[1];
    Message rpl;

    rpl.set_source(event_user.make_source(1));
    rpl.set_cmd_type(TOPIC);
    rpl.push_back(chan_name);
    rpl.push_back(":" + new_topic);

    chan_client_it = tmp_chan.get_client_list().begin();
    for (; chan_client_it != tmp_chan.get_client_list().end();
         ++chan_client_it) {
      chan_client_it->second.push_back_msg(rpl.to_raw_msg());
      ft_send(chan_client_it->second.get_pfd());
    }
  }
}

void Server::cmd_invite(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk
  if (msg.get_params_size() < 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  /*
    > 2024/05/09 21:54:32.000536657
    INVITE dy2 #a\r
    < 2024/05/09 21:54:32.000536960
    :dy2!~memememe@localhost 341 dy dy2 #a\r
  */
  const String& invited_user_nick = msg[0];
  const String& chan_name = msg[1];

  // 유저 존재하는지 확인
  std::map<String, int>::iterator user_it = nick_to_soc.find(invited_user_nick);
  if (user_it == nick_to_soc.end()) {
    // :irc.example.net 401 dy lkkllk :No such nick or channel name\r
    event_user.push_back_msg(
        Message::rpl_401(serv_name, event_user_nick, invited_user_nick)
            .to_raw_msg());
    return;
  }

  // 채널 존재하는지 확인
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        Message::rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  User& invited_user = (*this)[user_it->second];
  Channel& tmp_chan = chan_it->second;

  // 초대자가 채널에 들어가 있는지 확인
  std::map<String, User&>::iterator chan_client_it =
      tmp_chan.get_client_list().find(event_user_nick);
  if (chan_client_it == tmp_chan.get_client_list().end()) {
    event_user.push_back_msg(
        Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대자가 권한이 있는지 확인
  if (tmp_chan.is_operator(event_user_nick) == false) {
    event_user.push_back_msg(
        Message::rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대받은자가 이미 채널에 있는지 확인
  chan_client_it = tmp_chan.get_client_list().find(invited_user_nick);
  if (chan_client_it != tmp_chan.get_client_list().end()) {
    event_user.push_back_msg(Message::rpl_443(serv_name, event_user_nick,
                                              invited_user_nick, chan_name)
                                 .to_raw_msg());
    return;
  }

  event_user.push_back_msg(Message::rpl_341(invited_user.make_source(1),
                                            event_user_nick, invited_user_nick,
                                            chan_name)
                               .to_raw_msg());
  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(INVITE);
  rpl.push_back(invited_user_nick);
  rpl.push_back(chan_name);
  invited_user.push_back_msg(rpl.to_raw_msg());

  ft_send(event_user.get_pfd());
  ft_send(invited_user.get_pfd());
}

void Server::cmd_kick(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk
  if (msg.get_params_size() < 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  const String& chan_name = msg[0];
  // 채널 존재하는지 확인
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        Message::rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  Channel& chan = chan_it->second;
  // 추방하는 유저가 채널에 들어가 있는지 확인
  std::map<String, User&>::iterator chan_client_it =
      chan.get_client_list().find(event_user_nick);
  if (chan_client_it == chan.get_client_list().end()) {
    event_user.push_back_msg(
        Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 추방하는 유저가 권한이 있는지 확인
  if (chan.is_operator(event_user_nick) == false) {
    event_user.push_back_msg(
        Message::rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 추방할 이름이 여러 개 들어올 수 있음. ','를 기준으로 구분.
  std::vector<String> name_vec;
  ft_split(msg[1], ",", name_vec);

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(KICK);
  rpl.push_back(event_user_nick);
  rpl.push_back("");
  if (msg.get_params_size() >= 3) {
    rpl.push_back(":" + msg[2]);
  }

  for (size_t i = 0; i < name_vec.size(); ++i) {
    // 유저 존재하는지 확인
    std::map<String, int>::iterator user_it = nick_to_soc.find(name_vec[i]);
    if (user_it == nick_to_soc.end()) {
      event_user.push_back_msg(
          Message::rpl_401(serv_name, event_user_nick, name_vec[i])
              .to_raw_msg());
      continue;
    }

    // 추방당할 자가 채널에 있는지 확인
    chan_client_it = chan.get_client_list().find(name_vec[i]);
    if (chan_client_it == chan.get_client_list().end()) {
      /*< 2024/06/12 19:40:21.000676392
        :irc.example.net 441 test jjjj #chan :They aren't on that channel\r*/
      event_user.push_back_msg(
          Message::rpl_441(serv_name, event_user_nick, name_vec[i], chan_name)
              .to_raw_msg());
      continue;
    }

    rpl[1] = name_vec[i];
    chan_client_it = chan.get_client_list().begin();
    for (; chan_client_it != chan.get_client_list().end(); ++chan_client_it) {
      chan_client_it->second.push_back_msg(rpl.to_raw_msg());
      ft_send(chan_client_it->second.get_pfd());
    }
    chan.remove_client(name_vec[i]);
  }
}

void Server::cmd_who(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk
  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  // 채널인지 유저인지 확인
  if (msg[0][0] == REGULAR_CHANNEL || msg[0][0] == LOCAL_CHANNEL) {
    const String& chan_name = msg[0];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it != channel_list.end()) {
      Channel& tmp_chan = chan_it->second;
      std::map<String, User&>::iterator chan_client_it =
          tmp_chan.get_client_list().begin();
      for (; chan_client_it != tmp_chan.get_client_list().end();
           ++chan_client_it) {
        if (tmp_chan.is_operator(chan_client_it->first) == true) {
          event_user.push_back_msg(
              Message::rpl_352(serv_name, event_user_nick, chan_name,
                               chan_client_it->second, serv_name, "H@", 0)
                  .to_raw_msg());
        } else {
          event_user.push_back_msg(
              Message::rpl_352(serv_name, event_user_nick, chan_name,
                               chan_client_it->second, serv_name, "H", 0)
                  .to_raw_msg());
        }
      }
    }
  } else {
    const String& user_name = msg[0];
    std::map<String, int>::iterator user_it = nick_to_soc.find(user_name);
    if (user_it != nick_to_soc.end()) {
      event_user.push_back_msg(Message::rpl_352(serv_name, event_user_nick, "*",
                                                (*this)[user_it->second],
                                                serv_name, "H", 0)
                                   .to_raw_msg());
    }
  }
  event_user.push_back_msg(
      Message::rpl_315(serv_name, event_user_nick, msg[0]).to_raw_msg());
  ft_send(event_user.get_pfd());
}

void Server::cmd_pass(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  String pass_tmp;

  if (event_user.get_password_chk() == NOT_YET) {
    if (msg.get_params_size() < 1) {
      event_user.push_back_msg(Message::rpl_461(serv_name,
                                                event_user.get_nick_name(),
                                                msg.get_raw_cmd())
                                   .to_raw_msg());
      return;
    }
    pass_tmp = msg[0];
    if (password == pass_tmp) {
      event_user.set_password_chk(OK);
    } else {
      event_user.set_password_chk(FAIL);
    }
    return;
  } else {
    event_user.push_back_msg(
        Message::rpl_462(serv_name, event_user.get_nick_name()).to_raw_msg());
    return;
  }
}

void Server::cmd_nick(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();
  String new_nick;

  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  } else {
    new_nick = msg[0];
    if (('0' <= new_nick[0] && new_nick[0] <= '9') ||
        new_nick.find_first_of(CHANTYPES + String(": \n\t\v\f\r")) !=
            String::npos ||
        new_nick.length() > NICKLEN) {
      event_user.push_back_msg(
          Message::rpl_432(serv_name, event_user_nick, new_nick).to_raw_msg());
      return;
    }
    try {
      (*this)[new_nick];
      event_user.push_back_msg(
          Message::rpl_433(serv_name, event_user_nick, new_nick).to_raw_msg());
      return;
    } catch (std::invalid_argument& e) {
      if (event_user.get_is_authenticated() == OK) {
        String old_nick = event_user_nick;
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.set_cmd_type(NICK);
        rpl.push_back(":" + new_nick);
        (*this).change_nickname(old_nick, new_nick);
        event_user.remove_all_invitations();

        const std::map<String, int>& user_chan = event_user.get_channels();
        std::map<String, int>::const_iterator it;
        for (it = user_chan.begin(); it != user_chan.end(); ++it) {
          channel_list[it->first].change_client_nickname(old_nick, new_nick);
        }

        std::map<int, User>::iterator user_it;
        for (user_it = user_list.begin(); user_it != user_list.end();
             ++user_it) {
          user_it->second.push_back_msg(rpl.to_raw_msg());
          ft_send(user_it->second.get_pfd());
        }
        return;
      } else {
        (*this).change_nickname(event_user_nick, new_nick);
        event_user.set_nick_init_chk(OK);
      }
    }
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

void Server::cmd_user(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  String user_tmp;

  if (msg.get_params_size() < 4) {
    event_user.push_back_msg(Message::rpl_461(serv_name,
                                              event_user.get_nick_name(),
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
    event_user.set_user_name(user_tmp);
    event_user.set_real_name(msg[3]);
    event_user.set_user_init_chk(OK);
  } else {
    if (event_user.get_is_authenticated() == OK) {
      Message rpl = Message::rpl_462(serv_name, event_user.get_nick_name());
      event_user.push_back_msg(rpl.to_raw_msg());
      return;
    } else {
      Message rpl = Message::rpl_451(serv_name, event_user.get_nick_name());
      event_user.push_back_msg(rpl.to_raw_msg());
      return;
    }
  }
}

void Server::cmd_ping(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];

  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        Message::rpl_409(serv_name, event_user.get_nick_name()).to_raw_msg());
    return;
  }

  Message rpl;

  rpl.set_source(serv_name);
  rpl.set_cmd_type(PONG);
  rpl.push_back(serv_name);
  rpl.push_back(":" + serv_name);
  event_user.push_back_msg(rpl.to_raw_msg());
}

void Server::cmd_pong(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];

  event_user.set_last_ping(std::time(NULL));
}

void Server::cmd_quit(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  pollfd& tmp_pfd = event_user.get_pfd();
  Message rpl;

  rpl.set_cmd_type(ERROR);
  rpl.push_back(":Closing connection");
  event_user.set_have_to_disconnect(true);
  event_user.push_back_msg(rpl.to_raw_msg());
  std::clog << "Connection close at " << recv_fd << '\n';
  if ((*this).send_msg_at_queue(event_user.get_user_socket()) == -1) {
    tmp_pfd.events = POLLOUT;
  } else {
    connection_fin(tmp_pfd);
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
NAMES #chan_a,#chan_b
:irc.example.net 353 sss = #chan_a :sss kkk ccc @test
:irc.example.net 366 sss #chan_a :End of NAMES list
:irc.example.net 353 sss = #chan_b :@jjjj
:irc.example.net 366 sss #chan_b :End of NAMES list

NAMES
:irc.example.net 353 sss = #chan_b :@jjjj
:irc.example.net 353 sss = #chan_a :ccc @test
:irc.example.net 353 sss = #kick_test :ccc test
:irc.example.net 353 sss = #test :@ccc
:irc.example.net 353 sss = #chan :ccc @test
:irc.example.net 353 sss * * :sss
:irc.example.net 366 sss * :End of NAMES list

NAMES
:irc.example.net 353 sss = #chan_b :@jjjj
:irc.example.net 353 sss = #chan_a :sss kkk ccc @test
:irc.example.net 353 sss = #kick_test :ccc test
:irc.example.net 353 sss = #test :@ccc
:irc.example.net 353 sss = #chan :ccc @test
:irc.example.net 366 sss * :End of NAMES list
*/

void Server::cmd_names(int recv_fd, const Message& msg) {
  std::map<String, Channel>::const_iterator it;
  std::cout << "[FT_IRC Server] <Channel Status> :: [";

  for (it = channel_list.begin(); it != channel_list.end(); ++it) {
    const String& chan_name = it->first;
    std::cout << chan_name << "=> ";
  }
  std::cout << "]" << std::endl << std::endl;
}

void Server::cmd_privmsg(int recv_fd, const Message& msg) {
  User& source_user = (*this)[recv_fd];

  String sourceNickName = source_user.get_nick_name();
  String sourceUserName = source_user.get_user_name();
  String targetNickName = msg.get_params().front();
  // targetNickName 의 첫번째 글자가 '#'일 경우 => 채널간 client 들과의 소통
  if (targetNickName[0] == '#') {
    // 채널에 속한 유저들 fd에 message다 적어서 쏴주기
    String targetChannelStr = msg.get_params()[0];
    String::size_type pos = targetChannelStr.find('#');
    if (pos != String::npos) {
      targetChannelStr.erase(pos, 1);
    }

    if (get_channel(get_channel_iterator(targetChannelStr))
            .found_client(sourceNickName) == false) {
      return;
    }
    std::map<String, User&> map =
        get_channel(get_channel_iterator(targetChannelStr)).get_client_list();
    std::map<String, User&>::iterator it;
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

      rpl.set_source(sourceNickName + String("!~") + sourceUserName +
                     String("@localhost"));
      rpl.set_cmd_type(PRIVMSG);
      rpl.push_back(msg.get_params()[0]);
      String string;
      for (int i = 1; i < msg.get_params_size(); ++i) {
        string += msg.get_params()[i];
        // rpl.push_back(msg.get_params()[i]);
      }
      rpl.push_back(string);
      target_user.push_back_msg(rpl.to_raw_msg());

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
    source_user.push_back_msg(
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
  rpl.set_source(sourceNickName + String("!") + String("~") + sourceUserName +
                 String("@localhost"));
  rpl.set_cmd_type(PRIVMSG);

  int i = 0;
  for (i = 0; i < msg.get_params_size() - 1; ++i) {
    rpl.push_back(msg.get_params()[i]);
  }
  rpl.push_back(String(":") + msg[i]);
  std::cout << YELLOW << rpl.to_raw_msg() << std::endl;
  target_user.push_back_msg(rpl.to_raw_msg());
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

void Server::cmd_join(int recv_fd, const Message& msg) {
  // [TO DO] :: channel 목록 capacity를 넘으면 더이상 받지 않기 => RFC 에서
  // 어떻게 리스폰스를 주는지 확인해볼것

  try {
    // mode +l 이 켜져있다면의 조건을 추가해야함
    // if (get_current_channel_num() > get_max_channel_num())  // && channel이
    // 새로운 채널인지 확인하고  // 맞다면 에러를 뱉어야함
    //   throw(channel_list_capacity_error());
    String targetChannelStr = msg.get_params()[0];
    String::size_type pos = targetChannelStr.find('#');
    if (pos != String::npos) {
      targetChannelStr.erase(pos, 1);
    }
    channel_iterator = get_channel_iterator(targetChannelStr);

    Message response;
    User& incomingClient = (*this)[recv_fd];
    response.set_source(incomingClient.get_nick_name() + String("!") +
                        String("@localhost"));
    response.set_cmd_type(JOIN);
    response.push_back(msg.get_params()[0]);

    // 서버에 채널은 등록이 되어있고
    if (channel_iterator != channel_list.end()) {
      // 들어가려는 채널이 초대받은 사람만 갈수 있는 경우
      if (get_channel(get_channel_iterator(targetChannelStr))
              .isMode(CHAN_FLAG_I)) {
        if ((*this)[recv_fd].isInvited(targetChannelStr)) {
          incomingClient.push_back_msg(response.to_raw_msg());

          get_channel(get_channel_iterator(targetChannelStr))
              .addClient((*this)[recv_fd]);
          incomingClient.push_back_msg(
              Message::rpl_353(
                  serv_name,
                  get_channel(get_channel_iterator(targetChannelStr)),
                  incomingClient.get_nick_name(), msg.get_params()[0])
                  .to_raw_msg());
          incomingClient.push_back_msg(
              Message::rpl_366(serv_name, incomingClient.get_nick_name(),
                               msg.get_params()[0])
                  .to_raw_msg());

          std::map<String, User&> clients =
              get_channel(get_channel_iterator(targetChannelStr))
                  .get_channel_client_list();
          std::map<String, User&>::iterator it;

          for (it = clients.begin(); it != clients.end(); ++it) {
            String clientNickName = it->second.get_nick_name();
            int fd = it->second.get_user_socket();
            if (clientNickName == (*this)[recv_fd].get_nick_name()) continue;
            (*this)[fd].push_back_msg(response.to_raw_msg());
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
          incomingClient.push_back_msg(
              Message::rpl_473(serv_name, incomingClient.get_nick_name(), msg)
                  .to_raw_msg());
          return;
        }
      } else {
        // 들어가려는 채널이 초대 제한이 없는 경우
        Message response;
        User& incomingClient = (*this)[recv_fd];
        response.set_source(incomingClient.get_nick_name() + String("!") +
                            String("@localhost"));
        response.set_cmd_type(JOIN);
        response.push_back(msg.get_params()[0]);
        incomingClient.push_back_msg(response.to_raw_msg());

        get_channel(get_channel_iterator(targetChannelStr))
            .addClient((*this)[recv_fd]);

        std::map<String, User&> clients =
            get_channel(get_channel_iterator(targetChannelStr))
                .get_channel_client_list();
        std::map<String, User&>::iterator it;
        for (it = clients.begin(); it != clients.end(); ++it) {
          String clientNickName = it->second.get_nick_name();
          int fd = it->second.get_user_socket();
          if (clientNickName == (*this)[recv_fd].get_nick_name()) continue;
          (*this)[fd].push_back_msg(response.to_raw_msg());
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
        incomingClient.push_back_msg(
            Message::rpl_353(
                serv_name, get_channel(get_channel_iterator(targetChannelStr)),
                incomingClient.get_nick_name(), msg.get_params()[0])
                .to_raw_msg());
        incomingClient.push_back_msg(
            Message::rpl_366(serv_name, incomingClient.get_nick_name(),
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
      incomingClient.push_back_msg(
          Message::rpl_353(serv_name,
                           get_channel(get_channel_iterator(targetChannelStr)),
                           incomingClient.get_nick_name(), msg.get_params()[0])
              .to_raw_msg());
      incomingClient.push_back_msg(
          Message::rpl_366(serv_name, incomingClient.get_nick_name(),
                           msg.get_params()[0])
              .to_raw_msg());
    }
    // [STEP 1] :: JOIN 요청을 수신 후 => 클라이언트와 닉네임 사용자 정보를
    // 나타내줌

    // [STEP 2] :: 이 채널에 몇명의 어떤 클라이언트들이 있는지 반응을 보내줌
    // example => :irc.example.net 353 lfkn___ = #b :lfkn___ lfkn__ lfkn_
    // @lfkn\r for 문으로 map을 순회하면서 닉네임을 만들어줄것
    // incomingClient.push_back_msg(Message::rpl_353(serv_name,
    // get_channel(get_channel_iterator(targetChannelStr)),
    // incomingClient.get_nick_name(), msg.get_params()[0]) .to_raw_msg());

    // [STEP 3] ::
    // :irc.example.net 366 lfkn___ #b :End of NAMES list\r
    // incomingClient.push_back_msg(Message::rpl_366(serv_name,
    // incomingClient.get_nick_name(), msg.get_params()[0]).to_raw_msg());
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
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
    String targetChannelStr = msg.get_params()[0];
    String::size_type pos = targetChannelStr.find('#');
    if (pos != String::npos) {
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
      event_user.push_back_msg(rpl.to_raw_msg());
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
              .is_operator((*this)[re.get_nick_name() cv_fd]) == false) {
        User& event_user = (*this)[recv_fd];
        Message rpl =
            Message::rpl_482(serv_name, (*this)[recv_fd].get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
        return;
      }

      String targetChannelStr = msg.get_params()[0];

      // 채널명에서 #없애주기 => 순회하면서 채널명을 찾기위함 (채널리스트에서)
      String::size_type pos = targetChannelStr.find('#');
      if (pos != String::npos) {
        targetChannelStr.erase(pos, 1);
      }
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (channel_iterator == channel_list.end()) {
        // ERR_NOSUCHCHANNEL (403)
        User& event_user = (*this)[recv_fd];
        Message rpl =
            Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
        return;
      }

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (get_channel(get_channel_iterator(targetChannelStr))
              .chk_mode(CHAN_FLAG_I))
        return;

      // 여기서 어떤 모드인지에 따라 RESPONSE 메세지를 처리해줌
      // 즉, 채널 속성값을 여기서 변경함

      get_channel(get_channel_iterator(targetChannelStr)).set_mode(CHAN_FLAG_I);

      User& event_user = (*this)[recv_fd];

      Message rpl;
      rpl.set_source(event_user.get_nick_name() + String("!~") +
                     event_user.get_user_name() + String("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr)).set_mode(CHAN_FLAG_I);
      // BROADCASTING
      std::map<String, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_client_list()
                     .end();
           ++it) {
        String clientNickName = it->second.get_nick_name();
        it->second.push_back_msg(rpl.to_raw_msg());
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
      rpl.set_source(event_user.get_nick_name() + String("!~") +
                     event_user.get_user_name() + String("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (!get_channel(get_channel_iterator(targetChannelStr))
               .chk_mode(CHAN_FLAG_I))
        return;

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr))
          .unset_mode(CHAN_FLAG_I);

      // BROADCASTING
      std::map<String, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_client_list()
                     .end();
           ++it) {
        String clientNickName = it->second.get_nick_name();
        it->second.push_back_msg(rpl.to_raw_msg());
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
    return;
  }
  rpl.set_source(event_user.get_nick_name() + String("!") +
                 event_user.get_user_name() + String("@localhost"));
  rpl.set_cmd_type(MODE);
  rpl.push_back(event_user.get_nick_name());
  rpl.push_back(":" + msg[0]);
  event_user.push_back_msg(rpl.to_raw_msg());
}
