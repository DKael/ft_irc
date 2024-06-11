#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>

#define BUF_SIZE 1024
void error_handling(char *message);

int main(int argc, char *argv[]) {
  int sock;
  char message[BUF_SIZE];
  int str_len;
  struct sockaddr_in serv_adr;

  if (argc != 3) {
    printf("Usage : %s <IP> <port>\n", argv[0]);
    exit(1);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1) error_handling("socket() error");

  memset(&serv_adr, 0, sizeof(serv_adr));
  serv_adr.sin_family = AF_INET;
  serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
  serv_adr.sin_port = htons(atoi(argv[2]));

  if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    error_handling("connect() error!");
  else
    puts("Connected...........");

  write(sock, "PASS secretpasswordhere\r\n",
        strlen("PASS secretpasswordhere\r\n"));
  write(sock, "NICK Wiz\r\n", strlen("NICK Wiz\r\n"));
  write(sock, "USER guest 0 * :Ronnie Reagan\r\n",
        strlen("USER guest 0 * :Ronnie Reagan\r\n"));

  while (1) {
    fputs("Input message(Q to quit): ", stdout);
    memset(message, 0, BUF_SIZE);
    fgets(message, BUF_SIZE, stdin);

    if (!strcmp(message, "q\n") || !strcmp(message, "Q\n")) break;

    str_len = strlen(message);
    message[str_len - 1] = '\r';
    message[str_len] = '\n';
    // message[str_len + 1] = '\n';

    send(sock, message, strlen(message), MSG_DONTWAIT);
    // str_len = recv(sock, message, BUF_SIZE - 1, MSG_DONTWAIT);
    // message[str_len] = 0;
    // printf("Message from server: %s", message);
  }

  // while (1) {
  //   if (recv(sock, message, BUF_SIZE, MSG_DONTWAIT) == 0) {
  //     break;
  //   }
  // }
  std::cout << "socket close\n";
  close(sock);
  return 0;
}

void error_handling(char *message) {
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}