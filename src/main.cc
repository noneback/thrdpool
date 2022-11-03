

#include <unistd.h>

#include <thread>

#include "thrdpool.hpp"

void print(int num) { std::cout << "[thrd] task print:" << num << std::endl; }

int main() {
  thrdpool::Thrdpool pool(4);
  for (int i = 0; i < 2; i++) {
    pool.PushTask(print, i);
  }
  sleep(2);
  for (int i = 0; i < 2; i++) {
    pool.PushTask(print, i);
  }
  pool.Wait();
  pool.Stop();
}
