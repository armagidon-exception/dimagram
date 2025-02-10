#include "cp.h"
#include "arena.h"
#include "boyer/graph.h"
#include "dynarray.h"
#include "graph_utils.h"
#include "igraph_components.h"
#include "igraph_conversion.h"
#include "igraph_datatype.h"
#include "igraph_interface.h"
#include "igraph_matrix.h"
#include "igraph_operators.h"
#include "igraph_vector.h"
#include "intqueue.h"
#include "log.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef igraph_matrix_t mat;

#define GINT(i) ((gint)i)
#define FACE(facelist, face) ((facelist).array[face])

typedef struct {
  gint prev, next;
  gint leftface;
  gint eid;
} face_edge_t;

DEFINE_VECTOR(face_edge_list, gint)
DEFINE_VECTOR(face_list, face_edge_list_t)

typedef struct {
  face_edge_t *edges;
  face_list_t faces;
} rhw_result_t;

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

typedef struct {
  int *N;
  int *A;
  int *F;
  bool *inGk;
  intqueue_t ready_queue;
  rhw_result_t rhw;
  const igraph_t *G;
} co_state;

#define AS_EMEDGE(em, edge) ((em).edges[(edge)])
#define AS_FACEEDGE(facelist, edge) ((facelist).edges[(edge)])
#define EDGE_TARGET(em, edge) ((em).edges[(edge)].neighbor)
#define EDGE_SOURCE(em, edge) ((em).edges[(edge) ^ 1].neighbor)

#define CO(i) co.array[i]
#define TNODE_NIL -1
#define NEXT(v) (Ls[v].right_child)

typedef struct Tnode {
  gint first_child;
  gint right_child;
  gint xOffset;
  gint y;
  gint parent;
} Tnode;

static hs_error_t canonical_ordering(igraph_t *graph, embedding_t *em,
                                     co_t *output);
static inline void rightHandWalk(const embedding_t *em, rhw_result_t *result,
                                 Arena *arena);
static void updateVertex(gint v, co_state *state, embedding_t *em);
static inline void findInterval(const embedding_t *em, const bool *inGk, gint v,
                                gint *start, gint *end);
static void rhw_result_init(Arena *arena, gint M, rhw_result_t *result);
static int init_embedding(igraph_t *g, embedding_t *em, Arena *arena);
static void updateFace(co_state *state, const embedding_t *em, gint fi);
static inline gint findExteriorEdge(const embedding_t *em);
static void printFace(face_list_t *faces, face_edge_t *edges, embedding_t *em,
                      gint fi);

static inline bool hasEdge(const igraph_t *G, gint v, gint w);
static void accumulateOffsets(gint v, gint d, mat *output, Tnode *Ls);
static inline gint findEdge(const embedding_t *em, gint source, gint dest);

