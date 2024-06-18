#include "Bot.hpp"

Bot::Bot(char** argv) : remain_msg(false) {
  if (ipv4_chk(argv[1]) == false) {
    std::cerr << "Invalid ip format!\n";
    throw std::exception();
  } else {
    ipv4 = argv[1];
  }

  if (port_chk(argv[2]) == false) {
    std::cerr << "Port range error!\n";
    throw std::exception();
  } else {
    port = std::atoi(argv[2]);
  }

  password = ft_strip(argv[3]);
  if (password.length() == 0) {
    std::cerr << "Empty password!\n";
    throw std::exception();
  }

  nickname = ft_strip(argv[4]);
  if (nickname.length() == 0 || ('0' <= nickname[0] && nickname[0] <= '9') ||
      nickname.find_first_of(": \n\t\v\f\r") != String::npos) {
    std::cerr << "Invalid bot nickname!\n";
    throw std::exception();
  }

  std::ifstream menu_file_read(argv[5]);
  if (menu_file_read.is_open() == false) {
    std::cerr << "Cannot open file!\n";
    throw std::exception();
  }

  String buf;

  while (getline(menu_file_read, buf)) {
    buf = ft_strip(buf);
    if (buf.length() > 0) {
      menu.push_back(buf);
    }
  }
  menu_file_read.close();
  std::cout << "menu size : " << menu.size() << '\n';
  if (menu.size() == 0) {
    std::cerr << "Blank menu file!\n";
    throw std::exception();
  }
}

void Bot::connect_to_serv(void) {
  bot_sock = socket(PF_INET, SOCK_STREAM, 0);
  if (bot_sock == -1) {
    perror("socket() error");
    throw std::exception();
  }

  memset(&bot_addr, 0, sizeof(bot_addr));
  bot_addr.sin_family = AF_INET;
  bot_addr.sin_addr.s_addr = inet_addr(ipv4.c_str());
  bot_addr.sin_port = htons(port);

  if (connect(bot_sock, (sockaddr*)&bot_addr, sizeof(bot_addr)) == -1) {
    perror("connect() error");
    throw std::exception();
  }
  int bufSize = SOCKET_BUFFER_SIZE;
  if (setsockopt(bot_sock, SOL_SOCKET, SO_SNDBUF, &bufSize, sizeof(bufSize)) ==
      -1) {
    perror("setsockopt() error");
    send(bot_sock, "QUIT :leaving\r\n", 15, O_NONBLOCK);
    close(bot_sock);
    throw std::exception();
  }

  std::cout << "Bot connected to " << ipv4 << ':' << port << '\n';
}

void Bot::step_auth(void) {
  int auth_flag = 0;
  int nick_retry_cnt = 0;
  String nick_retry;
  std::vector<String> msg_list;

  to_send.push_back(String("PASS ") + password + String("\r\n"));
  to_send.push_back(String("NICK ") + nickname + String("\r\n"));
  to_send.push_back(String("USER ") + nickname + String(" 0 * :") + nickname +
                    String("\r\n"));

  send_msg_at_queue();

  while (auth_flag != 0x1F) {
    if (remain_msg == true) {
      send_msg_at_queue();
    }
    try {
      msg_list.clear();
      read_msg_from_socket(msg_list);

      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (std::size_t i = 0; i < msg_list.size(); i++) {
        Message msg(bot_sock, msg_list[i]);

        if (msg.get_numeric() == String("001")) {
          auth_flag |= NUMERIC_001;
        } else if (msg.get_numeric() == String("002")) {
          auth_flag |= NUMERIC_002;
        } else if (msg.get_numeric() == String("003")) {
          auth_flag |= NUMERIC_003;
        } else if (msg.get_numeric() == String("004")) {
          auth_flag |= NUMERIC_004;
          serv_name = msg[1];
        } else if (msg.get_numeric() == String("005")) {
          auth_flag |= NUMERIC_005;
        } else if (msg.get_numeric() == String("433")) {
          std::stringstream int_to_str;
          String tmp;

          nick_retry_cnt++;
          int_to_str << nick_retry_cnt;
          int_to_str >> tmp;
          nick_retry = nickname + tmp;
          to_send.push_back(String("NICK ") + nick_retry + String("\r\n"));
          send_msg_at_queue();
        } else if (msg.get_numeric() == String("421") ||
                   msg.get_numeric() == String("432") ||
                   msg.get_numeric() == String("451") ||
                   msg.get_numeric() == String("462") ||
                   msg.get_numeric() == String("464") ||
                   msg.get_cmd_type() == ERROR) {
          std::cerr << msg_list[i];
          close(bot_sock);
          exit(1);
        }
      }
    } catch (const std::bad_alloc& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Not enough memory so can't excute vector.push_back "
                   "or another things require additional memory\n";
    } catch (const std::length_error& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Maybe index out of range error or String is too "
                   "long to store\n";
    } catch (const std::exception& e) {
      // error handling
      std::cerr << e.what() << '\n';
      std::cerr << "unexpected exception occur! Program terminated!\n";
      exit(1);
    }
  }
  if (nick_retry_cnt != 0) {
    nickname = nick_retry;
  }
}

