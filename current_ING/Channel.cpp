#include "Channel.hpp"

Channel::Channel(std::string channelName)
	: channel_name(channelName)
	, client_limit(50) 					// 추후 논의 후 구체적인 값 결정할것
	{}

Channel::Channel(const Channel& other)
	: channel_name(other.channel_name)
	, client_limit(other.client_limit)
	{}

Channel::~Channel() {}

void	Channel::addClient(User& newClient) {
	if (channel_client_list.size() >= get_channel_capacity_limit()) {
		/*
			ERR_CHANNELISFULL (471) 
			"<client> <channel> :Cannot join channel (+l)"
			Returned to indicate that a JOIN command failed because the client limit
			mode has been set and the maximum number of users are already joined to the channel.
			The text used in the last param of this message may vary. 
		*/
		throw(channel_client_capacity_error());
	}	
	// channel_client_list.insert(std::make_pair(newClient.get_nick_name(), newClient));
	channel_client_list.insert(std::pair<std::string, User&>(newClient.get_nick_name(), newClient));
}

void	Channel::addOperator(User& Client) {
	ops.push_back(Client);
}



int Channel::get_channel_capacity_limit(void) { return client_limit; }

bool Channel::get_invite_mode_setting(void) { return invite_only; };

std::string Channel::get_password(void) { return pwd; };

std::string Channel::get_topic(void) { return topic; };

std::map<std::string, User> Channel::get_channel_client_list(void) { return channel_client_list; };
std::map<std::string, User> Channel::get_channel_banned_list(void) { return channel_banned_list; };
std::vector<User> Channel::get_channel_operator_list(void) { return ops; };

const std::string& Channel::get_channel_name(void) const { return channel_name; }

// [DEBUG]
void	Channel::visualizeClientList(void) {
	std::map<std::string, User>::const_iterator it;
	std::cout << "Visualizng Channel Client lists for Channel :: " << this->get_channel_name() << std::endl;
	std::cout << YELLOW;
	for (it = channel_client_list.begin(); it != channel_client_list.end(); ++it) {
		const std::string& nickName = it->first;
		const User& user = it->second;
		std::cout << nickName << std::endl;
	}
	std::cout << WHITE;
}

void	Channel::visualizeBannedClientList(void) {
	std::map<std::string, User>::const_iterator it;
	for (it = channel_banned_list.begin(); it != channel_banned_list.end(); ++it) {
		const std::string& nickName = it->first;
		const User& user = it->second;

		std::cout << nickName << std::endl;
	}
}

// [OVERLOADING] operator<<
std::ostream& operator<<(std::ostream& out, Channel channel) {
	out
		<< "[channel name] :: " << channel.get_channel_name() << std::endl
		<< "[client limit] :: " << channel.get_channel_capacity_limit() << std::endl
		<< "[invite mode] :: ";
			if (channel.get_invite_mode_setting() == true)
				out << "ON" << std::endl;
			else
				out << "OFF" << std::endl;
	out
		<< channel.get_password() << std::endl;

	std::map<std::string, User>clientList = channel.get_channel_client_list();
	std::map<std::string, User>bannedList = channel.get_channel_banned_list();
	std::vector<User>operators = channel.get_channel_operator_list();

	std::map<std::string, User>::const_iterator cit;

	out << "=============== Client List ===============";
	for (cit = clientList.begin(); cit != clientList.end(); ++cit) {
		const std::string& nickName = cit->first;
		// const User& user = cit->second;
		out << nickName << " , ";  		
	}
	out << "\n";
	out << "=============== Banned List ===============";
	std::map<std::string, User>::const_iterator cit2;
	for (cit2 = bannedList.begin(); cit2 != bannedList.end(); ++cit2) {
		const std::string& nickName = cit2->first;
		// const User& user = cit2->second;
		out << nickName << " , ";  		
	}
	out << "\n";

	out << "=============== Operators ===============";
	for (std::vector<User>::iterator it = operators.begin(); it != operators.end(); ++it) {
		const std::string& nickName = it->get_nick_name();
		out << nickName << " , ";
	}
	out << "\n";

	return out;
}




