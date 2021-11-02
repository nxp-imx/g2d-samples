#include <unistd.h>

typedef struct {
  void *dst_vaddr;
  int dst_paddr;

  size_t dst_width;
  size_t dst_height;
  unsigned dst_top;
  unsigned dst_left;
  unsigned dst_rotation;
  unsigned dst_color_format;

  unsigned src_color_format;
  unsigned src_tiling;
  void *src_buf;
  size_t src_sz;
  size_t src_width;
  size_t src_height;
  unsigned src_set_alpha;
  unsigned src_set_blur;

} test_context;

extern test_context *test_context_alloc(size_t dst_width, size_t dst_height);
extern void paint_pixels(test_context *tc);
