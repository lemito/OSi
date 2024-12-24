#include "main.h"

// размер страницы
#define MIN_BLOCK_SIZE 1
// самый большой блок (не факт даже что будет и нужен)
#define MAX_BLOCK_SIZE (INT32_MAX)
// кол-во списков пусть всего 2^31
#define MAX_BLOCK_CNT 31

typedef struct block_t {
  struct block_t *next;  // следующий свободный
  size_t size;           // размер блока
} block_t;

typedef struct p2Alloc {
  size_t total_size;        // общий размер
  void *memory;             // указатель на память
  long long in_use_mem;     // занятая память
  long long requested_mem;  // запрашиваемая память
  block_t *free_lists[MAX_BLOCK_CNT];  // массив списков свободных блоков
} p2Alloc;

// узнаем размер блока, который можно забить под нужный размер
size_t get_block_size(size_t size) {
  if (size == 1) {
    return 1;
  }
  size_t block_size = 2;
  while (block_size < size) {  // + sizeof(block_t)
    block_size <<= 1;
  }
  return block_size;
}

// получаем индекс списка блоков свободных необходимого размера
int get_list_index(size_t block_size) {
  int index = 0;
  while (block_size > MIN_BLOCK_SIZE) {
    block_size >>= 1;
    index++;
  }
  return index - 1;
}

EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
  if (size < sizeof(p2Alloc) || memory == NULL) {
    return NULL;
  }

  // Выделение памяти для структуры аллокатора
  p2Alloc *allocator = (p2Alloc *)memory;
  allocator->memory = (void *)((uintptr_t)memory + sizeof(p2Alloc));
  allocator->total_size = size - sizeof(p2Alloc);
  allocator->in_use_mem = 0;
  allocator->requested_mem = 0;

  // Инициализация списков свободных блоков
  memset(allocator->free_lists, 0, sizeof(allocator->free_lists));

  if (allocator->total_size < MIN_BLOCK_SIZE) {
    return (Allocator *)allocator;
  }

  // Начинаем разбиение памяти на блоки
  size_t min_block_size = get_block_size(sizeof(block_t));
  size_t cnt = 0;
  while (min_block_size <= size) {
    cnt++;
    min_block_size <<= 1;
  }

  // LOG("cnt = %zu\n", cnt);
  size_t remaining_size = allocator->total_size;
  size_t cur = sizeof(p2Alloc);

  for (long long i = cnt - 1; i >= 0; i--) {
    if (remaining_size < MIN_BLOCK_SIZE) break;

    size_t block_size = MIN_BLOCK_SIZE << i;
    if (cur < size && block_size <= remaining_size) {
      block_t *block = (block_t *)((uint8_t *)memory + cur);
      block->size = block_size;
      block->next = NULL;

      allocator->free_lists[i] = block;
      cur += block_size;
      remaining_size -= block_size;
    }
  }

  return (Allocator *)allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) {
    return;
  }
  p2Alloc *alloc = (p2Alloc *)allocator;
  memset(alloc->free_lists, 0, sizeof(alloc->free_lists));
  alloc->total_size = 0;
  alloc->in_use_mem = 0;
  alloc->requested_mem = 0;
  alloc->memory = NULL;
  return;
}

EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (NULL == allocator || size == 0) {
    return NULL;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  if (alloc->total_size + sizeof(Allocator) <= size) {
    return NULL;
  }

  size_t block_size = get_block_size(size);
  int index = get_list_index(block_size);

  if (index < 0 || index >= MAX_BLOCK_CNT) {
    return NULL;
  }

  // Ищем свободный блок подходящего размера или больше
  for (int i = index; i < MAX_BLOCK_CNT; ++i) {
    if (alloc->free_lists[i] != NULL) {
      block_t *block = alloc->free_lists[i];
      alloc->free_lists[i] = block->next;  // Удаляем его из списка

      // Разбиваем блок на меньшие, если он больше нужного размера
      while (i > index) {
        i--;
        size_t smaller_block_size = 1 << i;  // Размер меньшего блока
        uintptr_t split_addr = (uintptr_t)block + smaller_block_size;

        // Создаём новый меньший блок
        block_t *split_block = (block_t *)split_addr;
        split_block->size = smaller_block_size;
        split_block->next = alloc->free_lists[i];
        alloc->free_lists[i] = split_block;
      }

      block->size = block_size;
      alloc->in_use_mem += block_size;
      alloc->requested_mem += size;

      return (void *)((uintptr_t)block + sizeof(block_t));
    }
  }

  // Попытка выделить память за пределами списка свободных блоков
  if (alloc->total_size - alloc->in_use_mem >= block_size) {
    uintptr_t overflow_addr = (uintptr_t)alloc->memory + alloc->in_use_mem;
    block_t *block = (block_t *)overflow_addr;
    block->size = block_size;

    alloc->in_use_mem += block_size + sizeof(block_t);
    alloc->requested_mem += size;

    return (void *)((uintptr_t)block + sizeof(block_t));
  }

  // Если подходящего блока нет и переполнение невозможно
  return NULL;
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (NULL == allocator || NULL == memory) {
    return;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  if (alloc->total_size == 0) {
    return;
  }

  uintptr_t memory_addr = (uintptr_t)memory - sizeof(block_t);
  uintptr_t alloc_start = (uintptr_t)alloc->memory;
  uintptr_t alloc_end = alloc_start + alloc->total_size;

  if (memory_addr < alloc_start || memory_addr >= alloc_end) {
    return;
  }

  block_t *block = (block_t *)((uintptr_t)memory - sizeof(block_t));

  size_t block_size = block->size;

  if (block_size > MAX_BLOCK_SIZE || block_size < MIN_BLOCK_SIZE) {
    return;
  }

  int index = get_list_index(block_size);

  if (index < 0 || index >= MAX_BLOCK_CNT) {
    return;
  }

  memset(block, 0, block_size);
  block->next = alloc->free_lists[index];
  alloc->free_lists[index] = block;
  alloc->in_use_mem -= (block_size + sizeof(block_t));
  alloc->requested_mem -= block_size - sizeof(block_t);

  // LOG("meow meow %lld __ %lld\n", alloc->requested_mem / alloc->in_use_mem);
}