static hs_error_t canonical_ordering(igraph_t *g, embedding_t *em,
                                     co_t *output) {
  co_state state;
  state.G = g;
  Arena co_arena = {0};
  Arena *arenaP = &co_arena;

  // Init embedding
  switch (init_embedding(g, em, arenaP)) {
  case 1:
    arena_free(arenaP);
    return HS_DISCONNECTED;
  case 2:
    arena_free(arenaP);
    return HS_NONPLANAR;
  }

  state.inGk = arena_calloc(arenaP, UCAST(em->N), sizeof(bool));

  // log_debug("right hand walk");
  rhw_result_init(arenaP, em->M, &state.rhw);

  rightHandWalk(em, &state.rhw, arenaP);

  // log_debug("Faces");
  // for (int i = 0; i < SCAST(state.rhw.faces.N); i++) {
  //   printf("%d ", i);
  //   printFace(&state.rhw.faces, state.rhw.edges, em, i);
  // }

  // Array of flags that indicate wether a vertex is in Gk to avoid looping over
  // the output array
  // Initialize A, N, F to 0
  state.A = arena_calloc(arenaP, state.rhw.faces.N, sizeof(int));
  state.N = arena_calloc(arenaP, UCAST(em->N), sizeof(int));
  state.F = arena_calloc(arenaP, UCAST(em->N), sizeof(int));

  gint extFaceEdge = findExteriorEdge(em);
  // Pick edge (v1, v2) on the boundary of exterior face of G
  gint v1 = EDGE_SOURCE(*em, extFaceEdge);
  gint v2 = EDGE_TARGET(*em, extFaceEdge);
  // Add its endpoints to output array
  co_push(output, v1);
  co_push(output, v2);
  state.inGk[v1] = true;
  state.inGk[v2] = true;
  state.N[v1]++;
  state.N[v2]++;

  intqueue_init(&state.ready_queue, arenaP, em->N);

  // Update their neighbours as in (1)
  for (int i = 0; i < SCAST(output->N); i++) {
    gint v = output->array[i];
    updateVertex(v, &state, em);
  }
  // log_debug("ready queue N=%ld", state.ready_queue.N);

  // Set A[f] to 1 of f, the left face of (v1, v2).
  gint fi = AS_FACEEDGE(state.rhw, extFaceEdge).leftface;
  state.A[fi] = 1;
  // log_debug("Face %ld is now in G_k", fi);
  // printFace(&state.rhw.faces, state.rhw.edges, em, fi);

  // If f is a triangle set F(v3) to 1 since it ready.
  gint v3 = EDGE_TARGET(*em, AS_FACEEDGE(state.rhw, extFaceEdge ^ 1).next);
  gint v3p = EDGE_SOURCE(*em, AS_FACEEDGE(state.rhw, extFaceEdge ^ 1).prev);
  // log_debug("v_3 = %ld, v'_3 = %ld", v3, v3p);
  if (v3 == v3p) {
    state.F[v3] = 1;
    intqueue_enqueue(&state.ready_queue, v3);
    // log_debug("f is a triangle. Push on the ready queue: N=%ld",
    //           state.ready_queue.N);
  }

  // log_debug("External face");
  // printFace(&state.rhw.faces, state.rhw.edges, em,
  //           state.rhw.edges[extFaceEdge ^ 1].leftface);

  // for (int i = 0; i < state.rhw.faces.N; i++) {
  //   printFace(&state.rhw.faces, state.rhw.edges, &em, i);
  //
  //   for (int j = 0; j < SCAST(FACE(state.rhw.faces, i).N); j++) {
  //     face_edge_t fe = state.rhw.edges[FACE(state.rhw.faces, i).array[j]];
  //     log_debug("face=%ld", fe.leftface);
  //     log_debug("edge=%ld", fe.eid);
  //   }
  // }

  // intqueue_print(&state.ready_queue);

  for (int k = 2; k < em->N; k++) {
    // log_debug("k=%ld", k);
    gint vk = intqueue_dequeue(&state.ready_queue);
    // log_debug("vk=%ld", vk);
    co_push(output, vk);
    state.inGk[vk] = true;

    updateVertex(vk, &state, em);

    gint start, end;
    findInterval(em, state.inGk, vk, &start, &end);
    assert(start >= 0 && end >= 0);
    // log_debug("Inverval found start=%ld, end=%ld", em->edges[start].neighbor,
    //           em->edges[end].neighbor);
    gint f1 = AS_FACEEDGE(state.rhw, start).leftface;
    gint fp1 = AS_FACEEDGE(state.rhw, end ^ 1).leftface;
    // log_debug("f_1 = %ld, f_p+1=%ld", f1, fp1);
    assert(f1 >= 0);
    assert(fp1 >= 0);
    state.A[f1]++;
    state.A[fp1]++;
    // log_debug("A[f_1]=%ld, A[f_p+1]=%ld", state.A[f1], state.A[fp1]);
    updateFace(&state, em, f1);
    updateFace(&state, em, fp1);
  }

  arena_free(arenaP);
  return HS_OK;
}

static void rightHandWalk(const embedding_t *em, rhw_result_t *result,
                          Arena *arena) {
  gint toVisit = em->M;
  intqueue_t queue;
  intqueue_init(&queue, arena, em->M);

  // Enqueue
  for (int eid = 0; eid < em->M; eid++) {
    intqueue_enqueue(&queue, eid);
    result->edges[eid].eid = eid;
    result->edges[eid].leftface = -1;
  }

  while (toVisit > 0) {
    gint start = intqueue_peek(&queue);
    face_edge_list_t face;
    face_edge_list_init(&face, arena);
    gint current = start;
    do {
      gint next = em->edges[current ^ 1].link[0];
      result->edges[next ^ 1].leftface =
          SCAST(result->faces.N); // Set left face.
      result->edges[next].prev = current;
      result->edges[current].next = next;

      face_edge_list_push(&face, current);

      // If edge is not visited, remove from visit queue
      toVisit--;

      assert(toVisit >= 0);
      intqueue_pull(&queue, next);
      current = next;
    } while (current != start);

    face_list_push(&result->faces, face);
  }
}

