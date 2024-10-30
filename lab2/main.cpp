#include <algorithm>
#include <atomic>
#include <barrier>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <random>
// #include <stdarg.h>
#include <cstdarg>
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

// pthread_barrier_t barrier;
// int cnt = 0;

std::atomic_int ac = 0;
// std::mutex m;

bool monteCarlo() {
  std::vector<int> deck(DECK_NUM);
  for (size_t i = 0; i < DECK_NUM; i++) {
    deck[i] = i;
  }
  std::random_device rd;
  std::mt19937 random_generator(rd());
  std::shuffle(all(deck), random_generator);
  return (deck[0] % 13) == (deck[1] % 13); // одинаковые МАСТИ
}

void *just_do(void *args) {
  size_t round = *(size_t *)args;
  for (size_t i = 0; i < round; i++) {
    if (monteCarlo()) {
      ++ac;
      // m.lock();
      // cnt++;
      // m.unlock();
    }
  }
  // std::cout << "I am done! ";
  return NULL;
}

void _print(char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  char BUF[BUFSIZ];
  vsprintf(BUF, fmt, args);
  if (-1 == write(STDOUT_FILENO, BUF, strlen(BUF))) {
    exit(EXIT_FAILURE);
  }
  va_end(args);
}

int main(int argc, char **argv) {
  if (argc != 3) {
    _print("Input error. Use: %s <threads> <rounds>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // ограничение потоков и количество раундов
  size_t threads_num = atol(argv[1]);
  size_t rounds = atol(argv[2]);
  // std::barrier barrier(threads_num);
  std::vector<pthread_t> threads(threads_num);
  // pthread_barrier_init(&barrier, NULL, BARRIERS_NUM);
  for (size_t i = 0; i < threads_num; i++) {
    if (-1 == pthread_create(&(threads[i]), NULL, just_do, (void *)&rounds)) {
      _print("Error. Thread %d nor created\n", i);
      exit(EXIT_FAILURE);
    };
  }

  for (auto &thread : threads) {
    if (-1 == pthread_join(thread, NULL)) {
      _print("Error. Thread not joined\n");
      exit(EXIT_FAILURE);
    };
  }

  {
    int b = ac.load();
    _print("== %d ==\n", b);
  }

  {
    int total_rounds = threads_num * rounds;
    int f = ac.load();
    double probability = (double)f / (double)total_rounds;
    char BUF[BUFSIZ];
    sprintf(BUF, "== %f ==\n", probability);
    _print("%s\n", BUF);
  }

  // pthread_barrier_destroy(&barrier);
  return EXIT_SUCCESS;
}