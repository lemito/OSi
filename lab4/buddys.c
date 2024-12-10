#include "main.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

typedef struct BuddyAllocator {
  size_t total_size;  // общий размер
  void *memory;       // начало памяти
  size_t min_block_size;  // размер наименьшего блока памяти
  unsigned int levels;    // уровни (log2N)
  unsigned char *bitmap;  // битмап свободных
} BuddyAllocator;

EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
  if (memory == NULL) return NULL;
  if (size < 16) return NULL;
  BuddyAllocator *allocator = (BuddyAllocator *)memory;
  allocator->total_size = size;
  allocator->memory = (void *)((char *)memory + sizeof(BuddyAllocator));
  allocator->min_block_size = 16;
  allocator->levels = (unsigned int)log2(size / allocator->min_block_size);
  allocator->bitmap = (unsigned char *)((char *)allocator->memory + size);
  memset(allocator->bitmap, 0, (1 << allocator->levels) - 1);
  return (Allocator *)allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) return;
  allocator->size = 0;
  allocator->data = NULL;
  return;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (!allocator || size == 0) return NULL;

  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;
  if (size > b_allocator->total_size) return NULL;

  void *alloc_ptr = b_allocator->memory;
  b_allocator->memory = (void *)((char *)b_allocator->memory + size);
  b_allocator->total_size -= size;
  return alloc_ptr;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (memory == NULL || allocator == NULL) return;
  (void)allocator;
  (void)memory;
  return;
}