#ifndef STRING_FUNC_HPP
#define STRING_FUNC_HPP

#include <sstream>
#include <string>

std::string ft_itos(const int input);
std::string ft_strip(const std::string& origin);

template <typename T>
T& ft_split(const std::string& str, const std::string& del, T& box) {
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

template <typename T>
T& ft_split_no_blank(const std::string& str, const std::string& del, T& box) {
  size_t idx1 = 0;
  size_t idx2 = 0;
  std::string tmp;
  while (idx1 < str.length()) {
    idx2 = str.find(del, idx1);
    if (idx2 != std::string::npos) {
      tmp = str.substr(idx1, idx2 - idx1);
      if (tmp.length() != 0) {
        box.push_back(tmp);
      }
      idx2 += del.length();
      idx1 = idx2;
    } else {
      tmp = str.substr(idx1);
      if (tmp.length() != 0) {
        box.push_back(tmp);
      }
      break;
    }
  }
  return box;
}

#endif