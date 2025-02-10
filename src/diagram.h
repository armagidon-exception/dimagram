#pragma once

#include "graph_utils.h"
#include <cairo.h>

typedef enum {
  DIAGRAM_EIO,
  DIAGRAM_EUNKIDEN,
  DIAGRAM_ENONPLANAR,
  DIAGRAM_EEMPTY,
  DIAGRAM_ESYNTAX,
  DIAGRAM_EDISCONNECTED,
  DIAGRAM_EDUP,
  DIAGRAM_OK,
} diagram_status_e;

typedef struct {
  Arena *arena;
  cairo_surface_t **textures;
  igraph_t G;
  const char *errmsg;
} diagram_t;

diagram_status_e diagram_parse_diagram_from_stream(diagram_t *diagram,
                                                   FILE *stream, Arena *arena);

cairo_surface_t *diagram_lay_out(diagram_t *D, FILE *os, diagram_status_e *err);
