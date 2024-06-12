#include "Channel.hpp"
#include "Server.hpp"

Channel::Channel(std::string channelName)
    : channel_name(channelName),
      client_limit(50)  // 추후 논의 후 구체적인 값 결정할것
{}

Channel::Channel(const Channel& other)
    : channel_name(other.channel_name), client_limit(other.client_limit) {}

Channel::~Channel() {}

void Channel::addClient(User& newClient) {
  if (channel_client_list.size() >= get_channel_capacity_limit()) {
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
  // channel_client_list.insert(std::make_pair(newClient.get_nick_name(),
  // newClient));
  channel_client_list.insert(
      std::pair<std::string, User&>(newClient.get_nick_name(), newClient));
  // ops.insert(std::pair<int, std::string>(newClient.get_user_socket(),
  // newClient.get_nick_name()));
}

// void	Channel::addOperator(User& Client) {
// 	ops.push_back(Client);
// }

void Channel::addOperator(User& Client) {
  ops.insert(std::pair<int, std::string>(Client.get_user_socket(),
                                         Client.get_nick_name()));
}

int Channel::get_channel_capacity_limit(void) const { return client_limit; }

bool Channel::get_invite_mode_setting(void) const { return invite_only; };

std::string Channel::get_password(void) const { return pwd; };

std::string Channel::get_topic(void) const { return topic; };

std::map<std::string, User&>& Channel::get_channel_client_list(void) {
  return channel_client_list;
};

const std::map<int, std::string>& Channel::get_channel_operator_list(
    void) const {
  return ops;
};

const std::string& Channel::get_channel_name(void) const {
  return channel_name;
}

// [DEBUG]
void Channel::visualizeClientList(void) {
  std::map<std::string, User&>::const_iterator it;
  std::cout << "Visualizng Channel Client lists for Channel :: "
            << this->get_channel_name() << std::endl;
  std::cout << YELLOW;
  for (it = channel_client_list.begin(); it != channel_client_list.end();
       ++it) {
    const std::string& nickName = it->first;
    const User& user = it->second;
    std::cout << nickName << std::endl;
  }
  std::cout << WHITE;
}

// [OVERLOADING] operator<<
std::ostream& operator<<(std::ostream& out, Channel& channel) {
  out << BLUE << "[channel name] :: " << channel.get_channel_name() << std::endl
      << "[client limit] :: " << channel.get_channel_capacity_limit()
      << std::endl
      << "[invite mode] :: ";
  if (channel.isMode(FLAG_I) == true)
    out << "ON" << std::endl;
  else if (channel.isMode(FLAG_I) == false)
    out << "OFF" << std::endl;
  out << channel.get_password() << std::endl;

  const std::map<std::string, User&>& clientList =
      channel.get_channel_client_list();
  const std::map<int, std::string> operators =
      channel.get_channel_operator_list();

  std::map<std::string, User&>::const_iterator cit;

  out << "=============== Client List ===============" << std::endl;
  int i = 1;
  for (cit = clientList.begin(); cit != clientList.end(); ++cit) {
    const std::string& nickName = cit->first;
    // const User& user = cit->second;
    out << i << ". " << nickName << std::endl;
    i++;
  }
  out << "\n";
  out << "=============== Operators =================";
  i = 1;
  out << "\n";
  for (std::map<int, std::string>::const_iterator it = operators.begin();
       it != operators.end(); ++it) {
    const std::string& nickName = it->second;
    out << i << ". " << nickName << std::endl;
    i++;
  }
  out << std::endl << WHITE;

  return out;
}

// [ADD]
bool Channel::isOperator(User& user) {
  std::map<int, std::string>::iterator it;
  std::string nickName = user.get_nick_name();
  std::string candidate;

  for (it = ops.begin(); it != ops.end(); ++it) {
    candidate = it->second;
    if (candidate == nickName) return true;
  }
  return false;
}

void Channel::removeOperator(User& user) {
  int fd = user.get_user_socket();

  std::map<int, std::string>::iterator it = ops.find(fd);

  if (it != ops.end()) {
    std::map<int, std::string>::iterator nextIt = std::next(it);
    ops.erase(it);
    it = nextIt;
  }
}

bool Channel::foundClient(std::string nickName) {
  std::map<std::string, User&>::iterator it;

  for (it = channel_client_list.begin(); it != channel_client_list.end();
       ++it) {
    std::string candidate;
    candidate = it->second.get_nick_name();
    if (candidate == nickName) 
      return true;
  }
  return false;
}

void Channel::changeClientNickName(std::string old_nick, std::string new_nick) {
  std::map<std::string, User&>::iterator it;

  // 채널에 있는 유저 매핑된거 새로 추가해주고, 기존에 있던거 없애주기
  for (it = channel_client_list.begin(); it != channel_client_list.end();++it) {
    std::string candidate;
    candidate = it->second.get_nick_name();
    if (candidate == old_nick) {
        User& user = it->second;
        
        // nickName 바꿔주기
        user.set_nick_name(new_nick);
        channel_client_list.insert(std::pair<std::string, User&>(user.get_nick_name(), user));
        // channel_client_list[new_nick] = user;
        channel_client_list.erase(it);
        
        break;
    }
  }

  // OPERATOR도 수정해주기
  std::map<int, std::string>::iterator ops_it;
  for (ops_it = ops.begin(); ops_it != ops.end(); ++ops_it) {
    std::string candidate;
    candidate = ops_it->second;
    if (candidate == old_nick) 
      ops_it->second = new_nick;
  }


}

// 채널 모드 세팅
void Channel::setMode(int flag) { mode |= flag; }
void Channel::unsetMode(int flag) { mode &= ~flag; }

// 채널 모드 확인
const bool Channel::isMode(int flag) const { return mode & flag; }





