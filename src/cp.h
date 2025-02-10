#pragma once

#include "graph_utils.h"
#include "igraph_datatype.h"
#include "igraph_matrix.h"

#include "dynarray.h"

DEFINE_VECTOR(co, gint)


typedef enum {
  HS_OK,
  HS_NONPLANAR,
  HS_INVALID_INPUT,
  HS_DISCONNECTED,
  HS_EMPTY_GRAPH
} hs_error_t;

hs_error_t harel_sardes_layout(igraph_t *G, igraph_matrix_t *output);
