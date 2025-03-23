#include "hs-utils.h"
#include "appconst.h"
#include "collections.h"
#include "dynarray.h"
#include "graph_utils.h"
#include "igraph_interface.h"
#include "igraph_operators.h"
#include "log.h"
#include "utils.h"
#include <assert.h>

DEFINE_VECTOR(componentlist, bitset_t)

typedef struct {
  gint dfi;
  gint lowpoint;
  gint componentId;
  gint children;
  bool cut;
} vinfo_t;

typedef struct {
  vinfo_t *infos;
  graphP G;
  gint dfiC;
  intvector_t edgeStack;
  componentlist_t components;
  Arena *arena;
} state_t;

#define Gv(g, i) ((g)->G[i].v)

static void hs_graph_getcomps(state_t *state, gint v, gint u, int depth);

int hs_graph_biconnect(igraph_t *g, graphP G) {
  Arena a = {0};
  gint N = igraph_vcount(g);
  int status = 0;

  state_t s = {arena_calloc(&a, N, sizeof(vinfo_t)), G, 0, {0}, {0}, &a};
  componentlist_init(&s.components, &a);

  for (gint v = 0; v < N; v++) {
    s.infos[v] = (vinfo_t){-1, N, -1, 0, false};
  }
  intvector_init(&s.edgeStack, &a);
  gint *dfiorder = arena_calloc(&a, N, sizeof(gint));
  hs_graph_getcomps(&s, 0, -1, 1);

  bitset_t component;
  bitset_init(&component, UCAST(s.G->N), s.arena);
  if (s.edgeStack.N) {

    while (s.edgeStack.N) {
      gint compEdge = intvector_popback(&s.edgeStack);
      gint cv = Gv(s.G, compEdge);

      s.infos[cv].componentId = SCAST(s.components.N);
      bitset_add(&component, UCAST(cv));
      bitset_add(&component, UCAST(Gv(G, compEdge ^ 1)));
    }
    gint compEdge = intvector_popback(&s.edgeStack);
    gint cv = Gv(s.G, compEdge);
    bitset_add(&component, UCAST(cv));
    bitset_add(&component, UCAST(Gv(G, compEdge ^ 1)));
    s.infos[cv].componentId = SCAST(s.components.N);
  }
  s.infos[0].componentId = SCAST(s.components.N);
  componentlist_push(&s.components, component);

  for (gint v = 0; v < N; v++) {
    dfiorder[s.infos[v].dfi] = v;
  }

  for (gint dfi = 0; dfi < N; dfi++) {
    gint v = dfiorder[dfi];
    if (s.infos[v].cut) {
      edge_iterator_t iterator;
      edge_iterator_init(&iterator, v, G);
      gint startedge = -1;
      while (edge_iterator_has_next(&iterator)) {
        gint edge = edge_iterator_next(&iterator);
        gint u = Gv(G, edge);
        gint edge2 = G->G[edge].link[1];
        if (edge2 < N)
          edge2 = G->G[v].link[1];
        gint w = Gv(G, edge2);

        bitset_t *comp1 = &s.components.array[s.infos[u].componentId];
        if (!bitset_has(comp1, w)) {
          startedge = edge2;
          break;
        }
      }
      edge_iterator_init_with_start(&iterator, v, startedge, G);
      while (edge_iterator_has_next(&iterator)) {
        gint edge = edge_iterator_next(&iterator);
        gint u = Gv(G, edge);
        gint edge2 = G->G[edge].link[1];
        if (edge2 < N)
          edge2 = G->G[v].link[1];
        if (edge2 == startedge)
          break;
        gint w = Gv(G, edge2);
        bitset_t *comp1 = &s.components.array[s.infos[u].componentId];
        bitset_t *comp2 = &s.components.array[s.infos[w].componentId];

        if (!bitset_has(comp1, w)) {
          status = gp_InsertArc(G, u, w, edge ^ 1, edge2 ^ 1, 0, 1);
          bitset_add(comp1, w);
          bitset_add(comp2, u);
          igraph_add_edge(g, u, w);
        }
      }
    }
  }
  igraph_simplify(g, true, true, 0);

  arena_free(&a);
  return status;
}

void hs_copy_from_igraph(const igraph_t *g, graphP G) {
  for (gint i = 0; i < igraph_ecount(g); i++) {
    gint v, w;
    igraph_edge(g, i, &v, &w);
    gp_AddEdge(G, (int)v, 0, (int)w, 0);
  }
}

static void hs_graph_getcomps(state_t *state, gint v, gint u, int depth) {
  if (depth > MAX_RECURSION_DEPTH) {
    log_error("Max recursion level reached %d", MAX_RECURSION_DEPTH);
    abort();
  }
  state->infos[v].dfi = state->dfiC++;
  state->infos[v].lowpoint = state->infos[v].dfi;

  edge_iterator_t iterator;
  edge_iterator_init(&iterator, v, state->G);

  while (edge_iterator_has_next(&iterator)) {
    gint edge = edge_iterator_next(&iterator);
    gint w = state->G->G[edge].v;

    if (state->infos[w].dfi < 0) {
      state->infos[v].children++;
      intvector_push(&state->edgeStack, edge);
      hs_graph_getcomps(state, w, v, depth + 1);
      state->infos[v].lowpoint =
          MIN(state->infos[v].lowpoint, state->infos[w].lowpoint);
      if (state->infos[w].lowpoint >= state->infos[v].dfi) {
        if (u != -1)
          state->infos[v].cut = true;

#define DFI(s) (state->infos[s].dfi)
#define EDGESTART(edge) (state->G->G[(edge) ^ 1].v)

        bitset_t component;
        bitset_init(&component, UCAST(state->G->N), state->arena);

        while (state->edgeStack.N) {
          gint top = VECTOREND(state->edgeStack);
          if (EDGESTART(top) == v && EDGESTART(top ^ 1) == w)
            break;

          gint compEdge = intvector_popback(&state->edgeStack);
          gint cv = Gv(state->G, compEdge);

          state->infos[cv].componentId = SCAST(state->components.N);
          bitset_add(&component, UCAST(cv));
          bitset_add(&component, UCAST(Gv(state->G, compEdge ^ 1)));
        }
        gint compEdge = intvector_popback(&state->edgeStack);
        gint cv = Gv(state->G, compEdge);
        bitset_add(&component, UCAST(cv));
        bitset_add(&component, UCAST(Gv(state->G, compEdge ^ 1)));
        state->infos[cv].componentId = SCAST(state->components.N);
        componentlist_push(&state->components, component);
      }
    } else if (DFI(w) < DFI(v) && w != u) {
      intvector_push(&state->edgeStack, edge);
      state->infos[v].lowpoint =
          MIN(state->infos[v].lowpoint, state->infos[w].dfi);
    }
  }
  if (u == -1 && state->infos[v].children > 1)
    state->infos[v].cut = true;
}
