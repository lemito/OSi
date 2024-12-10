// #include "main.h"

// // NOTE: MSVC compiler does not export symbols unless annotated
// #ifdef _MSC_VER
// #define EXPORT __declspec(dllexport)
// #else
// #define EXPORT
// #endif

// EXPORT Allocator *allocator_create(void *const memory, const size_t size) {
//   return NULL;
// }

// EXPORT void allocator_destroy(Allocator *const allocator) { return; }

// EXPORT void *allocator_alloc(Allocator *const allocator, const size_t size) {
//   return;
// }

// EXPORT void allocator_free(Allocator *const allocator, void *const memory) {
//   return;
// }