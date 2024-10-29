#include <atomic>
#include <iostream>
#include <pthread.h>
#include <stdatomic.h>
#include <vector>

#define THREADS_NUM 20
#define DECK_NUM 52

void *just_do(void *args) {
  std::cout << "Meow" << std::endl;
  return NULL;
}

int main(int argc, const char **argv) {
  //   pthread_t threads[THREADS_NUM];
  std::vector<pthread_t> threads(THREADS_NUM);
  for (size_t i = 0; i < THREADS_NUM; i++) {
    pthread_create(&(threads[i]), NULL, just_do, NULL);
  }

  std::atomic<int> ac = 0;
  ac++;
  std::cout << ac << std::endl;

  return 0;
}