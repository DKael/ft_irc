#include "Message.hpp"

/*
:irc.example.net 001 kael :Welcome to the Internet Relay Network kael!~kael@localhost\r
:irc.example.net 002 kael :Your host is irc.example.net, running version ngircd-26.1 (aarch64/apple/darwin23.0.0)\r
:irc.example.net 003 kael :This server has been started Tue Jun 11 2024 at 22:33:30 (KST)\r
:irc.example.net 004 kael irc.example.net ngircd-26.1 abBcCFiIoqrRswx abehiIklmMnoOPqQrRstvVz\r
:irc.example.net 005 kael RFC2812 IRCD=ngIRCd CHARSET=UTF-8 CASEMAPPING=ascii PREFIX=(qaohv)~&@%+ CHANTYPES=#&+ CHANMODES=beI,k,l,imMnOPQRstVz CHANLIMIT=#&+:10 :are supported on this server\r
:irc.example.net 005 kael CHANNELLEN=50 NICKLEN=9 TOPICLEN=490 AWAYLEN=127 KICKLEN=400 MODES=5 MAXLIST=beI:50 EXCEPTS=e INVEX=I PENALTY FNC :are supported on this server\r
*/

Message Message::rpl_001(const String& source, const String& user,
                         const String& user_source) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("001");
  rpl.push_back(user);
  rpl.push_back(":Welcome to the Internet Relay Network " + user_source);

  return rpl;
}

Message Message::rpl_002(const String& source, const String& user,
                         const String& server_name,
                         const String& server_version) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("002");
  rpl.push_back(user);
  rpl.push_back(":Your host is " + server_name + ", running version " +
                server_version);

  return rpl;
}

Message Message::rpl_003(const String& source, const String& user,
                         const String& server_created_time) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("003");
  rpl.push_back(user);
  rpl.push_back(":This server has been started " + server_created_time +
                "(KST)");

  return rpl;
}
Message Message::rpl_004(const String& source, const String& user,
                         const String& server_name,
                         const String& server_version,
                         const String& available_user_modes,
                         const String& available_channel_modes) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("004");
  rpl.push_back(user);
  rpl.push_back(server_name);
  rpl.push_back(server_version);
  rpl.push_back(available_user_modes);
  rpl.push_back(available_channel_modes);

  return rpl;
}
Message Message::rpl_005(const String& source, const String& user,
                         std::vector<String> specs) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("004");
  rpl.push_back(user);
  for (size_t i = 0; i < specs.size(); ++i) {
    rpl.push_back(specs[i]);
  }
  rpl.push_back(":are supported on this server");

  return rpl;
}

Message Message::rpl_315(const String& source, const String& user,
                         const String& mask) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("315");
  rpl.push_back(user);
  rpl.push_back(mask);
  rpl.push_back(":End of WHO list");

  return rpl;
}

Message Message::rpl_321(const String& source, const String& user) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("321");
  rpl.push_back(user);
  rpl.push_back("Channel :Users  Name");

  return rpl;
}

Message Message::rpl_322(const String& source, const String& user,
                         const String& channel, const String& client_count,
                         const String& topic) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("322");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(client_count);
  rpl.push_back(":" + topic);

  return rpl;
}

Message Message::rpl_323(const String& source, const String& user) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("323");
  rpl.push_back(user);
  rpl.push_back(":End of LIST");

  return rpl;
}

Message Message::rpl_331(const String& source, const String& user,
                         const String& channel) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("331");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":No topic is set");

  return rpl;
}

Message Message::rpl_332(const String& source, const String& user,
                         const String& channel, const String& topic) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("332");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":" + topic);

  return rpl;
}

Message Message::rpl_333(const String& source, const String& user,
                         const String& channel, const String& nick,
                         const String& setat) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("333");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(nick);
  rpl.push_back(setat);

  return rpl;
}

// reply message functions
Message Message::rpl_341(const String& source, const String& user,
                         const String& nick, const String& channel) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("341");
  rpl.push_back(user);
  rpl.push_back(nick);
  rpl.push_back(channel);

  return rpl;
}

