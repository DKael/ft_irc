#include "Message.hpp"

std::map<Command, std::string> Message::etos;
std::map<std::string, Command> Message::stoe;

void Message::map_init(void) {
  static bool flag = false;
  if (flag == false) {
    flag = true;
    etos[CAP] = "CAP", etos[AUTHENTICATE] = "AUTHENTICATE", etos[PASS] = "PASS",
    etos[NICK] = "NICK", etos[USER] = "USER", etos[PING] = "PING",
    etos[PONG] = "PONG", etos[OPER] = "OPER", etos[QUIT] = "QUIT",
    etos[ERROR] = "ERROR", etos[JOIN] = "JOIN", etos[PART] = "PART",
    etos[TOPIC] = "TOPIC", etos[NAMES] = "NAMES", etos[LIST] = "LIST",
    etos[INVITE] = "INVITE", etos[KICK] = "KICK", etos[MOTD] = "MOTD",
    etos[VERSION] = "VERSION", etos[ADMIN] = "ADMIN", etos[CONNECT] = "CONNECT",
    etos[LUSERS] = "LUSERS", etos[TIME] = "TIME", etos[STATS] = "STATS",
    etos[HELP] = "HELP", etos[INFO] = "INFO", etos[MODE] = "MODE",
    etos[PRIVMSG] = "PRIVMSG", etos[NOTICE] = "NOTICE", etos[WHO] = "WHO",
    etos[WHOIS] = "WHOIS", etos[WHOWAS] = "WHOWAS", etos[KILL] = "KILL",
    etos[REHASH] = "REHASH", etos[RESTART] = "RESTART", etos[SQUIT] = "SQUIT",
    etos[AWAY] = "AWAY", etos[LINKS] = "LINKS", etos[USERHOST] = "USERHOST",
    etos[WALLOPS] = "WALLOPS", etos[NONE] = "NONE", etos[NORPL] = "NORPL";

    stoe["CAP"] = CAP, stoe["AUTHENTICATE"] = AUTHENTICATE, stoe["PASS"] = PASS,
    stoe["NICK"] = NICK, stoe["USER"] = USER, stoe["PING"] = PING,
    stoe["PONG"] = PONG, stoe["OPER"] = OPER, stoe["QUIT"] = QUIT,
    stoe["ERROR"] = ERROR, stoe["JOIN"] = JOIN, stoe["PART"] = PART,
    stoe["TOPIC"] = TOPIC, stoe["NAMES"] = NAMES, stoe["LIST"] = LIST,
    stoe["INVITE"] = INVITE, stoe["KICK"] = KICK, stoe["MOTD"] = MOTD,
    stoe["VERSION"] = VERSION, stoe["ADMIN"] = ADMIN, stoe["CONNECT"] = CONNECT,
    stoe["LUSERS"] = LUSERS, stoe["TIME"] = TIME, stoe["STATS"] = STATS,
    stoe["HELP"] = HELP, stoe["INFO"] = INFO, stoe["MODE"] = MODE,
    stoe["PRIVMSG"] = PRIVMSG, stoe["NOTICE"] = NOTICE, stoe["WHO"] = WHO,
    stoe["WHOIS"] = WHOIS, stoe["WHOWAS"] = WHOWAS, stoe["KILL"] = KILL,
    stoe["REHASH"] = REHASH, stoe["RESTART"] = RESTART, stoe["SQUIT"] = SQUIT,
    stoe["AWAY"] = AWAY, stoe["LINKS"] = LINKS, stoe["USERHOST"] = USERHOST,
    stoe["WALLOPS"] = WALLOPS, stoe["NONE"] = NONE, stoe["NORPL"] = NORPL;
  }
}

Message::Message() : raw_msg(""), socket_fd(-1) {}

