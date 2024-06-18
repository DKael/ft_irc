#include "Bot.hpp"

Bot* g_bot_ptr;

void on_sigint(int sig) {
  signal(sig, SIG_IGN);

  send(g_bot_ptr->get_bot_sock(), "QUIT :leaving\r\n", 15, MSG_EOF);
  close(g_bot_ptr->get_bot_sock());
  exit(130);
}

int main(int argc, char** argv) {
  if (argc != 6) {
    std::cerr
        << "Usage : " << argv[0]
        << " <IP> <port> <password to connect> <bot_nickname> <file_dir>\n";
    return 1;
  }

  try {
    Bot bot(argv);

    g_bot_ptr = &bot;
    signal(SIGINT, on_sigint);
    Message::map_init();
    timeval t;
    gettimeofday(&t, NULL);
    std::srand(static_cast<unsigned int>(t.tv_usec));

    bot.connect_to_serv();
    bot.step_auth();
    bot.step_listen();

  } catch (const std::exception& e) {
    std::cerr << "Unexpected exception occured!\n";
    std::cerr << e.what();
    exit(1);
  }
}
