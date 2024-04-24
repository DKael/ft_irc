#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <map>
#include <string>
#include <vector>

enum Command {
  CAP = 0,
  AUTHENTICATE,
  PASS,
  NICK,
  USER,
  PING,
  PONG,
  OPER,
  QUIT,
  ERROR,
  JOIN,
  PART,
  TOPIC,
  NAMES,
  LIST,
  INVITE,
  KICK,
  MOTD,
  VERSION,
  ADMIN,
  CONNECT,
  LUSERS,
  TIME,
  STATS,
  HELP,
  INFO,
  MODE,
  PRIVMSG,
  NOTICE,
  WHO,
  WHOIS,
  WHOWAS,
  KILL,
  REHASH,
  RESTART,
  SQUIT,
  AWAY,
  LINKS,
  USERHOST,
  WALLOPS,
};

const std::map<Command, std::string> etos = {
    {CAP, "CAP"},           {AUTHENTICATE, "AUTHENTICATE"},
    {PASS, "PASS"},         {NICK, "NICK"},
    {USER, "USER"},         {PING, "PING"},
    {PONG, "PONG"},         {OPER, "OPER"},
    {QUIT, "QUIT"},         {ERROR, "ERROR"},
    {JOIN, "JOIN"},         {PART, "PART"},
    {TOPIC, "TOPIC"},       {NAMES, "NAMES"},
    {LIST, "LIST"},         {INVITE, "INVITE"},
    {KICK, "KICK"},         {MOTD, "MOTD"},
    {VERSION, "VERSION"},   {ADMIN, "ADMIN"},
    {CONNECT, "CONNECT"},   {LUSERS, "LUSERS"},
    {TIME, "TIME"},         {STATS, "STATS"},
    {HELP, "HELP"},         {INFO, "INFO"},
    {MODE, "MODE"},         {PRIVMSG, "PRIVMSG"},
    {NOTICE, "NOTICE"},     {WHO, "WHO"},
    {WHOIS, "WHOIS"},       {WHOWAS, "WHOWAS"},
    {KILL, "KILL"},         {REHASH, "REHASH"},
    {RESTART, "RESTART"},   {SQUIT, "SQUIT"},
    {AWAY, "AWAY"},         {LINKS, "LINKS"},
    {USERHOST, "USERHOST"}, {WALLOPS, "WALLOPS"}};

const std::map<std::string, Command> stoe = {
    {"CAP", CAP},           {"AUTHENTICATE", AUTHENTICATE},
    {"PASS", PASS},         {"NICK", NICK},
    {"USER", USER},         {"PING", PING},
    {"PONG", PONG},         {"OPER", OPER},
    {"QUIT", QUIT},         {"ERROR", ERROR},
    {"JOIN", JOIN},         {"PART", PART},
    {"TOPIC", TOPIC},       {"NAMES", NAMES},
    {"LIST", LIST},         {"INVITE", INVITE},
    {"KICK", KICK},         {"MOTD", MOTD},
    {"VERSION", VERSION},   {"ADMIN", ADMIN},
    {"CONNECT", CONNECT},   {"LUSERS", LUSERS},
    {"TIME", TIME},         {"STATS", STATS},
    {"HELP", HELP},         {"INFO", INFO},
    {"MODE", MODE},         {"PRIVMSG", PRIVMSG},
    {"NOTICE", NOTICE},     {"WHO", WHO},
    {"WHOIS", WHOIS},       {"WHOWAS", WHOWAS},
    {"KILL", KILL},         {"REHASH", REHASH},
    {"RESTART", RESTART},   {"SQUIT", SQUIT},
    {"AWAY", AWAY},         {"LINKS", LINKS},
    {"USERHOST", USERHOST}, {"WALLOPS", WALLOPS}
};

class Message {
 private:
  std::string raw_msg;
  Command type;
  std::vector<std::string> params;

  // not use
  Message();

 public:
  Message(const std::string& _raw_msg);
};

class SendMessage : public Message {
 private:
 public:
};

class RecvMessage : public Message {
 private:
 public:
};

#endif