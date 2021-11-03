#include <unistd.h>

typedef struct {
  void *shm_data;
  int phy_data;
  size_t width;
  size_t height;

} test_context;

extern test_context *test_context_alloc(size_t width, size_t height);
extern void paint_pixels(test_context *tc);
