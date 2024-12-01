
extern int FILE_SIZE;

#pragma once

#include <fcntl.h>
#include <semaphore.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define SEM_PARENT_READY "/sem_parent_ready"
#define SEM_CHILD_READY "/sem_child_ready"
#define SHM_NAME "/fileshm\0"
#define RES_SHM "/resshm\0"

// размер входного файла и shm, должен быть протянут и в client.c

typedef enum PRINT_MODES {
  SUCCESS,
  ERROR,
} PRINT_MODES;

int _print(char mode, char* fmt, ...) {
  if (fmt == NULL) {
    write(STDERR_FILENO, "ERROR", 6);
  }
  va_list vargs;
  va_start(vargs, fmt);
  char msg[BUFSIZ];
  vsprintf(msg, fmt, vargs);
  write(mode == ERROR ? STDERR_FILENO : STDOUT_FILENO, msg, strlen(msg));
  va_end(vargs);
  return 0;
}

int* create_mmap_int(const char* name) {
  int new_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
  ftruncate(new_fd, sizeof(int));

  int* FILE_SIZE =
      mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED, new_fd, 0);
  return FILE_SIZE;
}

void destroy_mmap_int(int* var, const char* name) {
  munmap(var, sizeof(int));
  shm_unlink(name);
}