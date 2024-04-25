
#include <iostream>
#include <string>

#include "../Message.hpp"

int main() {
  Message::map_init();
  Message m1("NicK kael", 0);
  Message m2(":irc.example.com CAP * LIST :", 0);
  Message m3("CAP * LS :multi-prefix sasl ", 0);
  Message m4("CAP REQ :sasl message-tags foo", 0);
  Message m5(":dan!d@localhost PRIvmSG #chan :Hey!", 0);
  Message m6(":dan!d@localhost PRIVMSG #chan Hey!", 0);
  Message m7(":dan!d@localhost PriVMSG #chan ::-)", 0);
  Message m8(":", 0);
  Message m9(":dsfdsfw ", 0);
  Message m10("fniowd", 0);
  Message m11("fniowd dfwwee", 0);
  Message m12(":dsfdsfw NICK dan", 0);

  std::cout << m1 << '\n' << m1.to_raw_msg() << '\n';
  std::cout << m2 << '\n' << m2.to_raw_msg() << '\n';
  std::cout << m3 << '\n' << m3.to_raw_msg() << '\n';
  std::cout << m4 << '\n' << m4.to_raw_msg() << '\n';
  std::cout << m5 << '\n' << m5.to_raw_msg() << '\n';
  std::cout << m6 << '\n' << m6.to_raw_msg() << '\n';
  std::cout << m7 << '\n' << m7.to_raw_msg() << '\n';
  std::cout << m8 << '\n' << m8.to_raw_msg() << '\n';
  std::cout << m9 << '\n' << m9.to_raw_msg() << '\n';
  std::cout << m10 << '\n' << m10.to_raw_msg() << '\n';
  std::cout << m11 << '\n' << m11.to_raw_msg() << '\n';
  std::cout << m12 << '\n' << m12.to_raw_msg() << '\n';
};