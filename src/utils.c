#include "utils.h"
#include "cairo-svg.h"
#include "cairo.h"
#include "log.h"
#include <assert.h>
#include <stdint.h>

unsigned long long_to_ulong(long l) {
  assert(l >= 0 && "Integer overflow when casting");
  return (unsigned long)l;
}
unsigned int int_to_uint(int i) {
  assert(i >= 0 && "Integer overflow when casting");
  return (unsigned int)i;
}
long ulong_to_long(unsigned long l) {
  assert(l <= INT64_MAX && "Integer underflow when casting");
  return (long)l;
}
int uint_to_int(unsigned int l) {
  assert(l <= INT32_MAX && "Integer underflow when casting");
  return (int)l;
}

static cairo_status_t cairo_no_op_func(void *closure, const uint8_t *data,
                                       uint32_t len) {
  (void)closure;
  (void)data;
  (void)len;
  return CAIRO_STATUS_SUCCESS;
}

cairo_surface_t* cairo_tmp_svg_surface(double width, double height) {
  cairo_surface_t *surf =
      cairo_svg_surface_create_for_stream(cairo_no_op_func, 0, width, height);

  return surf;
}
