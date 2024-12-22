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
  size_t total_size;     // общий размер
  void *memory;          // указатель на память
  long long in_use_mem;  // занятая память (с учётом накладных расходов)
  long long requested_mem;  // запрашиваемая память (без накладных расходов)
  block_t *free_lists[MAX_BLOCK_CNT];  // массив списков свободных блоков
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
    return NULL;  // Невозможно создать аллокатор без памяти
  }

  // Проверка на минимально необходимую память
  if (size < sizeof(p2Alloc)) {
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

  // Начинаем разбиение памяти на блоки
  size_t offset = 0;
  size_t remaining_size = allocator->total_size;

  while (remaining_size >= MIN_BLOCK_SIZE) {
    size_t block_size = MIN_BLOCK_SIZE;  // Всегда начинаем с минимального блока

    // Разбиваем оставшийся размер на блоки, начиная от минимального до
    // максимально возможного
    while (block_size <= remaining_size && block_size <= MAX_BLOCK_SIZE) {
      int index = get_list_index(block_size);

      if (index >= MAX_BLOCK_CNT) {
        break;  // Если индекс выходит за пределы доступных списков
      }

      block_t *block = (block_t *)((uintptr_t)allocator->memory + offset);
      block->size = block_size;  // Устанавливаем размер блока
      block->next =
          allocator->free_lists[index];  // Помещаем блок в список свободных
      allocator->free_lists[index] = block;

      offset += block_size;
      remaining_size -= block_size;

      block_size <<= 1;
    }

    if (remaining_size < MIN_BLOCK_SIZE) {
      break;  // Если оставшаяся память меньше минимального размера, прекращаем
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
    if (alloc->free_lists[i] != NULL) {  // Если найден блок
      block_t *block = alloc->free_lists[i];
      alloc->free_lists[i] = block->next;  // Удаляем его из списка

      // Если блок найденного размера больше нужного, разрезаем его
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

      block->size = block_size;  // Устанавливаем размер блока
      alloc->in_use_mem += block_size;  // Увеличиваем занятый объём памяти
      alloc->requested_mem += size;  // Учитываем реальный запрос пользователя
      return (void *)((uintptr_t)block + sizeof(block_t));  // Возвращаем память
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
  alloc->in_use_mem -= block_size;
  alloc->requested_mem -= block_size - sizeof(block_t);
}

EXPORT double allocator_usage_factor(Allocator *const allocator) {
  if (NULL == allocator) {
    return 0.0;
  }

  p2Alloc *alloc = (p2Alloc *)allocator;

  if (alloc->in_use_mem == 0) {
    return 0.0;
  }

  LOG("meow meow %lld __ %lld\n", alloc->requested_mem / alloc->in_use_mem);

  return (double)alloc->requested_mem / (double)alloc->in_use_mem;
}