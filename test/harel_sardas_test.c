#include "boyer/graph.h"
#include "graph_utils.h"
#include "hs.h"
#include "igraph_components.h"
#include "igraph_operators.h"
#include "log.h"
#include "tester.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SAMPLE_SIZE_LIMIT 500

#define HS_TEST_ENV
#include "hs.c"

typedef struct {
  const igraph_t *g;
} graph_test_data;

// typedef void (*graph_tester_func)(const igraph_t *g, int sampleNumber);
static void read_graph(const char *dataset, int logger_fd, tester_func func);
static void biconnector_tester(testcase_state_t *state, void *data,
                               size_t datalen);
static void embedder_tester(testcase_state_t *state, void *data,
                            size_t datalen);
static void canonical_ordering_tester(testcase_state_t *state, void *data,
                                      size_t datalen);

Test(test_biconnector) {
  read_graph("graphs.txt", state->write_pipe, biconnector_tester);
}

Test(test_embedder) {
  read_graph("graphs.txt", state->write_pipe, embedder_tester);
}

Test(test_canonical_ordering) {
  read_graph("graphs.txt", state->write_pipe, canonical_ordering_tester);
}

int main(void) {
  run_tests();
  return 0;
}

static void read_graph(const char *dataset, int logger_fd, tester_func func) {
  FILE *f = fopen(dataset, "r");
  setvbuf(f, NULL, _IONBF, 0);
  assert(f != 0);

  int status;
  int S;

  status = fscanf(f, "%d", &S);

  assert(status == 1);
  char *buf = calloc(BUFSIZ, sizeof(char));
  size_t ll = BUFSIZ;
  for (int i = 0; i < MIN(S, SAMPLE_SIZE_LIMIT); i++) {
    igraph_t g;
    int N = -1;
    status = fscanf(f, "%d", &N);
    assert_eq(N, 8);
    assert_eq(status, 1);
    igraph_empty(&g, N, false);

    for (int j = 0; j < N; j++) {
      long read = getdelim(&buf, &ll, ';', f);
      buf[--read] = '\0';
      int v, off = 0, nc;
      while (sscanf(buf + off, "%d%n", &v, &nc) > 0) {
        igraph_add_edge(&g, v, j);
        off += nc;
      }
    }
    igraph_simplify(&g, 1, 1, 0);

    // func(0, &g, sizeof(g));
    char test_name[BUFSIZ] = {0};
    sprintf(test_name, "%d", i);
    testcase_t c = {test_name, func, 0, 0, logger_fd, false};
    run_test(&c, &g, sizeof(g));
  }
  fclose(f);
  free(buf);
}

static void biconnector_tester(testcase_state_t *state, void *data,
                               size_t datalen) {
  const igraph_t *g = (igraph_t *)data;
  Arena a = {0};
  bool biconnected;
  igraph_is_biconnected(g, &biconnected);

  graphP G = gp_New();
  gp_InitGraph(G, (int)igraph_vcount(g));
  hs_copy_from_igraph(g, G);
  int status;
  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, 0);
  status = gp_SortVertices(G);
  assert_eq(status, 0);

  gp_Write(G, "stdout", WRITE_ADJLIST);
  if (!biconnected) {
    log_debug("biconnected");
    status = hs_graph_biconnect(g, G);
    assert_eq(status, 0);
  }

  gp_ReinitializeGraph(G);
  hs_copy_from_igraph(g, G);

  gp_Write(G, "stdout", WRITE_ADJLIST);

  for (int i = 0; i < G->N; i++) {
    int j = G->G[i].link[1];
    bool *array = arena_calloc(&a, G->N, sizeof(bool));
    while (j >= G->N) {
      // fprintf(Outfile, " %d", theGraph->G[j].v);
      assert_eq(array[G->G[j].v], false);
      array[G->G[j].v] = true;
      j = G->G[j].link[1];
    }
  }

  status = gp_Embed(G, EMBEDFLAGS_PLANAR);
  assert_eq(status, 0);
  status = gp_SortVertices(G);
  assert_eq(status, 0);

  igraph_is_biconnected(g, &biconnected);
  assert_eq(biconnected, true);

  bool connected = false;
  igraph_is_connected(g, &connected, 0);
  assert_eq(connected, true);

  igraph_destroy(g);
  gp_Free(&G);
  arena_free(&a);
}

static void embedder_tester(testcase_state_t *state, void *data,
                            size_t datalen) {

  igraph_t *g = (igraph_t *)data;
  Arena arena = {0};
  int status;
  igraph_t icopy;
  igraph_copy(&icopy, g);

  bool biconnected;
  igraph_is_biconnected(&icopy, &biconnected);
  if (biconnected)
    return;

  embedding_t em;
  status = init_embedding(&icopy, &em, &arena);
  assert_eq(status, 0);

  arena_free(&arena);
  igraph_destroy(&icopy);
}

static void canonical_ordering_tester(testcase_state_t *state, void *data,
                                      size_t datalen) {
  igraph_t *g = (igraph_t *)data;
  Arena arena = {0};
  igraph_t icopy;
  igraph_copy(&icopy, g);

  graphP G = gp_New();
  gp_InitGraph(G, (int)igraph_vcount(g));
  hs_copy_from_igraph(g, G);
  int status;

  co_t co;
  co_init(&co, &arena);
  embedding_t em;
  status = (int)canonical_ordering(&icopy, &em, &co);
  if (status == HS_NONPLANAR)
    log_debug("Graph is non planar");

  assert_eq(status, HS_OK);
  arena_free(&arena);
  igraph_destroy(&icopy);
}
