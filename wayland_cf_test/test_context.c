#include "test_context.h"
#include <stdlib.h>

test_context *test_context_alloc(size_t width, size_t height) {
  test_context *rv = calloc(1, sizeof(test_context));
  rv->dst_width = width;
  rv->dst_height = height;
  return rv;
}
