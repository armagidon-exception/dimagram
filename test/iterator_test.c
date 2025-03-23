#include "boyer/graph.h"
#include "collections.h"
#include "dynarray.h"
#include "graph_utils.h"
#include "tester.h"
#include <unistd.h>

static void iterator_tester_multiple(void);
static void iterator_tester_single(void);

Test(test_iterator_multiple) {
  (void)state;
  iterator_tester_multiple();
}
Test(test_iterator_single) {
  (void)state;
  iterator_tester_single();
}

static void iterator_tester_multiple(void) {
  Arena a = {0};
  intvector_t vertices;
  intvector_init(&vertices, &a);

  graphP G = gp_New();
  gp_InitGraph(G, 6);
  gp_AddEdge(G, 0, 0, 1, 0);
  gp_AddEdge(G, 0, 0, 2, 0);
  gp_AddEdge(G, 0, 0, 3, 0);
  gp_AddEdge(G, 0, 0, 4, 0);
  gp_AddEdge(G, 0, 0, 5, 0);
  int status = 0;
  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, OK);
  status = gp_SortVertices(G);
  assert_eq(status, OK);

  edge_iterator_t iterator;
  edge_iterator_init(&iterator, 0, G);
  while (edge_iterator_has_next(&iterator)) {
    gint current = edge_iterator_next(&iterator);
    int v = G->G[current].v;
    intvector_push(&vertices, v);
  }

  assert_eq(vertices.N, (unsigned long) 5);

  for (int i = 0; i < 5; i++) {
    assert_eq(vertices.array[i], (long long) i + 1);
  }

  arena_free(&a);
}

static void iterator_tester_single(void) {
  Arena a = {0};
  intvector_t vertices;
  intvector_init(&vertices, &a);

  graphP G = gp_New();
  gp_InitGraph(G, 2);
  gp_AddEdge(G, 0, 0, 1, 0);
  int status = 0;
  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, OK);
  status = gp_SortVertices(G);
  assert_eq(status, OK);

  edge_iterator_t iterator;
  edge_iterator_init(&iterator, 0, G);
  while (edge_iterator_has_next(&iterator)) {
    gint current = edge_iterator_next(&iterator);
    int v = G->G[current].v;
    intvector_push(&vertices, v);
  }

  assert_eq(vertices.N, 1);
  assert_eq(vertices.array[0], 1);

  arena_free(&a);
}

int main(void) { run_tests(); }
