#include "Channel.hpp"

// OCCF
Channel::Channel(const std::string& _channel_name, char _channel_type)
    : channel_name(_channel_name),
      channel_type(_channel_type),
      created_time(std::time(NULL)),
      invite_only(false),
      mode(0),
      client_limit(MAX_CLIENT_LIMIT) {}

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
const std::string& Channel::get_raw_channel_name(void) const {
  return channel_name;
}

const std::string Channel::get_channel_name(void) const {
  if (channel_type == REGULAR_CHANNEL) {
    return std::string(REGULAR_CHANNEL_PREFIX) + channel_name;
  } else if (channel_type == LOCAL_CHANNEL) {
    return std::string(LOCAL_CHANNEL_PREFIX) + channel_name;
  } else {
    return channel_name;
  }
}

char Channel::get_channel_type(void) const { return channel_type; }

const std::string& Channel::get_password(void) const { return pwd; };

int Channel::get_client_limit(void) const { return client_limit; }

bool Channel::get_invite_only(void) const { return invite_only; };

const std::string& Channel::get_topic(void) const { return topic; };

const std::map<std::string, User&>& Channel::get_client_list(void) const {
  return client_list;
};

const std::map<std::string, User&>& Channel::get_banned_list(void) const {
  return banned_list;
};

const std::map<std::string, User&>& Channel::get_operator_list(void) const {
  return operator_list;
};

void Channel::set_password(const std::string& _pwd) { pwd = _pwd; }

void Channel::set_client_limit(int _client_limit) {
  client_limit = _client_limit;
}

void Channel::set_invite_only(bool _invite_only) { invite_only = _invite_only; }

void Channel::set_topic(const std::string& _topic) { topic = _topic; }

// METHOD FUNCTIONS
void Channel::add_client(User& newClient) {
  std::map<std::string, User&>::iterator it =
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
      std::pair<std::string, User&>(newClient.get_nick_name(), newClient));
}

void Channel::add_operator(User& Client) {
  std::map<std::string, User&>::iterator it =
      operator_list.find(Client.get_nick_name());

  if (it != operator_list.end()) {
    return;
  }
  operator_list.insert(
      std::pair<std::string, User&>(Client.get_nick_name(), Client));
}

bool Channel::is_operator(const std::string& nickname) const {
  std::map<std::string, User&>::const_iterator cit =
      operator_list.find(nickname);

  if (cit != operator_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::remove_operator(const std::string& nickname) {
  std::map<std::string, User&>::iterator it = operator_list.find(nickname);

  if (it != operator_list.end()) {
    operator_list.erase(it);
  }
}

bool Channel::chk_client_join(const std::string& nickname) const {
  std::map<std::string, User&>::const_iterator cit = client_list.find(nickname);

  if (cit != client_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::change_client_nickname(const std::string& old_nick,
                                     const std::string& new_nick) {
  std::map<std::string, User&>::iterator it = client_list.find(old_nick);

  if (it != client_list.end()) {
    client_list.insert(std::pair<std::string, User&>(new_nick, it->second));
    client_list.erase(it);

    it = operator_list.find(old_nick);
    if (it != operator_list.end()) {
      operator_list.insert(std::pair<std::string, User&>(new_nick, it->second));
      operator_list.erase(it);
    }
  }
}

// 채널 모드 세팅
void Channel::set_mode(int flag) { mode |= flag; }

void Channel::unset_mode(int flag) { mode &= ~flag; }

// 채널 모드 확인
bool Channel::chk_mode(int flag) const { return mode & flag; }

// [OVERLOADING] operator<<
std::ostream& operator<<(std::ostream& out, Channel& channel) {
  out << BLUE << "[channel name] :: " << channel.get_channel_name() << '\n'
      << "[client limit] :: " << channel.get_client_limit() << '\n'
      << "[invite mode] :: ";
  if (channel.chk_mode(FLAG_I) == true)
    out << "ON" << std::endl;
  else
    out << "OFF" << std::endl;
  out << channel.get_password() << '\n';

  const std::map<std::string, User&>& clientList = channel.get_client_list();
  const std::map<std::string, User&>& operators = channel.get_operator_list();
  std::map<std::string, User&>::const_iterator cit;

  int i = 1;
  out << "=============== Client List ===============" << std::endl;
  for (cit = clientList.begin(); cit != clientList.end(); ++cit, ++i) {
    const std::string& nickName = cit->first;
    // const User& user = cit->second;
    out << i << ". " << nickName << std::endl;
  }
  out << "\n";
  out << "=============== Operators =================";
  i = 1;
  out << "\n";
  for (cit = operators.begin(); cit != operators.end(); ++cit, ++i) {
    const std::string& nickName = cit->first;
    out << i << ". " << nickName << std::endl;
  }
  out << std::endl << WHITE;

  return out;
}

// [DEBUG]
void Channel::visualizeClientList(void) {
  std::map<std::string, User&>::const_iterator cit;
  std::cout << "Visualizng Channel Client lists for Channel :: "
            << this->get_channel_name() << std::endl;
  std::cout << YELLOW;
  for (cit = client_list.begin(); cit != client_list.end(); ++cit) {
    const std::string& nickName = cit->first;
    const User& user = cit->second;
    std::cout << nickName << std::endl;
  }
  std::cout << WHITE;
}