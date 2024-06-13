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

#define CHAN_FLAG_I (1 << 0)
#define CHAN_FLAG_T (1 << 1)
#define CHAN_FLAG_K (1 << 2)
#define CHAN_FLAG_O (1 << 3)
#define CHAN_FLAG_L (1 << 4)
#define CHAN_FLAG_S (1 << 5)

#define OPERATOR_PREFIX '@'
#define HALFOP_PREFIX '%'
#define VOICE_PREFIX '+'

#define REGULAR_CHANNEL 0
#define REGULAR_CHANNEL_PREFIX '#'
#define LOCAL_CHANNEL 1
#define LOCAL_CHANNEL_PREFIX '&'

#define INIT_USER_LIMIT 50

class Channel {
 private:
  const String channel_name;
  char channel_type;
  const std::time_t created_time;
  String pwd;
  int user_limit;
  bool invite_only;
  String topic;
  String topic_set_nick;
  std::time_t topic_set_time;
  std::map<String, User&> user_list;
  std::map<String, User&> banned_list;
  std::map<String, User&> operator_list;
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
  Channel(const String& _channel_name);
  Channel(const Channel& other);
  ~Channel();

  // GETTER && SETTER
  const String& get_channel_name(void) const;
  char get_channel_type(void) const;
  const String& get_password(void) const;
  int get_user_limit(void) const;
  bool get_invite_only(void) const;
  const String& get_topic(void) const;
  const String& get_topic_set_nick(void) const;
  std::time_t get_topic_set_time(void) const;
  std::map<String, User&>& get_user_list(void);
  std::map<String, User&>& get_banned_list(void);
  std::map<String, User&>& get_operator_list(void);
  const std::map<String, User&>& get_user_list(void) const;
  const std::map<String, User&>& get_banned_list(void) const;
  const std::map<String, User&>& get_operator_list(void) const;

  const String& get_user_list_str(bool is_joined) const;

  void set_password(const String& _pwd);
  void set_user_limit(int _user_limit);
  void set_invite_only(bool _invite_only);
  void set_topic(const String& _topic);
  void set_topic_set_nick(const String& _nick);
  void set_topic_set_time(std::time_t _t);

  // METHOD FUNCTIONS
  void add_user(User& user);
  void add_operator(User& user);

  bool is_operator(const String& nickname) const;
  void remove_user(const String& nickname);
  void remove_operator(const String& nickname);
  bool chk_user_join(const String& nickname) const;
  void change_user_nickname(const String& old_nick, const String& new_nick);

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