static void updateVertex(gint v, co_state *state, embedding_t *em) {
  // log_debug("==========UPDATE VERTEX==========");
  // log_debug("Vertex v=%ld is being updated.", v);
  gint current = em->vertices[v][1];
  do {
    em_edge edge = em->edges[current];
    gint w = edge.neighbor;
    // log_debug("Inspecting neighbour w=%ld", w);
    if (!state->inGk[w]) {
      // log_debug("w is not in the G_k yet.");
      state->N[w]++;
      // log_debug("w now has N[w] = %ld neighbours in G_k", state->N[w]);
      if (intqueue_get(&state->ready_queue, w)->present) {
        // log_debug("w was already in ready queue. Pulling from ready queue");
        intqueue_pull(&state->ready_queue, w);
      } else {
        // log_debug("w was not in the ready queue");
      }

      if (state->N[w] == 1) {
        // log_debug("w has a singular neighbour in G_k");
        gint left = AS_EMEDGE(*em, edge.link[0]).neighbor;
        gint right = AS_EMEDGE(*em, edge.link[1]).neighbor;
        // log_debug("w_i-1 = %ld, w_i+1=%ld", left, right);
        assert(left >= 0);
        assert(right >= 0);
        if (state->inGk[left] || state->inGk[right]) {
          // log_debug("One of them forms legal support. Pushing to ready
          // queue");
          intqueue_enqueue(&state->ready_queue, w);
        } else {
          // log_debug("w has no legal support.");
        }
      } else {
        // log_debug("N[w]=%ld", state->N[w]);
      }
    } else {
      // log_debug("w is in Gk");
    }

    current = edge.link[1];
  } while (current != em->vertices[v][1]);

  // log_debug("==========UPDATE VERTEX==========");
}

static void findInterval(const embedding_t *em, const bool *inGk, gint v,
                         gint *start, gint *end) {
  assert(v >= 0);
  // log_debug("Finding interval for %ld", v);

  gint extEdge = findExteriorEdge(em);
  *start = -1, *end = -1;

  gint current = em->edges[extEdge].link[1];
  do {
    bool flag = false;
    gint next = current;
    do {
      if (em->edges[next].neighbor == v) {
        *start = next;
        goto start_done;
      } else if (inGk[em->edges[next].neighbor]) {
        flag = true;
        break;
      }
      next = em->edges[next].link[1];
    } while (next != em->edges[current ^ 1].link[1]);
    assert(flag);

    current = em->edges[next ^ 1].link[1];
  } while (current != (extEdge ^ 1));
start_done:
  assert(*start >= 0);
  // log_debug("start=%ld", *start);

  current = em->edges[extEdge ^ 1].link[0];
  do {
    bool flag = false;
    gint next = current;
    do {
      if (em->edges[next].neighbor == v) {
        *end = next;
        goto end_done;
      } else if (inGk[em->edges[next].neighbor]) {
        flag = true;
        break;
      }
      next = em->edges[next].link[0];
    } while (next != em->edges[current ^ 1].link[0]);
    assert(flag);

    current = em->edges[next ^ 1].link[0];
  } while (current != (extEdge));
end_done:
  assert(*end >= 0);
  // log_debug("end=%ld", *end);
}

static void rhw_result_init(Arena *arena, gint M, rhw_result_t *result) {
  result->edges = arena_calloc(arena, UCAST(M), sizeof(face_edge_t));
  face_list_init(&result->faces, arena);
}

