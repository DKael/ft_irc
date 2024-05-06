#include "util.hpp"

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
      msg_list.push_back(std::string("connection finish"));
      break;
    }
  }
}