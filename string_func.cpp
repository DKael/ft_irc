#include "string_func.hpp"

String ft_itos(int input) {
  std::stringstream ss_tmp;
  String ret;

  ss_tmp << input;
  ss_tmp >> ret;

  return ret;
}

String ft_ltos(long input) {
  std::stringstream ss_tmp;
  String ret;

  ss_tmp << input;
  ss_tmp >> ret;

  return ret;
}

String ft_strip(const String& origin) {
  std::size_t front_pos;
  std::size_t back_pos;

  front_pos = origin.find_first_not_of(" \n\t\v\f\r");
  back_pos = origin.find_last_not_of(" \n\t\v\f\r");
  if (front_pos == String::npos || back_pos == String::npos) {
    return "";
  }
  return origin.substr(front_pos, back_pos - front_pos + 1);
}

String ft_strip(const char* origin) {
  String coverted(origin);
  std::size_t front_pos;
  std::size_t back_pos;

  front_pos = coverted.find_first_not_of(" \n\t\v\f\r");
  back_pos = coverted.find_last_not_of(" \n\t\v\f\r");
  if (front_pos == String::npos || back_pos == String::npos) {
    return "";
  }
  return coverted.substr(front_pos, back_pos - front_pos + 1);
}

String make_random_string(std::size_t len, const String comp) {
  std::size_t comp_len = comp.length();
  String result;
  std::size_t idx = 0;
  timeval t;

  gettimeofday(&t, NULL);
  std::srand(static_cast<unsigned int>(t.tv_usec));
  result.reserve(len + 1);
  while (idx < len) {
    result.push_back(comp[std::rand() % comp_len]);
    idx++;
  }
  return result;
}

String& ft_upper(String& origin) {
  std::size_t len = origin.length();
  std::size_t idx = 0;

  while (idx < len) {
    if ('a' <= origin[idx] && origin[idx] <= 'z') {
      origin[idx] = origin[idx] - 32;
    }
    idx++;
  }
  return origin;
}

String ft_upper(const String& origin) {
  std::size_t len = origin.length();
  std::size_t idx = 0;
  String ret = "";

  while (idx < len) {
    if ('a' <= origin[idx] && origin[idx] <= 'z') {
      ret += (origin[idx] - 32);
    } else {
      ret += origin[idx];
    }
    idx++;
  }
  return ret;
}