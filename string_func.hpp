#ifndef STRING_FUNC_HPP
#define STRING_FUNC_HPP

#include <sstream>
#include <string>

#if !defined(M_NO_BLANK) && !defined(M_BLANK)
#define M_NO_BLANK 0
#define M_BLANK 1
#endif

std::string ft_itos(const int input);
std::string ft_strip(const std::string& origin);

template <typename T>
T& ft_split(const std::string& str, const std::string& del, T& box,
            int mode = M_NO_BLANK) {
  size_t idx1 = 0;
  size_t idx2 = 0;
  std::string tmp;
  int del_len = del.length();

  while (idx1 < str.length()) {
    idx2 = str.find(del, idx1);
    if (idx2 != std::string::npos) {
      tmp = str.substr(idx1, idx2 - idx1);
      idx2 += del_len;
      idx1 = idx2;
    } else {
      tmp = str.substr(idx1);
      idx1 = str.length();
    }
    if (mode == M_NO_BLANK) {
      if (tmp.length() != 0) {
        box.push_back(tmp);
      }
    } else {
      box.push_back(tmp);
    }
  }
  return box;
}

#endif