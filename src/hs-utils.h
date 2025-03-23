#pragma once

#include "boyer/graph.h"
#include "graph_utils.h"
#include "igraph_datatype.h"
#include <stdbool.h>

void hs_copy_from_igraph(const igraph_t *g, graphP G);
int hs_graph_biconnect(igraph_t *g, graphP G);
