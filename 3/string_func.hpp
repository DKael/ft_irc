#ifndef STRING_FUNC_HPP
#define STRING_FUNC_HPP

#include <sstream>
#include <string>

std::string ft_itos(const int input);
std::vector<std::string>& ft_split(const std::string& str,
                                   const std::string& del,
                                   std::vector<std::string>& box);
std::string ft_strip(const std::string& origin);

#endif