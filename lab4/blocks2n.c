#include "main.h"

// размер страницы
#define MIN_BLOCK_SIZE 1
// самый большой блок (не факт даже что будет и нужен)
#define MAX_BLOCK_SIZE (4096)
// кол-во списков пусть всего 2^31
#define MAX_BLOCK_CNT 12

typedef struct block_t {
  struct block_t *next;  // следующий свободный
  size_t size;           // размер блока
} block_t;

typedef struct p2Alloc {
  size_t total_size;  // общий размер
  void *memory;       // указатель на память
  block_t *free_lists[MAX_BLOCK_CNT];  // массив списков свободных блоков
  // size_t in_use_mem;  // занятая память
} p2Alloc;

// узнаем размер блока, который можно забить под нужный размер
size_t get_block_size(size_t size) {
  size_t block_size = sizeof(block_t);
  while (block_size < size + sizeof(block_t)) {
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
  if (size == 0 || memory == NULL) {
    return NULL;
  }

  p2Alloc *allocator = (p2Alloc *)memory;

  if (size < sizeof(p2Alloc)) {
    return NULL;
  }

  allocator->memory = (void *)((uintptr_t)memory + sizeof(p2Alloc));
  allocator->total_size = size - sizeof(p2Alloc);
  // allocator->in_use_mem = 0;
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
      block->size = block_size;
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
    return;
  }
  p2Alloc *alloc = (p2Alloc *)allocator;
  memset(alloc->free_lists, 0, sizeof(alloc->free_lists));
  alloc->total_size = 0;
  // alloc->in_use_mem = 0;
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

  // if (alloc->in_use_mem + sizeof(Allocator) + size >= alloc->total_size) {
  //   return NULL;
  // }

  size_t block_size = get_block_size(size);
  // Определяем индекс списка
  int index = get_list_index(block_size);

  if (index < 0 || index >= MAX_BLOCK_CNT) {
    return NULL;
  }

  // Ищем свободный блок подходящего размера или больше
  for (int i = index; i < MAX_BLOCK_CNT; ++i) {
    if (alloc->free_lists[i] != NULL) {  // Есть свободные блоки
      block_t *block = alloc->free_lists[i];
      alloc->free_lists[i] = block->next;  // Удаляем блок из списка

      // Разделяем блок, если он больше, чем требуется
      while (i > index) {
        i--;
        size_t smaller_block_size = 1 << i;  // Размер меньшего блока
        uintptr_t split_addr = (uintptr_t)block + smaller_block_size;

        // Проверяем, что мы не выходим за пределы выделенной памяти
        if (split_addr + smaller_block_size >
            (uintptr_t)alloc->memory + alloc->total_size) {
          ERROR("Выход за границы памяти при разделении блока\n");
          return NULL;
        }

        // Создаём новый меньший блок
        block_t *split_block = (block_t *)split_addr;
        split_block->size = smaller_block_size;
        split_block->next = alloc->free_lists[i];
        alloc->free_lists[i] = split_block;
      }

      block->size = block_size;  // Устанавливаем размер блока
      // alloc->in_use_mem += block_size;
      return (void *)((uintptr_t)block + sizeof(block_t));
    }
  }

  // Нет доступных блоков
  // ERROR("Нехватка памяти: не удалось выделить блок размера %zu\n", size);
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

  block_t *block = (block_t *)memory_addr;
  size_t block_size = block->size;

  if (block_size > MAX_BLOCK_SIZE || block_size < MIN_BLOCK_SIZE) {
    return;
  }

  int index = get_list_index(block_size);

  if (index < 0 || index >= MAX_BLOCK_CNT) {
    return;
  }

  block->next = alloc->free_lists[index];
  alloc->free_lists[index] = block;
  // alloc->in_use_mem -= block_size;
}