void Bot::step_listen(void) {
  time_t last_ping_chk = time(NULL);
  time_t ping_send_time = time(NULL);
  bool is_ping_sent = false;
  bool is_pong_received = false;
  std::vector<String> msg_list;

  while (true) {
    if (remain_msg == true) {
      send_msg_at_queue();
    }
    try {
      if (is_ping_sent == false && time(NULL) > last_ping_chk + PING_INTERVAL) {
        to_send.push_back("PING " + serv_name + "\r\n");
        send_msg_at_queue();
        ping_send_time = time(NULL);
        is_ping_sent = true;
        is_pong_received = false;
      } else if (is_ping_sent == true && is_pong_received == false &&
                 time(NULL) > ping_send_time + PONG_TIMEOUT) {
        to_send.push_back("QUIT :leaving\r\n");
        send_msg_at_queue();
        close(bot_sock);
        exit(0);
      }

      msg_list.clear();
      read_msg_from_socket(msg_list);
      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (std::size_t i = 0; i < msg_list.size(); i++) {
        Message msg(bot_sock, msg_list[i]);

        if (msg.get_cmd_type() == PONG) {
          is_ping_sent = false;
          is_pong_received = true;
          last_ping_chk = time(NULL);
        } else if (msg.get_cmd_type() == PRIVMSG &&
                   msg.get_params_size() >= 2) {
          Message rpl;
          rpl.set_cmd_type(PRIVMSG);

          size_t tail = msg.get_source().find_first_of("!");
          if (tail == String::npos) {
            continue;
          }
          String who_send = msg.get_source().substr(0, tail);
          rpl.push_back(who_send);
          if (msg[1] == "lunch menu recommend" || msg[1] == "lunch") {
            int select = std::rand() % menu.size();
            rpl.push_back(String(":Today's lunch menu recommendation : ") +
                          menu[select]);
          } else if (ft_upper(msg[1]) == "HELLO" || ft_upper(msg[1]) == "HI") {
            rpl.push_back(":Hi there");
          } else {
            rpl.push_back(":unknown command");
          }
          to_send.push_back(rpl.to_raw_msg());
          send_msg_at_queue();
        }
      }
    } catch (const std::bad_alloc& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Not enough memory so can't excute vector.push_back "
                   "or another things require additional memory\n";
    } catch (const std::length_error& e) {
      std::cerr << e.what() << '\n';
      std::cerr << "Maybe index out of range error or String is too "
                   "long to store\n";
    } catch (const std::exception& e) {
      // error handling
      std::cerr << e.what() << '\n';
      std::cerr << "unexpected exception occur! Program terminated!\n";
      exit(1);
    }
  }
}

int Bot::get_port(void) { return port; }

int Bot::get_bot_sock(void) { return bot_sock; }

const String& Bot::get_ipv4(void) { return ipv4; }

const sockaddr_in& Bot::get_bot_adr(void) { return bot_addr; }

const String& Bot::get_password(void) { return password; }

const String& Bot::get_nickname(void) { return nickname; }

int Bot::send_msg_at_queue(void) {
  size_t to_send_num = to_send.size();
  size_t msg_len;
  size_t idx;
  ssize_t bytes_sent;
  bool error_flag;

  while (to_send_num > 0) {
    const String& msg = to_send.front();
    msg_len = msg.length();
    idx = 0;
    error_flag = false;

    while (idx < msg_len) {
      String msg_blk = msg.substr(idx, SOCKET_BUFFER_SIZE);
      bytes_sent = send_msg_block(bot_sock, msg_blk);
      if (bytes_sent == static_cast<ssize_t>(msg_blk.length())) {
        idx += msg_blk.length();
      } else {
        error_flag = true;
        break;
      }
    }
    to_send.pop_front();
    if (error_flag == true) {
      to_send.push_front(msg.substr(idx + bytes_sent));
      remain_msg = true;
      return -1;
    }
    to_send_num--;
  }
  remain_msg = false;
  return 0;
}

ssize_t Bot::send_msg_block(int socket_fd, const String& blk) {
  const char* c_blk = blk.c_str();
  size_t blk_len = blk.length();
  ssize_t bytes_sent = 0;
  size_t total_bytes_sent = 0;

  while (total_bytes_sent < blk_len) {
    bytes_sent = send(socket_fd, c_blk + total_bytes_sent,
                      blk_len - total_bytes_sent, MSG_DONTWAIT);
    if (bytes_sent < 0) {
      break;
    }
    total_bytes_sent += bytes_sent;
  }
  return total_bytes_sent;
}

void Bot::read_msg_from_socket(std::vector<String>& msg_list) {
  char read_block[SOCKET_BUFFER_SIZE] = {
      0,
  };
  int repeat_cnt = 5;
  int read_cnt = 0;
  String read_buf;
  size_t end_idx;

  while (--repeat_cnt >= 0) {
    read_cnt = recv(bot_sock, read_block, SOCKET_BUFFER_SIZE - 1, MSG_DONTWAIT);
    read_block[read_cnt] = '\0';
    read_buf += read_block;
    if (read_cnt < SOCKET_BUFFER_SIZE - 1) {
      break;
    }
  }
  if (read_cnt == 0) {
    exit(0);
  }
  if (read_buf.length() == 0) {
    return;
  }

  size_t front_pos = read_buf.find_first_not_of(" \n\t\v\f\r");
  size_t back_pos = read_buf.find_last_not_of(" \n\t\v\f\r");
  if (front_pos == String::npos || back_pos == String::npos) {
    return;
  }

  ft_split(read_buf.substr(front_pos, back_pos - front_pos + 1), "\r\n",
           msg_list);
  if (remain_input.length() != 0) {
    msg_list[0] = remain_input + msg_list[0];
    remain_input = "";
  }
  end_idx = read_buf.find_last_not_of("\r\n");
  if (end_idx == read_buf.length() - 1) {
    remain_input = *(msg_list.end() - 1);
    msg_list.erase(msg_list.end() - 1);
  }
}
