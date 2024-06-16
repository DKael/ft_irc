#include "Server.hpp"

void Server::cmd_topic(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 1 ~ 2개가 정상적.
  if (msg.get_params_size() < 1 || msg.get_params_size() > 2) {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  // 채널 존재하는지 확인
  const String& chan_name = msg[0];
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 메세지 날린 유저가 채널에 존재하는지 확인
  Channel& chan = chan_it->second;
  if (chan.chk_user_join(event_user_nick) == false) {
    event_user.push_back_msg(
        rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  const String& topic = chan.get_topic();
  if (msg.get_params_size() == 1) {
    //check the topic

    // topic set chk
    if (topic.length() == 0) {
      event_user.push_back_msg(
          rpl_331(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    } else {
      event_user.push_back_msg(
          rpl_332(serv_name, event_user_nick, chan_name, topic).to_raw_msg());
      event_user.push_back_msg(rpl_333(serv_name, event_user_nick, chan_name,
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
          rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    /*
    < 2024/05/11 15:09:25.000532736  length=45 from=4305 to=4349
    :dy!~memememe@localhost TOPIC #test :welfkn\r
    */
    const String& new_topic = msg[1];

    if (new_topic.length() > TOPICLEN) {
      chan.set_topic(new_topic.substr(0, TOPICLEN) + "[CUT]");
    } else {
      chan.set_topic(new_topic);
    }

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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
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
        rpl_401(serv_name, event_user_nick, invited_user_nick).to_raw_msg());
    return;
  }

  // 채널 존재하는지 확인
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  User& invited_user = (*this)[user_it->second];
  Channel& chan = chan_it->second;

  // 초대자가 채널에 들어가 있는지 확인
  if (chan.chk_user_join(event_user_nick) == false) {
    event_user.push_back_msg(
        rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대자가 권한이 있는지 확인
  if (chan.is_operator(event_user_nick) == false) {
    event_user.push_back_msg(
        rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 초대받은자가 이미 채널에 있는지 확인
  if (chan.chk_user_join(invited_user_nick) == true) {
    event_user.push_back_msg(
        rpl_443(serv_name, event_user_nick, invited_user_nick, chan_name)
            .to_raw_msg());
    return;
  }

  invited_user.push_invitation(chan.get_channel_name());

  event_user.push_back_msg(rpl_341(invited_user.make_source(1), event_user_nick,
                                   invited_user_nick, chan_name)
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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  const String& chan_name = msg[0];
  // 채널 존재하는지 확인
  std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
  if (chan_it == channel_list.end()) {
    event_user.push_back_msg(
        rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 추방하는 유저가 채널에 들어가 있는지 확인
  Channel& chan = chan_it->second;
  if (chan.chk_user_join(event_user_nick) == false) {
    event_user.push_back_msg(
        rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 추방하는 유저가 권한이 있는지 확인
  if (chan.is_operator(event_user_nick) == false) {
    event_user.push_back_msg(
        rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
    return;
  }

  // 추방할 이름이 여러 개 들어올 수 있음. ','를 기준으로 구분.
  std::vector<String> name_vec;
  ft_split(msg[1], ",", name_vec);

  Message rpl;

  rpl.set_source(event_user.make_source(1));
  rpl.set_cmd_type(KICK);
  rpl.push_back(chan_name);
  rpl.push_back("");
  if (msg.get_params_size() >= 3) {
    if (msg[2].length() > KICKLEN) {
      rpl.push_back(":" + msg[2].substr(0, KICKLEN) + "[CUT]");
    } else {
      rpl.push_back(":" + msg[2]);
    }
  }

  for (size_t i = 0; i < name_vec.size(); ++i) {
    // 유저 존재하는지 확인
    std::map<String, int>::iterator user_it = nick_to_soc.find(name_vec[i]);
    if (user_it == nick_to_soc.end()) {
      event_user.push_back_msg(
          rpl_401(serv_name, event_user_nick, name_vec[i]).to_raw_msg());
      continue;
    }

    // 추방당할 자가 채널에 있는지 확인
    if (chan.chk_user_join(name_vec[i]) == false) {
      event_user.push_back_msg(
          rpl_441(serv_name, event_user_nick, name_vec[i], chan_name)
              .to_raw_msg());
      return;
    }

    rpl[1] = name_vec[i];
    send_msg_to_channel(chan, rpl.to_raw_msg());
    chan.remove_user(name_vec[i]);
    (*this)[(*this)[name_vec[i]]].part_channel(chan_name);
  }
}

void Server::cmd_who(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // short parameter chk. 1개가 정상적.
  if (msg.get_params_size() != 1) {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  // 채널인지 유저인지 확인
  if (is_channel_name(msg[0]) == true) {
    const String& chan_name = msg[0];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it != channel_list.end()) {
      Channel& chan = chan_it->second;
      std::map<String, User&>::iterator chan_user_it =
          chan.get_user_list().begin();
      for (; chan_user_it != chan.get_user_list().end(); ++chan_user_it) {
        if (chan.is_operator(chan_user_it->first) == true) {
          event_user.push_back_msg(rpl_352(serv_name, event_user_nick,
                                           chan_name, chan_user_it->second,
                                           serv_name, "H@", 0)
                                       .to_raw_msg());
        } else {
          event_user.push_back_msg(rpl_352(serv_name, event_user_nick,
                                           chan_name, chan_user_it->second,
                                           serv_name, "H", 0)
                                       .to_raw_msg());
        }
      }
    }
  } else {
    const String& user_name = msg[0];
    std::map<String, int>::iterator user_it = nick_to_soc.find(user_name);
    if (user_it != nick_to_soc.end()) {
      event_user.push_back_msg(rpl_352(serv_name, event_user_nick, "*",
                                       (*this)[user_it->second], serv_name, "H",
                                       0)
                                   .to_raw_msg());
    }
  }
  event_user.push_back_msg(
      rpl_315(serv_name, event_user_nick, msg[0]).to_raw_msg());
  ft_send(event_user.get_pfd());
}

void Server::cmd_pass(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  String pass_tmp;

  if (event_user.get_password_chk() == NOT_YET) {
    // 인자 개수 확인. 1개 이상이 정상적
    if (msg.get_params_size() < 1) {
      event_user.push_back_msg(
          rpl_461(serv_name, event_user.get_nick_name(), msg.get_raw_cmd())
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
        rpl_462(serv_name, event_user.get_nick_name()).to_raw_msg());
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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }
  new_nick = msg[0];
  if (('0' <= new_nick[0] && new_nick[0] <= '9') ||
      new_nick.find_first_of(CHANTYPES + String(": \n\t\v\f\r")) !=
          String::npos ||
      new_nick.length() > NICKLEN) {
    event_user.push_back_msg(
        rpl_432(serv_name, event_user_nick, new_nick).to_raw_msg());
    return;
  }
  if (nick_to_soc.find(new_nick) != nick_to_soc.end() ||
      tmp_nick_to_soc.find(new_nick) != tmp_nick_to_soc.end()) {
    event_user.push_back_msg(
        rpl_433(serv_name, event_user_nick, new_nick).to_raw_msg());
    return;
  }
  if (event_user.get_is_authenticated() == OK) {
    String old_nick = event_user_nick;
    Message rpl;

    rpl.set_source(event_user.make_source(1));
    rpl.set_cmd_type(NICK);
    rpl.push_back(":" + new_nick);
    event_user.push_back_msg(rpl.to_raw_msg());
    ft_send(event_user.get_pfd());

    (*this).change_nickname(old_nick, new_nick);
    event_user.remove_all_invitations();

    const std::map<String, int>& user_chan = event_user.get_channels();
    std::map<String, int>::const_iterator user_chan_it;
    for (user_chan_it = user_chan.begin(); user_chan_it != user_chan.end();
         ++user_chan_it) {
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(user_chan_it->first);
      if (chan_it != channel_list.end()) {
        chan_it->second.change_user_nickname(old_nick, new_nick);
      }
    }
    send_msg_to_connected_user(event_user, rpl.to_raw_msg());

    return;
  } else {
    (*this).change_nickname(event_user.get_nick_name_no_chk(), new_nick);
    event_user.set_nick_init_chk(OK);
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
    event_user.push_back_msg(
        rpl_461(serv_name, event_user.get_nick_name(), msg.get_raw_cmd())
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
      Message rpl = rpl_462(serv_name, event_user.get_nick_name());
      event_user.push_back_msg(rpl.to_raw_msg());
      return;
    } else {
      Message rpl = rpl_451(serv_name, event_user.get_nick_name());
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
        rpl_409(serv_name, event_user.get_nick_name()).to_raw_msg());
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

  // 인자 개수 확인. 1개가 정상적
  if (msg.get_params_size() != 1) {
    event_user.push_back_msg(
        rpl_409(serv_name, event_user.get_nick_name()).to_raw_msg());
    return;
  }

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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
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
        if (event_user_in_chan == false) {
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
        event_user.push_back_msg(
            rpl_353(serv_name, event_user_nick, symbol, chan_name, nicks)
                .to_raw_msg());
      }
    }

    std::map<int, User>::reverse_iterator user_it = user_list.rbegin();
    String nicks = "";
    for (; user_it != user_list.rend(); ++user_it) {
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
          rpl_353(serv_name, event_user_nick, "*", "*", nicks).to_raw_msg());
    }
    event_user.push_back_msg(
        rpl_366(serv_name, event_user_nick, "*").to_raw_msg());
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
          if (event_user_in_chan == false) {
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
          event_user.push_back_msg(
              rpl_353(serv_name, event_user_nick, symbol, chan_name, nicks)
                  .to_raw_msg());
        }
      }
      event_user.push_back_msg(
          rpl_366(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
    }
  } else {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
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
        rpl_411(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }
  if (msg.get_params_size() == 1) {
    event_user.push_back_msg(rpl_412(serv_name, event_user_nick).to_raw_msg());
    return;
  }
  if (msg.get_params_size() > 2) {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
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
    if (is_channel_name(target_vec[i]) == true) {
      // 채널 존재하는지 확인
      const String& chan_name = target_vec[i];
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(chan_name);
      if (chan_it == channel_list.end()) {
        event_user.push_back_msg(
            rpl_401(serv_name, event_user_nick, target_vec[i]).to_raw_msg());
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
            rpl_401(serv_name, event_user_nick, target_vec[i]).to_raw_msg());
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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  std::vector<String> chan_name_vec;
  std::vector<String> chan_pass_vec;

  ft_split(msg[0], ",", chan_name_vec);
  if (msg.get_params_size() == 2) {
    ft_split(msg[1], ",", chan_pass_vec);
  }

  for (size_t i = 0; i < chan_name_vec.size(); ++i) {
    if ((is_channel_name(chan_name_vec[i])) == false ||
        chan_name_vec[i].length() > CHANNELLEN) {
      event_user.push_back_msg(
          rpl_403(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
      continue;
    }

    if (event_user.get_channels().size() >= USERCHANLIMIT) {
      event_user.push_back_msg(
          rpl_405(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
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
            rpl_473(serv_name, event_user_nick, chan_name).to_raw_msg());
        continue;
      }
      event_user.remove_invitation(chan_name);

      if (chan.chk_mode(CHAN_FLAG_K) == true) {
        if (i >= chan_pass_vec.size() ||
            chan_pass_vec[i] != chan.get_password()) {
          event_user.push_back_msg(
              rpl_475(serv_name, event_user_nick, chan_name).to_raw_msg());
          continue;
        }
      }

      try {
        chan.add_user(event_user);
        event_user.join_channel(chan_name_vec[i]);
        // :zzz!~zzz@localhost JOIN :#chan_a\r
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.set_cmd_type(JOIN);
        rpl.push_back(":" + chan_name);
        send_msg_to_channel(chan, rpl.to_raw_msg());

        String symbol;
        if (chan.chk_mode(CHAN_FLAG_S) == true) {
          symbol = "@";
        } else {
          symbol = "=";
        }
        event_user.push_back_msg(rpl_353(serv_name, event_user_nick, symbol,
                                         chan_name,
                                         chan.get_user_list_str(true))
                                     .to_raw_msg());

        const String& chan_topic = chan.get_topic();
        if (chan_topic.length() != 0) {
          event_user.push_back_msg(
              rpl_332(serv_name, event_user_nick, chan_name, chan_topic)
                  .to_raw_msg());
          event_user.push_back_msg(rpl_333(serv_name, event_user_nick,
                                           chan_name, chan.get_topic_set_nick(),
                                           ft_ltos(chan.get_topic_set_time()))
                                       .to_raw_msg());
        }

        event_user.push_back_msg(
            rpl_366(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
      } catch (const channel_user_capacity_error e) {
        event_user.push_back_msg(
            rpl_471(serv_name, event_user_nick, chan_name).to_raw_msg());
        continue;
      }
    } else {
      Channel new_chan(chan_name_vec[i]);

      add_channel(new_chan);
      std::map<String, Channel>::iterator chan_it =
          channel_list.find(chan_name_vec[i]);
      if (chan_it != channel_list.end()) {
        Channel& chan_ref = chan_it->second;
        chan_ref.add_user(event_user);
        chan_ref.add_operator(event_user);
        event_user.join_channel(chan_name_vec[i]);
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.set_cmd_type(JOIN);
        rpl.push_back(":" + chan_ref.get_channel_name());
        event_user.push_back_msg(rpl.to_raw_msg());

        event_user.push_back_msg(rpl_353(serv_name, event_user_nick, "=",
                                         chan_name_vec[i],
                                         "@" + event_user_nick)
                                     .to_raw_msg());
        event_user.push_back_msg(
            rpl_366(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
      }
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
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
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
    if (is_channel_name(chan_name_vec[i]) == false) {
      event_user.push_back_msg(
          rpl_403(serv_name, event_user_nick, chan_name_vec[i]).to_raw_msg());
      continue;
    }

    const String& chan_name = chan_name_vec[i];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it == channel_list.end()) {
      event_user.push_back_msg(
          rpl_403(serv_name, event_user_nick, chan_name).to_raw_msg());
      continue;
    }

    Channel& chan = chan_it->second;
    if (chan.chk_user_join(event_user_nick) == false) {
      event_user.push_back_msg(
          rpl_442(serv_name, event_user_nick, chan_name).to_raw_msg());
      continue;
    }

    rpl[0] = chan_name;
    send_msg_to_channel(chan, rpl.to_raw_msg());
    chan.remove_user(event_user_nick);
    event_user.part_channel(chan_name);
  }
}

void Server::cmd_list(int recv_fd, const Message& msg) {
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 0 ~ 1개가 정상적.
  if (msg.get_params_size() > 1) {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  event_user.push_back_msg(rpl_321(serv_name, event_user_nick).to_raw_msg());
  if (msg.get_params_size() == 0) {
    std::map<String, Channel>::iterator chan_it = channel_list.begin();
    for (; chan_it != channel_list.end(); ++chan_it) {
      const String& chan_name = chan_it->first;
      Channel& chan = chan_it->second;

      if (chan.chk_mode(CHAN_FLAG_S) == true) {
        continue;
      }

      event_user.push_back_msg(rpl_322(serv_name, event_user_nick, chan_name,
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

        event_user.push_back_msg(rpl_322(serv_name, event_user_nick, chan_name,
                                         ft_itos(chan.get_user_list().size()),
                                         chan.get_topic())
                                     .to_raw_msg());
      }
    }
  }
  event_user.push_back_msg(rpl_323(serv_name, event_user_nick).to_raw_msg());
}

void Server::cmd_mode(int recv_fd, const Message& msg) {
  /*
  mode
  :irc.example.net 461 zzz mode :Syntax error
  mode aaa
  :irc.example.net 401 zzz aaa :No such nick or channel name
  mode bbb
  :irc.example.net 502 zzz :Can't set/get mode for other users
  mode bbb +z
  :irc.example.net 502 zzz :Can't set/get mode for other users
  mode zzz +z
  :irc.example.net 501 zzz :Unknown mode "+z"
  mode zzz -z
  :irc.example.net 501 zzz :Unknown mode "-z"
  mode zzz +1234567
  :irc.example.net 501 zzz :Unknown mode "+1"
  :irc.example.net 501 zzz :Unknown mode "+2"
  :irc.example.net 501 zzz :Unknown mode "+3"
  :irc.example.net 501 zzz :Unknown mode "+4"
  :irc.example.net 501 zzz :Unknown mode "+5"
  :irc.example.net 501 zzz :Unknown mode "+6"
  :irc.example.net 501 zzz :Unknown mode "+7"
  mode +i-i
  :irc.example.net 401 zzz +i-i :No such nick or channel name
  mode zzz +i-i
  :zzz!~zzz@localhost MODE zzz :+i-i
  mode zzz +ior
  :irc.example.net 481 zzz :Permission denied
  :zzz!~zzz@localhost MODE zzz :+ir
  mode zzz +w-iro
  :irc.example.net 484 zzz :Your connection is restricted
  :zzz!~zzz@localhost MODE zzz :+w-i
  mode zzz
  :irc.example.net 221 zzz +rw

  mode zzz +iii-ii+i
  :zzz!~zzz@localhost MODE zzz :+i-i+i
  */
  User& event_user = (*this)[recv_fd];
  const String& event_user_nick = event_user.get_nick_name();

  // 인자 개수 확인. 1개 이상이 정상적
  if (msg.get_params_size() < 1) {
    event_user.push_back_msg(
        rpl_461(serv_name, event_user_nick, msg.get_raw_cmd()).to_raw_msg());
    return;
  }

  if (is_channel_name(msg[0]) == true) {
    const String& chan_name = msg[0];
    std::map<String, Channel>::iterator chan_it = channel_list.find(chan_name);
    if (chan_it == channel_list.end()) {
      event_user.push_back_msg(
          rpl_401(serv_name, event_user_nick, chan_name).to_raw_msg());
      return;
    }

    Channel& chan = chan_it->second;
    if (msg.get_params_size() == 1) {
      std::vector<String> param_vec;

      if (chan.chk_mode(CHAN_FLAG_K) == true) {
        param_vec.push_back(chan.get_password());
      }
      if (chan.chk_mode(CHAN_FLAG_L) == true) {
        param_vec.push_back(ft_itos(chan.get_user_limit()));
      }

      event_user.push_back_msg(rpl_324(serv_name, event_user_nick, chan_name,
                                       chan.make_mode_str(), param_vec)
                                   .to_raw_msg());
      event_user.push_back_msg(rpl_329(serv_name, event_user_nick, chan_name,
                                       ft_ltos(chan.get_created_time()))
                                   .to_raw_msg());
    } else {
      const String& mode = msg[1];
      size_t param_idx = 2;
      bool set_mode = true;
      int old_mode = chan.get_mode();
      String done_set = "";
      String result = "";
      std::vector<String> param_vec;

      for (size_t i = 0; i < mode.length(); i++) {
        if (mode[i] == '+') {
          if (set_mode == false && done_set.length() != 0) {
            result += "-";
            result += done_set;
            done_set = "";
          }
          set_mode = true;
        } else if (mode[i] == '-') {
          if (set_mode == true && done_set.length() != 0) {
            result += "+";
            result += done_set;
            done_set = "";
          }
          set_mode = false;
        } else {
          if (String(AVAILABLE_CHANNEL_MODES).find(mode[i]) == String::npos) {
            event_user.push_back_msg(
                rpl_472(serv_name, event_user_nick,
                        String(set_mode ? "+" : "-") + mode[i], chan_name)
                    .to_raw_msg());
            continue;
          }
          if (mode[i] == CHAN_FLAG_O_CHAR) {
            if (chan.is_operator(event_user_nick) == false) {
              event_user.push_back_msg(
                  rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
              return;
            }
            String target_nick;
            if (param_idx < msg.get_params_size()) {
              target_nick = msg[param_idx];
              ++param_idx;
            } else {
              continue;
            }
            if (chan.chk_user_join(target_nick) == false) {
              event_user.push_back_msg(
                  rpl_441(serv_name, event_user_nick, target_nick, chan_name)
                      .to_raw_msg());
              continue;
            }
            if (set_mode == true && chan.is_operator(target_nick) == false) {
              chan.add_operator((*this)[(*this)[target_nick]]);
              param_vec.push_back(target_nick);
              done_set += mode[i];
            } else if (set_mode == false &&
                       chan.is_operator(target_nick) == true) {
              chan.remove_operator(target_nick);
              param_vec.push_back(target_nick);
              done_set += mode[i];
            }
          } else if (mode[i] == CHAN_FLAG_K_CHAR) {
            if (chan.is_operator(event_user_nick) == false) {
              event_user.push_back_msg(
                  rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
              return;
            }
            String password;
            if (param_idx < msg.get_params_size()) {
              password = msg[param_idx];
              ++param_idx;
            } else {
              continue;
            }
            if (set_mode == true && chan.chk_mode(mode[i]) == false) {
              chan.set_mode(mode[i]);
              chan.set_password(password);
              param_vec.push_back(password);
              done_set += mode[i];
            } else if (set_mode == false && chan.chk_mode(mode[i]) == true) {
              chan.unset_mode(mode[i]);
              chan.set_password("");
              param_vec.push_back("*");
              done_set += mode[i];
            }
          } else if (mode[i] == CHAN_FLAG_L_CHAR) {
            if (chan.is_operator(event_user_nick) == false) {
              event_user.push_back_msg(
                  rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
              return;
            }
            String user_limit;
            if (set_mode == true) {
              if (param_idx < msg.get_params_size()) {
                user_limit = msg[param_idx];
                ++param_idx;
              } else {
                continue;
              }
            }
            std::stringstream convert_to_int;
            int limit_num;

            convert_to_int << user_limit;
            convert_to_int >> limit_num;

            if (convert_to_int.fail()) {
              continue;
            }

            if (set_mode == true && chan.chk_mode(mode[i]) == false) {
              chan.set_mode(mode[i]);
              chan.set_user_limit(limit_num);
              param_vec.push_back(user_limit);
              done_set += mode[i];
            } else if (set_mode == false && chan.chk_mode(mode[i]) == true) {
              chan.unset_mode(mode[i]);
              done_set += mode[i];
            }
            param_idx++;
          } else if (mode[i] == CHAN_FLAG_I_CHAR ||
                     mode[i] == CHAN_FLAG_S_CHAR ||
                     mode[i] == CHAN_FLAG_T_CHAR) {
            if (chan.is_operator(event_user_nick) == false) {
              event_user.push_back_msg(
                  rpl_482(serv_name, event_user_nick, chan_name).to_raw_msg());
              return;
            }
            if (set_mode == true && chan.chk_mode(mode[i]) == false) {
              chan.set_mode(mode[i]);
              done_set += mode[i];
            } else if (set_mode == false && chan.chk_mode(mode[i]) == true) {
              chan.unset_mode(mode[i]);
              done_set += mode[i];
            }
          }
        }
      }
      if (done_set.length() != 0) {
        result += (set_mode ? "+" : "-");
        result += done_set;
      }
      if (old_mode != chan.get_mode() || param_vec.size() != 0) {
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.set_cmd_type(MODE);
        rpl.push_back(chan_name);
        rpl.push_back(result);
        for (size_t l = 0; l < param_vec.size(); ++l) {
          rpl.push_back(param_vec[l]);
        }
        std::vector<String>::iterator msg_it = rpl.get_params().end() - 1;
        if (*msg_it == "*") {
          rpl.get_params().erase(msg_it);
        }
        send_msg_to_channel(chan, rpl.to_raw_msg());
      }
    }
  } else {
    // 유저 존재하는지 확인
    std::map<String, int>::iterator user_it = nick_to_soc.find(msg[0]);
    if (user_it == nick_to_soc.end()) {
      event_user.push_back_msg(
          rpl_401(serv_name, event_user_nick, msg[0]).to_raw_msg());
      return;
    }

    if (event_user_nick != msg[0]) {
      event_user.push_back_msg(
          rpl_502(serv_name, event_user_nick).to_raw_msg());
      return;
    }

    if (msg.get_params_size() == 1) {
      event_user.push_back_msg(
          rpl_221(serv_name, event_user_nick, event_user.make_mode_str())
              .to_raw_msg());
      return;
    } else {
      const String& mode = msg[1];

      bool set_mode;
      if (msg[1][0] == '+') {
        set_mode = true;
      } else if (msg[1][0] == '-') {
        set_mode = false;
      } else {
        event_user.push_back_msg(rpl_501(serv_name, event_user_nick,
                                         String(set_mode ? "+" : "-") + mode[1])
                                     .to_raw_msg());
        return;
      }

      int old_mode = event_user.get_mode();
      String done_set = "";
      String result = "";
      for (size_t i = 1; i < mode.length(); i++) {
        if (mode[i] == '+') {
          if (set_mode == false && done_set.length() != 0) {
            result += "-";
            result += done_set;
            done_set = "";
          }
          set_mode = true;
        } else if (mode[i] == '-') {
          if (set_mode == true && done_set.length() != 0) {
            result += "+";
            result += done_set;
            done_set = "";
          }
          set_mode = false;
        } else {
          if (String(AVAILABLE_USER_MODES).find(mode[i]) == String::npos) {
            event_user.push_back_msg(
                rpl_501(serv_name, event_user_nick,
                        String(set_mode ? "+" : "-") + mode[i])
                    .to_raw_msg());
            continue;
          }
          if (set_mode == true && event_user.chk_mode(mode[i]) == false) {
            event_user.set_mode(mode[i]);
            done_set += mode[i];
          } else if (set_mode == false &&
                     event_user.chk_mode(mode[i]) == true) {
            event_user.unset_mode(mode[i]);
            done_set += mode[i];
          }
        }
      }
      if (done_set.length() != 0) {
        result += (set_mode ? "+" : "-");
        result += done_set;
      }
      if (old_mode != event_user.get_mode()) {
        Message rpl;

        rpl.set_source(event_user.make_source(1));
        rpl.set_cmd_type(MODE);
        rpl.push_back(event_user_nick);
        rpl.push_back(":" + result);
        event_user.push_back_msg(rpl.to_raw_msg());
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void Server::cmd_whois(int recv_fd, const Message& msg) {}