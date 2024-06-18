#include "Channel.hpp"

Channel::Channel(const String& _channel_name)
    : channel_name(_channel_name),
      created_time(std::time(NULL)),
      user_limit(INIT_USER_LIMIT),
      mode(0) {
  if (_channel_name.length() == 0 ||
      String(CHANTYPES).find(_channel_name[0]) == String::npos) {
    throw channel_prefix_error();
  }
  if (_channel_name[0] == REGULAR_CHANNEL_PREFIX) {
    channel_type = REGULAR_CHANNEL;
  } else if (_channel_name[0] == LOCAL_CHANNEL_PREFIX) {
    channel_type = LOCAL_CHANNEL;
  }
}

Channel::Channel(const Channel& other)
    : channel_name(other.channel_name),
      channel_type(other.channel_type),
      created_time(other.created_time),
      pwd(other.pwd),
      user_limit(other.user_limit),
      topic(other.topic),
      topic_set_nick(other.topic_set_nick),
      topic_set_time(other.topic_set_time),
      user_list(other.user_list),
      operator_list(other.operator_list),
      mode(other.mode) {}

Channel::~Channel() {}

// GETTER && SETTER
const String& Channel::get_channel_name(void) const { return channel_name; }

char Channel::get_channel_type(void) const { return channel_type; }

const String& Channel::get_password(void) const { return pwd; };

time_t Channel::get_created_time(void) const { return created_time; }

int Channel::get_user_limit(void) const { return user_limit; }

const String& Channel::get_topic(void) const { return topic; };

const String& Channel::get_topic_set_nick(void) const { return topic_set_nick; }

std::time_t Channel::get_topic_set_time(void) const { return topic_set_time; }

int Channel::get_mode(void) const { return mode; }

size_t Channel::get_user_num(void) const { return user_list.size(); }

std::map<String, User*>& Channel::get_user_list(void) { return user_list; }

std::map<String, User*>& Channel::get_operator_list(void) {
  return operator_list;
}

const std::map<String, User*>& Channel::get_user_list(void) const {
  return user_list;
};

const std::map<String, User*>& Channel::get_operator_list(void) const {
  return operator_list;
};

String Channel::get_user_list_str(bool is_joined) const {
  String nicks = "";
  std::map<String, User*>::const_reverse_iterator cit1 = user_list.rbegin();
  std::map<String, User*>::const_iterator cit2;

  for (; cit1 != user_list.rend(); ++cit1) {
    if (cit1->second->chk_mode(USER_FLAG_I) == true && is_joined == false) {
      continue;
    }
    cit2 = operator_list.find(cit1->first);
    if (cit2 != operator_list.end()) {
      nicks += (OPERATOR_PREFIX + cit1->first);
    } else {
      nicks += cit1->first;
    }
    nicks += " ";
  }
  nicks.erase(nicks.length() - 1);
  return nicks;
}

void Channel::set_password(const String& _pwd) { pwd = _pwd; }

void Channel::set_user_limit(int _user_limit) { user_limit = _user_limit; }

void Channel::set_topic(const String& _topic) { topic = _topic; }

void Channel::set_topic_set_nick(const String& _nick) {
  topic_set_nick = _nick;
}
void Channel::set_topic_set_time(std::time_t _t) { topic_set_time = _t; }

// METHOD FUNCTIONS
void Channel::add_user(User& newuser) {
  std::map<String, User*>::iterator it =
      user_list.find(newuser.get_nick_name());

  if (it != user_list.end()) {
    return;
  }
  if ((mode & CHAN_FLAG_L) &&
      static_cast<int>(user_list.size()) >= user_limit) {
    /*
      ERR_CHANNELISFULL (471)
      "<user> <channel> :Cannot join channel (+l)"
      Returned to indicate that a JOIN command failed because the user
      limit mode has been set and the maximum number of users are already
      joined to the channel. The text used in the last param of this message
      may vary.
    */
    throw(channel_user_capacity_error());
  }
  user_list.insert(std::pair<String, User*>(newuser.get_nick_name(), &newuser));
}

void Channel::add_operator(User& user) {
  std::map<String, User*>::iterator it =
      operator_list.find(user.get_nick_name());

  if (it != operator_list.end()) {
    return;
  }
  operator_list.insert(std::pair<String, User*>(user.get_nick_name(), &user));
}

