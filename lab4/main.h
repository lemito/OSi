#ifndef __BUDDYS_H
#define __BUDDYS_H

#include <assert.h>
#include <dlfcn.h>  // dlopen, dlsym, dlclose, RTLD_*
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <unistd.h>  // write

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define BUFSIZ 8192
typedef enum STATES { LOG_s, ERROR_s } STATES;

int _print(char mode, char *fmt, ...) {
  if (fmt == NULL) {
    write(STDERR_FILENO, "ERROR", 6);
    return 1;
  }
  va_list vargs;
  va_start(vargs, fmt);
  char msg[BUFSIZ];
  vsprintf(msg, fmt, vargs);
  write(mode == ERROR_s ? STDERR_FILENO : STDOUT_FILENO, msg, strlen(msg));
  va_end(vargs);
  return 0;
}

#define LOG(fmt, ...) _print(LOG_s, fmt, ##__VA_ARGS__)
#define ERROR(fmt, ...) _print(ERROR_s, fmt, ##__VA_ARGS__)

#define TIMER_INIT()            \
  clock_t start_time, end_time; \
  double timer_res;
#define TIMER_START() start_time = clock();
#define TIMER_END(text)                                         \
  end_time = clock();                                           \
  timer_res = (double)(end_time - start_time) / CLOCKS_PER_SEC; \
  LOG("%s %.6lf\n", text, timer_res);

struct Allocator {
  size_t total_size;     // общий размер
  void *memory;          // указатель на память
  long long in_use_mem;  // занятая память (с учётом накладных расходов)
  long long requested_mem;  // запрашиваемая память (без накладных расходов)
};
typedef struct Allocator Allocator;

typedef struct Allocator_extra {
  size_t total_size;     // общий размер
  void *memory;          // указатель на память
  long long in_use_mem;  // занятая память (с учётом накладных расходов)
  long long requested_mem;  // запрашиваемая память (без накладных расходов)
  size_t offset;
} Allocator_extra;

// инициализация аллокатора на памяти memory размера size
typedef Allocator *allocator_create_f(void *const memory, const size_t size);
// деинициализация структуры аллокатора
typedef void allocator_destroy_f(Allocator *const allocator);
// выделение памяти аллокатором памяти размера size
typedef void *allocator_alloc_f(Allocator *const allocator, const size_t size);
// возвращает выделенную память аллокатору
typedef void allocator_free_f(Allocator *const allocator, void *const memory);

typedef double allocator_usage_factor_f(Allocator *const allocator);

/* блок замененок */
static Allocator *allocator_create_extra(void *const memory,
                                         const size_t size) {
  if (memory == NULL || size == 0) {
    return NULL;
  }

  Allocator_extra *allocator = (Allocator_extra *)memory;
  allocator->memory = (void *)((uintptr_t)memory + sizeof(Allocator_extra));
  allocator->total_size = size - sizeof(Allocator_extra);
  allocator->offset = 0;
  return (Allocator *)allocator;
}

static void allocator_destroy_extra(Allocator *const allocator) {
  if (allocator == NULL) {
    return;
  }
  Allocator_extra *allo = (Allocator_extra *)allocator;

  allo->total_size = 0;
  allo->offset = 0;
}

static void *allocator_alloc_extra(Allocator *const allocator,
                                   const size_t size) {
  if (allocator == NULL || size == 0) {
    return NULL;
  }

  Allocator_extra *alloc = (Allocator_extra *)allocator;

  if (alloc->offset + size > alloc->total_size) {
    return NULL;
  }

  void *allocated_memory = (void *)((uintptr_t)alloc->memory + alloc->offset);
  alloc->offset += size;

  return allocated_memory;
}

static void allocator_free_extra(Allocator *const allocator,
                                 void *const memory) {
  (void)allocator;
  (void)memory;
}
/**/

#endif  // __BUDDYS_H