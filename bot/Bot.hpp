#ifndef BOT_HPP
#define BOT_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <fstream>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>

#include "../Message.hpp"
#include "../string_func.hpp"
#include "../util.hpp"

#define NUMERIC_001 1 << 0
#define NUMERIC_002 1 << 1
#define NUMERIC_003 1 << 2
#define NUMERIC_004 1 << 3
#define NUMERIC_005 1 << 4
#define PING_INTERVAL 20
#define PONG_TIMEOUT 20

class Bot {
 private:
  std::string ipv4;
  int port;
  int bot_sock;
  sockaddr_in bot_addr;
  std::string password;
  std::string nickname;
  std::string serv_name;

  std::vector<std::string> menu;
  std::queue<std::string> to_send;
  bool remain_msg;

  // not use
  Bot();
  Bot(const Bot& origin);
  Bot& operator=(const Bot& origin);

 public:
  Bot(char** argv);

  void connect_to_serv(void);
  void step_auth(void);
  void step_listen(void);

  const std::string& get_ipv4(void);
  const int get_port(void);
  const int get_bot_sock(void);
  const sockaddr_in& get_bot_adr(void);
  const std::string& get_password(void);
  const std::string& get_nickname(void);

  void send_msg_at_queue(void);
};

#endif