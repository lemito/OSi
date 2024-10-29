#include <atomic>
#include <iostream>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <vector>

#define THREADS_NUM 16
#define DECK_NUM 52

enum CARDS {
  PICKS = '1',  // пики
  BUBIS = '2',  // буби
  CHERVI = '3', // черви
  CRESTI = '4'  // крести
};
typedef enum CARDS CARDS;

std::atomic_int ac = 0;

void *just_do(void *args) {
  char DECK[DECK_NUM];
  for (size_t i = 0; i < DECK_NUM; i++) {
    DECK[i] = PICKS;
  }
  for (size_t i = 0; i < DECK_NUM; i++) {
    std::cout << DECK[i] << " ";
  }
  char *input = (char *)args;
  ac++;
  std::cout << input << std::endl;
  return NULL;
}

int main() {
  std::vector<pthread_t> threads(THREADS_NUM);
  char *mewo = strdup("meow");
  for (size_t i = 0; i < THREADS_NUM; i++) {
    pthread_create(&(threads[i]), NULL, just_do, mewo);
  }

  std::cout << "== " << ac << " ==" << std::endl;

  return 0;
}