Message::Message(int _socket_fd, const std::string& _raw_msg)
    : socket_fd(_socket_fd), raw_msg(ft_strip(_raw_msg)) {
  if (raw_msg.length() == 0) {
    set_cmd_type(NONE);
    numeric = "421";
    params.push_back(":Unknown command");
    return;
  }
  std::size_t idx1 = 0;
  std::size_t idx2 = 0;
  std::size_t pos = 0;
  std::string tmp_trailing;
  std::string tmp_type;

  // check source
  if (raw_msg[0] == ':') {
    // source exist
    pos = raw_msg.find_first_of(' ');
    if (pos == std::string::npos) {
      set_cmd_type(ERROR);
      params.push_back(":Prefix without command");
      return;
    }
    source = raw_msg.substr(1, pos - 1);
    if (source.find_first_of("\0\n\t\v\f\r") != std::string::npos) {
      set_cmd_type(ERROR);
      params.push_back(std::string(":Invalid prefix \"") + source +
                       std::string("\""));
      return;
    }
  }

  // get command
  pos = raw_msg.find_first_not_of(' ', pos);
  if (pos == std::string::npos) {
    set_cmd_type(ERROR);
    params.push_back(":Prefix without command");
    return;
  }
  idx1 = pos;
  pos = raw_msg.find_first_of(' ', pos);
  if (pos == std::string::npos) {
    tmp_type = raw_msg.substr(idx1);
  } else {
    tmp_type = raw_msg.substr(idx1, pos - idx1);
  }
  if (tmp_type.find_first_not_of("0123456789") != std::string::npos) {
    // type cmd
    cmd = tmp_type;
    raw_cmd = cmd;
    ft_upper(cmd);
    std::map<std::string, Command>::const_iterator it = stoe.find(cmd);
    if (it != stoe.end()) {
      cmd_type = stoe.at(cmd);
      if (pos == std::string::npos) {
        return;
      }
    } else {
      set_cmd_type(NONE);
      numeric = "421";
      params.push_back(":Unknown command");
      return;
    }
  } else {
    // type numeric
    numeric = tmp_type;
  }

  // check trailing before get parameters
  idx2 = raw_msg.rfind(" :");
  if (idx2 != std::string::npos) {
    // trailing exist
    tmp_trailing = raw_msg.substr(idx2 + 2);
  } else {
    idx2 = raw_msg.length();
  }

  // get parameters
  idx1 = pos;
  std::string params_str = raw_msg.substr(idx1, idx2 - idx1);
  ft_split(params_str, " ", params);
  for (std::size_t i = 0; i < params.size(); i++) {
    if (params[i].find_first_of("\0\n\t\v\f\r") != std::string::npos) {
      set_cmd_type(ERROR);
      params.push_back(":Invalid parameter");
      return;
    }
  }

  if (tmp_trailing.length() != 0) {
    params.push_back(tmp_trailing);
  }
  return;
}

void Message::set_source(const std::string& input) { source = input; }

void Message::set_cmd(const std::string& input) {
  cmd = input;
  cmd_type = stoe[cmd];
}

void Message::set_cmd_type(const Command input) {
  cmd_type = input;
  cmd = etos[cmd_type];
}

void Message::push_back(const std::string& input) { params.push_back(input); }

void Message::clear(void) { params.clear(); }

void Message::set_numeric(const std::string& input) { numeric = input; }

const std::string& Message::get_raw_msg(void) const { return raw_msg; }

int Message::get_socket_fd(void) const { return socket_fd; }

const std::string& Message::get_source(void) const { return source; }

const std::string& Message::get_raw_cmd(void) const { return raw_cmd; }

const std::string& Message::get_cmd(void) const { return cmd; }

Command Message::get_cmd_type(void) const { return cmd_type; }

const std::vector<std::string>& Message::get_params(void) const {
  return params;
}

std::size_t Message::get_params_size(void) const { return params.size(); }

const std::string& Message::operator[](const int idx) const {
  if (0 <= idx && idx < static_cast<int>(params.size())) {
    return params[idx];
  } else {
    throw std::out_of_range("params vector out of range");
  }
}

const std::string& Message::get_numeric(void) const { return numeric; }

