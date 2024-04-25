#include <cstring>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "message.hpp"
#include "Server.hpp"
#include "string_func.hpp"

#define BLOCK_SIZE 1025
#define BUFFER_SIZE 65536
#define POLL_TIMEOUT 5

void read_msg_from_socket(const int socket_fd, std::vector<std::string>& msg_list);

bool port_chk(const char* input_port) 
{
  std::stringstream port_chk;
  int port;

  port_chk << std::string(input_port);
  port_chk >> port;
  if (port_chk.fail()) 
    return false;
  else if (port < 0 || 65335 < port)
    return false;
  return true;
}

int main(int argc, char* argv[]) 
{
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << " <port> <password to connect>" << std::endl;
    return (1);
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return (1);
  }
  try
  {
    Message::map_init();
    Server        serv(argv[1], argv[2]);
    pollfd        observe_fd[MAX_USER];
    int           event_cnt = 0;

    socklen_t     user_addr_len;
    sockaddr_in   user_addr;
    int           user_socket;
    int           read_cnt = 0;
    int           flag;

    std::vector<std::string> msg_list;
    observe_fd[0].fd = serv.get_serv_socket();
    observe_fd[0].events = POLLIN;
    for (int i = 1; i < MAX_USER; i++)
      observe_fd[i].fd = -1;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    while (1) 
    {
      event_cnt = poll(observe_fd, MAX_USER, POLL_TIMEOUT * 1000);
      if (event_cnt == 0) {
        continue;
      } else if (event_cnt < 0) {
        perror("poll() error!");
        return (1);
      } else {
        // [STEP 1] :: MULTIPLEXING (POLL), MONITOR INCOMING CLIENTS
        if (observe_fd[0].revents & POLLIN) {
          while (true) {
            user_addr_len = sizeof(user_addr);
            std::memset(&user_addr, 0, user_addr_len);
            user_socket = ::accept(serv.get_serv_socket(), (sockaddr*)&user_addr, &user_addr_len);
            if (user_socket == -1) {
              if (errno == EWOULDBLOCK) {
                break;
              } else {
                std::cerr << "accept() error\n";
                exit(EXIT_FAILURE);
              }
            }
            if (::fcntl(user_socket, F_SETFL, O_NONBLOCK) == -1)
              std::cerr << "fcntl() error\n";
            std::cout << "current number of clients in the server :: "<< "[" << serv.get_user_cnt() << "]" <<  std::endl;
            if (serv.get_user_cnt() == MAX_USER) {
              std::cerr << "User connection limit exceed!\n";
            }
            // serv.add_tmp_user(user_socket, user_addr);
            for (int i = 1; i < MAX_USER; i++) {
              if (observe_fd[i].fd == -1) {
                observe_fd[i].fd = user_socket;
                observe_fd[i].events = POLLIN;
                std::cout << "client" << "[" << i << "]" << "client socket connected!" << std::endl;
                User user(user_socket, user_addr);
                serv.add_user(user);
                break;
              }
            }
          }
          event_cnt--;
        }
        // exit(1);
        for (int i = 1; i < MAX_USER && event_cnt > 0; i++) {
          // std::cout << i << std::endl;
          if (observe_fd[i].fd > 0) {
            if (observe_fd[i].revents & POLLIN) {
              try {
                read_msg_from_socket(observe_fd[i].fd, msg_list);
                // const char* welcome_message2 = ":server.example.com 451 * JOIN :Connection not registered";
                // ssize_t bytes_sent2 = send(user_socket, welcome_message2, strlen(welcome_message2), 0);

                // print msg_list to check message read is ok
                // this part will be removed
                // std::cerr << "printed at socket read part\n";
                std::cout << std::endl;
                for (int k = 0; k < msg_list.size(); k++) {
                  std::cerr << RED << "[" << msg_list[k] << "]" << WHITE << std::endl;
                  Message msg(msg_list[k]);
                  std::cout << msg;
                  if (msg.get_cmd() == "JOIN") {
                    const char* JOINMSG = ":irc.example.net 451 * :Connection not registered\r\n";
                    ssize_t bytes_sent0 = send(observe_fd[i].fd, JOINMSG, strlen(JOINMSG), 0);
                    // const char* JOINMSG = ":irc.example.net 451 * :Connection not registered\r\n";
                    // ssize_t bytes_sent0 = send(observe_fd[i].fd, JOINMSG, strlen(JOINMSG), 0);
                  } 
                  else if (msg.get_cmd() == "USER") 
                  {
                    // RPL_WELCOME (001)
                    const char* welcome_message1 = ":localhost 001 dong-kim :Welcome to the Internet Relay Network dong-kim!dong-kim@localhost\r\n";
                    ssize_t bytes_sent1 = send(observe_fd[i].fd, welcome_message1, strlen(welcome_message1), 0);
                    if (bytes_sent1 == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }
                    // RPL_YOURHOST (002)
                    const char* yourhost_message = ":localhost 002 dong-kim :Your host is localhost, running version 1.0\r\n";
                    ssize_t bytes_sent_yourhost = send(observe_fd[i].fd, yourhost_message, strlen(yourhost_message), 0);
                    if (bytes_sent_yourhost == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }

                    // RPL_CREATED (003)
                    const char* created_message = ":localhost 003 dong-kim :This server was created Fri Apr 25 2023 at 12:00:00 KST\r\n";
                    ssize_t bytes_sent_created = send(observe_fd[i].fd, created_message, strlen(created_message), 0);
                    if (bytes_sent_created == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }

                    // RPL_MYINFO (004)
                    const char* myinfo_message = ":localhost 004 dong-kim localhost 1.0 oiwszcrkfydnxbauqvhopm\r\n";
                    ssize_t bytes_sent_myinfo = send(observe_fd[i].fd, myinfo_message, strlen(myinfo_message), 0);
                    if (bytes_sent_myinfo == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }

                    // RPL_UMODEIS (221)
                    const char* umodeis_message = ":localhost 221 dong-kim +i\r\n";
                    ssize_t bytes_sent_umodeis = send(observe_fd[i].fd, umodeis_message, strlen(umodeis_message), 0);
                    if (bytes_sent_umodeis == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }
                  }
                  else if (msg.get_cmd() == "PING")
                  {
                    std::cout << GREEN << "@@@@" << WHITE << std::endl;
                    std::string ping_message = "localhost";  // PING 메시지의 파라미터 부분

                    // PONG 메시지 생성
                    std::string pong_message = ":localhost PONG localhost :" + ping_message + "\r\n";

                    // PONG 메시지 전송
                    ssize_t bytes_sent_pong = send(observe_fd[i].fd, pong_message.c_str(), pong_message.length(), 0);
                    if (bytes_sent_pong == -1) {
                        // 전송 오류 처리
                        perror("send");
                        // 오류 처리 로직 추가
                    }
                  }

                    // const char* welcome_message1 = ":localhost 001 dong-kim :Welcome to the Internet Relay Network dong-kim!dong-kim@localhost";
                    // ssize_t bytes_sent1 = send(observe_fd[i].fd, welcome_message1, strlen(welcome_message1), 0);
                    // const char* mode_message = ":<irc.example.net> 221 dong-kim +i\r";
                    // ssize_t bytes_sent_mode = send(observe_fd[i].fd, mode_message, strlen(mode_message), 0);
                    // const char* welcome_message2 = ":localhost 002 dong-kim :Your host is localhost, running version 1.0";
                    // ssize_t bytes_sent2 = send(observe_fd[i].fd, welcome_message2, strlen(welcome_message2), 0);
                    // std::cout << msg;
                }
                // const char* welcome_message = ":localhost CAP * LS :multi-prefix sasl\r\n";
                // ssize_t bytes_sent = send(observe_fd[i].fd, welcome_message, strlen(welcome_message), 0);
                
                
                // User& event_user = serv [observe_fd[i].fd];
                // if (event_user.get_is_authenticated() == true) {
                // } else {
                //   /*
                //     code for not authenticated user
                //     only PASS, NICK, USER command accepted
                //   */
                // }
              }
              catch (const std::bad_alloc& e) {
                std::cerr << e.what() << '\n';
                std::cerr
                    << "Not enough memory so can't excute vector.push_back "
                    << "or another things require additional memory"
                    << std::endl;
              }
              catch (const std::length_error& e) {
                std::cerr << e.what() << std::endl;
                std::cerr
                    << "Maybe index out of range error or std::string is too "
                    << "long to store"
                    << std::endl;
              }
              catch (const std::exception& e) {
                // error handling
                std::cerr << e.what() << std::endl;
                std::cerr << "unexpected exception occur! Program terminated!" << std::endl;
                exit(EXIT_FAILURE);
              }
            }
            // if (observe_fd[i].revents & POLLHUP) {
            //   observe_fd[i].fd = -1;
            // }
            // if (observe_fd[i].revents) {
            //   event_cnt--;
            // }
          }
        }
      }
    }
  }
  catch (std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

/*
  socket에서 수신받은 것들을 읽어서 \r\n 단위로 쪼개서 msg_list에 담는다
  \r\n으로 안 끝나는 메세지가 있을 경우 이전 \r\n까지만 메세지로 만들고
  남는 것은 내부에서 가지고 있는다
*/

void read_msg_from_socket(const int socket_fd, std::vector<std::string>& msg_list)
{
  static std::string remains = "";
  static bool incomplete = false;

  char read_block[BLOCK_SIZE] = {0,};
  int read_cnt = 0;
  int idx;
  std::vector<std::string> box;

  msg_list.clear();
  while (true) 
  {
    read_cnt = ::recv(socket_fd, read_block, BLOCK_SIZE - 1, MSG_DONTWAIT);
    if (0 < read_cnt && read_cnt <= BLOCK_SIZE - 1) 
    {
      read_block[read_cnt] = '\0';
      box.clear();
      ft_split(std::string(read_block), "\r\n", box);
      idx = 0;
      if (incomplete == true) {
        remains += box[0];
        msg_list.push_back(remains);
        remains = "";
        idx = 1;
      }
      if (read_block[read_cnt - 2] == '\r' && read_block[read_cnt - 1] == '\n')
      {
        for (; idx < box.size(); idx++)
        {
          if (box[idx].length() != 0) {
            msg_list.push_back(box[idx]);
          }
        }
        incomplete = false;
      }
      else
      {
        for (; idx < box.size() - 1; idx++) {
          if (box[idx].length() != 0) {
            msg_list.push_back(box[idx]);
          }
        }
        remains = box[idx];
        incomplete = true;
      }
      if (read_cnt == BLOCK_SIZE - 1) {
        continue;
      } else {
        break;
      }
    } else if (read_cnt == -1) {
      if (errno == EWOULDBLOCK) {
        // * non-blocking case *
        break;
      } else {
        // * recv() function error *
        // todo : error_handling
        std::cerr << "recv() error\n";
      }
    } else if (read_cnt == 0) {
      // socket connection finish
      // 이 함수는 소켓으로부터 메세지만 읽어들이는 함수이니 여기서 close()
      // 함수를 부르지 말고 외부에서 부를 수 있게 메세지를 남기자
      msg_list.push_back(std::string("connection finish at socket ") + ft_itos(socket_fd));
      break;
    }
  }
}
