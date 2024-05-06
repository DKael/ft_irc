#ifndef CUSTOMEXCEPTION_HPP
#define CUSTOMEXCEPTION_HPP

#include <exception>

class socket_create_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

class port_range_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

class socket_bind_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

class socket_listening_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

class server_channel_list_capacity_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

class channel_client_capacity_error : public std::exception {
 private:
 public:
  const char* what() const throw();
};

#endif