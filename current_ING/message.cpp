#include "message.hpp"
#include <iostream> // [DEBUG]
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
    cmd = raw_msg.substr(idx1);
  } else {
    cmd = raw_msg.substr(idx1, pos - idx1);
  }
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
  for (int i = 0; i < params.size(); i++) {
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

const int Message::get_socket_fd(void) const { return socket_fd; }

const std::string& Message::get_source(void) const { return source; }

const std::string& Message::get_raw_cmd(void) const { return raw_cmd; }

const std::string& Message::get_cmd(void) const { return cmd; }

const Command Message::get_cmd_type(void) const { return cmd_type; }

const std::vector<std::string>& Message::get_params(void) const {
  return params;
}

const std::size_t Message::get_params_size(void) const { return params.size(); }

const std::string& Message::operator[](const int idx) const {
  if (0 <= idx && idx < params.size()) {
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

std::ostream& operator<<(std::ostream& out, Message msg) {
  int i = 0;

  out << "< Message contents > \n"
      << "fd \t\t: " << msg.get_socket_fd() << '\n'
      << "source\t\t: " << msg.get_source() << '\n'
      << "command\t\t: " << msg.get_cmd() << '\n'
      << "params\t\t: ";
  if (msg.get_params_size() > 0) {
    for (i = 0; i < msg.get_params_size() - 1; i++) {
      out << msg[i] << ", ";
    }
    out << msg[i] << "\n";
  } else {
    out << '\n';
  }
  out << "numeric\t\t: " << msg.get_numeric() << '\n';

  return out;
}

Message Message::rpl_401(const std::string& source, const std::string& client,
                         const std::string& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("401");
  rpl.push_back(client);
  rpl.push_back(nick);
  rpl.push_back(":No such nick or channel name");

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
  return rpl;
}

Message Message::rpl_432(const std::string& source, const std::string& client, const std::string& nick) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("432");
  rpl.push_back(client);
  rpl.push_back(nick);
  rpl.push_back(":Erroneous nickname");

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
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

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
  return rpl;
}

Message Message::rpl_451(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("451");
  rpl.push_back(client);
  rpl.push_back(":Connection not registered");

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
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
  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
  return rpl;
}

Message Message::rpl_462(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("462");
  rpl.push_back(client);
  rpl.push_back(":Connection already registered");

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
  return rpl;
}

Message Message::rpl_464(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("464");
  rpl.push_back(client);
  rpl.push_back(":Password incorrect");

  /////////////////
  // [DEBUG]
  std::cout << CYAN
            << rpl
            << std::endl
            << rpl.to_raw_msg();
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
Message Message::rpl_353(const std::string& source, const std::string& client, const std::string& channelName) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("353");
  rpl.push_back(client);
  rpl.push_back("=");
  rpl.push_back(channelName);
  rpl.push_back(":@" + client);
  
  return rpl;
}

Message Message::rpl_366(const std::string& source, const std::string& client) {
  Message rpl;

  rpl.source = source;
  rpl.set_numeric("366");
  rpl.push_back(client);
  return rpl;
}


