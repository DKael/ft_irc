#include "string_func.hpp"

std::string ft_itos(const int input) {
  std::stringstream ss_tmp;
  std::string ret;

  ss_tmp << input;
  ss_tmp >> ret;

  return ret;
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