static int init_embedding(igraph_t *g, embedding_t *em, Arena *arena) {

  bool connected;
  igraph_is_connected(g, &connected, IGRAPH_WEAK);
  if (!connected) {
    return 1;
  }

  BM_graph *G = gp_New();
  gp_InitGraph(G, (int)igraph_vcount(g));
  for (gint i = 0; i < igraph_ecount(g); i++) {
    gint v, w;
    igraph_edge(g, i, &v, &w);
    gp_AddEdge(G, (int)v, 0, (int)w, 0);
  }
  if (gp_Embed(G, EMBEDFLAGS_PLANAR) || gp_SortVertices(G)) {
    gp_Free(&G);
    return 2;
  }

#define TRICONNECT 0

#if TRICONNECT == 1
  for (int vi = 0; vi < igraph_vcount(g); vi++) {
    gint edge1 = G->G[vi].link[0];
    do {
      edge1 = G->G[edge1].link[0];
      gint edge2 = G->G[edge1].link[0];
      int n1 = G->G[edge1].v;
      int n2 = G->G[edge2].v;

      if (!hasEdge(g, n1, n2)) {
        gp_AddEdge(G, n1, 0, n2, 0);
        igraph_add_edge(g, n1, n2);

        log_debug("Augmented edge between %d and %d", n1, n2);
      }
    } while (edge1 != G->G[vi].link[0]);
  }

  gp_ReinitializeGraph(G);

  gp_Free(&G);
  G = gp_New();
  gp_InitGraph(G, (int)igraph_vcount(g));
  for (gint i = 0; i < igraph_ecount(g); i++) {
    gint v, w;
    igraph_edge(g, i, &v, &w);
    gp_AddEdge(G, (int)v, 0, (int)w, 0);
  }
  if (gp_Embed(G, EMBEDFLAGS_PLANAR) || gp_SortVertices(G)) {
    gp_Free(&G);
    return 2;
  }
#endif

#if TRICONNECT == 0
  bool biconnected;
  igraph_is_biconnected(g, &biconnected);
  if (!biconnected) {
    veci bicomN;
    igraph_vector_int_init(&bicomN, igraph_vcount(g));
    veci cutvertices = veci_new();
    vec2int comps = vec2int_new();

    igraph_biconnected_components(g, 0, 0, 0, &comps, &cutvertices);
    for (int comp = 0; comp < igraph_vector_int_list_size(&comps); comp++) {
      veci *component = igraph_vector_int_list_get_ptr(&comps, comp);
      for (int vi = 0; vi < vector_size(component); vi++) {
        vecget(bicomN, vecget(*component, vi)) = comp;
      }
    }

    for (int vi = 0; vi < vector_size(&cutvertices); vi++) {

      gint edge1 = G->G[vecget(cutvertices, vi)].link[0];

      do {
        edge1 = G->G[edge1].link[0];
        gint edge2 = G->G[edge1].link[0];
        int n1 = G->G[edge1].v;
        int n2 = G->G[edge2].v;

        if (vecget(bicomN, n1) != vecget(bicomN, n2)) {
          if (!hasEdge(g, n1, n2)) {
            gp_AddEdge(G, n1, 0, n2, 0);
            igraph_add_edge(g, n1, n2);
            log_debug("Augmented edge between %d and %d", n1, n2);
          }
          // gp_AddEdge(G, (int)n1, 0, (int)n2, 0);
        }
      } while (edge1 != G->G[vecget(cutvertices, vi)].link[0]);
    }

    gp_ReinitializeGraph(G);

    veci_dest(&bicomN);
    veci_dest(&cutvertices);
    vec2int_dest(&comps);

    gp_Free(&G);
    G = gp_New();
    gp_InitGraph(G, (int)igraph_vcount(g));
    for (gint i = 0; i < igraph_ecount(g); i++) {
      gint v, w;
      igraph_edge(g, i, &v, &w);
      gp_AddEdge(G, (int)v, 0, (int)w, 0);
    }
    if (gp_Embed(G, EMBEDFLAGS_PLANAR) || gp_SortVertices(G)) {
      gp_Free(&G);
      return 2;
    }
  }
#endif

  em->N = igraph_vcount(g);
  em->M = 2 * igraph_ecount(g);
  em->edges = arena_calloc(arena, UCAST(em->M), sizeof(em_edge));
  em->vertices = arena_calloc(arena, UCAST(em->N), sizeof(em_vertex));
  em->degrees = arena_calloc(arena, UCAST(em->N), sizeof(gint));

  // gp_Write(G, "stdout", WRITE_ADJLIST);

  em->M = 2 * G->M;
  gint C = 2 * em->N;

  for (int v = 0; v < em->N; v++) {
    em->vertices[v][0] = GINT(G->G[v].link[0]) - C;
    em->vertices[v][1] = GINT(G->G[v].link[1]) - C;
    em->degrees[v] = degree(g, v);

    G->G[G->G[v].link[0]].link[1] = G->G[v].link[1];
    G->G[G->G[v].link[1]].link[0] = G->G[v].link[0];
  }

  for (int eid = 0; eid < em->M; eid++) {
    graphNode edge = G->G[eid + C];
    int link0 = edge.link[0], link1 = edge.link[1];
    em->edges[eid] = (em_edge){{link0 - C, link1 - C}, edge.v};
  }

  // gint edge1 = em->vertices[0][0];
  // gint edge2 = em->vertices[0][1];

  // log_debug("n1=%ld, n2=%ld", em->edges[edge1].neighbor,
  //           em->edges[edge2].neighbor);

  gp_Free(&G);
  return 0;
}

