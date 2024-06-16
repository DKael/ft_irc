#include "util.hpp"

bool port_chk(const char* input_port) {
  std::stringstream port_chk;
  int port;

  port_chk << String(input_port);
  port_chk >> port;
  if (port_chk.fail()) {
    return false;
  } else if (port < 0 || 65335 < port) {
    return false;
  }
  return true;
}

bool ipv4_chk(const char* input_ipv4) {
  String ipv4_tmp = input_ipv4;
  String tmp_str;
  int tmp_int;
  std::stringstream to_num;
  std::size_t idx1 = 0;
  std::size_t idx2 = 0;

  for (int i = 0; i < 3; i++) {
    idx1 = ipv4_tmp.find('.', idx1);
    if (idx1 != String::npos) {
      tmp_str = ipv4_tmp.substr(idx2, idx1 - idx2);
      if (tmp_str.find_first_not_of("0123456789") != String::npos) {
        return false;
      }
      to_num.clear();
      to_num << tmp_str;
      to_num >> tmp_int;
      if (to_num.fail() || !(0 <= tmp_int && tmp_int <= 255)) {
        return false;
      }
      idx1++;
      idx2 = idx1;
    } else {
      return false;
    }
  }
  tmp_str = ipv4_tmp.substr(idx2);
  if (tmp_str.find_first_not_of("0123456789") != String::npos) {
    return false;
  }
  to_num.clear();
  to_num << tmp_str;
  to_num >> tmp_int;
  if (to_num.fail() || !(0 <= tmp_int && tmp_int <= 255)) {
    return false;
  }
  return true;
}