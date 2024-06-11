
#include <iostream>
#include <string>

#include "../Message.hpp"

int main() {
  Message::map_init();
  Message m1(0, "NicK kael");
  Message m2(0, ":irc.example.com CAP * LIST :");
  Message m3(0, "CAP * LS :multi-prefix sasl ");
  Message m4(0, "CAP REQ :sasl message-tags foo");
  Message m5(0, ":dan!d@localhost PRIvmSG #chan :Hey!");
  Message m6(0, ":dan!d@localhost PRIVMSG #chan Hey!");
  Message m7(0, ":dan!d@localhost PriVMSG #chan ::-)");
  Message m8(0, ":");
  Message m9(0, ":dsfdsfw ");
  Message m10(0, "fniowd");
  Message m11(0, "fniowd dfwwee");
  Message m12(0, ":dsfdsfw NICK dan");
  Message m13(0, "nick \"        \"");

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
  std::cout << m13 << '\n' << m13.to_raw_msg() << '\n';
};