/*
WHO #chan_a\r

:irc.example.net 352 ccc #chan_a ~test_user localhost irc.example.net ccc H :0 Hyungdo Kim\r
:irc.example.net 352 ccc #chan_a ~test_user localhost irc.example.net test H@ :0 Hyungdo Kim\r
:irc.example.net 315 ccc #chan_a :End of WHO list\r

WHO kkk\r

:irc.example.net 352 ccc * ~test_user localhost irc.example.net kkk H :0 Hyungdo Kim\r
:irc.example.net 315 ccc kkk :End of WHO list\r
*/
Message Message::rpl_352(const String& source, const String& user,
                         const String& channel, const User& _u,
                         const String& server, const String& flags,
                         int hopcount) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("352");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(_u.get_user_name());
  rpl.push_back(_u.get_host_ip());
  rpl.push_back(server);
  rpl.push_back(_u.get_nick_name());
  rpl.push_back(flags);
  rpl.push_back(":" + ft_itos(hopcount));
  rpl.push_back(_u.get_real_name());

  return rpl;
}

/*
  > 2024/05/05 14:52:02.000706851  length=9 from=503 to=511
  JOIN #b\r

  < 2024/05/05 14:52:02.000707138  length=124 from=2769 to=2892
  :[lfkn]!~[memememe]@localhost JOIN :#b\r
  :irc.example.net 353 lfkn = #b :@lfkn\r
  :irc.example.net 366 lfkn #b :End of NAMES list\r
*/
Message Message::rpl_353(const String& source, const String& user,
                         const String& symbol, const String& channel,
                         const String& nicks) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("353");
  rpl.push_back(user);
  rpl.push_back(symbol);
  rpl.push_back(channel);
  rpl.push_back(":" + nicks);

  return rpl;
}

Message Message::rpl_366(const String& source, const String& user,
                         const String& channel) {
  // :irc.example.net 366 lfkn__ #a :End of NAMES list\r
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("366");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(String(":End of NAMES list"));
  return rpl;
}

Message Message::rpl_401(const String& source, const String& user,
                         const String& nickname) {
  /*
  ERR_NOSUCHNICK (401)
  "<user> <nickname> :No such nick/channel"
  Indicates that no user can be found for the supplied nickname. The text
  used in the last param of this message may vary.

  :irc.example.net 401 lfkn slkfdn :No such nick or channel name\r
  (hostname) (nickname) (msg)
  */
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("401");
  rpl.push_back(user);
  rpl.push_back(nickname);
  rpl.push_back(":No such nick or channel name");

  return rpl;
}

Message Message::rpl_403(const String& source, const String& user,
                         const String& channel) {
  /* ERR_NOSUCHCHANNEL (403)
    "<user> <channel> :No such channel"
    Indicates that no channel can be found for the supplied channel name.
    The text used in the last param of this message may vary.
    :irc.example.net 403 lfkn__ #asdfw :No such channel\r
    :ft_irc 403 lfkn #asdf :
    /kick #없는채널명 어딘가에있는클라이언트명 으로 하면 이 에러 나옴*/
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("403");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":No such channel");

  return rpl;
}

Message Message::rpl_404(const String& source, const String& user,
                         const String& channel) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("404");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":Cannot send to channel");

  return rpl;
}

Message Message::rpl_405(const String& source, const String& user,
                         const String& channel) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("405");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":You have joined too many channels");

  return rpl;
}

Message Message::rpl_409(const String& source, const String& user) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("409");
  rpl.push_back(user);
  rpl.push_back(":No origin specified");

  return rpl;
}

Message Message::rpl_411(const String& source, const String& user,
                         const String& command) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("411");
  rpl.push_back(user);
  rpl.push_back(":No recipient given (<" + command + ">)");

  return rpl;
}

Message Message::rpl_412(const String& source, const String& user) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("412");
  rpl.push_back(user);
  rpl.push_back(":No text to send");

  return rpl;
}

