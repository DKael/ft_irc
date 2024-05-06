#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

#include "../Message.cpp"
#include "../Message.hpp"
#include "../string_func.cpp"
#include "../string_func.hpp"
#include "../util.cpp"
#include "../util.hpp"

#define BUF_SIZE 1024
#define NUMERIC_001 1 << 0
#define NUMERIC_002 1 << 1
#define NUMERIC_003 1 << 2
#define NUMERIC_004 1 << 3
#define NUMERIC_005 1 << 4
#define PING_INTERVAL 20
#define PONG_TIMEOUT 20

std::string add_number_suffix(const std::string& bot_nick) {
  static int suffix = 1;
  std::string str_suffix;
  std::stringstream num_to_str;

  num_to_str << suffix;
  num_to_str >> str_suffix;
  suffix++;
  return bot_nick + str_suffix;
}

int main(int argc, char* argv[]) {
  int bot_sock;
  char message[BUF_SIZE];
  int str_len;
  sockaddr_in serv_adr;
  std::string bot_nickname;
  Message::map_init();

  if (argc != 5) {
    std::cerr << "Usage : " << argv[0]
              << " <IP> <port> <password to connect> <bot_nickname>\n";
    exit(1);
  }

  bot_sock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (bot_sock == -1) {
    std::cerr << "socket() error\n";
    exit(1);
  }

  std::memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (connect(bot_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
    perror("connect() error");
    std::cerr << "errno : " << errno << '\n';
    exit(1);
  } else {
    std::cout << "Bot connected to " << argv[1] << ':' << argv[2] << '\n';
  }

  bot_nickname = std::string(argv[4]);

  std::string init =
      std::string("PASS ") + std::string(argv[3]) + std::string("\r\n");
  send(bot_sock, init.c_str(), init.length(), MSG_DONTWAIT);
  init = std::string("NICK ") + bot_nickname + std::string("\r\n");
  send(bot_sock, init.c_str(), init.length(), MSG_DONTWAIT);
  init = std::string("USER ") + bot_nickname + std::string(" 0 * :") +
         bot_nickname + std::string("\r\n");
  send(bot_sock, init.c_str(), init.length(), MSG_DONTWAIT);

  std::vector<std::string> msg_list;
  std::string serv_name;
  int auth_flag = 0;

  while (auth_flag != 0x1F) {
    try {
      read_msg_from_socket(bot_sock, msg_list);

      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (int i = 0; i < msg_list.size(); i++) {
        Message msg(bot_sock, msg_list[i]);

        if (msg.get_numeric() == std::string("001")) {
          auth_flag |= NUMERIC_001;
        } else if (msg.get_numeric() == std::string("002")) {
          auth_flag |= NUMERIC_002;
        } else if (msg.get_numeric() == std::string("003")) {
          auth_flag |= NUMERIC_003;
        } else if (msg.get_numeric() == std::string("004")) {
          auth_flag |= NUMERIC_004;
          serv_name = msg[1];
        } else if (msg.get_numeric() == std::string("005")) {
          auth_flag |= NUMERIC_005;
        } else if (msg.get_numeric() == std::string("433")) {
          std::string nick_retry = std::string("NICK ") +
                                   add_number_suffix(bot_nickname) +
                                   std::string("\r\n");
          send(bot_sock, nick_retry.c_str(), nick_retry.length(), MSG_DONTWAIT);
        } else if (msg.get_numeric() == std::string("421") ||
                   msg.get_numeric() == std::string("432") ||
                   msg.get_numeric() == std::string("451") ||
                   msg.get_numeric() == std::string("462") ||
                   msg.get_numeric() == std::string("464") ||
                   msg.get_cmd_type() == ERROR) {
          std::cerr << msg_list[i];
          close(bot_sock);
          exit(1);
        }
      }
    } catch (const std::bad_alloc& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Not enough memory so can't excute vector.push_back "
                   "or another things require additional memory\n";
    } catch (const std::length_error& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Maybe index out of range error or std::string is too "
                   "long to store\n";
    } catch (const std::exception& e) {
      // error handling
      std::cerr << e.what() << '\n';
      std::cerr << "unexpected exception occur! Program terminated!\n";
      exit(1);
    }
  }

  // auth done
  time_t last_ping_chk = time(NULL);
  time_t ping_send_time = time(NULL);
  bool is_ping_sent = false;
  bool is_pong_received = false;
  while (true) {
    try {
      read_msg_from_socket(bot_sock, msg_list);

      if (is_ping_sent == false && time(NULL) > last_ping_chk + PING_INTERVAL) {
        Message ping;
        ping.set_cmd_type(PING);
        ping.push_back(serv_name);
        std::string raw_msg = ping.to_raw_msg();
        send(bot_sock, raw_msg.c_str(), raw_msg.length(), MSG_DONTWAIT);
        ping_send_time = time(NULL);
        is_ping_sent = true;
        is_pong_received = false;
      } else if (is_ping_sent == true && is_pong_received == false &&
                 time(NULL) > ping_send_time + PONG_TIMEOUT) {
        Message rpl;
        rpl.set_cmd_type(ERROR);
        rpl.push_back(":leaving");
        std::string raw_msg = rpl.to_raw_msg();
        send(bot_sock, raw_msg.c_str(), raw_msg.length(), MSG_DONTWAIT);
        close(bot_sock);
        exit(0);
      }

      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (int i = 0; i < msg_list.size(); i++) {
        std::cerr << "msg " << i << " : " << msg_list[i] << '\n';
        Message msg(bot_sock, msg_list[i]);

        if (msg.get_cmd_type() == PONG) {
          is_ping_sent = false;
          is_pong_received = true;
          last_ping_chk = time(NULL);
        } else if (msg.get_cmd_type() == PRIVMSG &&
                   msg.get_params_size() >= 2) {
          Message rpl;
          rpl.set_cmd_type(PRIVMSG);

          std::size_t tail = msg.get_source().find("!");
          std::string who_send = msg.get_source().substr(0, tail);
          rpl.push_back(who_send);
          if (msg[1] == std::string("lunch menu recommend")) {
            rpl.push_back(":bot reply for lunch menu recommend");
          } else {
            rpl.push_back(":unknown command");
          }
          std::string raw_msg = rpl.to_raw_msg();
          send(bot_sock, raw_msg.c_str(), raw_msg.length(), MSG_DONTWAIT);
        }
      }
    } catch (const std::bad_alloc& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Not enough memory so can't excute vector.push_back "
                   "or another things require additional memory\n";
    } catch (const std::length_error& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Maybe index out of range error or std::string is too "
                   "long to store\n";
    } catch (const std::exception& e) {
      // error handling
      std::cerr << e.what() << '\n';
      std::cerr << "unexpected exception occur! Program terminated!\n";
      exit(1);
    }
  }
}
