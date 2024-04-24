#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstring>
#include <sstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/poll.h>

class Client {
public:
    int socket;
    std::string nickname;
    std::string username;
    std::vector<std::string> channels;

    Client(int socket) : socket(socket) {}
};

class Channel {
public:
    std::string name;
    std::vector<Client*> members;

    Channel(const std::string& name) : name(name) {}
};

class IRCServer {
private:
    int serverSocket;
    std::unordered_map<int, Client> clients;
    std::unordered_map<std::string, Channel> channels;

public:
    void start(int port) 
    {
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            return;
        }

        sockaddr_in serverAddress;
        memset(&serverAddress, 0, sizeof(serverAddress));
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = INADDR_ANY;
        serverAddress.sin_port = htons(port);

        if (bind(serverSocket, reinterpret_cast<struct sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
            std::cerr << "Failed to bind socket" << std::endl;
            return;
        }

        if (listen(serverSocket, 5) < 0) {
            std::cerr << "Failed to listen on socket" << std::endl;
            return;
        }

        std::cout << "IRC server started on port " << port << std::endl;

        sockaddr_in         clientAddress;
        socklen_t           clientAddressLength;
        int                 clientSocket;
        struct pollfd       fds[100];
        fds[0].fd = serverSocket;
        fds[0].events = POLLIN;
        int num_fds = 1;

        while (true) 
        {
            int pollCnt = poll(fds, num_fds, -1); if (pollCnt == -1) { std::cerr << "poll() failed." << std::endl;  break ;}
            if (fds[0].revents & POLLIN) {
                clientAddressLength = sizeof(clientAddress);
                int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, (socklen_t *)&clientAddressLength);
                if (clientSocket < 0) { std::cerr << "Failed to accept client connection" << std::endl; continue; }
                if (num_fds < 100 + 1) {
                    fds[num_fds].fd = clientSocket;
                    fds[num_fds].events = POLLIN;
                    num_fds++;
                }   else {
                    std::cerr << "Exceeded maximum number of clients." << std::endl;
                    close(clientSocket);
                }
            }
            clients.insert(std::make_pair(clientSocket, Client(clientSocket)));
            // std::cout << "New client connected: " << clientSocket << std::endl;
            handleClient(clientSocket);
        }
    }

    void handleClient(int clientSocket) {
        char buffer[1024];
        ssize_t bytesRead;

        while ((bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
            std::string message(buffer, bytesRead);
            std::cout << "Received message from client " << clientSocket << ": " << message << std::endl;

            // Parse the received message
            std::istringstream iss(message);
            std::string command;
            iss >> command;

            std::cout << "Command: " << command << std::endl;
            std::cout << "Parameters: ";
            std::string parameter;
            while (iss >> parameter) {
                std::cout << parameter << " ";
            }
            std::cout << std::endl;

            // TODO: Handle the parsed command and parameters
        }

        // std::cout << "Client disconnected: " << clientSocket << std::endl;
        // clients.erase(clientSocket);
        // close(clientSocket);
    }
};

int main() {
    IRCServer server;
    server.start(6667);

    return 0;
}