#define FACE(facelist, face) ((facelist).array[face])

static void updateFace(co_state *state, const embedding_t *em, gint fi) {
  // log_debug("==========UPDATE FACE==========");
  // log_debug("face=%ld", fi);
  face_edge_list_t *face = &FACE(state->rhw.faces, fi);
  gint edge_count = SCAST(face->N);
  // log_debug("edge_count=%ld", edge_count);
  // log_debug("A[f]=%ld", state->A[fi]);
  if (state->A[fi] == edge_count - 2) {
    // log_debug("face is ready");
    for (int i = 0; i < edge_count; i++) {
      gint v = EDGE_TARGET(*em, face->array[i]);
      // log_debug("Vertex on the face %ld", v);
      if (!state->inGk[v]) {
        // log_debug("Found vertex not in G_k");
        state->F[v]++;
        // log_debug("Vertex now has %ld ready faces", state->F[v]);
        if (state->N[v] == state->F[v] + 1) {
          // log_debug("All faces are ready");
          intqueue_enqueue(&state->ready_queue, v);
        }
        break;
      }
    }
  } else {
    // log_debug("Face %ld is not ready", fi);
  }
  // log_debug("==========UPDATE FACE==========");
}

static void accumulateOffsets(gint v, gint d, mat *output, Tnode *Ls) {
  if (v != TNODE_NIL) {
    Ls[v].xOffset += d;
    MATRIX(*output, v, 0) = (double)Ls[v].xOffset;
    MATRIX(*output, v, 1) = (double)Ls[v].y;
    accumulateOffsets(Ls[v].first_child, Ls[v].xOffset, output, Ls);
    accumulateOffsets(Ls[v].right_child, Ls[v].xOffset, output, Ls);
  }
}

static inline gint findExteriorEdge(const embedding_t *em) {
  gint edge1 = em->vertices[0][0];
  return edge1;
}

static void printFace(face_list_t *faces, face_edge_t *edges, embedding_t *em,
                      gint fi) {
  for (int i = 0; i < SCAST(FACE(*faces, fi).N); i++) {
    face_edge_t fe = edges[FACE(*faces, fi).array[i]];
    gint v = EDGE_TARGET(*em, fe.eid);
    printf("%ld -> ", v);
  }
  printf("\n");
}

static inline gint findEdge(const embedding_t *em, gint v, gint w) {
  gint edge = em->vertices[v][1];
  do {
    edge = em->edges[edge].link[1];
    gint n = em->edges[edge].neighbor;
    if (n == w) {
      return edge;
    }
  } while (edge != em->vertices[v][1]);

  return -1;
}

static bool hasEdge(const igraph_t *G, gint v, gint w) {
  gint eid;
  igraph_get_eid(G, &eid, v, w, IGRAPH_DIRECTED, false);
  return eid >= 0;
}

