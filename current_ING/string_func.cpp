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

std::string make_random_string(std::size_t len, const std::string comp) {
  std::size_t comp_len = comp.length();
  std::string result;
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

std::string& ft_upper(std::string& origin) {
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