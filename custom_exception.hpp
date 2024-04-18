#ifndef CUSTOMEXCEPTION_HPP
#define CUSTOMEXCEPTION_HPP

#include <exception>
#include <iostream>

class Error : public std::exception
{
  private:
    const char* err_msg;
  public:
    Error(std::string str);
    virtual ~Error() throw();
    const char* what() const throw();
};

#endif