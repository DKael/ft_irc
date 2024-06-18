#include "Message.hpp"

std::map<Command, String> Message::etos;
std::map<String, Command> Message::stoe;

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

Message::Message(int _socket_fd, const String& _raw_msg)
    : raw_msg(ft_strip(_raw_msg)),
      socket_fd(_socket_fd),
      trailing_exist(false) {
  if (raw_msg.length() == 0) {
    set_cmd_type(NONE);
    numeric = "421";
    params.push_back(":Unknown command");
    return;
  }
  std::size_t idx1 = 0;
  std::size_t idx2 = 0;
  std::size_t pos = 0;
  String tmp_trailing;
  String tmp_type;

  // check source
  if (raw_msg[0] == ':') {
    // source exist
    pos = raw_msg.find_first_of(' ');
    if (pos == String::npos) {
      set_cmd_type(ERROR);
      params.push_back(":Prefix without command");
      return;
    }
    source = raw_msg.substr(1, pos - 1);
    if (source.find_first_of("\0\n\t\v\f\r") != String::npos) {
      set_cmd_type(ERROR);
      params.push_back(String(":Invalid prefix \"") + source + String("\""));
      return;
    }
  }

  // get command
  pos = raw_msg.find_first_not_of(' ', pos);
  if (pos == String::npos) {
    set_cmd_type(ERROR);
    params.push_back(":Prefix without command");
    return;
  }
  idx1 = pos;
  pos = raw_msg.find_first_of(' ', pos);
  if (pos == String::npos) {
    tmp_type = raw_msg.substr(idx1);
  } else {
    tmp_type = raw_msg.substr(idx1, pos - idx1);
  }
  if (tmp_type.find_first_not_of("0123456789") != String::npos) {
    // type cmd
    cmd = tmp_type;
    raw_cmd = cmd;
    ft_upper(cmd);
    std::map<String, Command>::const_iterator it = stoe.find(cmd);
    if (it != stoe.end()) {
      cmd_type = stoe.at(cmd);
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
  if (pos == String::npos) {
    return;
  }

  // check trailing before get parameters
  idx2 = raw_msg.find(" :", pos);
  if (idx2 != String::npos) {
    // trailing exist
    tmp_trailing = raw_msg.substr(idx2 + 2);
    trailing_exist = true;
  } else {
    idx2 = raw_msg.length();
  }

  // get parameters
  idx1 = pos;
  String params_str = raw_msg.substr(idx1, idx2 - idx1);
  ft_split(params_str, " ", params);
  for (std::size_t i = 0; i < params.size(); i++) {
    if (params[i].find_first_of("\0\n\t\v\f\r") != String::npos) {
      set_cmd_type(ERROR);
      params.push_back(":Invalid parameter");
      return;
    }
  }

  if (trailing_exist == true) {
    params.push_back(tmp_trailing);
  }
  return;
}

void Message::set_source(const String& input) { source = input; }

void Message::set_cmd(const String& input) {
  cmd = input;
  cmd_type = stoe[cmd];
}

void Message::set_cmd_type(const Command input) {
  cmd_type = input;
  cmd = etos[cmd_type];
}

void Message::push_back(const String& input) { params.push_back(input); }

void Message::clear(void) { params.clear(); }

void Message::set_numeric(const String& input) { numeric = input; }

void Message::set_trailing_exist(bool input) { trailing_exist = input; }

const String& Message::get_raw_msg(void) const { return raw_msg; }

int Message::get_socket_fd(void) const { return socket_fd; }

const String& Message::get_source(void) const { return source; }

const String& Message::get_raw_cmd(void) const { return raw_cmd; }

const String& Message::get_cmd(void) const { return cmd; }

Command Message::get_cmd_type(void) const { return cmd_type; }

const std::vector<String>& Message::get_params(void) const { return params; }

std::vector<String>& Message::get_params(void) { return params; }

std::size_t Message::get_params_size(void) const { return params.size(); }

String& Message::operator[](const int idx) {
  if (0 <= idx && idx < static_cast<int>(params.size())) {
    return params[idx];
  } else {
    throw std::out_of_range("params vector out of range");
  }
}

const String& Message::operator[](const int idx) const {
  if (0 <= idx && idx < static_cast<int>(params.size())) {
    return params[idx];
  } else {
    throw std::out_of_range("params vector out of range");
  }
}

const String& Message::get_numeric(void) const { return numeric; }

bool Message::get_trailing_exist(void) const { return trailing_exist; }

String Message::to_raw_msg(void) {
  String raw_msg = "";
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

#ifdef DEBUG

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

#endif