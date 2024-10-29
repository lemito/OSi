#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <random>
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
#define all(x) x.begin(), x.end()
#define THREADS_NUM 16
#define BARRIERS_NUM 16
#define DECK_NUM 52

pthread_barrier_t barrier;

std::atomic_int ac = 0;
std::mutex m;

bool monteCarlo() {
  std::vector<int> deck(DECK_NUM);
  for (size_t i = 0; i < DECK_NUM; i++) {
    deck[i] = i;
  }
  std::random_device rd;
  std::mt19937 random_generator(rd());
  std::shuffle(all(deck), random_generator);
  return (deck[0] % 13) == (deck[1] % 13);
}

void *just_do(void *args) {
  int cnt = 0;
  size_t round = *(size_t *)args;
  for (size_t i = 0; i < round; i++) {
    if (monteCarlo()) {
      ac++;
    }
  }
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
    sprintf(BUF, "Input error. Use: %s <threads> <rounds>\n", argv[0]);
    myCout(BUF);
    exit(EXIT_FAILURE);
  }
  // ограничение потоков и количество раундов
  size_t threads_num = atol(argv[1]);
  size_t rounds = atol(argv[2]);

  std::vector<pthread_t> threads(THREADS_NUM);
  pthread_barrier_init(&barrier, NULL, BARRIERS_NUM);
  for (size_t i = 0; i < THREADS_NUM; i++) {
    pthread_create(&(threads[i]), NULL, just_do, (void *)&rounds);
  }

  for (auto &thread : threads) {
    pthread_join(thread, NULL);
  }

  {
    char BUF[BUFSIZ];
    int b = ac.load();
    sprintf(BUF, "== %d ==\n", b);
    myCout(BUF);
  }

  {
    int total_rounds = threads_num * rounds;
    int f = ac.load();
    double probability = (double)f / (double)total_rounds;
    char BUF[BUFSIZ];
    sprintf(BUF, "== %f ==\n", probability);
    myCout(BUF);
  }

  pthread_barrier_destroy(&barrier);
  return EXIT_SUCCESS;
}