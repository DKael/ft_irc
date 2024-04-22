#include <cstring>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "Server.hpp"

#define BLOCK_SIZE 1025
#define BUFFER_SIZE 65536
#define POLL_TIMEOUT 5

void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list);
std::vector<std::string>& ft_split(const std::string& str,
                                   const std::string& del,
                                   std::vector<std::string>& box);

bool port_chk(const char* input_port) {
  std::stringstream port_chk;
  int port;

  port_chk << std::string(input_port);
  port_chk >> port;
  if (port_chk.fail()) {
    return false;
  } else if (port < 0 || 65335 < port) {
    return false;
  }
  return true;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage : " << argv[0] << " <port> <password to connect>\n";
    return 1;
  } else if (port_chk(argv[1]) == false) {
    std::cerr << "Port range error!\n";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);
    pollfd observe_fd[MAX_USER];
    int event_cnt = 0;

    socklen_t user_addr_len;
    sockaddr_in user_addr;
    int user_socket;
    int read_cnt = 0;
    int flag;

    std::vector<std::string> msg_list;

    observe_fd[0].fd = serv.get_serv_socket();
    observe_fd[0].events = POLLIN;

    for (int i = 1; i < MAX_USER; i++)
      observe_fd[i].fd = -1;

    while (1)
    {
      event_cnt = poll(observe_fd, MAX_USER, POLL_TIMEOUT * 1000);

      if (event_cnt == 0) {
        continue;
      } else if (event_cnt < 0) {
        // std::cerr << "poll() error!\n";
        perror("poll() error!");
        return 1;
      } else {
        // 새로운 클라이언트 연결 요청 감지 => observe_fd[0]
        if (observe_fd[0].revents & POLLIN)
        {
          if (serv.get_user_cnt() == MAX_USER) {
            // todo : send some error message to client
            // 근데 이거 클라이언트한테 메세지 보내려면 소켓을 연결해야 하는데
            // 에러 메세지 답장 안 보내고 연결요청 무시하려면 되려나
            // 아니면 예비소켓 하나 남겨두고 잠시 연결했다 바로 연결 종료하는
            // 용도로 사용하는 것도 방법일 듯
            std::cerr << "User connection limit exceed!\n";
            continue ;
          }
          while (1) {
            memset(&user_addr, 0, sizeof(user_addr));
            memset(&user_addr_len, 0, sizeof(user_addr_len));
            user_addr_len = sizeof(user_addr);
            user_socket = ::accept(serv.get_serv_socket(), (sockaddr*)&user_addr, &user_addr_len);
            if (user_socket == -1)
            {
              if (errno == EWOULDBLOCK) // 더 이상 대기 중이 클라이언트가 없음
              {
                close(user_socket);
                break ;
              }
              else
              {
                // error_handling
                // accept 함수 에러나는 경우 찾아보자
                std::cerr << "accept() error\n";
              }
            }
            serv.add_tmp_user(user_socket, user_addr);
            for (int i = 1; i < MAX_USER; i++)
            {
              if (observe_fd[i].fd == -1)
              {
                observe_fd[i].fd = user_socket;
                observe_fd[i].events = POLLIN;
                break;
              }
            }
          }
        }

        for (int i = 1; i < MAX_USER && event_cnt > 0; i++)
        {
          if (observe_fd[i].fd > 0) 
          {
            if (observe_fd[i].revents & POLLIN)
            {
              try 
              {
                read_msg_from_socket(observe_fd[i].fd, msg_list);

                // print msg_list to check message read is ok
                // this part will be removed
                std::cerr << "printed at socket read part\n";
                for (int i = 0; i < msg_list.size(); i++) {
                  std::cerr << msg_list[i] << '\n';
                }

                User& event_user = serv[observe_fd[i].fd];

                if (event_user.get_is_authenticated() == true) 
                {
                  
                }
                else
                {
                  /*
                  code for not authenticated user
                  only PASS, NICK, USER command accepted
                  */
                }

              }
              catch (const std::bad_alloc& e)
              {
                std::cerr << e.what() << '\n';
                std::cerr
                    << "Not enough memory so can't excute vector.push_back "
                       "or another things require additional memory\n";
              }
              catch (const std::length_error& e)
              {
                std::cerr << e.what() << '\n';
                std::cerr
                    << "Maybe index out of range error or std::string is too "
                       "long to store\n";
              }
              catch (const std::exception& e)
              {
                // error handling
                std::cerr << e.what() << '\n';
                std::cerr
                    << "unexpected exception occur! Program terminated!\n";
                exit(1);
              }
            }
            if (observe_fd[i].revents & POLLHUP)
            {
              observe_fd[i].fd = -1;
            }
            if (observe_fd[i].revents)
            {
              event_cnt--;
            }
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

  char read_block[BLOCK_SIZE] = {
      0,
  };
  int read_cnt = 0;
  int idx;
  std::vector<std::string> box;

  msg_list.clear();
  while (true)
  {
    read_cnt = ::recv(socket_fd, read_block, BLOCK_SIZE - 1, MSG_DONTWAIT);
    if (0 < read_cnt && read_cnt <= BLOCK_SIZE - 1) {
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
      if (read_block[read_cnt - 2] == '\r' &&
          read_block[read_cnt - 1] == '\n') {
        for (; idx < box.size(); idx++) {
          msg_list.push_back(box[idx]);
        }
        incomplete = false;
      } else {
        for (; idx < box.size() - 1; idx++) {
          msg_list.push_back(box[idx]);
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
      msg_list.push_back(std::string("connection finish at socket ") +
                         ft_itos(socket_fd));
      break;
    }
  }
}

std::string ft_itos(const int input) {
  std::stringstream ss_tmp;
  std::string ret;

  ss_tmp << input;
  ss_tmp >> ret;

  return ret;
}

std::vector<std::string>& ft_split(const std::string& str,
                                   const std::string& del,
                                   std::vector<std::string>& box) {
  size_t idx1 = 0;
  size_t idx2 = 0;
  while (idx1 < str.length()) {
    idx2 = str.find(del, idx1);
    if (idx2 != std::string::npos) {
      box.push_back(str.substr(idx1, idx2 - idx1));
      idx2 += del.length();
      idx1 = idx2;
    } else {
      box.push_back(str.substr(idx1));
      break;
    }
  }
  return box;
}