#ifndef HS_TEST_ENV
hs_error_t harel_sardes_layout(igraph_t *G, igraph_matrix_t *output) {
  switch (igraph_vcount(G)) {
  case 0:
    return HS_EMPTY_GRAPH;
  case 1:
    MATRIX(*output, 0, 0) = 0;
    MATRIX(*output, 0, 1) = 0;
    return HS_OK;
  case 2:
    MATRIX(*output, 0, 0) = 0;
    MATRIX(*output, 0, 1) = 0;
    MATRIX(*output, 1, 0) = 1;
    MATRIX(*output, 1, 1) = 0;
    return HS_OK;
  default:
    break;
  }

  co_t co;
  Arena a = {0};
  co_init(&co, &a);

  embedding_t em;
  hs_error_t err;
  if ((err = canonical_ordering(G, &em, &co)) != HS_OK) {
    return err;
  }
  log_info("Canonical ordering calculated");

  for (int i = 0; i < SCAST(co.N); i++) {
    log_debug("%d: %d", i, co.array[i]);
  }

  gint vcount = g_vcount(G);
  assert(vcount >= 3 && "Number of vertices must be greater than 2.");

  if (output->ncol < 2) {
    if (igraph_matrix_add_cols(output, 2 - output->ncol))
      return HS_INVALID_INPUT;
  }
  if (output->nrow < vcount) {
    if (igraph_matrix_add_rows(output, vcount - output->nrow))
      return HS_INVALID_INPUT;
  }

  Tnode *Ls = ucalloc((size_t)vcount, sizeof(Tnode));
  for (gint i = 0; i < vcount; i++) {
    Ls[i].first_child = TNODE_NIL;
    Ls[i].right_child = TNODE_NIL;
  }
  Ls[CO(0)] = (Tnode){TNODE_NIL, CO(2), 0, 0, -1};
  Ls[CO(1)] = (Tnode){TNODE_NIL, TNODE_NIL, 1, 0, CO(2)};
  Ls[CO(2)] = (Tnode){TNODE_NIL, CO(1), 1, 1, CO(0)};

  igraph_matrix_t adjmat;
  igraph_matrix_init(&adjmat, 0, 0);
  igraph_get_adjacency(G, &adjmat, IGRAPH_GET_ADJACENCY_BOTH, 0,
                       IGRAPH_NO_LOOPS);

  bool *inGk = arena_calloc(&a, UCAST(vcount), sizeof(bool));
  inGk[CO(0)] = true;
  inGk[CO(1)] = true;
  inGk[CO(2)] = true;

  for (int k = 3; k < vcount; k++) {
    gint vk = CO(k);

    // Aquire neighbours on the contour
    gint start_interval, end_interval;
    findInterval(&em, inGk, vk, &start_interval, &end_interval);
    start_interval ^= 1;
    end_interval ^= 1;

    gint p = em.edges[start_interval].neighbor;
    gint q = em.edges[end_interval].neighbor;
    gint prev_q = Ls[q].parent;
    // gint prev_q = em.edges[em.edges[end_interval ^ 1].link[0]].neighbor;

    if (p == q) {
      if (p == CO(0)) {
        q = NEXT(p);
        prev_q = p;
      } else if (p == CO(1)) {
        gint temp = p;
        p = prev_q;
        q = temp;
      } else {
        em_edge edge = em.edges[start_interval ^ 1];
        gint left = AS_EMEDGE(em, edge.link[0]).neighbor;
        gint right = AS_EMEDGE(em, edge.link[1]).neighbor;
        if (left == prev_q) {
          gint temp = p;
          p = prev_q;
          q = temp;
        } else if (right == NEXT(p)) {
          q = NEXT(p);
          prev_q = p;
        } else if (inGk[left]){
          gint temp = p;
          p = prev_q;
          q = temp;
        } else if (inGk[right]){
          q = NEXT(p);
          prev_q = p;
        } else {
          log_error(
              "Error at embedding legal support: vertex has no legal support");
          log_error("vk=%ld", vk);
          log_error("left=%d,right;=%ld", left, right);
          log_error("prev=%ld,w_p=%ld,next=%ld", prev_q,p, NEXT(p));
          abort();
        }
      }
    }

    inGk[vk] = true;
    Ls[NEXT(p)].xOffset++;
    Ls[q].xOffset++;

    gint dwpq = 0;
    for (gint i = NEXT(p); i != NEXT(q); i = NEXT(i))
      dwpq += Ls[i].xOffset;

    Ls[vk].xOffset = (-Ls[p].y + dwpq + Ls[q].y) >> 1;
    Ls[vk].y = (Ls[p].y + dwpq + Ls[q].y) >> 1;
    Ls[q].xOffset = dwpq - Ls[vk].xOffset;

    if (NEXT(p) != q) {
      Ls[NEXT(p)].xOffset -= Ls[vk].xOffset;
    }

    gint pp = NEXT(p);
    Ls[p].right_child = vk;
    Ls[vk].parent = p;
    Ls[vk].right_child = q;
    Ls[q].parent = vk;

    if (pp != q) {
      Ls[vk].first_child = pp;
      Ls[prev_q].right_child = TNODE_NIL;
    } else {
      Ls[vk].first_child = TNODE_NIL;
    }
  }

  accumulateOffsets(CO(0), 0, output, Ls);

  igraph_matrix_destroy(&adjmat);
  ufree(Ls);

  arena_free(&a);
  return HS_OK;
}

#endif
