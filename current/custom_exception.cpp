#include "custom_exception.hpp"

const char* socket_create_error::what() const throw() {
  return "Socket create fail!";
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