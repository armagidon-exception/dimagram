#include "boyer/graph.h"
#include "dynarray.h"
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

  graphP G = gp_New();
  gp_InitGraph(G, 4);
  gp_AddEdge(G, 0, 0, 1, 0);
  gp_AddEdge(G, 1, 0, 2, 0);
  gp_AddEdge(G, 2, 0, 3, 0);
  gp_AddEdge(G, 3, 0, 0, 0);

  int status = 0;
  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, OK);
  status = gp_SortVertices(G);
  assert_eq(status, OK);

  assert_eq(G->G[G->G[0].link[1]].v, 1);
  assert_eq(G->G[G->G[0].link[0]].v, 3);
  assert_eq(G->G[G->G[2].link[1]].v, 3);
  assert_eq(G->G[G->G[2].link[0]].v, 1);

  status = gp_InsertArc(G, 0, 2, G->G[0].link[1], G->G[2].link[0], 1, 0);
  assert_eq(status, OK);
  assert_eq(G->G[G->G[G->G[0].link[1]].link[1]].v, 2);
  assert_eq(G->G[G->G[G->G[2].link[0]].link[0]].v, 0);

  arena_free(&a);
}

static void iterator_tester_single(void) {
  Arena a = {0};

  graphP G = gp_New();
  gp_InitGraph(G, 3);
  gp_AddEdge(G, 0, 0, 1, 0);
  gp_AddEdge(G, 1, 0, 2, 0);
  int status = 0;
  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, OK);
  status = gp_SortVertices(G);
  assert_eq(status, OK);

  assert_eq(G->G[0].link[0], G->G[0].link[1]);

  assert_eq(G->G[2].link[0], G->G[2].link[1]);
  status = gp_InsertArc(G, 0, 2, G->G[0].link[0], G->G[2].link[0], 1, 0);
  assert_eq(status, OK);
  assert_eq(G->M, 3);
  assert_eq(G->G[G->G[0].link[1]].v, 1);
  assert_eq(G->G[G->G[0].link[0]].v, 2);
  assert_eq(G->G[G->G[2].link[1]].v, 0);
  assert_eq(G->G[G->G[2].link[0]].v, 1);

  arena_free(&a);
}

int main(void) { run_tests(); }
