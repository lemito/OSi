#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include <dlfcn.h>  // dlopen, dlsym, dlclose, RTLD_*
#include <unistd.h> // write

#include "main.h"

// static fabsf_func *fabsf;
// static cosf_func *cosf;

// NOTE: Functions stubs will be used, if library failed to load
// NOTE: Stubs are better than NULL function pointers,
//       you don't need to check for NULL before calling a function
static float func_impl_stub(float x) {
  (void)x; // NOTE: Compiler will warn about unused parameter otherwise
  return 0.0f;
}

int main(int argc, char **argv) {
  (void)argc;

  void *library = dlopen(argv[1], RTLD_LOCAL | RTLD_NOW);

  /* библиотека смогла открыться */
  if (argc > 1 && library != NULL) {
    // fabsf = dlsym(library, "fabsf");
    // if (fabsf == NULL) {
    //   const char msg[] =
    //       "warning: failed to find abs function implementation\n";
    //   write(STDERR_FILENO, msg, sizeof(msg));
    //   fabsf = func_impl_stub;
    // }

    // cosf = dlsym(library, "cosf");
    // if (cosf == NULL) {
    //   const char msg[] =
    //       "warning: failed to find cosine function implementation\n";
    //   write(STDERR_FILENO, msg, sizeof(msg));
    //   cosf = func_impl_stub;
    // }
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
    const float x = -3.14159f;
    const float y = fabsf(x);

    char buf[1024];
    int length =
        snprintf(buf, sizeof(buf) - 1, "Abs of %.10f is %.10f\n", x, y);
    buf[length] = '\0';
    write(STDOUT_FILENO, buf, length);
  }

  {
    const float x = 0.5f;
    const float y = cosf(x);

    char buf[1024];
    int length =
        snprintf(buf, sizeof(buf) - 1, "Cosine of %f is %.10f\n", x, y);
    buf[length] = '\0';
    write(STDOUT_FILENO, buf, length);
  }
  /* ==================================== */
  /* Отключение библиотек при наличии */
  if (library) {
    dlclose(library);
  }
  /* ==================================== */
}