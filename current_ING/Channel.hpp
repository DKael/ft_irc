#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include "User.hpp"
#include "custom_exception.hpp"
#include "message.hpp"
#include "Channel.hpp"

#include "message.hpp"
#include "User.hpp"
#include "custom_exception.hpp"
#include "Channel.hpp"


#include <iostream>
#include <map>
#include <vector>



class User;
class Message;

class Channel
{
private:
	// CHANNEL NAME
	const std::string 			channel_name;

	// USER LIMIT
	int 						client_limit;

	// INVITE MODE ONLY
	bool						invite_only;

	// PASSWORD
	std::string					pwd;

	// CLIENT LIST (원본을 가지고 다닐것)
	std::map<std::string, User&> channel_client_list;    // nickname 과 user
	std::map<std::string, User&> channel_banned_list;   	// nickname 과 banned user

	// OPERATORS
	// std::vector<User>			ops;
	std::map<int, std::string>		ops;
	// TOPIC
	std::string						topic;

	/*
		i : set / remove Invite only channel
		t : set / remove the restrictions of the TOPIC command to channel operators
		k : set / remove the channel key (password)
		o : give / take channel operator privilege
		l : set / remove the user limit to channel
	*/

	Channel();
	Channel& operator=(const Channel& other);

public:
	// OCCF
	Channel(std::string channelName);
	Channel(const Channel& other);
	~Channel();

	// GETTER && SETTER
	int										get_channel_capacity_limit(void) const;
	const std::string&						get_channel_name(void) const;
	bool									get_invite_mode_setting(void) const;
	std::string								get_password(void) const;
	std::string								get_topic(void) const;
	std::map<std::string, User&>& 			get_channel_client_list(void);
	const std::map<std::string, User&>&		get_channel_banned_list(void) const;
	// const std::vector<User>&				get_channel_operator_list(void) const;
	const std::map<int, std::string>&		get_channel_operator_list(void) const;

	// METHOD FUNCTIONS
	void									addClient(User& user);
	void									addOperator(User& user);
	void 									updateTopic(std::string topic);

	bool									isOperator(User& user);
	void									removeOperator(User& user);
	bool									foundClient(std::string nickName);
	// FOR DEBUG PURPOSE ONLY [VISUALIZE]
	void 									visualizeClientList(void);
	void									visualizeBannedClientList(void);
	void 									visualizeOperators(void);
};

std::ostream& operator<<(std::ostream&, Channel& channel);

#endif
