#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  int serverSocket;
  struct sockaddr_in serv_adr;

  serverSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocket == -1) {
    exit(1);
  }

  int bufSize = 1024 * 8;
  socklen_t len = sizeof(bufSize);
  setsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize));
  getsockopt(serverSocket, SOL_SOCKET, SO_SNDBUF, &bufSize, &len);
  printf("socket buffer size: %d\n", bufSize);

  close(serverSocket);
  return 0;
}
