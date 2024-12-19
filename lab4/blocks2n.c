#include "main.h"

// NOTE: MSVC compiler does not export symbols unless annotated
#ifdef _MSC_VER
#define EXPORT __declspec(dllexport)
#else
#define EXPORT
#endif

#define PAGE_SIZE 4096
// размер блока максимум
#define MAX_BLOCK_SIZE (16 * 1024)
// кол-во списков
#define MAX_BLOCK_CNT 32

// выравнивалка к степеням
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

typedef struct BlockHeader {
  struct BlockHeader *next;
} BlockHeader;

typedef struct p2Alloc {
  void *memory;       // сама память
  size_t total_size;  // размер
  BlockHeader *free_lists[MAX_BLOCK_CNT];  // массив списков свободных блоков
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

  allocator->memory = (void *)((uintptr_t)memory + sizeof(p2Alloc));
  allocator->total_size = size;
  memset(allocator->free_lists, 0, sizeof(allocator->free_lists));

  size_t usable_size = size - sizeof(p2Alloc);
  size_t block_size = next_power_of_two(usable_size);

  int index = get_list_index(block_size);
  allocator->free_lists[index] = (BlockHeader *)allocator->memory;

  BlockHeader *current = allocator->free_lists[index];
  while ((uintptr_t)current + block_size <= (uintptr_t)memory + size) {
    BlockHeader *next = (BlockHeader *)((uintptr_t)current + block_size);
    current->next = ((uintptr_t)next + block_size <= (uintptr_t)memory + size)
                        ? next
                        : NULL;
    current = next;
  }

  LOG("Блоки 2n готовы\n");

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
  // индекс получился слишком большим
  if (index >= MAX_BLOCK_CNT || !alloc->free_lists[index]) {
    return NULL;
  }

  BlockHeader *block = alloc->free_lists[index];
  alloc->free_lists[index] = block->next;

  return (void *)block;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (NULL == allocator || NULL == memory) {
    return;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  uintptr_t memory_addr = (uintptr_t)memory;
  uintptr_t alloc_start = (uintptr_t)alloc->memory;
  uintptr_t alloc_end = alloc_start + alloc->total_size - sizeof(p2Alloc);

  if (memory_addr < alloc_start || memory_addr >= alloc_end) {
    return;
  }

  size_t block_size = next_power_of_two(memory_addr - alloc_start);
  int index = get_list_index(block_size);

  BlockHeader *block = (BlockHeader *)memory;
  block->next = alloc->free_lists[index];
  alloc->free_lists[index] = block;

  return;
}