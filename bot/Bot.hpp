#ifndef BOT_HPP
#define BOT_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <cstring>
#include <csignal>
#include <fstream>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "Message_bot.hpp"
#include "string_func_bot.hpp"
#include "util_bot.hpp"

#define SOCKET_BUFFER_SIZE 8192

#define NUMERIC_001 1 << 0
#define NUMERIC_002 1 << 1
#define NUMERIC_003 1 << 2
#define NUMERIC_004 1 << 3
#define NUMERIC_005 1 << 4
#define PING_INTERVAL 20
#define PONG_TIMEOUT 20

typedef std::string String;

class Bot {
 private:
  String ipv4;
  int port;
  int bot_sock;
  sockaddr_in bot_addr;
  String password;
  String nickname;
  String serv_name;

  std::vector<String> menu;
  std::list<String> to_send;
  bool remain_msg;

  // not use
  Bot();
  Bot(const Bot& origin);
  Bot& operator=(const Bot& origin);

 public:
  String remain_input;

  Bot(char** argv);

  void connect_to_serv(void);
  void step_auth(void);
  void step_listen(void);

  int get_port(void);
  int get_bot_sock(void);
  const String& get_ipv4(void);
  const sockaddr_in& get_bot_adr(void);
  const String& get_password(void);
  const String& get_nickname(void);

  int send_msg_at_queue(void);
  ssize_t send_msg_block(int socket_fd, const String& blk);
  void read_msg_from_socket(std::vector<String>& msg_list);
};

#endif