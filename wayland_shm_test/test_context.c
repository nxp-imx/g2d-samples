#include "test_context.h"

int test_context_alloc(size_t width, size_t height) {
  test_context *rv = malloc(sizeof(test_context));
  rv->width = width;
  rv->height = height;
  return rv;
}
