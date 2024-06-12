#include "Channel.hpp"

Channel::Channel(const String& _channel_name, char _channel_type)
    : channel_name(_channel_name),
      created_time(std::time(NULL)),
      invite_only(false),
      mode(0),
      client_limit(INIT_CLIENT_LIMIT) {
  if (_channel_name.length() == 0 ||
      String(CHANTYPES).find(_channel_name[0]) == String::npos) {
    throw std::exception();
  }
  if (_channel_name[0] == REGULAR_CHANNEL_PREFIX[0]) {
    channel_type = REGULAR_CHANNEL;
  } else {
    channel_type = LOCAL_CHANNEL;
  }
}

Channel::Channel(const Channel& other)
    : channel_name(other.channel_name),
      channel_type(other.channel_type),
      created_time(other.created_time),
      pwd(other.pwd),
      invite_only(other.invite_only),
      client_limit(other.client_limit),
      topic(other.topic),
      client_list(other.client_list),
      banned_list(other.banned_list),
      operator_list(other.operator_list) {}

Channel::~Channel() {}

// GETTER && SETTER
const String& Channel::get_channel_name(void) const { return channel_name; }

char Channel::get_channel_type(void) const { return channel_type; }

const String& Channel::get_password(void) const { return pwd; };

int Channel::get_client_limit(void) const { return client_limit; }

bool Channel::get_invite_only(void) const { return invite_only; };

const String& Channel::get_topic(void) const { return topic; };

const String& Channel::get_topic_set_nick(void) const { return topic_set_nick; }
std::time_t Channel::get_topic_set_time(void) const { return topic_set_time; }

std::map<String, User&>& Channel::get_client_list(void) { return client_list; }
std::map<String, User&>& Channel::get_banned_list(void) { return banned_list; }
std::map<String, User&>& Channel::get_operator_list(void) {
  return operator_list;
}

const std::map<String, User&>& Channel::get_client_list(void) const {
  return client_list;
};

const std::map<String, User&>& Channel::get_banned_list(void) const {
  return banned_list;
};

const std::map<String, User&>& Channel::get_operator_list(void) const {
  return operator_list;
};

void Channel::set_password(const String& _pwd) { pwd = _pwd; }

void Channel::set_client_limit(int _client_limit) {
  client_limit = _client_limit;
}

void Channel::set_invite_only(bool _invite_only) { invite_only = _invite_only; }

void Channel::set_topic(const String& _topic) { topic = _topic; }

void Channel::set_topic_set_nick(const String& _nick) {
  topic_set_nick = _nick;
}
void Channel::set_topic_set_time(std::time_t _t) { topic_set_time = _t; }

// METHOD FUNCTIONS
void Channel::add_client(User& newClient) {
  std::map<String, User&>::iterator it =
      client_list.find(newClient.get_nick_name());

  if (it != client_list.end()) {
    return;
  }
  if (client_list.size() >= client_limit) {
    /*
      ERR_CHANNELISFULL (471)
      "<client> <channel> :Cannot join channel (+l)"
      Returned to indicate that a JOIN command failed because the client
      limit mode has been set and the maximum number of users are already
      joined to the channel. The text used in the last param of this message
      may vary.
    */
    throw(channel_client_capacity_error());
  }
  client_list.insert(
      std::pair<String, User&>(newClient.get_nick_name(), newClient));
}

void Channel::add_operator(User& Client) {
  std::map<String, User&>::iterator it =
      operator_list.find(Client.get_nick_name());

  if (it != operator_list.end()) {
    return;
  }
  operator_list.insert(
      std::pair<String, User&>(Client.get_nick_name(), Client));
}

bool Channel::is_operator(const String& nickname) const {
  std::map<String, User&>::const_iterator cit = operator_list.find(nickname);

  if (cit != operator_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::remove_client(const String& nickname) {
  std::map<String, User&>::iterator it = client_list.find(nickname);

  if (it != client_list.end()) {
    remove_operator(nickname);
    client_list.erase(it);
  }
}

void Channel::remove_operator(const String& nickname) {
  std::map<String, User&>::iterator it = operator_list.find(nickname);

  if (it != operator_list.end()) {
    operator_list.erase(it);
  }
}

bool Channel::chk_client_join(const String& nickname) const {
  std::map<String, User&>::const_iterator cit = client_list.find(nickname);

  if (cit != client_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::change_client_nickname(const String& old_nick,
                                     const String& new_nick) {
  std::map<String, User&>::iterator it = client_list.find(old_nick);

  if (it != client_list.end()) {
    client_list.insert(std::pair<String, User&>(new_nick, it->second));
    client_list.erase(it);

    it = operator_list.find(old_nick);
    if (it != operator_list.end()) {
      operator_list.insert(std::pair<String, User&>(new_nick, it->second));
      operator_list.erase(it);
    }
  }
}

// 채널 모드 세팅
void Channel::set_mode(int flag) { mode |= flag; }

void Channel::unset_mode(int flag) { mode &= ~flag; }

// 채널 모드 확인
bool Channel::chk_mode(int flag) const { return mode & flag; }

#ifdef DEBUG
std::ostream& operator<<(std::ostream& out, Channel& channel) {
  out << BLUE << "[channel name] :: " << channel.get_channel_name() << '\n'
      << "[client limit] :: " << channel.get_client_limit() << '\n'
      << "[invite mode] :: ";
  if (channel.chk_mode(CHAN_FLAG_I) == true)
    out << "ON" << std::endl;
  else
    out << "OFF" << std::endl;
  out << channel.get_password() << '\n';

  const std::map<String, User&>& clientList = channel.get_client_list();
  const std::map<String, User&>& operators = channel.get_operator_list();
  std::map<String, User&>::const_iterator cit;

  int i = 1;
  out << "=============== Client List ===============" << std::endl;
  for (cit = clientList.begin(); cit != clientList.end(); ++cit, ++i) {
    const String& nickName = cit->first;
    // const User& user = cit->second;
    out << i << ". " << nickName << std::endl;
  }
  out << "\n";
  out << "=============== Operators =================";
  i = 1;
  out << "\n";
  for (cit = operators.begin(); cit != operators.end(); ++cit, ++i) {
    const String& nickName = cit->first;
    out << i << ". " << nickName << std::endl;
  }
  out << std::endl << WHITE;

  return out;
}
#endif