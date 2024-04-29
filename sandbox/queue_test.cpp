#include <iostream>
#include <queue>
#include <string>
#include <vector>

void vec_test(std::vector<std::string> params) {
  for (std::size_t i = 0; i < params.size(); i++) {
    std::cout << params[i] << '\n';
  }
}

int main() {
  std::queue<std::string> qq;

  qq.push("34");
  qq.push("633");

  while (qq.size() != 0) {
    std::cout << qq.front() << '\n';
    qq.pop();
  }
  std::cout << '_' << qq.front() << '_' << '\n';
  std::cout << '_' << qq.front() << '_' << '\n';

  std::queue<std::string> qqq;

  std::cout << '_' << qq.front() << '_' << '\n';

  // vec_test(std::vector<std::string>(std::string("1"), "2", "3", "4"));
}
