#pragma once
#include "graph_utils.h"

#include "igraph_datatype.h"

typedef struct {
  gint link[2];
  gint neighbor;
} em_edge;

typedef struct {
  gint link[2];
} em_vertex;

typedef struct {
  em_edge *edges;
  gint *degrees;
  gint (*vertices)[2];
  gint N;
  gint M;
} embedding_t;

// Simple implentation of boyer-myrvold planar testing algorithm for embedding
// planar graphs. This implentation does not allow disconnected graphs as input
// TODO (maybe): allow to take disconnected graphs as input
// TODO: actually write it myself...
// const igraph_t* g: pointer to graph g to be embedded
// vec2int* embedding: vector list that contains vectercies in clockwise
// circular order of the embedding
int bm04_planar_embedding(const igraph_t *g, vec2int *embedding);

int bm04_embed_graph(const igraph_t *g, embedding_t *embedding);

void bm04_embedding_destroy(embedding_t* embedding);
