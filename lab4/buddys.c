#include "main.h"

#define PAGE_SIZE 1

typedef struct block_t {
  size_t size;  // Размер блока
} block_t;

typedef struct BuddyAllocator {
  size_t total_size;        // общий размер
  void *memory;             // указатель на память
  long long in_use_mem;     // занятая память
  long long requested_mem;  // запрашиваемая память
  size_t block_size;        // размер блока памяти
  size_t num_blocks;        // Общее количество блоков
  uint8_t *bitmap;  // Битовая карта для отслеживания свободных/занятых блоков
} BuddyAllocator;

EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
  if (memory == NULL) {
    return NULL;
  }
  if (size < sizeof(BuddyAllocator)) {
    return NULL;
  }

  BuddyAllocator *allocator = (BuddyAllocator *)memory;
  // Устанавливаем параметры
  allocator->block_size = PAGE_SIZE;
  // Определяем количество блоков
  allocator->num_blocks = size / PAGE_SIZE;
  allocator->in_use_mem = 0;
  // bitmap = 1bit/block; флаг занятости
  size_t bitmap_size = (allocator->num_blocks + 7) / 8;

  allocator->memory =
      (void *)((uintptr_t)memory + sizeof(BuddyAllocator) + bitmap_size);
  allocator->bitmap = (uint8_t *)((char *)memory + sizeof(BuddyAllocator));

  // Обнуляем битовую карту
  memset(allocator->bitmap, 0, bitmap_size);
  allocator->total_size = size - bitmap_size - sizeof(BuddyAllocator);

  LOG("Buddys готовы\n");

  return (Allocator *)allocator;
}

EXPORT void allocator_destroy(Allocator *const allocator) {
  if (allocator == NULL) return;
  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

  size_t bitmap_size = (b_allocator->num_blocks + 7) / 8;

  b_allocator->bitmap = NULL;
  b_allocator->block_size = 0;
  b_allocator->num_blocks = 0;
  b_allocator->total_size = 0;
  b_allocator->memory = NULL;
  b_allocator->in_use_mem = 0;
  return;
}

// EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
//   if (allocator == NULL || size == 0) return NULL;
//   BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

//   size_t block_size = b_allocator->block_size;
//   while (block_size < size) {
//     block_size *= 2;  // делаем кратным 2
//   }

//   if (size > b_allocator->total_size) return NULL;

//   // блоки подходящего размера (их поиск с помощью битовой карты)
//   for (size_t i = 0; i < b_allocator->num_blocks; i++) {
//     size_t byte_index = i / 8;
//     size_t bit_index = i % 8;

//     if (!(b_allocator->bitmap[byte_index] & (1 << bit_index))) {
//       // Если блок свободен, метим его как занятый
//       b_allocator->bitmap[byte_index] |= (1 << bit_index);
//       b_allocator->in_use_mem += block_size;
//       return (void *)((uintptr_t)b_allocator->memory + i * block_size);
//     }
//   }

//   return NULL;
// }
EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
  if (allocator == NULL || size == 0) {
    return NULL;
  }
  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

  // Вычисляем минимальный размер блока, кратного степени двойки
  size_t block_size = b_allocator->block_size;
  size_t block_index = 0;
  while (block_size < size + sizeof(block_t)) {
    block_size *= 2;
    block_index++;
  }

  if (block_size > b_allocator->total_size) {
    return NULL;
  }

  // Поиск подходящего блока
  size_t i = block_index;
  while (i < b_allocator->num_blocks &&
         (b_allocator->bitmap[i / 8] & (1 << (i % 8)))) {
    i++;
  }

  if (i >= b_allocator->num_blocks) {
    ERROR("Не найден свободный блок подходящего размера\n");
    return NULL;
  }

  // Разделение блоков на меньшие, если требуется
  while (i > block_index) {
    i--;
    size_t buddy_index = i ^ 1;  // Находим индекс соседа
    b_allocator->bitmap[buddy_index / 8] &= ~(1 << (buddy_index % 8));
  }

  // Помечаем найденный блок как занятый
  b_allocator->bitmap[i / 8] |= (1 << (i % 8));

  // Вычисляем адрес блока
  uintptr_t block_address = (uintptr_t)b_allocator->memory + i * block_size;

  // Сохраняем заголовок
  block_t *header = (block_t *)block_address;
  header->size = block_size;

  // Обновляем статистику
  b_allocator->in_use_mem += block_size;

  // Возвращаем указатель на память сразу после заголовка
  return (void *)(block_address + sizeof(block_t));
}

EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
  if (memory == NULL || allocator == NULL) {
    return;
  }
  BuddyAllocator *b_allocator = (BuddyAllocator *)allocator;

  // Получаем адрес заголовка
  uintptr_t block_address = (uintptr_t)memory - sizeof(block_t);
  block_t *header = (block_t *)block_address;

  size_t block_size = header->size;
  size_t block_index =
      (block_address - (uintptr_t)b_allocator->memory) / block_size;

  // Освобождаем блок в битовой карте
  size_t byte_index = block_index / 8;
  size_t bit_index = block_index % 8;
  b_allocator->bitmap[byte_index] &= ~(1 << bit_index);

  // Обновляем статистику
  b_allocator->in_use_mem -= block_size;

  // Пытаемся объединить блоки
  while (block_size < b_allocator->total_size) {
    size_t buddy_index = block_index ^ 1;  // Находим индекс соседа
    if (!(b_allocator->bitmap[buddy_index / 8] & (1 << (buddy_index % 8)))) {
      // Если соседний блок свободен, объединяем
      b_allocator->bitmap[buddy_index / 8] &= ~(1 << (buddy_index % 8));
      block_index /= 2;  // Переходим к объединенному блоку
      block_size *= 2;
    } else {
      break;  // Если сосед занят, прекращаем объединение
    }
  }

  // memset(header, 0, block_size);
  return;
}
