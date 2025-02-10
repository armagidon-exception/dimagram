#pragma once
#include "arena.h"
#include "igraph_datatype.h"
#include "igraph_interface.h"
#include "igraph_stack.h"
#include "igraph_types.h"
#include "igraph_vector_list.h"
#include "igraph_vector_ptr.h"

typedef igraph_integer_t gint;
typedef igraph_real_t greal;
typedef igraph_vector_int_list_t vec2int;
typedef igraph_vector_int_t veci;
// typedef igraph_vector_t vec;
typedef igraph_vector_ptr_t vecptr;
typedef igraph_t graph;
typedef igraph_stack_int_t istack;

#define vector_size(v) igraph_vector_size((igraph_vector_t *)(v))
#define veci_size(v) ((v).end - (v).stor_begin)
#define veclist_size igraph_vector_list_size
#define vecilist_size igraph_vector_int_list_size

// #define vec_dest igraph_vector_destroy
// #define vec_init igraph_vector_init
// #define vec_push igraph_vector_push_back
// #define vec_pop igraph_vector_pop_back

#define veci_dest igraph_vector_int_destroy
#define veci_init igraph_vector_int_init
#define veci_push igraph_vector_int_push_back
#define veci_pop igraph_vector_int_pop_back
#define veci_insert igraph_vector_int_insert
#define veci_prepend(v, n) veci_insert(v, 0, n)

#define vecptr_dest igraph_vector_ptr_destroy
#define vecptr_init igraph_vector_ptr_init
#define vecptr_push igraph_vector_ptr_push_back
#define vecptr_pop igraph_vector_ptr_pop_back

#define vec2int_dest igraph_vector_int_list_destroy
#define vec2int_init igraph_vector_int_list_init
#define vec2int_push igraph_vector_int_list_push_back
#define vec2int_pop igraph_vector_int_list_pop_back

#define vec2iget igraph_vector_int_list_get_ptr
#define vecget(v, i) VECTOR(v)[i]

#define g_vcount(G) igraph_vcount(G)


veci getneigs(const graph *G, gint v);
veci veci_new(void);
// vec vec_new(void);
vecptr vecptr_new(void);
vec2int vec2int_new(void);

gint degree(const igraph_t *G, gint v);

