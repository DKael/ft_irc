#include <iostream>
#include <string>

#include "../string_func.cpp"
#include "../string_func.hpp"

int main() {
  std::size_t pos;
  std::string test = "abcdefghijklmnopqrstuvwyz";

  std::cout << "len : " << test.length() << '\n';

  std::cout << "_" << test.substr(0, 0) << "_\n";
  std::cout << "_" << test.substr(2) << "_\n";
  std::cout << "_" << test.substr(25) << "_\n";
  // std::cout << "_" << test.substr(26) << "_\n";

  std::string tmp_trailing;
  std::string test1 = ":dan!d@localhost PRIVMSG #chan :Hey!";

  pos = test1.find(" :");
  if (pos != std::string::npos) {
    // trailing exist
    tmp_trailing = test1.substr(pos + 2);
    std::cout << "_" << tmp_trailing << "_\n";
  }

  std::string test2 = ":dan!d@localhost PRIVMSG #chan :";

  pos = test2.find(" :");
  if (pos != std::string::npos) {
    // trailing exist
    tmp_trailing = test2.substr(pos + 2);
    std::cout << "_" << tmp_trailing << "_\n";
  }

  std::cout << "-------------------------------------------\n";
  // std::cout << "_\r_\n";
  std::cout << "_\n_";

  std::string split_test =
      "aaa\r\nbbb\r\n\r\n\r\nccccc\rddddddddd\neeeee\n\rffffff\r\n";
  std::vector<std::string> box;

  ft_split(split_test, "\r\n", box);
  for (int i = 0; i < box.size(); i++) {
    std::cout << "box[" << i << "] : " << box[i] << '\n';
  }

  std::cout << "------------------------------------------------\n\n";
  box.clear();
  ft_split(split_test, "\r\n", box, M_BLANK);
  for (int i = 0; i < box.size(); i++) {
    std::cout << "box[" << i << "] : " << box[i] << '\n';
  }

  std::cout << "------------------------------------------------\n\n";
  std::string null_char_test = "12345";

  if (null_char_test.find_first_of("\0") != std::string::npos) {
    std::cout << "find null char\n";
  } else {
    std::cout << "cannot find null char\n";
  }
}