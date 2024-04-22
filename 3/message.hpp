#ifndef MESSAGE_HPP
#define MESSAGE_HPP

/*
The protocol messages must be extracted from the contiguous stream of
octets.  The current solution is to designate two characters, CR and
LF, as message separators.   Empty  messages  are  silently  ignored,
which permits  use  of  the  sequence  CR-LF  between  messages
without extra problems.

The extracted message is parsed into the components <prefix>,
<command> and list of parameters matched either by <middle> or
<trailing> components.

The BNF representation for this is:

<message>  ::= [':' <prefix> <SPACE> ] <command> <params> <crlf>
<prefix>   ::= <servername> | <nick> [ '!' <user> ] [ '@' <host> ]
<command>  ::= <letter> { <letter> } | <number> <number> <number>
<SPACE>    ::= ' ' { ' ' }
<params>   ::= <SPACE> [ ':' <trailing> | <middle> <params> ]

<middle>   ::= <Any *non-empty* sequence of octets not including SPACE
               or NUL or CR or LF, the first of which may not be ':'>
<trailing> ::= <Any, possibly *empty*, sequence of octets not including
                 NUL or CR or LF>

<crlf>     ::= CR LF

NOTES:

  1)    <SPACE> is consists only of SPACE character(s) (0x20).
        Specially notice that TABULATION, and all other control
        characters are considered NON-WHITE-SPACE.

  2)    After extracting the parameter list, all parameters are equal,
        whether matched by <middle> or <trailing>. <Trailing> is just
        a syntactic trick to allow SPACE within parameter.

  3)    The fact that CR and LF cannot appear in parameter strings is
        just artifact of the message framing. This might change later.

  4)    The NUL character is not special in message framing, and
        basically could end up inside a parameter, but as it would
        cause extra complexities in normal C string handling. Therefore
        NUL is not allowed within messages.

  5)    The last parameter may be an empty string.

  6)    Use of the extended prefix (['!' <user> ] ['@' <host> ]) must
        not be used in server to server communications and is only
        intended for server to client messages in order to provide
        clients with more useful information about who a message is
        from without the need for additional queries.
*/

#include <map>
#include <string>
#include <vector>

#include "string_func.hpp"

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
  NONE
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
    {USERHOST, "USERHOST"}, {WALLOPS, "WALLOPS"},
    {NONE, "NONE"}};

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
    {"USERHOST", USERHOST}, {"WALLOPS", WALLOPS},
    {"NONE", NONE}};

class Message {
 private:
  std::string raw_msg;
  std::string source;
  std::string cmd;
  Command cmd_type;
  std::vector<std::string> params;

  std::string numeric;
  std::string ret_msg;

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