#include <cstring>
#include <exception>
#include <iostream>
#include <list>
#include <sstream>
#include <string>
#include <vector>

#include "Message.hpp"
#include "Server.hpp"
#include "string_func.hpp"

#define BLOCK_SIZE 1025
#define BUFFER_SIZE 65536
#define POLL_TIMEOUT 5

void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list);

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
  } else if (ft_strip(std::string(argv[2])).length() == 0) {
    std::cerr << "Empty password!";
    return 1;
  }

  try {
    Server serv(argv[1], argv[2]);
    pollfd observe_fd[MAX_USER];
    int event_cnt = 0;
    Message::map_init();

    socklen_t user_addr_len;
    sockaddr_in user_addr;
    int user_socket;
    int read_cnt = 0;
    int flag;

    std::vector<std::string> msg_list;

    observe_fd[0].fd = serv.get_serv_socket();
    observe_fd[0].events = POLLIN;

    for (int i = 1; i < MAX_USER; i++) {
      observe_fd[i].fd = -1;
    }

    while (1) {
      event_cnt = poll(observe_fd, MAX_USER, POLL_TIMEOUT * 1000);

      if (event_cnt == 0) {
        continue;
      } else if (event_cnt < 0) {
        // std::cerr << "poll() error!\n";
        perror("poll() error!");
        return 1;
      } else {
        if (observe_fd[0].revents & POLLIN) {
          while (true) {
            user_addr_len = sizeof(user_addr);
            std::memset(&user_addr, 0, user_addr_len);
            user_socket = ::accept(serv.get_serv_socket(),
                                   (sockaddr*)&user_addr, &user_addr_len);
            if (user_socket == -1) {
              if (errno == EWOULDBLOCK) {
                break;
              } else {
                // error_handling
                // accept 함수 에러나는 경우 찾아보자
                std::cerr << "accept() error\n";
                exit(1);
              }
            }

            if (::fcntl(user_socket, F_SETFL, O_NONBLOCK) == -1) {
              // error_handling
              std::cerr << "fcntl() error\n";
            }

            if (serv.get_user_cnt() == MAX_USER) {
              // todo : send some error message to client
              // 근데 이거 클라이언트한테 메세지 보내려면 소켓을 연결해야 하는데
              // 에러 메세지 답장 안 보내고 연결요청 무시하려면 되려나
              // 아니면 예비소켓 하나 남겨두고 잠시 연결했다 바로 연결 종료하는
              // 용도로 사용하는 것도 방법일 듯
              std::cerr << "User connection limit exceed!\n";
              send(user_socket, "ERROR :Too many users on server",
                   std::strlen("ERROR :Too many users on server"),
                   MSG_DONTWAIT);
              close(user_socket);
            }

            serv.add_tmp_user(user_socket, user_addr);
            for (int i = 1; i < MAX_USER; i++) {
              if (observe_fd[i].fd == -1) {
                observe_fd[i].fd = user_socket;
                observe_fd[i].events = POLLIN;
                break;
              }
            }
          }

          event_cnt--;
        }

        for (int i = 1; i < MAX_USER && event_cnt > 0; i++) {
          if (observe_fd[i].fd > 0) {
            if (observe_fd[i].revents & POLLOUT) {
              if (serv[observe_fd[i].fd].get_have_to_disconnect() == false) {
                if (serv.send_msg(observe_fd[i].fd) == -1) {
                  observe_fd[i].events = POLLIN | POLLOUT;
                } else {
                  observe_fd[i].events = POLLIN;
                }
              } else {
                if (serv.send_msg(observe_fd[i].fd) != -1) {
                  serv.remove_user(observe_fd[i].fd);
                }
              }
            }
            if (observe_fd[i].revents & POLLIN) {
              try {
                read_msg_from_socket(observe_fd[i].fd, msg_list);

                User& event_user = serv[observe_fd[i].fd];

                if (event_user.get_is_authenticated() == OK) {
                  for (int j = 0; j < msg_list.size(); j++) {
                    Message msg(observe_fd[i].fd, msg_list[j]);
                    int cmd_type = msg.get_cmd_type();
                    if (cmd_type == MODE) {
                      rpl.set_source(event_user.get_nick_name() +
                                     std::string("!") +
                                     event_user.get_user_name() +
                                     std::string("@localhost"));
                      rpl.set_cmd_type(MODE);
                      rpl.push_back(event_user.get_nick_name());
                      rpl.set_trailing(msg[0]);
                      event_user.push_msg(rpl.to_raw_msg());
                    } else if (cmd_type == PING) {
                      rpl.set_source(serv.get_serv_name());
                      rpl.set_cmd_type(PONG);
                      rpl.push_back(serv.get_serv_name());
                      rpl.set_trailing(serv.get_serv_name());
                      event_user.push_msg(rpl.to_raw_msg());
                    }
                    if (serv.send_msg(event_user.get_user_socket()) == -1) {
                      observe_fd[i].events = POLLIN | POLLOUT;
                    } else {
                      observe_fd[i].events = POLLIN;
                    }
                  }

                } else {
                  /*
                  code for not authenticated user
                  only PASS, NICK, USER command accepted
                  */

                  for (int j = 0; j < msg_list.size(); j++) {
                    Message msg(observe_fd[i].fd, msg_list[j]);

                    int cmd_type = msg.get_cmd_type();
                    if (cmd_type == PASS) {
                      serv.cmd_pass(observe_fd[i].fd, msg);
                    } else if (cmd_type == NICK) {
                      if (event_user.get_password_chk() == NOT_YET) {
                        event_user.set_password_chk(FAIL);
                      }
                      if (msg.get_params_size() == 0) {
                        rpl.set_source(serv.get_serv_name());
                        rpl.set_numeric("461");
                        if (event_user.get_nick_init_chk() == NOT_YET) {
                          rpl.push_back("*");
                        } else {
                          rpl.push_back(event_user.get_nick_name());
                        }
                        rpl.push_back(msg.get_cmd());
                        rpl.set_trailing("Syntax error");
                        event_user.push_msg(rpl.to_raw_msg());
                      } else {
                        try {
                          serv[msg[0]];
                          rpl.set_source(serv.get_serv_name());
                          rpl.set_numeric("433");
                          if (event_user.get_nick_init_chk() == NOT_YET) {
                            rpl.push_back("*");
                          } else {
                            rpl.push_back(event_user.get_nick_name());
                          }
                          rpl.push_back(msg[0]);
                          rpl.set_trailing("Nickname already in use");
                          event_user.push_msg(rpl.to_raw_msg());
                        } catch (std::invalid_argument& e) {
                          serv.change_nickname(event_user.get_nick_name(),
                                               msg[0]);
                          event_user.set_nick_init_chk(OK);
                        }
                      }
                    } else if (cmd_type == USER) {
                      if (event_user.get_password_chk() == NOT_YET) {
                        event_user.set_password_chk(FAIL);
                      }
                      if (!((msg.get_params_size() == 4 &&
                             msg.get_trailing().length() == 0) ||
                            (msg.get_params_size() == 3 &&
                             msg.get_trailing().length() != 0))) {
                        rpl.set_source(serv.get_serv_name());
                        rpl.set_numeric("461");
                        if (event_user.get_nick_init_chk() == NOT_YET) {
                          rpl.push_back("*");
                        } else {
                          rpl.push_back(event_user.get_nick_name());
                        }
                        rpl.push_back(msg.get_cmd());
                        rpl.set_trailing("Syntax error");
                        event_user.push_msg(rpl.to_raw_msg());
                      } else {
                        if (event_user.get_user_init_chk() == NOT_YET) {
                          if (serv.get_enable_ident_protocol() == true) {
                            event_user.set_user_name(msg[0]);
                          } else {
                            event_user.set_user_name(std::string("~") + msg[0]);
                          }
                          if (msg.get_params_size() == 4) {
                            event_user.set_real_name(msg[3]);
                          } else {
                            event_user.set_real_name(msg.get_trailing());
                          }
                          event_user.set_user_init_chk(OK);
                        } else {
                          rpl.set_source(serv.get_serv_name());
                          rpl.set_numeric("451");
                          if (event_user.get_nick_init_chk() == NOT_YET) {
                            rpl.push_back("*");
                          } else {
                            rpl.push_back(event_user.get_nick_name());
                          }
                          rpl.set_trailing("Connection not registered");
                          event_user.push_msg(rpl.to_raw_msg());
                        }
                      }
                    } else if (cmd_type == CAP) {
                      continue;
                    } else if (cmd_type == ERROR) {
                      event_user.push_msg(msg.to_raw_msg());
                    } else {
                      rpl.set_source(serv.get_serv_name());
                      rpl.set_numeric("451");
                      if (event_user.get_nick_init_chk() == NOT_YET) {
                        rpl.push_back("*");
                      } else {
                        rpl.push_back(event_user.get_nick_name());
                      }
                      rpl.set_trailing("Connection not registered");
                      event_user.push_msg(rpl.to_raw_msg());
                    }
                    if (serv.send_msg(event_user.get_user_socket()) == -1) {
                      observe_fd[i].events = POLLIN | POLLOUT;
                    } else {
                      observe_fd[i].events = POLLIN;
                    }
                    if (event_user.get_nick_init_chk() == OK &&
                        event_user.get_user_init_chk() == OK) {
                      if (event_user.get_password_chk() != OK) {
                        event_user.set_have_to_disconnect(true);
                        rpl.set_cmd_type(ERROR);
                        rpl.set_trailing("Access denied: Bad password?");
                        event_user.push_msg(rpl.to_raw_msg());
                        if (serv.send_msg(event_user.get_user_socket()) == -1) {
                          observe_fd[i].events = POLLOUT;
                        } else {
                          serv.remove_user(event_user.get_user_socket());
                        }
                      } else {
                        // authenticate complete
                        int tmp_fd = event_user.get_user_socket();
                        serv.move_tmp_user_to_user_list(
                            event_user.get_user_socket());
                        User& event_user1 = serv[tmp_fd];
                        rpl.set_source(serv.get_serv_name());
                        rpl.push_back(event_user.get_nick_name());

                        rpl.set_numeric("001");
                        rpl.set_trailing(
                            "Welcome to the Internet Relay Network");
                        event_user1.push_msg(rpl.to_raw_msg());

                        rpl.set_numeric("002");
                        rpl.set_trailing(std::string("Your host is ") +
                                         serv.get_serv_name());
                        event_user1.push_msg(rpl.to_raw_msg());

                        rpl.set_numeric("003");
                        rpl.set_trailing("This server has been started ~");
                        event_user1.push_msg(rpl.to_raw_msg());

                        rpl.set_numeric("004");
                        rpl.set_trailing(serv.get_serv_name());
                        event_user1.push_msg(rpl.to_raw_msg());

                        rpl.set_numeric("005");
                        rpl.push_back("RFC2812");
                        rpl.push_back("IRCD=ngIRCd");
                        rpl.push_back("CHARSET=UTF-8");
                        rpl.push_back("CASEMAPPING=ascii");
                        rpl.push_back("PREFIX=(qaohv)~&@%+");
                        rpl.push_back("CHANTYPES=#&+");
                        rpl.push_back("CHANMODES=beI,k,l,imMnOPQRstVz");
                        rpl.push_back("CHANLIMIT=#&+:10");
                        rpl.set_trailing("are supported on this server");
                        event_user1.push_msg(rpl.to_raw_msg());

                        rpl.clear();
                        rpl.push_back(event_user.get_nick_name());
                        rpl.push_back("CHANNELLEN=50");
                        rpl.push_back("NICKLEN=9");
                        rpl.push_back("TOPICLEN=490");
                        rpl.push_back("AWAYLEN=127");
                        rpl.push_back("KICKLEN=400");
                        rpl.push_back("MODES=5");
                        rpl.push_back("MAXLIST=beI:50");
                        rpl.push_back("EXCEPTS=e");
                        rpl.push_back("PENALTY");
                        rpl.push_back("FNC");
                        event_user1.push_msg(rpl.to_raw_msg());
                        event_user1.set_is_authenticated(OK);

                        if (serv.send_msg(event_user1.get_user_socket()) ==
                            -1) {
                          observe_fd[i].events = POLLIN | POLLOUT;
                        } else {
                          observe_fd[i].events = POLLIN;
                        }
                      }
                    }
                  }
                }
              } catch (const std::bad_alloc& e) {
                std::cerr << e.what() << '\n';
                std::cerr
                    << "Not enough memory so can't excute vector.push_back "
                       "or another things require additional memory\n";
              } catch (const std::length_error& e) {
                std::cerr << e.what() << '\n';
                std::cerr
                    << "Maybe index out of range error or std::string is too "
                       "long to store\n";
              } catch (const std::exception& e) {
                // error handling
                std::cerr << e.what() << '\n';
                std::cerr
                    << "unexpected exception occur! Program terminated!\n";
                exit(1);
              }
            }
            if (observe_fd[i].revents & POLLHUP) {
              observe_fd[i].fd = -1;
            }
            if (observe_fd[i].revents) {
              event_cnt--;
            }
          }
        }
      }
    }
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}

/*
socket에서 수신받은 것들을 읽어서 \r\n 단위로 쪼개서 msg_list에 담는다
\r\n으로 안 끝나는 메세지가 있을 경우 이전 \r\n까지만 메세지로 만들고
남는 것은 내부에서 가지고 있는다
*/
void read_msg_from_socket(const int socket_fd,
                          std::vector<std::string>& msg_list) {
  static std::string remains = "";
  static bool incomplete = false;

  char read_block[BLOCK_SIZE] = {
      0,
  };
  int read_cnt = 0;
  int idx;
  std::vector<std::string> box;

  msg_list.clear();
  while (true) {
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
          if (box[idx].length() != 0) {
            msg_list.push_back(box[idx]);
          }
        }
        incomplete = false;
      } else {
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
      msg_list.push_back(std::string("connection finish at socket ") +
                         ft_itos(socket_fd));
      break;
    }
  }
}
