#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <ctime>
#include <map>
#include <string>

#include "User.hpp"
#include "custom_exception.hpp"

typedef std::string String;

#define CHANTYPES "#&"
#define CHANMODES "k,l,ist"
#define CHANLIMIT "#&:10"
#define CHANNELLEN 50
#define TOPICLEN 490
#define KICKLEN 400

#define FLAG_I (1 << 0)
#define FLAG_T (1 << 1)
#define FLAG_K (1 << 2)
#define FLAG_O (1 << 3)
#define FLAG_L (1 << 4)
#define FLAG_S (1 << 5)

#define OPERATOR_PREFIX "@"
#define HALFOP_PREFIX "%"
#define VOICE_PREFIX "+"

#define REGULAR_CHANNEL 0
#define REGULAR_CHANNEL_PREFIX "#"
#define LOCAL_CHANNEL 1
#define LOCAL_CHANNEL_PREFIX "&"

#define INIT_CLIENT_LIMIT 10

class Channel {
 private:
  const std::string channel_name;
  char channel_type;
  const std::time_t created_time;
  std::string pwd;
  int client_limit;
  bool invite_only;
  std::string topic;
  std::string topic_set_nick;
  std::time_t topic_set_time;
  std::map<std::string, User&> client_list;
  std::map<std::string, User&> banned_list;
  std::map<std::string, User&> operator_list;
  /* <MODE>
    i : set / unset Invite only channel
    t : set / unset the restrictions of the TOPIC command to channel operators
    k : set / unset the channel key (password)
    o : give / take channel operator privilege
    l : set / unset the user limit to channel */
  int mode;

  // not use
  Channel();
  Channel& operator=(const Channel& other);

 public:
  // OCCF
  Channel(const std::string& _channel_name, char _channel_type);
  Channel(const Channel& other);
  ~Channel();

  // GETTER && SETTER
  const std::string& get_channel_name(void) const;
  char get_channel_type(void) const;
  const std::string& get_password(void) const;
  int get_client_limit(void) const;
  bool get_invite_only(void) const;
  const std::string& get_topic(void) const;
  const std::string& get_topic_set_nick(void) const;
  std::time_t get_topic_set_time(void) const;
  std::map<std::string, User&>& get_client_list(void);
  std::map<std::string, User&>& get_banned_list(void);
  std::map<std::string, User&>& get_operator_list(void);
  const std::map<std::string, User&>& get_client_list(void) const;
  const std::map<std::string, User&>& get_banned_list(void) const;
  const std::map<std::string, User&>& get_operator_list(void) const;

  void set_password(const std::string& _pwd);
  void set_client_limit(int _client_limit);
  void set_invite_only(bool _invite_only);
  void set_topic(const std::string& _topic);
  void set_topic_set_nick(const std::string& _nick);
  void set_topic_set_time(std::time_t _t);

  // METHOD FUNCTIONS
  void add_client(User& user);
  void add_operator(User& user);

  bool is_operator(const std::string& nickname) const;
  void remove_client(const std::string& nickname);
  void remove_operator(const std::string& nickname);
  bool chk_client_join(const std::string& nickname) const;
  void change_client_nickname(const std::string& old_nick,
                              const std::string& new_nick);

  /* MODE */
  void set_mode(int flag);
  void unset_mode(int flag);
  bool chk_mode(int flag) const;
};

#ifdef DEBUG

#include <iostream>
std::ostream& operator<<(std::ostream&, const Channel& channel);

#endif

#endif