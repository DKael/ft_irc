#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <map>
#include <vector>

#include "Message.hpp"
#include "User.hpp"
#include "custom_exception.hpp"

#define FLAG_I 1 << 0
#define FLAG_T 1 << 1
#define FLAG_K 1 << 2
#define FLAG_O 1 << 3
#define FLAG_L 1 << 4

class Channel {
 private:
  // CHANNEL NAME
  const std::string channel_name;

  // USER LIMIT
  int client_limit;

  // INVITE MODE ONLY
  bool invite_only;

  // PASSWORD
  std::string pwd;

  // CLIENT LIST (원본을 가지고 다닐것)
  std::map<std::string, User&> channel_client_list;  // nickname 과 user
  std::map<std::string, User&> channel_banned_list;  // nickname 과 banned user

  // OPERATORS
  // std::vector<User>			ops;
  std::map<int, std::string> ops;

  // TOPIC
  std::string topic;

  // int mode;

  // MODE

  /*
    i : set / remove Invite only channel
    
    t : set / remove the restrictions of the TOPIC command to channel
    
    operators k : set / remove the channel key (password) o : give / take
    
    channel operator privilege l : set / remove the user limit to channel
  
  */

  Channel();
  Channel& operator=(const Channel& other);

 public:
  // OCCF
  Channel(std::string channelName);
  Channel(const Channel& other);
  ~Channel();

  // GETTER && SETTER
  int get_channel_capacity_limit(void) const;
  const std::string& get_channel_name(void) const;
  bool get_invite_mode_setting(void) const;
  std::string get_password(void) const;
  std::string get_topic(void) const;
  std::map<std::string, User&>& get_channel_client_list(void);
  const std::map<std::string, User&>& get_channel_banned_list(void) const;
  // const std::vector<User>&
  // get_channel_operator_list(void) const;
  const std::map<int, std::string>& get_channel_operator_list(void) const;

  // METHOD FUNCTIONS
  void addClient(User& user);
  void addOperator(User& user);
  void updateTopic(std::string topic);

  bool isOperator(User& user);
  void removeOperator(User& user);
  bool foundClient(std::string nickName);
  void changeClientNickName(std::string old_nick, std::string new_nick);
  // FOR DEBUG PURPOSE ONLY [VISUALIZE]
  void visualizeClientList(void);
  void visualizeBannedClientList(void);
  void visualizeOperators(void);
};

std::ostream& operator<<(std::ostream&, Channel& channel);

#endif
