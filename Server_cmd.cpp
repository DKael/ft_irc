#include "Server.hpp"

void Server::cmd_topic(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 1 ~ 2개가 정상적.
  if (msg.get_params_size() < 1 || msg.get_params_size() > 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  // 채널 존재하는지 확인
  const String& chan_name = msg[0];
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        Message::rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 메세지 날린 유저가 채널에 존재하는지 확인
  Channel& chan = chan_it->second;
  if (chan.chk_user_join(event_user_nick) == false) {
    event_user.push_back_msg(
        Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  const String& topic = chan.get_topic();
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
                           chan.get_topic_set_nick(),
                           ft_ltos(chan.get_topic_set_time()))
              .to_raw_msg());
      return;
    }
  } else {
    // set the topic

    // if channel mode set to +t
    // user privilege chk
    if (chan.chk_mode(CHAN_FLAG_T) == true &&
        chan.is_operator(event_user_nick) == false) {
      event_user.push_back_msg(
          Message::rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    /*
    < 2024/05/11 15:09:25.000532736  length=45 from=4305 to=4349
    :dy!~memememe@localhost TOPIC #test :welfkn\r
    */
    const String& new_topic = msg[1];

    chan.set_topic(new_topic);

    Message rpl;

    rpl.set_source(event_user.make_source(1));
    rpl.set_cmd_type(TOPIC);
    rpl.push_back(chan_name);
    rpl.push_back(":" + new_topic);
    send_msg_to_channel(chan, rpl.to_raw_msg());
  }
}

void Server::cmd_invite(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 2개가 정상적.
  if (msg.get_params_size() != 2) {
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
  Channel& chan = chan_it->second;

  // 초대자가 채널에 들어가 있는지 확인
  if (chan.chk_user_join(event_user_nick) == false) {
    event_user.push_back_msg(
        Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대자가 권한이 있는지 확인
  if (chan.is_operator(event_user_nick) == false) {
    event_user.push_back_msg(
        Message::rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대받은자가 이미 채널에 있는지 확인
  if (chan.chk_user_join(invited_user_nick) == true) {
    event_user.push_back_msg(Message::rpl_443(serv_name, event_user_nick,
                                              invited_user_nick, chan_name)
                                 .to_raw_msg());
    return;
  }

  invited_user.push_invitation(chan.get_channel_name());

  event_user.push_back_msg(Message::rpl_341(invited_user.make_source(1),
                                            event_user_nick, invited_user_nick,
                                            chan_name)
                               .to_raw_msg());
  ft_send(event_user.get_pfd());

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(INVITE);
  rpl.push_back(invited_user_nick);
  rpl.push_back(chan_name);
  invited_user.push_back_msg(rpl.to_raw_msg());
  ft_send(invited_user.get_pfd());
}

void Server::cmd_kick(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 2 ~3 개가 정상적
  if (msg.get_params_size() < 2 || msg.get_params_size() > 3) {
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

  // 추방하는 유저가 채널에 들어가 있는지 확인
  Channel& chan = chan_it->second;
  if (chan.chk_user_join(event_user_nick) == false) {
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
    if (chan.chk_user_join(name_vec[i]) == false) {
      event_user.push_back_msg(
          Message::rpl_441(serv_name, event_user_nick, name_vec[i], chan_name)
              .to_raw_msg());
      return;
    }

    rpl[1] = name_vec[i];
    send_msg_to_channel(chan, rpl.to_raw_msg());
    chan.remove_user(name_vec[i]);
  }
}

void Server::cmd_who(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk. 1개가 정상적.
  if (msg.get_params_size() != 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  // 채널인지 유저인지 확인
  if (msg[0][0] == REGULAR_CHANNEL_PREFIX ||
      msg[0][0] == LOCAL_CHANNEL_PREFIX) {
    const String& chan_name = msg[0];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it != channel_list.end()) {
      Channel& chan = chan_it->second;
      std::map<String, User&>::iterator chan_user_it =
          chan.get_user_list().begin();
      for (; chan_user_it != chan.get_user_list().end(); ++chan_user_it) {
        if (chan.is_operator(chan_user_it->first) == true) {
          event_user.push_back_msg(
              Message::rpl_352(serv_name, event_user_nick, chan_name,
                               chan_user_it->second, serv_name, "H@", 0)
                  .to_raw_msg());
        } else {
          event_user.push_back_msg(
              Message::rpl_352(serv_name, event_user_nick, chan_name,
                               chan_user_it->second, serv_name, "H", 0)
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
    // 인자 개수 확인. 1개 이상이 정상적
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

  // 인자 개수 확인. 1개가 정상적
  if (msg.get_params_size() != 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }
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
      std::map<String, int>::const_iterator user_chan_it;
      for (user_chan_it = user_chan.begin(); user_chan_it != user_chan.end();
           ++user_chan_it) {
        channel_list[user_chan_it->first].change_user_nickname(old_nick,
                                                               new_nick);
      }
      send_msg_to_connected_user(event_user, rpl.to_raw_msg());

      return;
    } else {
      (*this).change_nickname(event_user_nick, new_nick);
      event_user.set_nick_init_chk(OK);
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

  // 인자 개수 확인. 4 개가 정상적
  if (msg.get_params_size() != 4) {
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

  // 인자 개수 확인. 1개가 정상적
  if (msg.get_params_size() != 1) {
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

  event_user.set_have_to_ping_chk(false);
  event_user.set_last_ping(std::time(NULL));
}

void Server::cmd_quit(int recv_fd, const Message& msg) {
  /*
zzz

> quit
< :irc.example.net NOTICE zzz :Connection statistics: client 0.0 kb, server 1.2 kb.\r
< ERROR :Closing connection\r

> quit :quit message test
< :irc.example.net NOTICE zzz :Connection statistics: client 0.1 kb, server 1.3 kb.\r
< ERROR :"quit message test"\r

aaa
< :zzz!~zzz@localhost QUIT :"quit message test"\r

bbb
< :zzz!~zzz@localhost QUIT :"quit message test"\r
*/

  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();
  pollfd& tmp_pfd = event_user.get_pfd();

  // 인자 개수 확인. 0 ~ 1개가 정상적.
  if (msg.get_params_size() > 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  String trailing;
  if (msg.get_params_size() == 0) {
    trailing = ":Closing connection";
  } else {
    trailing = ":\"" + msg[0] + "\"";
  }

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(QUIT);
  rpl.push_back(trailing);
  send_msg_to_connected_user(event_user, rpl.to_raw_msg());

  std::map<String, int>::const_iterator con_it =
      event_user.get_connected_list().begin();
  for (; con_it != event_user.get_connected_list().end(); ++con_it) {
    (*this)[con_it->second].remove_connected(event_user_nick);
  }

  rpl.clear();
  rpl.set_source(serv_name);
  rpl.set_cmd_type(NOTICE);
  rpl.push_back(event_user_nick);
  rpl.push_back(":Connection statistics: client - kb, server - kb.");
  event_user.push_back_msg(rpl.to_raw_msg());

  rpl.clear();
  rpl.set_source("");
  rpl.set_cmd_type(ERROR);
  rpl.push_back(trailing);
  event_user.push_back_msg(rpl.to_raw_msg());

  std::clog << "Connection close at " << recv_fd << '\n';
  event_user.set_have_to_disconnect(true);
  ft_sendd(tmp_pfd);
}

void Server::cmd_names(int recv_fd, const Message& msg) {
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
  User& event_user = (*this)[recv_fd];
  const std::map<String, int>& event_user_chan = event_user.get_channels();
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk
  if (msg.get_params_size() == 0) {
    std::map<String, Channel>::iterator chan_it = channel_list.begin();
    String symbol;

    for (; chan_it != channel_list.end(); ++chan_it) {
      Channel& chan = chan_it->second;
      const String& chan_name = chan.get_channel_name();
      bool event_user_in_chan;

      if (chan.chk_user_join(event_user_nick) == true) {
        event_user_in_chan = true;
      } else {
        event_user_in_chan = false;
      }

      if (chan.chk_mode(CHAN_FLAG_S) == true) {
        if (event_user_in_chan = false) {
          continue;
        } else {
          symbol = "@";
        }
      } else {
        symbol = "=";
      }

      String nicks = "";

      nicks = chan.get_user_list_str(event_user_in_chan);
      if (nicks.length() > 0) {
        event_user.push_back_msg(Message::rpl_353(serv_name, event_user_nick,
                                                  symbol, chan_name, nicks)
                                     .to_raw_msg());
      }
    }

    std::map<int, User>::reverse_iterator user_it = user_list.rbegin();
    String nicks = "";
    for (; user_it < user_list.rend(); ++user_it) {
      if (user_it->second.chk_mode(USER_FLAG_I) == true) {
        continue;
      }
      if (user_it->second.get_channels().size() == 0) {
        nicks += user_it->second.get_nick_name();
        nicks += " ";
      }
    }
    if (nicks.length() > 0) {
      nicks.erase(nicks.length() - 1);
      event_user.push_back_msg(
          Message::rpl_353(serv_name, event_user_nick, "*", "*", nicks)
              .to_raw_msg());
    }
    event_user.push_back_msg(
        Message::rpl_366(serv_name, event_user_nick, "*").to_raw_msg());
  } else if (msg.get_params_size() == 1) {
    std::vector<String> chan_name_vec;
    ft_split(msg[1], ",", chan_name_vec);

    for (size_t i = 0; i < chan_name_vec.size(); ++i) {
      // 채널 존재하는지 확인
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(chan_name_vec[i]);
      if (chan_it != channel_list.end()) {
        Channel& chan = chan_it->second;
        const String& chan_name = chan.get_channel_name();
        bool event_user_in_chan;
        String symbol;

        if (chan.chk_user_join(event_user_nick) == true) {
          event_user_in_chan = true;
        } else {
          event_user_in_chan = false;
        }

        if (chan.chk_mode(CHAN_FLAG_S) == true) {
          if (event_user_in_chan = false) {
            continue;
          } else {
            symbol = "@";
          }
        } else {
          symbol = "=";
        }

        String nicks = "";

        nicks = chan.get_user_list_str(event_user_in_chan);
        if (nicks.length() > 0) {
          event_user.push_back_msg(Message::rpl_353(serv_name, event_user_nick,
                                                    symbol, chan_name, nicks)
                                       .to_raw_msg());
        }
      }
      event_user.push_back_msg(
          Message::rpl_366(serv_name, event_user_nick, chan_name_vec[i])
              .to_raw_msg());
    }
  } else {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }
}

void Server::cmd_privmsg(int recv_fd, const Message& msg) {
  /*
  aaa > PRIVMSG #chan1 :hi\r
  bbb < :aaa!~test_user@localhost PRIVMSG #chan1 :hi\r
  ccc < :aaa!~test_user@localhost PRIVMSG #chan1 :hi\r

  aaa > PRIVMSG bbb :send msg test\r
  bbb < :aaa!~test_user@localhost PRIVMSG bbb :send msg test\r
  */
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인
  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        Message::rpl_411(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }
  if (msg.get_params_size() == 1) {
    event_user.push_back_msg(
        Message::rpl_412(serv_name, event_user_nick).to_raw_msg());
    return;
  }
  if (msg.get_params_size() > 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  std::vector<String> target_vec;
  ft_split(msg[0], ",", target_vec);

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(PRIVMSG);
  rpl.push_back("");
  rpl.push_back(":" + msg[1]);

  for (size_t i = 0; i < target_vec.size(); i++) {
    if (target_vec[i][0] == REGULAR_CHANNEL_PREFIX ||
        target_vec[i][0] == LOCAL_CHANNEL_PREFIX) {
      // 채널 존재하는지 확인
      const String& chan_name = target_vec[i];
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(chan_name);
      if (chan_it == channel_list.end()) {
        event_user.push_back_msg(
            Message::rpl_401(serv_name, event_user_nick, target_vec[i])
                .to_raw_msg());
        return;
      }
      rpl[0] = chan_name;
      send_msg_to_channel_except_sender(chan_it->second, event_user_nick,
                                        rpl.to_raw_msg());
    } else {
      // 유저 존재하는지 확인
      std::map<String, int>::iterator user_it = nick_to_soc.find(target_vec[i]);
      if (user_it == nick_to_soc.end()) {
        event_user.push_back_msg(
            Message::rpl_401(serv_name, event_user_nick, target_vec[i])
                .to_raw_msg());
        return;
      }
      User& receiver = (*this)[user_it->second];
      rpl[0] = receiver.get_nick_name();
      receiver.push_back_msg(rpl.to_raw_msg());
      ft_send(receiver.get_pfd());
    }
  }
}

void Server::cmd_join(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 1 ~ 2개가 정상적.
  if (msg.get_params_size() < 1 || msg.get_params_size() > 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  std::vector<String> chan_name_vec;
  std::vector<String> chan_pass_vec;

  ft_split(msg[0], ",", chan_name_vec);
  if (msg.get_params_size() == 2) {
    ft_split(msg[1], ",", chan_pass_vec);
  }

  for (size_t i = 0; i < chan_name_vec.size(); ++i) {
    if (chan_name_vec[i][0] != REGULAR_CHANNEL_PREFIX &&
        chan_name_vec[i][0] != LOCAL_CHANNEL_PREFIX) {
      event_user.push_back_msg(
          Message::rpl_403(serv_name, event_user_nick, chan_name_vec[i])
              .to_raw_msg());
      continue;
    }

    if (event_user.get_channels().size() >= USERCHANLIMIT) {
      event_user.push_back_msg(
          Message::rpl_405(serv_name, event_user_nick, chan_name_vec[i])
              .to_raw_msg());
      continue;
    }

    std::map<String, Channel>::iterator chan_it =
        channel_list.find(chan_name_vec[i]);
    if (chan_it != channel_list.end()) {
      Channel& chan = chan_it->second;
      const String& chan_name = chan.get_channel_name();
      if (chan.chk_user_join(event_user_nick) == true) {
        continue;
      }

      if (chan.chk_mode(CHAN_FLAG_I) == true &&
          event_user.is_invited(chan_name) == false) {
        event_user.push_back_msg(
            Message::rpl_473(serv_name, event_user_nick, chan_name)
                .to_raw_msg());
        continue;
      }
      event_user.remove_invitation(chan_name);

      if (chan.chk_mode(CHAN_FLAG_K) == true) {
        if (i >= chan_pass_vec.size() ||
            chan_pass_vec[i] != chan.get_password()) {
          event_user.push_back_msg(
              Message::rpl_475(serv_name, event_user_nick, chan_name)
                  .to_raw_msg());
          continue;
        }
      }

      try {
        chan.add_user(event_user);

        std::map<String, User&>::iterator chan_user_it =
            chan.get_user_list().begin();
        for (; chan_user_it != chan.get_user_list().end(); ++chan_user_it) {
          event_user.add_connected(chan_user_it->first,
                                   chan_user_it->second.get_user_socket());
        }

        String symbol;
        if (chan.chk_mode(CHAN_FLAG_S) == true) {
          symbol = "@";
        } else {
          symbol = "=";
        }
        event_user.push_back_msg(Message::rpl_353(serv_name, event_user_nick,
                                                  symbol, chan_name,
                                                  chan.get_user_list_str(true))
                                     .to_raw_msg());

        const String& chan_topic = chan.get_topic();
        if (chan_topic.length() != 0) {
          event_user.push_back_msg(Message::rpl_332(serv_name, event_user_nick,
                                                    chan_name, chan_topic)
                                       .to_raw_msg());
          event_user.push_back_msg(
              Message::rpl_333(serv_name, event_user_nick, chan_name,
                               chan.get_topic_set_nick(),
                               ft_ltos(chan.get_topic_set_time()))
                  .to_raw_msg());
        }

        event_user.push_back_msg(
            Message::rpl_366(serv_name, event_user_nick, chan_name_vec[i])
                .to_raw_msg());

        // :zzz!~zzz@localhost JOIN :#chan_a\r
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.push_back(":" + chan_name);
        send_msg_to_channel(chan, rpl.to_raw_msg());

      } catch (const channel_user_capacity_error e) {
        event_user.push_back_msg(
            Message::rpl_471(serv_name, event_user_nick, chan_name)
                .to_raw_msg());
        continue;
      }
    } else {
      Channel new_chan(chan_name_vec[i]);

      add_channel(new_chan);
      Channel& chan_ref = channel_list[chan_name_vec[i]];
      chan_ref.add_user(event_user);
      chan_ref.add_operator(event_user);
      event_user.push_back_msg(Message::rpl_353(serv_name, event_user_nick, "=",
                                                chan_name_vec[i],
                                                ":@" + event_user_nick)
                                   .to_raw_msg());
      event_user.push_back_msg(
          Message::rpl_366(serv_name, event_user_nick, chan_name_vec[i])
              .to_raw_msg());
    }
  }
}

void Server::cmd_part(int recv_fd, const Message& msg) {
  /*
zzz
< part #chan_a,#chan_b
> :zzz!~zzz@localhost PART #chan_a :\r
> :zzz!~zzz@localhost PART #chan_b :\r

aaa (zzz와 chan_a, chan_b에 같이 있음)
> :zzz!~zzz@localhost PART #chan_a :\r
> :zzz!~zzz@localhost PART #chan_b :\r

bbb (zzz와 chan_a, chan_b에 같이 있음)
> :zzz!~zzz@localhost PART #chan_a :\r
> :zzz!~zzz@localhost PART #chan_b :\r

ccc
none
---------------------------------------------------------
zzz
< part #chan_a,#chan_b :part message test
> :zzz!~zzz@localhost PART #chan_a :part message test\r
> :zzz!~zzz@localhost PART #chan_b :part message test\r
*/

  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 1 ~ 2개가 정상적.
  if (msg.get_params_size() < 1 || msg.get_params_size() > 2) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  String reason;

  std::vector<String> chan_name_vec;
  ft_split(msg[0], ",", chan_name_vec);

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(PART);
  rpl.push_back("");
  if (msg.get_params_size() == 2) {
    rpl.push_back(":" + msg[1]);
  }

  for (size_t i = 0; i < chan_name_vec.size(); ++i) {
    if (chan_name_vec[i][0] != REGULAR_CHANNEL_PREFIX &&
        chan_name_vec[i][0] != LOCAL_CHANNEL_PREFIX) {
      event_user.push_back_msg(
          Message::rpl_403(serv_name, event_user_nick, chan_name_vec[i])
              .to_raw_msg());
      continue;
    }

    const String& chan_name = chan_name_vec[i];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it == channel_list.end()) {
      event_user.push_back_msg(
          Message::rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    Channel& chan = chan_it->second;
    if (chan.chk_user_join(event_user_nick) == false) {
      event_user.push_back_msg(
          Message::rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    rpl[0] = chan_name;
    send_msg_to_channel(chan, rpl.to_raw_msg());
    chan.remove_user(event_user_nick);
  }
}

void Server::cmd_list(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 0 ~ 1개가 정상적.
  if (msg.get_params_size() > 1) {
    event_user.push_back_msg(
        Message::rpl_461(serv_name, event_user_nick, msg.get_raw_cmd())
            .to_raw_msg());
    return;
  }

  event_user.push_back_msg(
      Message::rpl_321(serv_name, event_user_nick).to_raw_msg());
  if (msg.get_params_size() == 0) {
    std::map<String, Channel>::iterator chan_it = channel_list.begin();
    for (; chan_it != channel_list.end(); ++chan_it) {
      const String& chan_name = chan_it->first;
      Channel& chan = chan_it->second;

      if (chan.chk_mode(CHAN_FLAG_S) == true) {
        continue;
      }

      event_user.push_back_msg(
          Message::rpl_322(serv_name, event_user_nick, chan_name,
                           ft_itos(chan.get_user_list().size()),
                           chan.get_topic())
              .to_raw_msg());
    }
  } else {
    std::vector<String> chan_name_vec;
    ft_split(msg[0], ",", chan_name_vec);

    for (size_t i = 0; i < chan_name_vec.size(); ++i) {
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(chan_name_vec[i]);
      if (chan_it != channel_list.end()) {
        const String& chan_name = chan_it->first;
        Channel& chan = chan_it->second;

        if (chan.chk_mode(CHAN_FLAG_S) == true) {
          continue;
        }

        event_user.push_back_msg(
            Message::rpl_322(serv_name, event_user_nick, chan_name,
                             ft_itos(chan.get_user_list().size()),
                             chan.get_topic())
                .to_raw_msg());
      }
    }
  }
  event_user.push_back_msg(
      Message::rpl_323(serv_name, event_user_nick).to_raw_msg());
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

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
      if (get_channel(channel_iterator).isOperator((*this)[recv_fd]) == false) {
        User& event_user = (*this)[recv_fd];
        Message rpl =
            Message::rpl_482(serv_name, (*this)[recv_fd].get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
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
        event_user.push_back_msg(rpl.to_raw_msg());
        return;
      }

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (get_channel(get_channel_iterator(targetChannelStr))
              .isMode(CHAN_FLAG_I))
        return;

      // 여기서 어떤 모드인지에 따라 RESPONSE 메세지를 처리해줌
      // 즉, 채널 속성값을 여기서 변경함

      get_channel(get_channel_iterator(targetChannelStr)).setMode(CHAN_FLAG_I);

      User& event_user = (*this)[recv_fd];

      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr)).setMode(CHAN_FLAG_I);
      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd("MODE");
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(msg.get_params()[1]);

      // CHANNEL MODE 가 이미 +i 이면 중복 메세지 혹은 중복 세팅은 불피요 함으로
      // return ; 으로 더 이상 진행 못하게 바꿔줌
      if (!get_channel(get_channel_iterator(targetChannelStr))
               .isMode(CHAN_FLAG_I))
        return;

      // 해당 채널에서 모드값 스위치 ON / OFF 해주기
      get_channel(get_channel_iterator(targetChannelStr))
          .unsetMode(CHAN_FLAG_I);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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
    // ##################################### [ /mode +o ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 && msg.get_params()[1] == OPERATING_MODE_ON) {
      // check for privilege
      if (get_channel(channel_iterator).isOperator(event_user) == false) {
        Message rpl =
            Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
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
        Message rpl =
            Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
        return;
      }

      // 서버에 등록되어 있지 않은 경우
      try {
        (*this)[msg.get_params()[2]];
      } catch (std::invalid_argument& e) {
        event_user.push_back_msg(
            Message::rpl_401_mode_operator(
                serv_name, event_user.get_nick_name(), msg.get_params()[2])
                .to_raw_msg());
        return;
      }

      // check for invalid nickname => 이 경우는 서버에는 등록되어 있는 유저이지만 채널에 없는 경우임
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (get_channel(channel_iterator).foundClient(msg.get_params()[2]) ==
          false) {
        // < 2024/06/12 16:19:38.000655856  length=65 from=19794 to=19858
        // :irc.example.net 441 dy_ dy__ #zzz :They aren't on that channel\r
        event_user.push_back_msg(Message::rpl_441(serv_name, msg).to_raw_msg());
        return;
      }

      // assign the target client into the OP list

      if (get_channel(channel_iterator).isOperator(msg.get_params()[2])) {
        return;
      } else {
        int fd = (*this)[msg.get_params()[2]];
        User& targetClient = (*this)[fd];
        get_channel(channel_iterator).addOperator(targetClient);
      }

      // RESPONSE
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd_type(MODE);
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(OPERATING_MODE_ON);
      rpl.push_back(msg.get_params()[2]);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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
    // ##################################### [ /mode -o ]
    // ###############################################
    // ##################################################################################################
    if (msg.get_params_size() > 1 &&
        msg.get_params()[1] == OPERATING_MODE_OFF) {
      // check for privilege
      if (get_channel(channel_iterator).isOperator(event_user) == false) {
        Message rpl =
            Message::rpl_482(serv_name, event_user.get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
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
        Message rpl =
            Message::rpl_403(serv_name, event_user.get_nick_name(), msg);
        event_user.push_back_msg(rpl.to_raw_msg());
        return;
      }

      // 서버에 등록되어 있지 않은 경우
      try {
        (*this)[msg.get_params()[2]];
      } catch (std::invalid_argument& e) {
        event_user.push_back_msg(
            Message::rpl_401_mode_operator(
                serv_name, event_user.get_nick_name(), msg.get_params()[2])
                .to_raw_msg());
        return;
      }

      // check for invalid nickname => 이 경우는 서버에는 등록되어 있는 유저이지만 채널에 그 유저가 없는 경우
      channel_iterator = get_channel_iterator(targetChannelStr);
      if (get_channel(channel_iterator).foundClient(msg.get_params()[2]) ==
          false) {
        // < 2024/06/12 16:19:38.000655856  length=65 from=19794 to=19858
        // :irc.example.net 441 dy_ dy__ #zzz :They aren't on that channel\r
        event_user.push_back_msg(Message::rpl_441(serv_name, msg).to_raw_msg());
        return;
      }

      // remove the target client from the OP list
      if (get_channel(channel_iterator).isOperator(msg.get_params()[2]) ==
          false) {
        return;
      } else {
        int fd = (*this)[msg.get_params()[2]];
        User& targetClient = (*this)[fd];
        get_channel(channel_iterator).removeOperator(targetClient);
      }

      // RESPONSE
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd_type(MODE);
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(OPERATING_MODE_OFF);
      rpl.push_back(msg.get_params()[2]);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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
        return;
      }
      get_channel(get_channel_iterator(targetChannelStr)).setMode(CHAN_FLAG_L);
      get_channel(get_channel_iterator(targetChannelStr))
          .setLimit(msg.get_params()[2]);

      // > 2024/06/12 22:44:00.000649604  length=18 from=3035 to=3052
      // MODE #test +l 22\r
      // < 2024/06/12 22:44:00.000649893  length=42 from=18211 to=18252
      // :dy!~memememe@localhost MODE #test +l 22\r

      // RESPONSE
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd_type(MODE);
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(LIMIT_ON);
      rpl.push_back(msg.get_params()[2]);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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

    } else if (msg.get_params_size() > 1 &&
               msg.starts_with(msg.get_params()[1], LIMIT_OFF) == true) {
      get_channel(get_channel_iterator(targetChannelStr))
          .unsetMode(CHAN_FLAG_L);
      get_channel(get_channel_iterator(targetChannelStr))
          .setLimit(INIT_CLIENT_LIMIT);

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
        char chanmodes[5] = {'k', 'l', 'i', 's', 't'};
        if (std::find(chanmodes, chanmodes + 5, *iterator) == chanmodes + 5) {
          // :irc.example.net 472 dy 1 :is unknown mode char for #test\r
          Message rpl;
          event_user.push_back_msg(Message::rpl_472(serv_name,
                                                    event_user.get_nick_name(),
                                                    *iterator, msg)
                                       .to_raw_msg());
        }
      }

      // RESPONSE
      Message rpl;
      rpl.set_source(event_user.get_nick_name() + std::string("!~") +
                     event_user.get_user_name() + std::string("@localhost"));
      rpl.set_cmd_type(MODE);
      rpl.push_back(msg.get_params()[0]);
      rpl.push_back(LIMIT_OFF);

      // BROADCASTING
      std::map<std::string, User&>::iterator it;
      for (it = get_channel(get_channel_iterator(targetChannelStr))
                    .get_channel_client_list()
                    .begin();
           it != get_channel(get_channel_iterator(targetChannelStr))
                     .get_channel_client_list()
                     .end();
           ++it) {
        std::string clientNickName = it->second.get_nick_name();
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
    // rpl.set_source(event_user.get_nick_name() + std::string("!") +
    //                event_user.get_user_name() + std::string("@localhost"));
    // rpl.set_cmd_type(MODE);
    // rpl.push_back(event_user.get_nick_name());
    // rpl.push_back(":" + msg[0]);
    // event_user.push_back_msg(rpl.to_raw_msg());
  }
}
