#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <iostream>
#include <map>
#include <vector>
#include "User.hpp"

class User;

class Channel
{
private:
	// CHANNEL NAME
	const std::string channel_name;

	// USER LIMIT
	int limit;

	// CLIENT LIST (원본을 가지고 다닐것)
	std::map<std::string, User> channelClientLst;    // nickname 과 user
	std::map<std::string, User> channelBannedlist;   // nickname 과 banned user

	// OPERATORS
	std::vector<User> operators;

	// TOPIC
	std::string topic;

	/*
		i : set / remove Invite only channel
		t : set / remove the restrictions of the TOPIC command to channel operators
		k : set / remove the channel key (password)
		o : give / take channel operator privilege
		l : set / remove the user limit to channel
	*/

	Channel();
	Channel(const Channel& other);
	Channel& operator=(const Channel& other);

public:
	// OCCF
	Channel(std::string channelName);
	~Channel();

	// GETTER && SETTER

	// METHOD FUNCTIONS
	void	addClient(User user);
	void	kickClient(User user);
	void 	updateTopic(std::string topic);

	// FOR DEBUG PURPOSE ONLY [VISUALIZE]
	void whoIsInTheChannel();
};

#endif