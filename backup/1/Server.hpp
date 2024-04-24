#ifndef SERVER_HPP
#define SERVER_HPP

#include <arpa/inet.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <map>
#include <string>

#include "Client.hpp"
#include "Exception.hpp"

#define MAX_USER 1024

class Server {
private:
    int port;
    std::string s_port;
    std::string password;
    int serv_socket;
    sockaddr_in serv_addr;
    std::map<std::string, User> user_list;  // key is user's nickname

    // not use
    Server();
    Server(const Server& origin);
    Server& operator=(const Server& origin);
public:
    Server(const char* _port, const char* _password);
    // Server(const std::string& _port, const std::string& _password);
    ~Server();
};

#endif