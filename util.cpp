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

bool ipv4_chk(const char* input_ipv4) {
  std::string ipv4_tmp = input_ipv4;
  std::string tmp_str;
  int tmp_int;
  std::stringstream to_num;
  std::size_t idx1 = 0;
  std::size_t idx2 = 0;

  for (int i = 0; i < 3; i++) {
    idx1 = ipv4_tmp.find('.', idx1);
    if (idx1 != std::string::npos) {
      tmp_str = ipv4_tmp.substr(idx2, idx1 - idx2);
      if (tmp_str.find_first_not_of("0123456789") != std::string::npos) {
        return false;
      }
      to_num.clear();
      to_num << tmp_str;
      to_num >> tmp_int;
      if (to_num.fail() || !(0 <= tmp_int && tmp_int <= 255)) {
        return false;
      }
      idx1++;
      idx2 = idx1;
    } else {
      return false;
    }
  }
  tmp_str = ipv4_tmp.substr(idx2);
  if (tmp_str.find_first_not_of("0123456789") != std::string::npos) {
    return false;
  }
  to_num.clear();
  to_num << tmp_str;
  to_num >> tmp_int;
  if (to_num.fail() || !(0 <= tmp_int && tmp_int <= 255)) {
    return false;
  }
  return true;
}