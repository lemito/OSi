#include "main.h"

// размер страницы
#define MIN_BLOCK_SIZE 2
// самый большой блок (не факт даже что будет и нужен)
#define MAX_BLOCK_SIZE (4096)
// кол-во списков пусть всего 2^31
#define MAX_BLOCK_CNT 12

typedef struct block_t {
  struct block_t *next;  // следующий свободный
} block_t;

typedef struct p2Alloc {
  void *memory;       // сама память
  size_t total_size;  // размер общий
  block_t *free_lists[MAX_BLOCK_CNT];  // массив списков свободных блоков
} p2Alloc;

// узнаем размер блока, который можно забить под нужный размер
size_t get_block_size(size_t size) {
  size_t block_size = 1;
  while (block_size < size) {
    block_size <<= 1;
  }
  return block_size;
}

// получаем индекс списка блоков свободных необходимого размера
int get_list_index(size_t block_size) {
  int index = 0;
  while (block_size > 1) {
    block_size >>= 1;
    index++;
  }
  return index - 1;
}

EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
  if (size == 0 || memory == NULL) {
    return NULL;
  }

  p2Alloc *allocator = (p2Alloc *)memory;

  if (size < sizeof(p2Alloc)) {
    return NULL;
  }

  allocator->memory = (void *)((uintptr_t)memory + sizeof(p2Alloc));
  allocator->total_size = size - sizeof(p2Alloc);
  memset(allocator->free_lists, 0, sizeof(allocator->free_lists));

  /* Инициализация всех списков свободных блоков */
  size_t offset = 0;
  size_t remaining_size = allocator->total_size;

  while (remaining_size >= MIN_BLOCK_SIZE) {
    size_t block_size = MIN_BLOCK_SIZE;

    while (block_size <= remaining_size && block_size <= MAX_BLOCK_SIZE) {
      int index = get_list_index(block_size);

      if (index >= MAX_BLOCK_CNT) {
        break;
      }

      block_t *block = (block_t *)((uintptr_t)allocator->memory + offset);
      block->next = allocator->free_lists[index];
      allocator->free_lists[index] = block;

      offset += block_size; 
      remaining_size -= block_size;

      block_size <<= 1;
    }

    if (block_size > remaining_size || block_size > MAX_BLOCK_SIZE) {
      break;
    }
  }

  return (Allocator *)allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) {
    // чилиииииииииим
    return;
  }
  p2Alloc *alloc = (p2Alloc *)allocator;
  memset(alloc->free_lists, 0, sizeof(alloc->free_lists));
  alloc->total_size = 0;
  alloc->memory = NULL;
  return;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (NULL == allocator || size == 0) {
    return NULL;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  size_t block_size = get_block_size(size);
  int index = get_list_index(block_size);

  if (index >= MAX_BLOCK_CNT) {
    return NULL;
  }

  // Ищем блоки подходящего размера или больше
  for (int i = index; i < MAX_BLOCK_CNT; ++i) {
    if (alloc->free_lists[i]) {
      block_t *block = alloc->free_lists[i];
      alloc->free_lists[i] = block->next;

      // Разделяем большие блоки на меньшие
      while (i > index) {
        i--;
        block_t *split_block = (block_t *)((uintptr_t)block + (1 << i));
        split_block->next = alloc->free_lists[i];
        alloc->free_lists[i] = split_block;
      }

      return (void *)block;
    }
  }

  return NULL;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (NULL == allocator || NULL == memory) {
    return;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  LOG("free\n");

  uintptr_t memory_addr = (uintptr_t)memory;
  uintptr_t alloc_start = (uintptr_t)alloc->memory;
  uintptr_t alloc_end = alloc_start + alloc->total_size;

  if (memory_addr < alloc_start || memory_addr >= alloc_end) {
    return;
  }

  size_t block_size = MIN_BLOCK_SIZE;
  uintptr_t offset = memory_addr - alloc_start;

  // пытаемся определить размер блока
  while (block_size <= MAX_BLOCK_SIZE && offset % block_size != 0) {
    block_size <<= 1;
  }

  if (block_size > MAX_BLOCK_SIZE) {
    return;
  }

  int index = get_list_index(block_size);

  block_t *block = (block_t *)memory;
  block->next = alloc->free_lists[index];
  alloc->free_lists[index] = block;
}