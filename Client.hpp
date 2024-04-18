#ifndef USER_HPP
#define USER_HPP

#include <arpa/inet.h>
#include <sys/socket.h>

#include <string>

class User {
 private:
  std::string nickName;
  std::string userName;
  std::string realName;
  int userSocket;
  sockaddr_in servAddr;

 public:
};

#endif