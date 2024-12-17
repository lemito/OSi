#include "main.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define PAGE_SIZE 4096

typedef struct BuddyAllocator {
  size_t total_size;  // общий размер
  void *memory;       // начало памяти
  size_t block_size;  // размер блока памяти
  size_t num_blocks;  // Общее количество блоков
  uint8_t *bitmap;  // Битовая карта для отслеживания свободных/занятых блоков
} BuddyAllocator;

EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
  if (memory == NULL) return NULL;
  if (size < 16) return NULL;
  // BuddyAllocator *allocator = (BuddyAllocator *)memory;
  BuddyAllocator *allocator =
      mmap(NULL, sizeof(BuddyAllocator), PROT_READ | PROT_WRITE,
           MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator == MAP_FAILED) {
    return NULL;
  }
  // Устанавливаем параметры
  allocator->total_size = size;
  allocator->block_size = PAGE_SIZE;
  // allocator->memory = (void *)((uintptr_t)memory + sizeof(BuddyAllocator));
  allocator->memory = memory;
  // Определяем количество блоков
  allocator->num_blocks = size / PAGE_SIZE;

  // bitmap = 1bit/block
  size_t bitmap_size = (allocator->num_blocks + 7) / 8;
  allocator->bitmap = (uint8_t *)((uintptr_t)allocator->memory + size);

  memset(allocator->bitmap, 0, bitmap_size);

  return (Allocator *)allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) return;
  allocator->total_size = 0;
  munmap(allocator, allocator->total_size);
  allocator->data = NULL;
  return;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (!allocator || size == 0) return NULL;
  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

  size_t block_size = b_allocator->block_size;
  while (block_size < size) {
    block_size *= 2;  // делаем кратным 2
  }

  if (size > b_allocator->total_size) return NULL;

  // блоки подходящего размера (их поиск с помощью битовой карты)
  for (size_t i = 0; i < b_allocator->num_blocks; i++) {
    size_t byte_index = i / 8;
    size_t bit_index = i % 8;

    if (!(b_allocator->bitmap[byte_index] & (1 << bit_index))) {
      // Если блок свободен, метим его как занятый
      b_allocator->bitmap[byte_index] |= (1 << bit_index);
      return (void *)((uintptr_t)b_allocator->memory + i * block_size);
    }
  }

  return NULL;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (memory == NULL || allocator == NULL) return;
  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

  uintptr_t offset = (uintptr_t)memory - (uintptr_t)b_allocator->memory;
  size_t block_index = offset / b_allocator->block_size;

  // ищем занятого чела
  size_t byte_index = block_index / 8;
  size_t bit_index = block_index % 8;

  b_allocator->bitmap[byte_index] &= ~(1 << bit_index);  // освобождаем

  // ищем поизиции друга
  size_t buddy_index = block_index ^ 1;

  // Если соседний блок тоже свободен, сливаем два блока
  if (!(b_allocator->bitmap[buddy_index / 8] & (1 << (buddy_index % 8)))) {
    b_allocator->bitmap[block_index / 8] &= ~(1 << (block_index % 8));
    b_allocator->bitmap[buddy_index / 8] &= ~(1 << (buddy_index % 8));

    // Сливаем два блока в один
    block_index = block_index / 2;
  }

  return;
}