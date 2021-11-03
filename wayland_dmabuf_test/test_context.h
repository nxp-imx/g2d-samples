#include <unistd.h>

struct client_buffer;

typedef struct {
  void *shm_data;
  int phy_data;
  size_t window_width;
  size_t window_height;
  void *user_data;
  struct client_buffer *dmabuffers[2];

} test_context;

extern int test_context_alloc(size_t width, size_t height);
extern void paint_pixels(test_context *tc);
