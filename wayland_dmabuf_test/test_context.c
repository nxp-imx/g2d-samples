#include "test_context.h"
#include <stdlib.h>

test_context *test_context_alloc(size_t width, size_t height) {
  test_context *rv = calloc(1, sizeof(test_context));
  rv->window_width = width;
  rv->window_height = height;
  return rv;
}
