#include "string_func.hpp"

std::string ft_itos(const int input) {
  std::stringstream ss_tmp;
  std::string ret;

  ss_tmp << input;
  ss_tmp >> ret;

  return ret;
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

std::string ft_strip(const std::string& origin) {
  std::size_t front_pos;
  std::size_t back_pos;

  front_pos = origin.find_first_not_of(" \n\t\v\f\r");
  back_pos = origin.find_last_not_of(" \n\t\v\f\r");
  if (front_pos == std::string::npos || back_pos == std::string::npos) {
    return "";
  }
  return origin.substr(front_pos, back_pos - front_pos + 1);
}