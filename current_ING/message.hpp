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
#include "Channel.hpp"

class Channel;

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
  NONE,
  NORPL,
};

#define BLACK       "\033[0;30m"
#define RED         "\033[0;31m"
#define GREEN       "\033[0;32m"
#define GREEN_BOLD  "\033[1;32m"
#define YELLOW      "\033[0;33m"
#define BLUE        "\033[0;34m"
#define PURPLE      "\033[0;35m"
#define CYAN        "\033[0;36m"
#define WHITE       "\033[0;37m"
#define DEF_COLOR   "\033[0;39m"
#define LF          "\e[1K\r"

class Message {
private:
  static std::map<Command, std::string>               etos;
  static std::map<std::string, Command>               stoe;

  const std::string                                   raw_msg;
  const int                                           socket_fd;
  std::string                                         source;
  std::string                                         raw_cmd;
  std::string                                         cmd;
  Command                                             cmd_type;
  std::vector<std::string>                            params;

  std::string                                         numeric;

public:
  static void                                         map_init(void);

  Message();
  Message(int socket_fd, const std::string& _raw_msg);

  void                                                set_source(const std::string& input);
  void                                                set_cmd(const std::string& input);
  void                                                set_cmd_type(const Command input);
  void                                                push_back(const std::string& input);
  void                                                clear(void);
  void                                                set_numeric(const std::string& input);

  const std::string&                                  get_raw_msg(void) const;
  const int                                           get_socket_fd(void) const;
  const std::string&                                  get_source(void) const;
  const std::string&                                  get_raw_cmd(void) const;
  const std::string&                                  get_cmd(void) const;
  const Command                                       get_cmd_type(void) const;
  const std::vector<std::string>&                     get_params(void) const;
  const std::size_t                                   get_params_size(void) const;
  const std::string&                                  get_numeric(void) const;

  std::string to_raw_msg(void);


  static Message                                      rpl_401(const std::string& source, const std::string& client, const std::string& nick);
  static Message                                      rpl_432(const std::string& source, const std::string& client, const std::string& nick);
  static Message                                      rpl_433(const std::string& source, const std::string& client, const std::string& nick);
  static Message                                      rpl_461(const std::string& source, const std::string& client, const std::string& cmd);
  static Message                                      rpl_451(const std::string& source, const std::string& client);
  static Message                                      rpl_462(const std::string& source, const std::string& client);
  static Message                                      rpl_464(const std::string& source, const std::string& client);
  static Message                                      rpl_353(const std::string& source, Channel& channel, const std::string& nickName, const std::string& channelName);
  static Message                                      rpl_366(const std::string& source, const std::string& client, const std::string& channelName);

  // KICK
  static Message                                      rpl_401(const std::string& source, const std::string& nickName, const Message& msg);
  static Message                                      rpl_403(const std::string& source, const std::string& nickName, const Message& msg);
  //   static Message                                      rpl_422();
  
  const std::string& operator[](const int idx) const;
};

std::ostream& operator<<(std::ostream& out, Message msg);

#endif
