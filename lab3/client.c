/**
 * @file client.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-10-02
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pool.h"

// extern int FILE_SIZE;

float summ(const char* src) {
  if (src == NULL) {
    return 0.0;
  }
  float sum = 0.0;
  char* endptr;

  while (*src) {
    while (*src && !isdigit(*src) && *src != '.' && *src != '-' &&
           *src != '+') {
      ++src;
    }

    float value = strtod(src, &endptr);
    if (value == HUGE_VAL || value == -HUGE_VAL) {
      return 0.0;
    }

    if (endptr != src) {
      sum += value;
      src = endptr;
    } else {
      ++src;
    }
  }

  return sum;
}

int main(int argc, char** argv) {
  /* Получение сигнала о том, что записано */
  sem_t* sem_parent_ready = sem_open(SEM_PARENT_READY, 0);
  if (sem_parent_ready == SEM_FAILED) {
    _print(ERROR, "sem_parent_ready open failed\n", SHM_NAME);
    exit(EXIT_FAILURE);
  }
  // -1 от семафора => если 0 - блокнут и нельзя ничего делать кроме как
  // ждать
  if (-1 == sem_wait(sem_parent_ready)) {
    _print(ERROR, "sem_parent_ready wait failed\n", SHM_NAME);
    exit(EXIT_FAILURE);
  }
  /* возврат размера файла */
  int tmp = shm_open("/file_size_shm\0", O_RDWR, 0666);
  if (tmp == -1) {
    _print(ERROR, "%s: open failed child\n", "/file_size_shm\0");
    exit(EXIT_FAILURE);
  }
  write(STDOUT_FILENO, "mmap1", 6);
  int* shared_variable = mmap(0, sizeof(int), PROT_READ, MAP_SHARED, tmp, 0);
  if (shared_variable == MAP_FAILED) {
    _print(ERROR, "%s: mapping failed child\n", "/file_size_shm\0");
    exit(EXIT_FAILURE);
  }

  int FILE_SIZE = *shared_variable;

  destroy_mmap_int(shared_variable, "/file_size_shm\0");
  /*=========================*/
  /* работа с файлом (входной поток) */
  int shm_fd;
  if ((shm_fd = shm_open(SHM_NAME, O_RDONLY, 0666)) == -1) {
    _print(ERROR, "error: shm_open child\n");
    exit(EXIT_FAILURE);
  }
 write(STDOUT_FILENO, "mmap2", 6);
  char* src = mmap(0, FILE_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
  if (src == MAP_FAILED) {
    _print(ERROR, "%s: mapping failed child\n", SHM_NAME);
    exit(EXIT_FAILURE);
  }

  float sum = summ(src);
  /*=========================*/
  /* запись в результативную память */
  int res_fd;
  if ((res_fd = shm_open(RES_SHM, O_CREAT | O_RDWR, 0666)) == -1) {
    _print(ERROR, "error: shm_open child\n");
    exit(EXIT_FAILURE);
  }

  ftruncate(res_fd, BUFSIZ);
 write(STDOUT_FILENO, "mmap3", 6);
  void* ressrc = mmap(0, BUFSIZ, PROT_READ | PROT_WRITE, MAP_SHARED, res_fd, 0);
  if (ressrc == MAP_FAILED) {
    perror("mmap");
    exit(EXIT_FAILURE);
  }

  if (0 == sprintf(ressrc, "%f", sum)) {
    perror("write result");
    exit(EXIT_FAILURE);
  }
  /* уведомление о готовности результата */
  sem_t* sem_child_ready = sem_open(SEM_CHILD_READY, O_CREAT, 0666, 0);
  if (sem_child_ready == SEM_FAILED) {
    _print(ERROR, "sem_child_ready open failed\n", SHM_NAME);
    exit(EXIT_FAILURE);
  }
  // +1 в семафор; т.е. можно есть
  if (-1 == sem_post(sem_child_ready)) {
    _print(ERROR, "sem_child_ready post failed\n", SHM_NAME);
    exit(EXIT_FAILURE);
  }
  /*=========================*/
  if (-1 == munmap(src, FILE_SIZE)) {
    perror("munmap");
    exit(EXIT_FAILURE);
  }
  if (-1 == munmap(ressrc, BUFSIZ)) {
    perror("munmap");
    exit(EXIT_FAILURE);
  }
  close(shm_fd);
  close(res_fd);
  sem_close(sem_parent_ready);
  exit(EXIT_SUCCESS);
}