bool Channel::is_operator(const String& nickname) const {
  std::map<String, User*>::const_iterator cit = operator_list.find(nickname);

  if (cit != operator_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::remove_user(const String& nickname) {
  std::map<String, User*>::iterator it = user_list.find(nickname);

  if (it != user_list.end()) {
    remove_operator(nickname);
    user_list.erase(it);
  }
}

void Channel::remove_operator(const String& nickname) {
  std::map<String, User*>::iterator it = operator_list.find(nickname);

  if (it != operator_list.end()) {
    operator_list.erase(it);
  }
}

bool Channel::chk_user_join(const String& nickname) const {
  std::map<String, User*>::const_iterator cit = user_list.find(nickname);

  if (cit != user_list.end()) {
    return true;
  } else {
    return false;
  }
}

void Channel::change_user_nickname(const String& old_nick,
                                   const String& new_nick) {
  std::map<String, User*>::iterator it = user_list.find(old_nick);

  if (it != user_list.end()) {
    User& tmp_user = *(it->second);

    user_list.erase(it);
    user_list.insert(std::pair<String, User*>(new_nick, &tmp_user));

    it = operator_list.find(old_nick);
    if (it != operator_list.end()) {
      operator_list.erase(it);
      operator_list.insert(std::pair<String, User*>(new_nick, &tmp_user));
    }
  }
}

// 채널 모드 세팅
void Channel::set_mode(int flag) { mode |= flag; }

void Channel::unset_mode(int flag) { mode &= ~flag; }

// 채널 모드 확인
bool Channel::chk_mode(int flag) const { return mode & flag; }

void Channel::set_mode(char flag) {
  if (flag == CHAN_FLAG_K_CHAR) {
    mode |= CHAN_FLAG_K;
  }
  if (flag == CHAN_FLAG_L_CHAR) {
    mode |= CHAN_FLAG_L;
  }
  if (flag == CHAN_FLAG_I_CHAR) {
    mode |= CHAN_FLAG_I;
  }
  if (flag == CHAN_FLAG_S_CHAR) {
    mode |= CHAN_FLAG_S;
  }
  if (flag == CHAN_FLAG_T_CHAR) {
    mode |= CHAN_FLAG_T;
  }
}
void Channel::unset_mode(char flag) {
  if (flag == CHAN_FLAG_K_CHAR) {
    mode &= ~CHAN_FLAG_K;
  }
  if (flag == CHAN_FLAG_L_CHAR) {
    mode &= ~CHAN_FLAG_L;
  }
  if (flag == CHAN_FLAG_I_CHAR) {
    mode &= ~CHAN_FLAG_I;
  }
  if (flag == CHAN_FLAG_S_CHAR) {
    mode &= ~CHAN_FLAG_S;
  }
  if (flag == CHAN_FLAG_T_CHAR) {
    mode &= ~CHAN_FLAG_T;
  }
}
bool Channel::chk_mode(char flag) const {
  if (flag == CHAN_FLAG_K_CHAR) {
    return mode & CHAN_FLAG_K;
  }
  if (flag == CHAN_FLAG_L_CHAR) {
    return mode & CHAN_FLAG_L;
  }
  if (flag == CHAN_FLAG_I_CHAR) {
    return mode & CHAN_FLAG_I;
  }
  if (flag == CHAN_FLAG_S_CHAR) {
    return mode & CHAN_FLAG_S;
  }
  if (flag == CHAN_FLAG_T_CHAR) {
    return mode & CHAN_FLAG_T;
  }
  return false;
}

String Channel::make_mode_str(void) {
  String mode_str = "";

  if (mode & CHAN_FLAG_I) {
    mode_str += "i";
  }
  if (mode & CHAN_FLAG_S) {
    mode_str += "s";
  }
  if (mode & CHAN_FLAG_K) {
    mode_str += "k";
  }
  if (mode & CHAN_FLAG_T) {
    mode_str += "t";
  }
  if (mode & CHAN_FLAG_L) {
    mode_str += "l";
  }

  return mode_str;
}

bool is_channel_name(const String& name) {
  if (name[0] == REGULAR_CHANNEL_PREFIX || name[0] == LOCAL_CHANNEL_PREFIX) {
    return true;
  } else {
    return false;
  }
}

#ifdef DEBUG
std::ostream& operator<<(std::ostream& out, Channel& channel) {
  out << BLUE << "[channel name] :: " << channel.get_channel_name() << '\n'
      << "[user limit] :: " << channel.get_user_limit() << '\n'
      << "[invite mode] :: ";
  if (channel.chk_mode(CHAN_FLAG_I) == true)
    out << "ON" << std::endl;
  else
    out << "OFF" << std::endl;
  out << channel.get_password() << '\n';

  const std::map<String, User*>& userList = channel.get_user_list();
  const std::map<String, User*>& operators = channel.get_operator_list();
  std::map<String, User*>::const_iterator cit;

  int i = 1;
  out << "=============== user List ===============" << std::endl;
  for (cit = userList.begin(); cit != userList.end(); ++cit, ++i) {
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