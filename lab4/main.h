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

typedef struct Allocator {
  size_t total_size;
  void *data;
} Allocator;

// инициализация аллокатора на памяти memory размера size
typedef Allocator *allocator_create_f(void *const memory, const size_t size);
// деинициализация структуры аллокатора
typedef void allocator_destroy_f(Allocator *const allocator);
// выделение памяти аллокатором памяти размера size
typedef void *allocator_alloc_f(Allocator *const allocator, const size_t size);
// возвращает выделенную память аллокатору
typedef void allocator_free_f(Allocator *const allocator, void *const memory);

/* блок замененок */
static Allocator *allocator_create_extra(void *const memory,
                                         const size_t size) {
  if (memory == NULL || size == 0) {
    return NULL;
  }
  Allocator *allocator =
      (Allocator *)mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator == MAP_FAILED) {
    return NULL;
  }

  allocator->data = memory;
  allocator->total_size = size;

  return allocator;
}
static void allocator_destroy_extra(Allocator *const allocator) {
  if (allocator == NULL) {
    return;
  }
  allocator->total_size = 0;
  munmap(allocator, sizeof(Allocator));
  allocator->data = NULL;
}
static void *allocator_alloc_extra(Allocator *const allocator,
                                   const size_t size) {}
static void allocator_free_extra(Allocator *const allocator,
                                 void *const memory) {}
/**/

#endif  // __BUDDYS_H