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

String ft_itos(int input);
String ft_ltos(long input);
String ft_strip(const String& origin);
String make_random_string(std::size_t len, const String comp = STR_COMP);
String& ft_upper(String& origin);
String ft_upper(const String& origin);

template <typename T>
T& ft_split(const String& str, const String& del, T& box,
            int mode = M_NO_BLANK) {
  size_t idx1 = 0;
  size_t idx2 = 0;
  String tmp;
  int del_len = del.length();

  while (idx1 < str.length()) {
    idx2 = str.find(del, idx1);
    if (idx2 != String::npos) {
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