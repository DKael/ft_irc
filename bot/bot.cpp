#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "../Message.hpp"

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
  int bot_sock;
  char message[BUF_SIZE];
  int str_len;
  sockaddr_in serv_adr;

  if (argc != 4) {
    std::cerr << "Usage : " << argv[0]
              << " <IP> <port> <password to connect>\n";
    exit(1);
  }

  bot_sock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (bot_sock == -1) {
    std::cerr << "socket() error\n";
    exit(1);
  }
  if (::fcntl(bot_sock, F_SETFL, O_NONBLOCK) == -1) {
    std::cerr << "fcntl() error\n";
    exit(1);
  }

  std::memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (connect(bot_sock, (sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) {
    std::cerr << "connect() error\n";
    exit(1);
  } else {
    std::cout << "Bot connected to " << argv[1] << ':' << argv[2];
  }

  std::string msg =
      std::string("PASS ") + std::string(argv[3]) + std::string("\r\n");
  send(bot_sock, msg.c_str(), msg.length(), MSG_DONTWAIT);
  msg = std::string("NICK ") + std::string("tmp_bot\r\n");
  send(bot_sock, msg.c_str(), msg.length(), MSG_DONTWAIT);
  msg = std::string("USER ") + std::string("tmp_bot 0 * :tmp_bot\r\n");
  send(bot_sock, msg.c_str(), msg.length(), MSG_DONTWAIT);

  // while (1) {
  //   fputs("Input message(Q to quit): ", stdout);
  //   memset(message, 0, BUF_SIZE);
  //   fgets(message, BUF_SIZE, stdin);

  //   if (!strcmp(message, "q\n") || !strcmp(message, "Q\n")) break;

  //   str_len = strlen(message);
  //   message[str_len - 1] = '\r';
  //   message[str_len] = '\n';
  //   // message[str_len + 1] = '\n';

  //   send(sock, message, strlen(message), MSG_DONTWAIT);
  //   while (true) {
  //     str_len = recv(sock, message, BUF_SIZE - 1, MSG_DONTWAIT);
  //     if (str_len == -1 && errno == EWOULDBLOCK) {
  //       break;
  //     }
  //     message[str_len] = 0;
  //     printf("Message from server: %s", message);
  //   }
  // }

  // // while (1) {
  // //   if (recv(sock, message, BUF_SIZE, MSG_DONTWAIT) == 0) {
  // //     break;
  // //   }
  // // }
  // std::cout << "socket close\n";
  // close(sock);
  // return 0;
}
