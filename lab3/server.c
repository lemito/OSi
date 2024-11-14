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
#include <sys/wait.h>
#include <unistd.h>

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

  // открываем поток (односторонний)
  int pipe1[2];
  if (pipe(pipe1) == -1) {
    const char msg[] = "error: failed to create pipe\n";
    write(STDERR_FILENO, msg, sizeof(msg));
    exit(EXIT_FAILURE);
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
      // pid_t pid = getpid();

      // подключаю родительский ввод к дочернему
      // то есть мой ввод - ввод родителя
      // dup2(STDIN_FILENO, pipe1[STDIN_FILENO]);
      // close(pipe1[STDOUT_FILENO]);

      int file = open(argv[1], O_RDONLY);
      if (file == -1) {
        perror("Ошибка при открытии файла");
        exit(EXIT_FAILURE);
      }

      // stdin_no теперь файл | файл теперь поток входа
      if (-1 == dup2(file, STDIN_FILENO)) {
        const char msg[] = "error: to dup file as STDIN_FILENO\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }
      close(file);

      // stdout_no теперь правый кончик pipe
      if (-1 == dup2(pipe1[STDOUT_FILENO], STDOUT_FILENO)) {
        const char msg[] =
            "error: to dup pipe1[STDOUT_FILENO] as STDOUT_FILENO\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(EXIT_FAILURE);
      }
      close(pipe1[STDIN_FILENO]);
      // close(pipe1[STDOUT_FILENO]);
      // {
      //   char msg[64];
      //   const int32_t length =
      //       snprintf(msg, sizeof(msg), "%d: I'm a child\n", pid);
      //   write(STDOUT_FILENO, msg, length);
      // }

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

      // {
      //   char msg[64];
      //   const int32_t length =
      //       snprintf(msg, sizeof(msg),
      //                "%d: I'm a parent, my child has PID %d\n", pid, child);
      //   write(STDOUT_FILENO, msg, length);
      // }

      // dup2(pipe1[STDIN_FILENO], STDOUT_FILENO);
      close(pipe1[STDOUT_FILENO]);

      // NOTE: `wait` blocks the parent until child exits
      // блокируем родителя до конца выполнения дочерних процессов
      int child_status;
      wait(&child_status);

      {
        char buffer[4096];
        ssize_t count;

        while ((count = read(pipe1[STDIN_FILENO], buffer, sizeof(buffer)))) {
          if (count < 0) {
            const char msg[] = "error: ferror reading from pipe\n";
            write(STDERR_FILENO, msg, sizeof(msg));
            exit(EXIT_FAILURE);
          } else if (buffer[0] == '\n') {
            break;
          }
        }

        fprintf(stdout, "%s\n", buffer);
      }

      if (child_status != EXIT_SUCCESS) {
        const char msg[] = "error: child exited with error\n";
        write(STDERR_FILENO, msg, sizeof(msg));
        exit(child_status);
      }
      close(pipe1[STDIN_FILENO]);
    } break;
  }
}
