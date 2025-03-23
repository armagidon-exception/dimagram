#include "collections.h"

void intvector_init(intvector_t *vec, Arena *a) {
  vec->arena = a;
  vec->cap = 1;
  vec->N = 0;
  vec->array = 0;
}

void intvector_push(intvector_t *vec, long long el) {
  if (vec->array) {
    if (vec->N >= vec->cap) {
      size_t oldsz = vec->cap * sizeof(long long);
      vec->array = arena_realloc(vec->arena, vec->array, oldsz, oldsz * 2);
      vec->cap *= 2;
    }
  } else {
    vec->array = arena_calloc(vec->arena, 1, sizeof(long long));
    vec->cap = 1;
    vec->N = 0;
  }
  vec->array[vec->N++] = el;
}

long long intvector_popback(intvector_t *vec) {
  assert(vec->N > 0 && "Cannot pop from empty array");
  return vec->array[--vec->N];
}

void intvector_clear(intvector_t *vec) {
  if (vec)
    vec->N = 0;
}

void edge_iterator_init(edge_iterator_t *iterator, __vertex_id v, graphP G) {
  *iterator = (edge_iterator_t){G->G[v].link[1], -1, v, G};
}

void edge_iterator_init_with_start(edge_iterator_t *iterator, __vertex_id v,
                                   __edge_id start, graphP G) {
  assert(G->G[start ^ 1].v == v);
  *iterator = (edge_iterator_t){start, -1, v, G};
}

bool edge_iterator_has_next(edge_iterator_t *iterator) {
  if (iterator->current == -1)
    return true;
  __edge_id next = iterator->G->G[iterator->current].link[1];
  if (next < iterator->G->N) {
    next = iterator->G->G[iterator->v].link[1];
  }
  return next != iterator->start;
  // if (iterator->G->G[iterator->current].link[1] == iterator)
}

__edge_id edge_iterator_next(edge_iterator_t *iterator) {
  if (!edge_iterator_has_next(iterator))
    return -1;

  iterator->current = iterator->current == -1
                          ? iterator->start
                          : iterator->G->G[iterator->current].link[1];

  if (iterator->current < iterator->G->N) {
    iterator->current = iterator->G->G[iterator->v].link[1];
  }
  return iterator->current;
}

void bitset_init(bitset_t *set, size_t N, Arena *arena) {
  size_t blocks = (N + sizeof(uint64_t) * 8 - 1) / sizeof(uint64_t) * 8;
  set->blocks = blocks;
  set->bitset = arena_calloc(arena, blocks, sizeof(uint64_t));
}

void bitset_add(bitset_t *set, uint32_t val) {
  int block = val / (sizeof(uint64_t) * 8);
  int inblocki = val % (sizeof(uint64_t) * 8);
  uint64_t mask = 1 << inblocki;
  if (set->bitset[block] & mask)
    return;
  set->bitset[block] |= (1 << inblocki);
}

bool bitset_has(bitset_t *set, uint32_t val) {
  int block = val / (sizeof(uint64_t) * 8);
  int inblocki = val % (sizeof(uint64_t) * 8);
  uint64_t mask = 1 << inblocki;
  return (set->bitset[block] & mask) != 0;
}

void bitset_remove(bitset_t *set, uint32_t val) {
  int block = val / (sizeof(uint64_t) * 8);
  int inblocki = val % (sizeof(uint64_t) * 8);
  uint64_t mask = 1 << inblocki;

  set->bitset[block] &= ~mask;
}
