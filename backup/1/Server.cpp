#include "Server.hpp"

Server::Server(const char* _port, const char* _password)
    : s_port(_port), password(_password) {
  port = std::atoi(_port);
  if (port < 0 || port > 65535) {
    throw (Error("Invalid port range."));
  }

  serv_socket = ::socket(PF_INET, SOCK_STREAM, 0);
  if (serv_socket == -1) {
    throw (Error("Failed to create socket."));
  }

  std::memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  if (::bind(serv_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) == -1) {
    throw (Error("Failed to bind socket."));
  }

  if (::listen(serv_socket, 128) == -1) {
    throw (Error("Failed to listen to socket."));
  }

  std::cout << "Server listening at " << ::inet_ntoa(serv_addr.sin_addr) << ":" << port << '\n';
  sockaddr_in client_addr;
  int client_addr_len, client_fd;
  struct pollfd fds[100];
  fds[0].fd = serv_socket;
  fds[0].events = POLLIN;
  int num_fds = 1;
  while (1) {
    int poll_count = poll(fds, num_fds, -1);

    if (poll_count == -1) {
        std::cerr << "poll() 함수 실패" << std::endl;
        break;
    }
  
    // 새로운 연결 수신
    if (fds[0].revents & POLLIN) {
        client_addr_len = sizeof(client_addr);
        client_fd = accept(serv_socket, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
        if (client_fd == -1) {
            std::cerr << "연결 수락 실패" << std::endl;
            continue;
        }

        if (num_fds < 100 + 1) {
            fds[num_fds].fd = client_fd;
            fds[num_fds].events = POLLIN;
            num_fds++;
        } else {
            std::cerr << "클라이언트 제한 초과" << std::endl;
            close(client_fd);
        }
    }

    // 데이터 수신 및 전송
    char buffer[1024];
    for (int i = 1; i < num_fds; i++) {
        if (fds[i].revents & POLLIN) {
            memset(buffer, 0, 1024);
            int bytes_received = recv(fds[i].fd, buffer, 1024 - 1, 0);
            if (bytes_received == 0) {
                close(fds[i].fd);
                fds[i].fd = -1;
                continue;
            }
            if (bytes_received == -1) {
                continue;
            std::cout << "수신 데이터: " << buffer;
            send(fds[i].fd, buffer, bytes_received, 0);
        }
    }
  }
}

// Server::Server(const std::string& _port, const std::string& _password) {}
Server::~Server() { ::close(serv_socket); }
