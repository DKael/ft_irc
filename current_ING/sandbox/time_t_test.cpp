#include <ctime>
#include <iostream>

int main() {
  time_t seconds;

  time(&seconds);

  printf("passed second during 1970 : %ld sec\n", seconds);

  return 0;
}