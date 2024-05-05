#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <map>
#include <vector>
#include "User.hpp"
#include "custom_exception.hpp"

class User;

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
	std::map<std::string, User> channel_client_list;    // nickname 과 user
	std::map<std::string, User> channel_banned_list;   	// nickname 과 banned user

	// OPERATORS
	std::vector<User>			ops;

	// TOPIC
	std::string					topic;

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
	int						get_channel_capacity_limit(void);
	const std::string&		get_channel_name(void) const;

	// METHOD FUNCTIONS
	void					addClient(User user);
	void					kickClient(User user);
	void 					updateTopic(std::string topic);

	// FOR DEBUG PURPOSE ONLY [VISUALIZE]
	void 					visualizeClientList(void);
	void					visualizeBannedClientList(void);
};

#endif