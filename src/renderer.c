#include "renderer.h"
#include <cairo-svg.h>
#include <cairo.h>
#include <math.h>
#include <stdlib.h>

typedef struct renderer_t {
  cairo_t *context;
  cairo_surface_t *surface;
  unsigned int width;
  unsigned int height;
} renderer_t;

renderer_t *render_svg_create(const char *filename, unsigned int width, unsigned int height) {
  if (!filename)
    return 0;

  renderer_t *renderer = calloc(1, sizeof(renderer_t));
  renderer->surface = cairo_svg_surface_create(filename, width, height);
  renderer->context = cairo_create(renderer->surface);
  renderer->width = width;
  renderer->height = height;
  return renderer;
}


void renderer_set_origin(renderer_t* renderer, double x, double y) {
  cairo_translate(renderer->context, x, y);
}

void renderer_destroy(renderer_t *renderer) {
  if (!renderer)
    return;

  cairo_destroy(renderer->context);
  cairo_surface_destroy(renderer->surface);

  free(renderer);
}

#ifdef DEBUG_ENGINE
static void render_orb(renderer_t *renderer, node_t *self, double x, double y) {
  cairo_set_source_rgb(renderer->context, 0, 0, 0);
  cairo_arc(renderer->context, x, y, 3, 0, 2 * M_PI);
  cairo_fill(renderer->context);
}

static void orb_destroy(node_t *self) { free(self); }

node_t *renderer_create_orb(void) {
  node_t *n = calloc(1, sizeof(node_t));
  n->render = render_orb;
  n->destroy = orb_destroy;

  return n;
}
#endif

void renderer_link(renderer_t *renderer, double a, double b, double c,
                   double d) {
  cairo_save(renderer->context);
  cairo_move_to(renderer->context, a, b);
  cairo_line_to(renderer->context, c, d);
  cairo_stroke(renderer->context);
  cairo_restore(renderer->context);
}
