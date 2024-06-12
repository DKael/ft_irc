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

  password = argv[3];
  if (ft_strip(password).length() == 0) {
    std::cerr << "Empty password!\n";
    throw std::exception();
  }

  nickname = argv[4];
  if (('0' <= nickname[0] && nickname[0] <= '9') ||
      nickname.find_first_of(": \n\t\v\f\r") != String::npos) {
    std::cerr << "Invalid bot nickname!\n";
    throw std::exception();
  }

  std::ifstream menu_file_read(argv[5]);
  if (menu_file_read.is_open() == false) {
    std::cerr << "Cannot open file!\n";
    throw std::exception();
  } else {
    String buf;

    while (getline(menu_file_read, buf)) {
      if (buf.length() > 0) {
        menu.push_back(buf);
      }
    }
    menu_file_read.close();
    std::cerr << "menu size : " << menu.size() << '\n';
    if (menu.size() == 0) {
      std::cerr << "Blank menu file!\n";
      throw std::exception();
    }
  }
}

void Bot::connect_to_serv(void) {
  bot_sock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (bot_sock == -1) {
    perror("socket() error");
    std::cerr << "errno : " << errno << '\n';
    throw std::exception();
  }

  std::memset(&bot_addr, 0, sizeof(bot_addr));
  bot_addr.sin_family = AF_INET;
  bot_addr.sin_addr.s_addr = inet_addr(ipv4.c_str());
  bot_addr.sin_port = htons(port);

  if (::connect(bot_sock, (sockaddr*)&bot_addr, sizeof(bot_addr)) == -1) {
    perror("connect() error");
    std::cerr << "errno : " << errno << '\n';
    throw std::exception();
  }
  std::cout << "Bot connected to " << ipv4 << ':' << port << '\n';
}

void Bot::step_auth(void) {
  int auth_flag = 0;
  int nick_retry_cnt = 0;
  String nick_retry;
  std::vector<String> msg_list;

  to_send.push(String("PASS ") + password + String("\r\n"));
  to_send.push(String("NICK ") + nickname + String("\r\n"));
  to_send.push(String("USER ") + nickname + String(" 0 * :") + nickname +
               String("\r\n"));

  send_msg_at_queue();

  while (auth_flag != 0x1F) {
    if (remain_msg == true) {
      send_msg_at_queue();
    }
    try {
      read_msg_from_socket(bot_sock, msg_list);

      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (std::size_t i = 0; i < msg_list.size(); i++) {
        if (msg_list[i] == String("connection finish")) {
          std::cerr << "Connection lost\n";
          close(bot_sock);
          exit(0);
        }
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
          Stringstream int_to_str;
          String tmp;

          nick_retry_cnt++;
          int_to_str << nick_retry_cnt;
          int_to_str >> tmp;
          nick_retry = nickname + tmp;
          to_send.push(String("NICK ") + nick_retry + String("\r\n"));
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
      read_msg_from_socket(bot_sock, msg_list);

      if (is_ping_sent == false && time(NULL) > last_ping_chk + PING_INTERVAL) {
        Message ping;
        ping.set_cmd_type(PING);
        ping.push_back(serv_name);
        to_send.push(ping.to_raw_msg());
        send_msg_at_queue();
        ping_send_time = time(NULL);
        is_ping_sent = true;
        is_pong_received = false;
      } else if (is_ping_sent == true && is_pong_received == false &&
                 time(NULL) > ping_send_time + PONG_TIMEOUT) {
        Message rpl;
        rpl.set_cmd_type(QUIT);
        rpl.push_back(":leaving");
        to_send.push(rpl.to_raw_msg());
        send_msg_at_queue();
        close(bot_sock);
        exit(0);
      }

      if (msg_list.size() == 0) {
        sleep(1);
        continue;
      }
      for (std::size_t i = 0; i < msg_list.size(); i++) {
        // std::cerr << "msg " << i << " : " << msg_list[i] << '\n';
        if (msg_list[i] == String("connection finish")) {
          std::cerr << "Connection lost\n";
          close(bot_sock);
          exit(0);
        }
        Message msg(bot_sock, msg_list[i]);

        if (msg.get_cmd_type() == PONG) {
          is_ping_sent = false;
          is_pong_received = true;
          last_ping_chk = time(NULL);
        } else if (msg.get_cmd_type() == PRIVMSG &&
                   msg.get_params_size() >= 2) {
          Message rpl;
          rpl.set_cmd_type(PRIVMSG);

          std::size_t tail = msg.get_source().find("!");
          String who_send = msg.get_source().substr(0, tail);
          rpl.push_back(who_send);
          if (msg[1] == String("lunch menu recommend")) {
            int select = std::rand() % menu.size();
            rpl.push_back(String(":Today's lunch menu recommendation : ") +
                          menu[select]);
          } else if (ft_upper(msg[1]) == String("HELLO") ||
                     ft_upper(msg[1]) == String("HI")) {
            rpl.push_back(":Hi there");
          } else {
            rpl.push_back(":unknown command");
          }
          to_send.push(rpl.to_raw_msg());
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

const String& Bot::get_ipv4(void) { return ipv4; }

int Bot::get_port(void) { return port; }

int Bot::get_bot_sock(void) { return bot_sock; }

const sockaddr_in& Bot::get_bot_adr(void) { return bot_addr; }

const String& Bot::get_password(void) { return password; }

const String& Bot::get_nickname(void) { return nickname; }

void Bot::send_msg_at_queue(void) {
  int send_result;
  std::size_t to_send_num = to_send.size();

  while (to_send_num > 0) {
    const String& msg_tmp = to_send.front();
    send_result =
        send(bot_sock, msg_tmp.c_str(), msg_tmp.length(), MSG_DONTWAIT);
    to_send.pop();
    if (send_result == -1) {
      remain_msg = true;
      return;
    }

    to_send.pop();
    to_send_num--;
  }
  remain_msg = false;
}
