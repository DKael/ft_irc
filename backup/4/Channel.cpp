#include "Channel.hpp"

Channel::Channel(std::string channelName) : channel_name(channelName), channelClientLst(), operators()
{
	;
}

Channel::~Channel() {}