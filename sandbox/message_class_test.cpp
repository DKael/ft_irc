
#include <iostream>
#include <string>

#include "../Message.hpp"

int main() {
  Message::map_init();
  Message m1("NICK kael");
  Message m2(":irc.example.com CAP * LIST :");
  Message m3("CAP * LS :multi-prefix sasl ");
  Message m4("CAP REQ :sasl message-tags foo");
  Message m5(":dan!d@localhost PRIVMSG #chan :Hey!");
  Message m6(":dan!d@localhost PRIVMSG #chan Hey!");
  Message m7(":dan!d@localhost PRIVMSG #chan ::-)");
  Message m8(":");
  Message m9(":dsfdsfw ");
  Message m10("fniowd");
  Message m11("fniowd dfwwee");
  Message m12(":dsfdsfw NICK dan");

  std::cout << m1 << '\n';
  std::cout << m2 << '\n';
  std::cout << m3 << '\n';
  std::cout << m4 << '\n';
  std::cout << m5 << '\n';
  std::cout << m6 << '\n';
  std::cout << m7 << '\n';
  std::cout << m8 << '\n';
  std::cout << m9 << '\n';
  std::cout << m10 << '\n';
  std::cout << m11 << '\n';
  std::cout << m12 << '\n';
};