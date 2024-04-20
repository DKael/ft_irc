#include "Exception.hpp"
#include <iostream>

const char* Error::what() const throw() {
  return err_msg;
};

Error::Error(std::string errMsg) {
  err_msg = errMsg.c_str();
}

Error::~Error() throw() {
  
}