std::string Message::to_raw_msg(void) {
  std::string raw_msg = "";
  std::size_t param_cnt = params.size();
  std::size_t idx = 0;

  if (source.length() != 0) {
    raw_msg += ":";
    raw_msg += source;
    raw_msg += " ";
  }
  if (numeric.length() != 0) {
    raw_msg += numeric;
  } else {
    raw_msg += cmd;
  }
  while (idx < param_cnt) {
    raw_msg += " ";
    raw_msg += params[idx];
    idx++;
  }
  raw_msg += "\r\n";

  return raw_msg;
}

bool Message::starts_with(const std::string& str, const std::string& prefix) const {
    if (str.length() < prefix.length()) {
        return (false);
    }
    for (size_t i = 0; i < prefix.length(); ++i) {
        if (str[i] != prefix[i]) {
            return (false);
        }
    }
    return (true);
}


std::ostream& operator<<(std::ostream& out, Message msg) {
  std::size_t i = 0;

  out << "< Message contents > \n"
      << "fd \t\t: " << msg.get_socket_fd() << '\n'
      << "source\t\t: " << msg.get_source() << '\n'
      << "command\t\t: " << msg.get_cmd() << '\n'
      << "params\t\t: ";
  if (msg.get_params_size() > 0) {
    for (i = 0UL; i + 1 < msg.get_params_size(); i++) {
      out << msg[i] << ", ";
    }
    out << msg[i] << "\n";
  } else {
    out << '\n';
  }
  out << "numeric\t\t: " << msg.get_numeric() << '\n';

  return out;
}

Message Message::rpl_401_invitation(const std::string& source, const std::string& invitingClientNickName, 
                                    const std::string& joiningClientNickName) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("401");
  rpl.push_back(invitingClientNickName);
  rpl.push_back(joiningClientNickName);
  rpl.push_back(":No such nick or channel name");

  return rpl;
}

Message Message::rpl_401_mode_operator(const std::string& source, const std::string& requestingClientNickName, 
                                    const std::string& targetClientNickName) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("401");
  rpl.push_back(requestingClientNickName);
  rpl.push_back(targetClientNickName);
  rpl.push_back(":No such nick or channel name");

  return rpl;
}

Message Message::rpl_432(const std::string& source, const std::string& client,
                         const std::string& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("432");
  rpl.push_back(client);
  rpl.push_back(nick);
  rpl.push_back(":Erroneous nickname");

  /////////////////
  // [DEBUG]
  std::cout << CYAN << rpl << std::endl << rpl.to_raw_msg();
  return rpl;
}

Message Message::rpl_433(const std::string& source, const std::string& client,
                         const std::string& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("433");
  rpl.push_back(client);
  rpl.push_back(nick);
  rpl.push_back(":Nickname is already in use");

  return rpl;
}

Message Message::rpl_451(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("451");
  rpl.push_back(client);
  rpl.push_back(":Connection not registered");

  return rpl;
}

Message Message::rpl_461(const std::string& source, const std::string& client,
                         const std::string& cmd) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("461");
  rpl.push_back(client);
  rpl.push_back(cmd);
  rpl.push_back(":Not enough parameters");

  return rpl;
}

Message Message::rpl_462(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("462");
  rpl.push_back(client);
  rpl.push_back(":Connection already registered");

  return rpl;
}

Message Message::rpl_464(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("464");
  rpl.push_back(client);
  rpl.push_back(":Password incorrect");

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
Message Message::rpl_353(const std::string& source, Channel& channel,
                         const std::string& nickName,
                         const std::string& channelName) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("353");
  rpl.push_back(nickName);
  rpl.push_back("=");
  rpl.push_back(channelName);
  // rpl.push_back(":");

  const std::map<std::string, User&>& clientMap =
      channel.get_channel_client_list();
  // const std::vector<User>& operatorVec = channel.get_channel_operator_list();
  const std::map<int, std::string>& operatorMap =
      channel.get_channel_operator_list();

  std::map<std::string, User&>::const_reverse_iterator cit;

  std::string user_list_str;

  for (cit = clientMap.rbegin(); cit != clientMap.rend(); cit++) {
    // std::vector<User>::const_iterator citOp = operatorMap.begin();
    std::map<int, std::string>::const_iterator citOp = operatorMap.begin();
    const std::string nickName = cit->first;
    const User& user = cit->second;
    for (; citOp != operatorMap.end(); ++citOp) {
      if (citOp->second == nickName) break;
    }
    if (citOp != operatorMap.end()) {
      std::string opNickName = "@" + nickName;
      user_list_str += opNickName;
    } else {
      user_list_str += nickName;
    }
    user_list_str += std::string(" ");
  }
  rpl.push_back(std::string(":") + user_list_str);

  return rpl;
}

