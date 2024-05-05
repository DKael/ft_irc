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

void	Channel::addClient(User newClient) {
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
	channel_client_list.insert(std::make_pair(newClient.get_nick_name(), newClient));
}

// Channel& Channel::operator=(const Channel& other) {
// 	if (this != &other) {
// 		this->channel_name = other.channel_name;
// 		this->client_limit = other.client_limit;
// 		this->invite_only = other.invite_only;
// 		this->pwd = other.pwd;
// 		this->channel_client_list = other.channel_client_list;
// 		this->channel_banned_list = other.channel_banned_list;
// 		this->ops = other.ops;
// 		this->topic = other.topic;
// 	}
// 	return *this;
// }

int Channel::get_channel_capacity_limit(void) { return client_limit; }

const std::string& Channel::get_channel_name(void) const { return channel_name; }


// [DEBUG]
void	Channel::visualizeClientList(void) {
	std::map<std::string, User>::const_iterator it;
	std::cout << "Visualizng Channel Client lists for Channel :: " << this->get_channel_name() << std::endl;
	for (it = channel_client_list.begin(); it != channel_client_list.end(); ++it) {
		std::cout << "?????????????" << std::endl;
		const std::string& nickName = it->first;
		const User& user = it->second;

		std::cout << nickName << std::endl;
	}
}

void	Channel::visualizeBannedClientList(void) {
	std::map<std::string, User>::const_iterator it;
	for (it = channel_banned_list.begin(); it != channel_banned_list.end(); ++it) {
		const std::string& nickName = it->first;
		const User& user = it->second;

		std::cout << nickName << std::endl;
	}
}






