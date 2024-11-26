#pragma once
#include <stdarg.h>

#define SHM_NAME "/fileshm\0"
#define RES_SHM "/resshm\0"

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
  write(mode == ERROR ? STDERR_FILENO : STDOUT_FILENO, msg, sizeof(msg));
  va_end(vargs);
}