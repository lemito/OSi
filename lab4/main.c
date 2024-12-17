#include "main.h"

#include <dlfcn.h>  // dlopen, dlsym, dlclose, RTLD_*
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>  // write

static allocator_create_f* allocator_create;
static allocator_destroy_f* allocator_destroy;
static allocator_alloc_f* allocator_alloc;
static allocator_free_f* allocator_free;

// NOTE: Functions stubs will be used, if library failed to load
// NOTE: Stubs are better than NULL function pointers,
//       you don't need to check for NULL before calling a function
static struct Allocator* func_impl_stub(void* const mem,
                                        const unsigned long x) {
  (void)x;  // NOTE: Compiler will warn about unused parameter otherwise
  (void)mem;
  return NULL;
}

static void func_impl_stub2(struct Allocator* const alloc) {
  (void)alloc;
  return;
}

static void* func_impl_stub3(struct Allocator* const alloc,
                             const unsigned long x) {
  (void)alloc;
  (void)x;
  return NULL;
}

static void func_impl_stub4(struct Allocator* const alloc, void* const mem) {
  (void)alloc;
  (void)mem;
  return;
}

int main(int argc, char** argv) {
  (void)argc;
  printf("mem create");

  // void* library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
  void* library = dlopen("./buddys.so", RTLD_LOCAL | RTLD_NOW);
  argc++;
  /* библиотека смогла открыться */
  if (argc > 1 && library != NULL) {
    allocator_create = dlsym(library, "allocator_create");
    if (allocator_create == NULL) {
      const char msg[] =
          "warning: failed to find allocator_create function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_create = func_impl_stub;
    }

    allocator_destroy = dlsym(library, "allocator_destroy");
    if (allocator_destroy == NULL) {
      const char msg[] =
          "warning: failed to find allocator_destroy function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_destroy = func_impl_stub2;
    }

    allocator_alloc = dlsym(library, "allocator_alloc");
    if (allocator_alloc == NULL) {
      const char msg[] =
          "warning: failed to find allocator_alloc function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_alloc = func_impl_stub3;
    }

    allocator_free = dlsym(library, "allocator_free");
    if (allocator_free == NULL) {
      const char msg[] =
          "warning: failed to find allocator_free function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_free = func_impl_stub4;
    }
  }
  /* ==================================== */
  /* испольхование стандартной библиотеки */
  else {
    const char msg[] =
        "warning: library failed to load, trying standard implemntations\n";
    write(STDERR_FILENO, msg, sizeof(msg));

    // NOTE: Trying standard implementations
    library = dlopen("libm.so.6", RTLD_GLOBAL | RTLD_LAZY);
    if (library == NULL) {
      const char msg[] = "error: failed to open standard math library\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      return EXIT_FAILURE;
    }

    // fabsf = dlsym(library, "fabsf");
    // cosf = dlsym(library, "cosf");
  }

  /* сами действие */
  {
    printf("mem create");
    void* memory = mmap(NULL, 40960, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    printf("meow create");
    Allocator* const meow = allocator_create(memory, 128);
    printf("meow alloc");
    memory = allocator_alloc(meow, 64);
    printf("mem == 52\n");
    *(int*)meow->data = 52;
    printf("meow == %d %d size == %zu and mem free\n", *(int*)memory,
           *(int*)meow->data, meow->size);
    allocator_free(meow, memory);
    printf("mem destroy");
    allocator_destroy(meow);
    munmap(memory, 40960);
  }

  /* ==================================== */
  /* Отключение библиотек при наличии */
  if (library) {
    dlclose(library);
  }
  /* ==================================== */
}