#pragma once
#include "arena.h"

#define DEFINE_VECTOR(DYN_VECTOR_NAME, DYN_VECTOR_TYPE)                        \
  typedef struct {                                                             \
    DYN_VECTOR_TYPE *array;                                                    \
    size_t N;                                                                  \
    size_t cap;                                                                \
    Arena *arena;                                                              \
  } DYN_VECTOR_NAME##_t;                                                       \
                                                                               \
  static inline void DYN_VECTOR_NAME##_init(DYN_VECTOR_NAME##_t *vec,          \
                                            Arena *a) {                        \
    vec->arena = a;                                                            \
    vec->cap = 1;                                                              \
    vec->N = 0;                                                                \
    vec->array = 0;                                                            \
  }                                                                            \
                                                                               \
  static inline void DYN_VECTOR_NAME##_push(DYN_VECTOR_NAME##_t *vec,          \
                                            DYN_VECTOR_TYPE el) {              \
    if (vec->array) {                                                          \
      if (vec->N >= vec->cap) {                                                \
        size_t oldsz = vec->cap * sizeof(DYN_VECTOR_TYPE);                     \
        vec->array = arena_realloc(vec->arena, vec->array, oldsz, oldsz * 2);  \
        vec->cap *= 2;                                                         \
      }                                                                        \
    } else {                                                                   \
      vec->array = arena_calloc(vec->arena, 1, sizeof(DYN_VECTOR_TYPE));       \
      vec->cap = 1;                                                            \
      vec->N = 0;                                                              \
    }                                                                          \
    vec->array[vec->N++] = el;                                                 \
  }                                                                            \
                                                                               \
  static inline DYN_VECTOR_TYPE DYN_VECTOR_NAME##_popback(                     \
      DYN_VECTOR_NAME##_t *vec) {                                              \
    assert(vec->N > 0 && "Cannot pop from empty array");                       \
    return vec->array[--vec->N];                                               \
  }                                                                            \
                                                                               \
  static inline void DYN_VECTOR_NAME##_clear(DYN_VECTOR_NAME##_t *vec) {       \
    if (vec)                                                                   \
      vec->N = 0;                                                              \
  }
