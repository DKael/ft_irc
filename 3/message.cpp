#include "message.hpp"

Message::Message(const std::string& _raw_msg) : raw_msg(ft_strip(_raw_msg)) {
  if (raw_msg.length() == 0) {
    cmd_type = NONE;
    numeric = "421";
    ret_msg = "Unknown command";
    return;
  }
  std::size_t idx1 = 0;
  std::size_t idx2 = raw_msg.length() - 1;
  std::size_t pos = 0;
  std::string tmp_trailing;

  if (raw_msg[0] == ':') {
    // source exist
    pos = raw_msg.find_first_of(" ");
    if (pos == std::string::npos) {
      cmd_type = NONE;
      numeric = "ERROR";
      ret_msg = "Prefix without command";
      return;
    }
    source = raw_msg.substr(1, pos - 1);
  }

  // get command
  pos = raw_msg.find_first_not_of(" ", pos);
  if (pos == std::string::npos) {
    cmd_type = NONE;
    numeric = "ERROR";
    ret_msg = "Prefix without command";
    return;
  }
  idx1 = pos;
  pos = raw_msg.find_first_of(" ", pos);

  pos = raw_msg.rfind(" :");
  if (pos != std::string::npos) {
    // trailing exist
    tmp_trailing = raw_msg.substr(pos + 2);
    idx2 = pos;
  }
}

// 01234567
// :abcde