Message Message::rpl_366(const std::string& source, const std::string& client,
                         const std::string& channelName) {
  // :irc.example.net 366 lfkn__ #a :End of NAMES list\r
  Message rpl;

  std::string str = std::string(":") + std::string("End of NAMES list");
  rpl.source = source;
  rpl.set_numeric("366");
  rpl.push_back(client);
  rpl.push_back(channelName);
  rpl.push_back(str);
  return rpl;
}

Message Message::rpl_403(const std::string& source, const std::string& nickName,
                         const Message& msg) {
  /*
      ERR_NOSUCHCHANNEL (403)
        "<client> <channel> :No such channel"
      Indicates that no channel can be found for the supplied channel name.
      The text used in the last param of this message may vary.
  */

  // :irc.example.net 403 lfkn__ #asdfw :No such channel\r
  // :ft_irc 403 lfkn #asdf :
  // /kick #없는채널명 어딘가에있는클라이언트명 으로 하면 이 에러 나옴

  Message rpl;
  std::string sentence;
  std::string space = std::string(" ");
  std::string colon = std::string(":");
  rpl.set_source(source);
  rpl.set_numeric("403");
  sentence = nickName + space + msg.get_params()[0] + space + colon +
             std::string("No such channel");
  rpl.push_back(sentence);

  return rpl;
}

Message Message::rpl_482(const std::string& source, const std::string& nickName,
                         const Message& msg) {
  /*
    ERR_CHANOPRIVSNEEDED (482) 
    "<client> <channel> :You're not channel operator"
    Indicates that a command failed because the client does not have the appropriate channel privileges. This numeric can apply for different prefixes such as halfop, operator, etc. The text used in the last param of this message may vary.
  */

  // < 2024/05/11 14:33:05.000862471  length=62 from=2493 to=2554
  // :irc.example.net 482 dy__ #test :Your privileges are too low\r

  Message rpl;
  std::string sentence;
  std::string space = std::string(" ");
  std::string colon = std::string(":");
  rpl.set_source(source);
  rpl.set_numeric("483");
  sentence = nickName + space + msg.get_params()[0] + space + colon +
             std::string("Your privileges are too low");
  rpl.push_back(sentence);

  return rpl;
}

Message Message::rpl_401(const std::string& source, const std::string& nickName, const Message& msg) {
  /*
    ERR_NOSUCHNICK (401)
      "<client> <nickname> :No such nick/channel"
      Indicates that no client can be found for the supplied nickname. The text
    used in the last param of this message may vary.
  */

  // :irc.example.net 401 lfkn slkfdn :No such nick or channel name\r
  // (hostname) (nickname) (msg)

  Message rpl;

  std::string sentence;
  std::string space = std::string(" ");
  std::string colon = std::string(":");
  rpl.set_source(source);
  rpl.set_numeric("401");
  sentence = nickName + space + msg.get_params()[1] + space + colon +
             std::string("No such nick or channel name");
  rpl.push_back(sentence);

  return rpl;
}

// Message Message::rpl_442() {
//   /*
//     ERR_NOTONCHANNEL (442)
//     "<client> <channel> :You're not on that channel"
//     Returned when a client tries to perform a channel-affecting command on a
//     channel which the client isn’t a part of.

//     채널에 속했든 안했든
//      /kick [#CHANNELNAME] [CLIENTNAME] 이런식으로 명령이 가능한데 만약 채널에
//      속하지 않은 유저가 명령을 내릴경우 442에러를 뱉어주면 됨.

