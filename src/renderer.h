#pragma once

#include <cairo.h>
#define DEBUG_ENGINE

typedef struct renderer_t renderer_t;

typedef struct node_t {
  void (*render)(renderer_t* renderer, struct node_t *self, double x, double y);
  void (*destroy)(struct node_t *self);
  void *data;
  double x;
  double y;
} node_t;

renderer_t *render_svg_create(const char *filename, unsigned int width, unsigned int height);

void renderer_link(renderer_t* renderer, double a, double b, double c, double d);

void renderer_destroy(renderer_t *renderer);

void renderer_set_origin(renderer_t* renderer, double x, double y) ;

#ifdef DEBUG_ENGINE
node_t *renderer_create_orb(void);
#endif
