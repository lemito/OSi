#ifndef __BUDDYS_H
#define __BUDDYS_H

#include <unistd.h>

typedef struct Allocator {
  size_t size;
  void *data;
} Allocator;

// инициализация аллокатора на памяти memory размера size
typedef Allocator *allocator_create(void *const memory, const size_t size);
// деинициализация структуры аллокатора
typedef void allocator_destroy(Allocator *const allocator);
// выделение памяти аллокатором памяти размера size
typedef void *allocator_alloc(Allocator *const allocator, const size_t size);
// возвращает выделенную память аллокатору
typedef void allocator_free(Allocator *const allocator, void *const memory);

#endif // __BUDDYS_H