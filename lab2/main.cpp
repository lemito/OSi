#include <atomic>
#include <iostream>
#include <pthread.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#define print(text, ...)                                                       \
  do {                                                                         \
    char BUF[BUFSIZ];                                                          \
    vsprintf(BUF, text, __VA_ARGS__);                                          \
    myCout(BUF);                                                               \
  } while (0)

#define THREADS_NUM 16
#define BARRIERS_NUM 16
#define DECK_NUM 52

pthread_barrier_t barrier;

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
  // for (size_t i = 0; i < DECK_NUM; i++) {
  //   std::cout << DECK[i] << " ";
  // }
  char *input = (char *)args;
  ac++;
  // std::cout << input << std::endl;
  return NULL;
}

void myCout(char *text) {
  if (text == NULL) {
    return;
  }
  if (-1 == write(STDOUT_FILENO, text, strlen(text))) {
    exit(EXIT_FAILURE);
  }
  return;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    char BUF[BUFSIZ];
    sprintf(BUF, "Input error. Use: %s %s %s\n", argv[0],
            "<threads>"
            "<rounds>");
    myCout(BUF);
    exit(EXIT_FAILURE);
  }
  // ограничение потоков и количество раундов
  char *endptr;
  size_t threads_num = atol(argv[1]);
  char *endptr;
  size_t rounds = atol(argv[2]);

  std::vector<pthread_t> threads(THREADS_NUM);
  char *mewo = strdup("meow");
  pthread_barrier_init(&barrier, NULL, BARRIERS_NUM);
  for (size_t i = 0; i < THREADS_NUM; i++) {
    pthread_create(&(threads[i]), NULL, just_do, mewo);
  }

  {
    char BUF[BUFSIZ];
    sprintf(BUF, "== %d ==\n", ac);
    myCout(BUF);
  }

  pthread_barrier_destroy(&barrier);
  return EXIT_SUCCESS;
}