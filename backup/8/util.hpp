#ifndef UTIL_HPP
#define UTIL_HPP

#define BLOCK_SIZE 1025

#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <vector>

#include "string_func.hpp"

typedef std::string String;

bool port_chk(const char* input_port);
bool ipv4_chk(const char* input_ipv4);

#endif