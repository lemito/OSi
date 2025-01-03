#include <algorithm>
#include <array>
#include <iostream>
#include <mutex>
#include <pthread.h>
#include <random>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#define all(x) x.begin(), x.end()
#define DECK_NUM 52

void _print(char *fmt, ...);

int ac = 0;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *just_do(void *args) {
  size_t round = *(size_t *)args;
  std::random_device rd;
  std::mt19937 random_generator(rd());
  int local = 0;
  std::array<int, DECK_NUM> deck;
  for (size_t j = 0; j < DECK_NUM; j++) {
    deck[j] = j;
  }
  for (size_t i = 0; i < round; i++) {
    std::shuffle(all(deck), random_generator);
    if (deck[0] % 4 == deck[1] % 4) {
      ++local;
    }
  }
  if (0 != pthread_mutex_lock(&m)) {
    _print("Mutex lock error\n");
    exit(EXIT_FAILURE);
  }
  ac += local;
  if (0 != pthread_mutex_unlock(&m)) {
    _print("Mutex unlock error\n");
    exit(EXIT_FAILURE);
  }

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
  if (argv[1] == NULL || argv[2] == NULL) {
    _print("Input error. Use: %s <threads> <rounds>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // ограничение потоков и количество раундов
  size_t threads_num = atol(argv[1]);
  size_t rounds = atol(argv[2]);
  size_t rounds_for_thread = rounds / threads_num;
  if (rounds % threads_num != 0) {
    threads_num++;
  }
  rounds_for_thread = rounds / threads_num;
  std::vector<pthread_t> threads(threads_num);
  for (size_t i = 0; i < threads_num; i++) {
    if (-1 == pthread_create(&(threads[i]), NULL, just_do,
                             (void *)&rounds_for_thread)) {
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
    // расчеты
    int total_rounds = rounds; // threads_num * rounds;
    _print("== %d ==\n", ac);
    double probability = (double)ac / (double)total_rounds;
    char BUF[BUFSIZ];
    sprintf(BUF, "== %f ==\n", probability);
    _print("%s\n", BUF);
  }

  pthread_mutex_destroy(&m);
  return EXIT_SUCCESS;
}