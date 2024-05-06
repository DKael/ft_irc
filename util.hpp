#ifndef UTIL_HPP
#define UTIL_HPP

#define BLOCK_SIZE 1025

#include <unistd.h>

#include <string>
#include <vector>

void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list);
#endif