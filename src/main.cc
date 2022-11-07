#include <unistd.h>

#include <thread>

#include "thrdpool.hpp"

void print(int num) {
  sleep(2);
  std::cout << "[thrd] task print:" << num << std::endl;
}

int printAndRet(int num) {
  print(num);
  sleep(rand() % 3);
  return num;
}

int main() {
  thrdpool::Thrdpool pool(4);
  for (int i = 0; i < 2; i++) {
    pool.PushTask(print, i);
  }
  pool.Wait();
  for (int i = 0; i < 2; i++) {
    auto pro = pool.Submit(printAndRet, i);
    std::cout << "[printAndRet] " << pro.get() << std::endl;
  }
  pool.Stop();
}
