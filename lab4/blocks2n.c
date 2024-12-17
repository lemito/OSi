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

// выравнивалка к степеням
#define ALIGN_UP(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

typedef struct BlockHeader {
  struct BlockHeader *next;
} BlockHeader;

typedef struct Allocator {
  void *memory;
  size_t total_size;
  BlockHeader *free_lists[32];  // массив списков свободных блоков
} Allocator;

// узнаем размер блока
size_t get_block_size(size_t size) {
  size_t block_size = 1;
  while (block_size < size) {
    block_size <<= 1;
  }
  return block_size;
}

// получаем индекс списка
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

  Allocator *allocator = mmap(NULL, sizeof(Allocator), PROT_READ | PROT_WRITE,
                              MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (allocator == MAP_FAILED) {
    return NULL;
  }

  allocator->memory = memory;
  allocator->total_size = size;
  memset(allocator->free_lists, 0, sizeof(allocator->free_lists));

  return allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) {
    // чилиииииииииим
    return;
  }
  munmap(allocator, sizeof(Allocator));
  return;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (NULL == allocator || size == 0) {
    return NULL;
  }

  size_t block_size = get_block_size(size + sizeof(BlockHeader));
  int index = get_list_index(block_size);
  // индекс получился слишком большим
  if (index >= 32) {
    return NULL;
  }

  if (NULL == allocator->free_lists[index]) {
    size_t total_size = ALIGN_UP(allocator->total_size, block_size);
    if (total_size < block_size) {
      return NULL;
    }

    BlockHeader *new_block = (BlockHeader *)allocator->memory;
    new_block->next = NULL;
    allocator->memory = (char *)allocator->memory + block_size;
    allocator->total_size -= block_size;
    allocator->free_lists[index] = new_block;
  }

  BlockHeader *block = allocator->free_lists[index];
  allocator->free_lists[index] = block->next;
  return (void *)(block + 1);
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (NULL == allocator || NULL == memory) {
    return;
  }

  BlockHeader *block = (BlockHeader *)memory - 1;
  size_t block_size = ALIGN_UP(sizeof(BlockHeader), sizeof(void *));
  int index = get_list_index(block_size);

  block->next = allocator->free_lists[index];
  allocator->free_lists[index] = block;

  return;
}