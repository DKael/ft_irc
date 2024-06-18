#ifndef UTIL_HPP
#define UTIL_HPP

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "string_func_bot.hpp"

typedef std::string String;

bool port_chk(const char* input_port);
bool ipv4_chk(const char* input_ipv4);

#endif