Message Message::rpl_421(const String& source, const String& user,
                         const String& command) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("421");
  rpl.push_back(user);
  rpl.push_back(command);
  rpl.push_back(":Unknown command");

  return rpl;
}

Message Message::rpl_432(const String& source, const String& user,
                         const String& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("432");
  rpl.push_back(user);
  rpl.push_back(nick);
  rpl.push_back(":Erroneous nickname");

  return rpl;
}

Message Message::rpl_433(const String& source, const String& user,
                         const String& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("433");
  rpl.push_back(user);
  rpl.push_back(nick);
  rpl.push_back(":Nickname is already in use");

  return rpl;
}

Message Message::rpl_441(const String& source, const String& user,
                         const String& nick, const String& channel) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("441");
  rpl.push_back(user);
  rpl.push_back(nick);
  rpl.push_back(channel);
  rpl.push_back(String(":They aren't on that channel"));

  return rpl;
}

Message Message::rpl_442(const String& source, const String& user,
                         const String& channel) {
  /*
    ERR_NOTONCHANNEL (442)
    "<user> <channel> :You're not on that channel"
    Returned when a user tries to perform a channel-affecting command on a
    channel which the user isn’t a part of.

    채널에 속했든 안했든
     /kick [#chan_name] [userNAME] 이런식으로 명령이 가능한데 만약 채널에
     속하지 않은 유저가 명령을 내릴경우 442에러를 뱉어주면 됨.
  */
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("442");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(String(":You're not on that channel"));

  return rpl;
}

Message Message::rpl_443(const String& source, const String& user,
                         const String& nick, const String& channel) {
  // :irc.example.net 443 dy dy #test :is already on channel\r
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("443");
  rpl.push_back(user);
  rpl.push_back(nick);
  rpl.push_back(channel);
  rpl.push_back(":is already on channel");

  return rpl;
}

Message Message::rpl_451(const String& source, const String& user) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("451");
  rpl.push_back(user);
  rpl.push_back(":Connection not registered");

  return rpl;
}

Message Message::rpl_461(const String& source, const String& user,
                         const String& command) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("461");
  rpl.push_back(user);
  rpl.push_back(command);
  rpl.push_back(":Not enough parameters");

  return rpl;
}

Message Message::rpl_462(const String& source, const String& user) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("462");
  rpl.push_back(user);
  rpl.push_back(":Connection already registered");

  return rpl;
}

Message Message::rpl_464(const String& source, const String& user) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("464");
  rpl.push_back(user);
  rpl.push_back(":Password incorrect");

  return rpl;
}

Message Message::rpl_471(const String& source, const String& user,
                         const String& channel) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("471");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":Cannot join channel (+l)");

  return rpl;
}

Message Message::rpl_473(const String& source, const String& user,
                         const String& channel) {
  // :irc.example.net 473 dy_ #test :Cannot join channel (+i) -- Invited users
  // only\r
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("473");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":Cannot join channel (+i) -- Invited users only");

  return rpl;
}

Message Message::rpl_475(const String& source, const String& user,
                         const String& channel) {
  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("475");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":Cannot join channel (+k)");

  return rpl;
}

Message Message::rpl_482(const String& source, const String& user,
                         const String& channel) {
  /*
    ERR_CHANOPRIVSNEEDED (482)
    "<user> <channel> :You're not channel operator"
    Indicates that a command failed because the user does not have the
    appropriate channel privileges. This numeric can apply for different
    prefixes such as halfop, operator, etc. The text used in the last param of
    this message may vary.

    < 2024/05/11 14:33:05.000862471  length=62 from=2493 to=2554
    :irc.example.net 482 dy__ #test :Your privileges are too low\r
  */

  Message rpl;

  rpl.set_source(source);
  rpl.set_numeric("482");
  rpl.push_back(user);
  rpl.push_back(channel);
  rpl.push_back(":Your privileges are too low");

  return rpl;
}
