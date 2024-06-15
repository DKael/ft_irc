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
        intended for server to user messages in order to provide
        users with more useful information about who a message is
        from without the need for additional queries.
*/

#include <map>
#include <string>
#include <vector>

#include "Channel.hpp"
#include "User.hpp"
#include "string_func.hpp"

typedef std::string String;

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

class Message {
 private:
  static std::map<Command, String> etos;
  static std::map<String, Command> stoe;

  const String raw_msg;
  const int socket_fd;
  String source;
  String raw_cmd;
  String cmd;
  Command cmd_type;
  std::vector<String> params;
  bool trailing_exist;

  String numeric;

 public:
  static void map_init(void);

  Message();
  Message(int socket_fd, const String& _raw_msg);

  void set_source(const String& input);
  void set_cmd(const String& input);
  void set_cmd_type(const Command input);
  void push_back(const String& input);
  void clear(void);
  void set_numeric(const String& input);
  void set_trailing_exist(bool input);

  const String& get_raw_msg(void) const;
  int get_socket_fd(void) const;
  const String& get_source(void) const;
  const String& get_raw_cmd(void) const;
  const String& get_cmd(void) const;
  Command get_cmd_type(void) const;
  const std::vector<String>& get_params(void) const;
  std::vector<String>& get_params(void);
  std::size_t get_params_size(void) const;
  const String& get_numeric(void) const;
  bool get_trailing_exist(void) const;

  String& operator[](const int idx);
  const String& operator[](const int idx) const;

  String to_raw_msg(void);
};

Message rpl_001(const String& source, const String& user,
                const String& user_source);
Message rpl_002(const String& source, const String& user,
                const String& server_name, const String& server_version);
Message rpl_003(const String& source, const String& user,
                const String& server_created_time);
Message rpl_004(const String& source, const String& user,
                const String& server_name, const String& server_version,
                const String& available_user_modes,
                const String& available_channel_modes);
Message rpl_005(const String& source, const String& user,
                std::vector<String> specs);
Message rpl_221(const String& source, const String& user,
                const String& user_modes);
Message rpl_315(const String& source, const String& user, const String& mask);
Message rpl_321(const String& source, const String& user);
Message rpl_322(const String& source, const String& user, const String& channel,
                const String& client_count, const String& topic);
Message rpl_323(const String& source, const String& user);
Message rpl_324(const String& source, const String& user, const String& channel,
                const String& modestring,
                const std::vector<String> mode_arguments);
Message rpl_329(const String& source, const String& user, const String& channel,
                const String& creationtime);
Message rpl_331(const String& source, const String& user,
                const String& channel);
Message rpl_332(const String& source, const String& user, const String& channel,
                const String& topic);
Message rpl_333(const String& source, const String& user, const String& channel,
                const String& nick, const String& setat);
Message rpl_341(const String& source, const String& user, const String& nick,
                const String& channel);
Message rpl_352(const String& source, const String& user, const String& channel,
                const User& _u, const String& server, const String& flags,
                int hopcount);
Message rpl_353(const String& source, const String& user, const String& symbol,
                const String& channel, const String& nicks);
Message rpl_366(const String& source, const String& user,
                const String& channel);
Message rpl_401(const String& source, const String& user,
                const String& nickname);
Message rpl_403(const String& source, const String& nickname,
                const String& channel);
Message rpl_404(const String& source, const String& user,
                const String& channel);
Message rpl_405(const String& source, const String& user,
                const String& channel);
Message rpl_409(const String& source, const String& nickname);
Message rpl_411(const String& source, const String& user,
                const String& command);
Message rpl_412(const String& source, const String& user);
Message rpl_421(const String& source, const String& user,
                const String& command);
Message rpl_432(const String& source, const String& user, const String& nick);
Message rpl_433(const String& source, const String& user, const String& nick);
Message rpl_441(const String& source, const String& user, const String& nick,
                const String& channel);
Message rpl_442(const String& source, const String& user,
                const String& channel);
Message rpl_443(const String& source, const String& user, const String& nick,
                const String& channel);
Message rpl_451(const String& source, const String& user);
Message rpl_461(const String& source, const String& user,
                const String& command);
Message rpl_462(const String& source, const String& user);
Message rpl_464(const String& source, const String& user);
Message rpl_471(const String& source, const String& user,
                const String& channel);
Message rpl_472(const String& source, const String& user,
                const String& modechar, const String& channel);
Message rpl_473(const String& source, const String& user,
                const String& channel);
Message rpl_475(const String& source, const String& user,
                const String& channel);
Message rpl_482(const String& source, const String& user,
                const String& channel);
Message rpl_501(const String& source, const String& user, const String& mode);
Message rpl_502(const String& source, const String& user);

#ifdef DEBUG

#include <iostream>

#define BLACK "\033[0;30m"
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define GREEN_BOLD "\033[1;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define PURPLE "\033[0;35m"
#define CYAN "\033[0;36m"
#define WHITE "\033[0;37m"
#define DEF_COLOR "\033[0;39m"
#define LF "\e[1K\r"

std::ostream& operator<<(std::ostream& out, Message msg);

#endif

#endif