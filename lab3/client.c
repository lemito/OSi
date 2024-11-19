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
#include <unistd.h>
// #include <bits/mman-linux.h>
#include <sys/mman.h>

#include "pool.h"

#define SIZE BUFSIZ

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
  char buf[4096];
  ssize_t bytes;

  // int shm_fd;

  // // Открытие существующей разделяемой памяти
  // if ((shm_fd = shm_open("FileSHM", O_RDONLY, 0444)) == -1) {
  //   perror("shm_open");
  //   const char msg[] = "error: shm_open child\n";
  //   write(STDERR_FILENO, msg, sizeof(msg));
  //   exit(EXIT_FAILURE);
  // }

  // // // Отображение разделяемой памяти
  char* src = mmap(0, SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);

  // Чтение из разделяемой памяти
  printf("Data from shared memory: %s\n", src);

  // Закрытие разделяемой памяти
  // munmap(src, SIZE);

  // pid_t pid = getpid();

  {
    char out_buf[1024];
    out_buf[0] = '\0';
    float_t sum = summ(src);
    sprintf(out_buf, "%f", sum);
    int32_t written = write(STDOUT_FILENO, out_buf, strlen(out_buf));
    if (written == -1) {
      const char msg[] = "error: failed to write to out buffer\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      exit(EXIT_FAILURE);
    }
  }

  // TODO: Check for count of actual bytes written
  const char term = '\0';
  write(STDOUT_FILENO, &term, sizeof(term));
  exit(EXIT_SUCCESS);
}
