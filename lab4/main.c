#include "main.h"

#define SIZE (1024 * 1024)

static allocator_create_f* allocator_create;
static allocator_destroy_f* allocator_destroy;
static allocator_alloc_f* allocator_alloc;
static allocator_free_f* allocator_free;

int main(int argc, char** argv) {
  (void)argc;
  LOG("mem create");

  void* library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);
  // void* library = dlopen("./buddys.so", RTLD_LOCAL | RTLD_NOW);
  /* библиотека смогла открыться */
  if (library != NULL) {
    allocator_create = dlsym(library, "allocator_create");
    if (allocator_create == NULL) {
      const char msg[] =
          "warning: failed to find allocator_create function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_create = allocator_create_extra;
    }

    allocator_destroy = dlsym(library, "allocator_destroy");
    if (allocator_destroy == NULL) {
      const char msg[] =
          "warning: failed to find allocator_destroy function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_destroy = allocator_destroy_extra;
    }

    allocator_alloc = dlsym(library, "allocator_alloc");
    if (allocator_alloc == NULL) {
      const char msg[] =
          "warning: failed to find allocator_alloc function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_alloc = allocator_alloc_extra;
    }

    allocator_free = dlsym(library, "allocator_free");
    if (allocator_free == NULL) {
      const char msg[] =
          "warning: failed to find allocator_free function implementation\n";
      write(STDERR_FILENO, msg, sizeof(msg));
      allocator_free = allocator_free_extra;
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
    LOG("Создаем memory\n");
    void* memory = mmap(NULL, SIZE, PROT_READ | PROT_WRITE,
                        MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (memory == MAP_FAILED) {
      perror("mmap failed");
      return 1;
    }

    LOG("Создаем аллокатор\n");
    Allocator* allocator = allocator_create(memory, SIZE);

    LOG("Аллоцируем\n");
    int* block1 = (int*)allocator_alloc(allocator, sizeof(int) * 52);
    for (size_t i = 0; i < 53; i++) {
      block1[i] = 27022005 - (i % 52);
    }

    int* test_for_free = block1 + 2;

    void* block2 = allocator_alloc(allocator, 2048);

    LOG("Алоцированный блок 1 живет по адресу %p и там есть %d\n", block1,
        *test_for_free);
    LOG("Алоцированный блок 1 живет по адресу %p\n", block2);

    allocator_free(allocator, block1);
    allocator_free(allocator, block2);

    LOG("Очищено\n");
    allocator_destroy(allocator);

    munmap(memory, SIZE);
  }

  /* ==================================== */
  /* Отключение библиотек при наличии */
  if (library) {
    dlclose(library);
  }
  /* ==================================== */
}