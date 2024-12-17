#ifndef __BUDDYS_H
#define __BUDDYS_H

#include <math.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

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

#endif  // __BUDDYS_H