//   */
// }

Message Message::rpl_341(const std::string& source, User& user, const Message& msg) 
{
/*
  < 2024/05/10 14:21:06.000167118  length=42 from=7812 to=7853
  :dy3!~memememe@localhost 341 dy1 dy3 new\r
*/

  Message rpl;

  std::string incomingClientNickName = msg.get_params()[0];
  std::string invitingClientNickName = user.get_nick_name();
  std::string userName = user.get_user_name();
  std::string targetChannelStr = msg.get_params()[1];
  std::string exclamationMark = std::string("!");
  std::string at = std::string("@");
  std::string localhost = std::string("localhost");
  rpl.set_source(incomingClientNickName + exclamationMark + userName + at + localhost);

  rpl.set_numeric("341");
  rpl.push_back(invitingClientNickName);
  rpl.push_back(incomingClientNickName);
  rpl.push_back(targetChannelStr);

  return rpl;
}

Message Message::rpl_473(const std::string& source, std::string joiningClientName, const Message& msg) {
  // :irc.example.net 473 dy_ #test :Cannot join channel (+i) -- Invited users only\r
  Message rpl;

  std::string joiningChannelName = msg.get_params()[0];
  std::string exclamationMark = std::string("!");
  std::string at = std::string("@");
  std::string localhost = std::string("localhost");
  rpl.set_source(source);
  rpl.set_numeric("473");
  rpl.push_back(joiningClientName);
  rpl.push_back(joiningChannelName);
  rpl.push_back(":");
  rpl.push_back("Cannot join channel (+i) -- Invited users only");
  
  return rpl;
}

// Message Message::rpl_443(const std::string& source, std::string joiningClientName, const Message& msg) {
//   // :irc.example.net 443 dy dy #test :is already on channel\r
//   Message rpl;

//   std::string joiningChannelName = msg.get_params()[0];
//   rpl.set_source(source);
//   rpl.set_numeric("443");
//   rpl.push_back(joiningClientName);
//   rpl.push_back(joiningClientName);
//   rpl.push_back(joiningChannelName);
//   rpl.push_back(":");
//   rpl.push_back("is already on channel");
  
//   return rpl;
// }

Message Message::rpl_441(const std::string& source, const Message& msg) {
  // < 2024/06/12 16:19:38.000655856  length=65 from=19794 to=19858
  // :irc.example.net 441 dy_ dy__ #zzz :They aren't on that channel\r

  Message rpl;

  std::string channelName = msg.get_params()[0];
  std::string requestingClientNickName = msg.get_params()[1];
  std::string targetClientNickName = msg.get_params()[2];
  rpl.set_source(source);
  rpl.set_numeric("441");
  rpl.push_back(requestingClientNickName);
  rpl.push_back(targetClientNickName);
  rpl.push_back(channelName);
  rpl.push_back(":");
  rpl.push_back(":");
  rpl.push_back("They aren't on that channel");

  return (rpl);
}

Message Message::rpl_472(const std::string& source, const std::string nickName, char c, const Message& msg) { 
  // :irc.example.net 472 dy 1 :is unknown mode char for #test\r

  Message rpl;
  std::string character;
  character = c;
  std::string channelName = msg.get_params()[0];
  rpl.set_source(source);
  rpl.set_numeric("472");
  rpl.push_back(nickName);
  rpl.push_back(character);
  rpl.push_back(":");
  rpl.push_back("is unknown mode char for ");
  rpl.push_back(channelName);

  return (rpl);
}

Message Message::rpl_471(const std::string& source, const std::string nickName, const std::string channelName) {
  // :irc.example.net 471 dy___ #test :Cannot join channel (+l) -- Channel is full, try later\r
  Message rpl;
  
  rpl.set_source(source);
  rpl.set_numeric("471");
  rpl.push_back(nickName);
  rpl.push_back(channelName);
  rpl.push_back(":");
  rpl.push_back("Cannot join channel (+l) -- Channel is full, try later");

  return rpl;
}