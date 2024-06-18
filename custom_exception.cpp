#include "custom_exception.hpp"

const char* socket_create_error::what() const throw() {
  return "Socket create fail!";
};

const char* socket_setting_error::what() const throw() {
  return "Socket setting fail!";
};

const char* port_range_error::what() const throw() {
  return "Port range invalid!";
};

const char* socket_bind_error::what() const throw() {
  return "Socket bind error!";
};

const char* socket_listening_error::what() const throw() {
  return "Socket listening error!";
};

const char* server_channel_list_capacity_error::what() const throw() {
  return "Channel capacity error!";
};

const char* channel_user_capacity_error::what() const throw() {
  return "Channel capacity error!";
};

const char* channel_prefix_error::what() const throw() {
  return "Channel prefix error!";
};