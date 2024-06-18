#include "Bot.hpp"

Bot* g_bot_ptr;

void on_sig(int sig) {
  signal(sig, SIG_IGN);

  if (sig != SIGPIPE) {
    send(g_bot_ptr->get_bot_sock(), "QUIT :leaving\r\n", 15, O_NONBLOCK);
  }
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
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);
    signal(SIGPIPE, on_sig);

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
