#pragma once

#include "boyer/graph.h"
#include "dynarray.h"
#include "graph_utils.h"
#include <stdbool.h>
#include <stdint.h>

typedef long long __edge_id;
typedef long long __vertex_id;

typedef struct {
  __edge_id start, current;
  __vertex_id v;
  graphP G;
} edge_iterator_t;

typedef struct {
  long long *array;
  size_t N;
  size_t cap;
  Arena *arena;
} intvector_t;

#define VECTOREND(vector) ((vector).array[(vector).N - 1])

void edge_iterator_init(edge_iterator_t *iterator, __vertex_id v, graphP G);
void edge_iterator_init_with_start(edge_iterator_t *iterator, __vertex_id v,
                                   __edge_id start, graphP G);

__edge_id edge_iterator_next(edge_iterator_t *iterator);

bool edge_iterator_has_next(edge_iterator_t *iterator);

void intvector_init(intvector_t *vec, Arena *a);

void intvector_push(intvector_t *vec, long long el);

long long intvector_popback(intvector_t *vec);

void intvector_clear(intvector_t *vec);

typedef struct bitset_t {
  uint64_t *bitset;
  size_t blocks;
} bitset_t;

void bitset_init(bitset_t *set, size_t N, Arena* arena);

void bitset_add(bitset_t *set, uint32_t val);

bool bitset_has(bitset_t *set, uint32_t val);

void bitset_remove(bitset_t *set, uint32_t val);
