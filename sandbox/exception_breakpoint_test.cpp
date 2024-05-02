#include <iostream>
#include <map>

int main() {
  int aaa[10] = {
      0,
  };
  std::map<int, int> test;

  try {
    // aaa[11] = 10;
    test.at(10);
  } catch (std::range_error& e) {
    std::cerr << e.what() << '\n';
  } catch (std::exception& e) {
    std::cerr << e.what() << '\n';
  }
}