#include <atomic>
#include <iostream>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <vector>

#define THREADS_NUM 20
#define DECK_NUM 52

enum CARDS {
  PICKS,  // пики
  BUBIS,  // буби
  CHERVI, // черви
  CRESTI  // крести
};
typedef enum CARDS CARDS;

void *just_do(void *args) {
  char DECK[DECK_NUM];
  char *input = (char *)args;
  std::cout << input << std::endl;
  return NULL;
}

int main() {
  //   pthread_t threads[THREADS_NUM];
  std::vector<pthread_t> threads(THREADS_NUM);
  char *mewo = strdup("meow");
  for (size_t i = 0; i < THREADS_NUM; i++) {
    pthread_create(&(threads[i]), NULL, just_do, mewo);
  }

  std::atomic<int> ac = 0;
  ac++;
  std::cout << ac << std::endl;

  return 0;
}