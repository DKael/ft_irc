// string::substr
#include <iostream>
#include <string>
#include <vector>

std::vector<std::string>& ft_split(const std::string& str,
                                   const std::string& del,
                                   std::vector<std::string>& box);

int main() {
  std::string str = "We think in generalities, but we live in details.";
  // (quoting Alfred N. Whitehead)

  std::string str2 = str.substr(3, 5);  // "think"

  std::size_t pos = str.find("live");  // position of "live" in str

  std::string str3 = str.substr(pos);  // get from "live" to the end

  std::cout << str2 << ' ' << str3 << '\n';

  std::size_t pos1 = str.find("think");
  std::cout << "pos1 : " << pos1 << '\n';

  std::string msg =
      "PASS secretpasswordhere\r\nNICK Wiz\r\nUSER guest 0 * :Ronnie "
      "Reagan\r\ndummy";
  std::vector<std::string> box;

  ft_split(msg, "\r\n", box);
  for (int i = 0; i < box.size(); i++) {
    std::cout << box[i] << '\n';
  }

  msg = "abcdef123abcdefgd123dfw";
  box.clear();
  ft_split(msg, "123", box);
  for (int i = 0; i < box.size(); i++) {
    std::cout << box[i] << '\n';
  }

  msg = "abcdef123abcdefgd123dfw123";
  box.clear();
  ft_split(msg, "123", box);
  for (int i = 0; i < box.size(); i++) {
    std::cout << box[i] << '\n';
  }

  return 0;
}

std::vector<std::string>& ft_split(const std::string& str,
                                   const std::string& del,
                                   std::vector<std::string>& box) {
  size_t idx1 = 0;
  size_t idx2 = 0;
  while (idx1 < str.length()) {
    idx2 = str.find(del, idx1);
    if (idx2 != std::string::npos) {
      box.push_back(str.substr(idx1, idx2 - idx1));
      idx2 += del.length();
      idx1 = idx2;
    } else {
      box.push_back(str.substr(idx1));
      break;
    }
  }
  return box;
}