#include "graph_utils.h"
#include "igraph_constants.h"
#include "igraph_interface.h"
#include "igraph_operators.h"
#include "igraph_visitor.h"
#include <stdbool.h>
#include <stdlib.h>

veci getneigs(const graph *G, gint v) {
  veci out;
  igraph_vector_int_init(&out, 0);
  igraph_neighbors(G, &out, v, IGRAPH_ALL);

  return out;
}

veci veci_new(void) {
  veci vector;
  veci_init(&vector, 0);

  return vector;
}
// vec vec_new(void) {
//   vec vector;
//   vec_init(&vector, 0);
//
//   return vector;
// }
vecptr vecptr_new(void) {
  vecptr vector;
  vecptr_init(&vector, 0);
  return vector;
}
vec2int vec2int_new(void) {
  vec2int vector;
  vec2int_init(&vector, 0);
  return vector;
}

gint degree(const igraph_t *G, gint v) {
  gint out;
  igraph_degree_1(G, &out, v, IGRAPH_ALL, false);

  return out;
}
