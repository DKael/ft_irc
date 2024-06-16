#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <ctime>
#include <map>
#include <string>

#include "User.hpp"
#include "custom_exception.hpp"

typedef std::string String;

#define CHANTYPES "#&"
#define AVAILABLE_CHANNEL_MODES "iklost"
#define CHANMODES ",k,l,ist"
#define CHANMODE_A ""
#define CHANMODE_B "ko"
#define CHANMODE_C "l"
#define CHANMODE_D "ist"
#define CHANLIMIT "#&:10"
#define PREFIX "(oh)@%"
#define CHANNELLEN 50
#define TOPICLEN 307
#define KICKLEN 307

#define CHAN_FLAG_O_CHAR 'o'

#define CHAN_FLAG_K_CHAR 'k'
#define CHAN_FLAG_K (1 << 0)

#define CHAN_FLAG_L_CHAR 'l'
#define CHAN_FLAG_L (1 << 1)

#define CHAN_FLAG_I_CHAR 'i'
#define CHAN_FLAG_I (1 << 2)

#define CHAN_FLAG_S_CHAR 's'
#define CHAN_FLAG_S (1 << 3)

#define CHAN_FLAG_T_CHAR 't'
#define CHAN_FLAG_T (1 << 4)

#define OPERATOR 0
#define OPERATOR_PREFIX '@'
#define HALFOP 1
#define HALFOP_PREFIX '%'

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
  String topic;
  String topic_set_nick;
  std::time_t topic_set_time;
  std::map<String, User&> user_list;
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
  time_t get_created_time(void) const;
  int get_user_limit(void) const;
  const String& get_topic(void) const;
  const String& get_topic_set_nick(void) const;
  std::time_t get_topic_set_time(void) const;
  int get_mode(void) const;
  std::map<String, User&>& get_user_list(void);
  std::map<String, User&>& get_operator_list(void);
  const std::map<String, User&>& get_user_list(void) const;
  const std::map<String, User&>& get_operator_list(void) const;

  String get_user_list_str(bool is_joined) const;

  void set_password(const String& _pwd);
  void set_user_limit(int _user_limit);
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
  void set_mode(char flag);
  void unset_mode(char flag);
  bool chk_mode(char flag) const;
  String make_mode_str(void);
};

bool is_channel_name(const String& name);

#ifdef DEBUG

#include <iostream>
std::ostream& operator<<(std::ostream&, const Channel& channel);

#endif

#endif