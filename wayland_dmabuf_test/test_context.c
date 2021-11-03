#include "test_context.h"
#include <stdlib.h>

int test_context_alloc(size_t width, size_t height) {
  test_context *rv = malloc(sizeof(test_context));
  rv->window_width = width;
  rv->window_height = height;
  return rv;
}
