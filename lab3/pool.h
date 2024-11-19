#pragma once

#define SHM_NAME "/myshm\0"

// читалка входного файла
extern void* src;

// shm для вывода ответа
int shm_fd;