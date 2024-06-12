#ifndef STRING_FUNC_HPP
#define STRING_FUNC_HPP

#include <sys/time.h>

#include <cstdlib>
#include <sstream>
#include <string>

typedef std::string String;

#if !defined(M_NO_BLANK) && !defined(M_BLANK)
#define M_NO_BLANK 0
#define M_BLANK 1
#endif

#define STR_COMP \
  "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"

std::string ft_itos(int input);
std::string ft_ltos(long input);
std::string ft_strip(const std::string& origin);
std::string make_random_string(std::size_t len,
                               const std::string comp = STR_COMP);
std::string& ft_upper(std::string& origin);
std::string ft_upper(const std::string& origin);

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