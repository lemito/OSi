/**
 * @file server.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2024-10-02
 *
 * @copyright Copyright (c) 2024
 *
 */
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "pool.h"

static char CLIENT_PROGRAM_NAME[] = "client";

int main(int argc, char** argv) {
  if (argc == 1) {
    char msg[1024];
    uint32_t len =
        snprintf(msg, sizeof(msg) - 1, "usage: %s filename\n", argv[0]);
    write(STDERR_FILENO, msg, len);
    exit(EXIT_SUCCESS);
  }

  // хде я?
  char progpath[1024];
  {
    // собственно узнаем где
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
      const char msg[] = "error: failed to read full program path\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      exit(EXIT_FAILURE);
    }

    // всё лишнее убираем
    while (progpath[len] != '/')
      --len;

    progpath[len] = '\0';
  }

  // создаем новый процесс
  const pid_t child = fork();

  switch (child) {
    case -1: {  // обработка ошибки
      const char msg[] = "error: failed to spawn new process\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      exit(EXIT_FAILURE);
    } break;

    case 0: {  // Я - ребенок и не знаю свой PID
      // собственно узнаю

      void* src;

      /* файл*/
      int file = open(argv[1], O_RDONLY);
      if (file == -1) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
      }

      struct stat file_stat;
      if (fstat(file, &file_stat) < 0) {
        const char msg[] = "error: failed create stat for file\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }

      if (lseek(file, file_stat.st_size - 1, SEEK_SET) == -1) {
        const char msg[] = "error: lseek\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }

      if ((src = mmap(0, file_stat.st_size, PROT_READ, MAP_SHARED, file, 0)) ==
          MAP_FAILED) {
        const char msg[] = "error: mmaping\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }

      /* запись ответа */
      int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      if (shm_fd == -1) {
        const char msg[] = "error: shm_open child\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }
      ftruncate(shm_fd, file_stat.st_size);

      char* read_ptr = mmap(0, file_stat.st_size, PROT_READ | PROT_WRITE,
                            MAP_SHARED, shm_fd, 0);
      if (read_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
      }

      fprintf(stdout, "%s\n", src);

      {
        char path[1024];
        snprintf(path, sizeof(path) - 1, "%s/%s", progpath,
                 CLIENT_PROGRAM_NAME);

        // аргументами для клиента являтся -- сам клиент (его запускатор)б
        // полезные аргументы (файл)б и терминатор, ведь exec требует список с
        // терминирующем нулем
        char* const args[] = {CLIENT_PROGRAM_NAME, argv[1], NULL};

        int32_t status = execv(path, args);

        if (status == -1) {
          const char msg[] =
              "error: failed to exec into new exectuable image\n";
          write(STDERR_FILENO, msg, sizeof(msg));
          exit(EXIT_FAILURE);
        }
      }
    } break;

    default: {  // Я родитель и знаю PID дочерный
      // pid_t pid = getpid();  // Получаем родительский PID

      // NOTE: `wait` blocks the parent until child exits
      // блокируем родителя до конца выполнения дочерних процессов
      int child_status;
      wait(&child_status);

      // чтение ответа
      int shm_fd;
      if ((shm_fd = shm_open(SHM_NAME, O_RDONLY, 0444)) == -1) {
        perror("meow");
        const char msg[] = "error: shm_open parrent\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }

      if (child_status != EXIT_SUCCESS) {
        const char msg[] = "error: child exited with error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(child_status);
      }
    } break;
  }

  // shm_unlink(SHM_NAME);
}
