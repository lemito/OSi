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
#include "pool.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static char CLIENT_PROGRAM_NAME[] = "client";

int main(int argc, char** argv) {
  if (argc != 2) {
    _print(ERROR, "usage: %s filename\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  // хде я?
  char progpath[1024] = {0};
  {
    // собственно узнаем где
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
      _print(ERROR, "error: failed to read full program path\n");
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
      _print(ERROR, "error: failed to spawn new process\n");
      exit(EXIT_FAILURE);
    } break;

    case 0: {  // Я - ребенок и не знаю свой PID
               // собственно узнаю

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

      int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
      if (shm_fd == -1) {
        _print(ERROR, "%s: open failed\n", SHM_NAME);
        exit(EXIT_FAILURE);
      }

      // укорачиваем файла строго до определенной длины
      if (ftruncate(shm_fd, file_stat.st_size) == -1) {
        _print(ERROR, "%s: ftruncate failed\n", SHM_NAME);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
      }

      char* src = mmap(NULL, file_stat.st_size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, shm_fd, 0);
      if (src == MAP_FAILED) {
        _print(ERROR, "%s: mapping failed\n", SHM_NAME);
        close(shm_fd);
        shm_unlink(SHM_NAME);
        exit(EXIT_FAILURE);
      }
      // char buf[BUFSIZ];
      read(file, src, file_stat.st_size);
      // strcpy(buf, src);
      // fprintf(stdout, "%s\n", buf);
      int* FILE_SIZE = create_mmap_int("/file_size_shm\0");

      *FILE_SIZE = file_stat.st_size;
      close(file);
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
      if ((shm_fd = shm_open(RES_SHM, O_RDONLY, 0666)) == -1) {
        _print(ERROR, "error: shm_open parrent\n");
        exit(EXIT_FAILURE);
      }
      ftruncate(shm_fd, BUFSIZ);

      char* res;
      if ((res = mmap(0, BUFSIZ, PROT_READ, MAP_SHARED, shm_fd, 0)) ==
          MAP_FAILED) {
        _print(ERROR, "error: res mmap parrent\n");
        exit(EXIT_FAILURE);
      }

      _print(SUCCESS, "res=%s\n", res);

      if (child_status != EXIT_SUCCESS) {
        const char msg[] = "error: child exited with error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        perror("");
        exit(child_status);
      }

      close(shm_fd);
      shm_unlink(SHM_NAME);
      shm_unlink(RES_SHM);
    } break;
  }
}
