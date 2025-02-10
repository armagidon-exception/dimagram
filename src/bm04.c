#include "bm04.h"
#include "bm04-utils.h"
#include "boyer/graph.h"
#include "graph_utils.h"
#include "igraph_conversion.h"
#include "igraph_datatype.h"
#include "igraph_interface.h"
#include "igraph_iterators.h"
#include "igraph_vector.h"
#include "igraph_vector_list.h"
#include "utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// #define ORIGINAL_IMPLEMENTATION

#ifndef ORIGINAL_IMPLEMENTATION
static void sort_vertecies(bm04_graphP G);

static void bm04_embed_arc(bm04_graphP G, gint v, gint c);

// static gint bm04_prepend_edge(bm04_graphP G, gint v, gint neighbour);

static gint bm04_append_edge(bm04_graphP G, gint v, gint neighbour);

static gint next_edge(bm04_graphP G, gint w, gint win, gint *sin);

static void walkup(bm04_graphP G, gint v, gint w);

#endif

int bm04_planar_embedding(const igraph_t *g,
                          igraph_vector_int_list_t *embedding) {
  gint vcount = igraph_vcount(g);
  vec2int_init(embedding, vcount);
  assert(vcount <= INT32_MAX && "Execeded maximum amount of verteces");

#ifndef ORIGINAL_IMPLEMENTATION
  bm04_graph G;
  bm04_init_graph(&G, g);
  gint dfi = 0;
  for (gint v = 0; v < vcount; v++) {
    if (bm04_VINFO(&G, v).dfi == NIL) {
      // printf("%ld\n", bm04_VINFO(&G, v).dfi);
      bm04_dfs(&G, g, &dfi, v, NIL);
    }
  }
  sort_vertecies(&G);

  for (gint vi = vcount - 1; vi >= 0; vi--) {
    gint v = bm04_FROMDFI(&G, vi);
    for (gint ci = 0; ci < bm04_CHILDCOUNT(G, v); ci++) {
      gint c = bm04_DFSCHILD(&G, v, ci);
      bm04_embed_arc(&G, v, c);
    }

    for (gint i = 0; i < vector_size(&bm04_VINFO(&G, vi).backEdges); i++) {
      gint w = vecget(bm04_VINFO(&G, vi).backEdges, i);
      walkup(&G, v, w);
    }
  }

#else
  veci edgelist = veci_new();
  BM_graph *G = gp_New();
  gp_InitGraph(G, (int)vcount);
  igraph_get_edgelist(g, &edgelist, true);
  gint EN = vector_size(&edgelist) / 2;
  for (gint i = 0; i < EN; i++) {
    gp_AddEdge(G, (int)vecget(edgelist, i), 0, (int)vecget(edgelist, i + EN),
               0);
  }
  veci_dest(&edgelist);
  if (gp_Embed(G, EMBEDFLAGS_PLANAR)) {
    gp_Free(&G);
    return -1;
  }
  gp_SortVertices(G);

  int I, J;
  for (I = 0; I < G->N; I++) {
    veci *order = vec2iget(embedding, I);
    J = G->G[I].link[1];
    while (J >= G->N) {
      veci_push(order, G->G[J].v);
      J = G->G[J].link[1];
    }
  }

  gp_Free(&G);

#endif
  return 0;
}

#ifndef ORIGINAL_IMPLEMENTATION

void bm04_init_graph(bm04_graph *G, const igraph_t *g) {
  G->N = igraph_vcount(g);
  G->M = 0;
  G->V = calloc(UCAST(2 * bm04_VCOUNT(G)), sizeof(bm04_verrec_t));
  G->VI = calloc(UCAST(bm04_VCOUNT(G)), sizeof(bm04_vertex_t));
  G->E = calloc(UCAST(6 * bm04_VCOUNT(G)), sizeof(bm04_edge_t));
  igraph_stack_int_init(bm04_GETSTACKP(G), 4 * bm04_VCOUNT(G));
  veci_init(&G->dfsNumbering, bm04_VCOUNT(G));

  for (gint vi = 0; vi < bm04_VCOUNT(G); vi++) {
    bm04_VINFO(G, vi) = (bm04_vertex_t){.dfi = NIL,
                                        .leastAncestor = NIL,
                                        .dfsParent = NIL,
                                        .lowpoint = NIL,
                                        .pertinentRoots = veci_new(),
                                        .directChildren = veci_new(),
                                        .backEdges = veci_new(),
                                        .separatedDFSChildList = veci_new()};
  }

  for (gint vi = 0; vi < bm04_VCOUNT(G); vi++) {
    bm04_VERTEX(G, vi).parent = -1;
  }
}

void bm04_dfs(bm04_graphP G, const igraph_t *g, gint *dfi, gint v, gint u) {
  bm04_VINFO(G, v).dfi = *dfi;
  bm04_VINFO(G, v).lowpoint = *dfi;
  bm04_FROMDFI(G, *dfi) = v;

  igraph_vector_int_t neis = getneigs(g, v);
  gint vs = vector_size(&neis);
  for (gint wi = 0; wi < vs; wi++) {
    gint w = VECTOR(neis)[wi];

    if (bm04_VINFO(G, w).dfi < 0) {
      (*dfi)++;
      bm04_dfs(G, g, dfi, w, v);

      bm04_VINFO(G, v).lowpoint = MIN(bm04_LOWPT(G, v), bm04_LOWPT(G, w));
      bm04_VINFO(G, w).dfsParent = v;
      veci_push(&bm04_VINFO(G, v).directChildren, w);
    } else if (bm04_VINFO(G, w).dfi < bm04_VINFO(G, v).dfi && w != u) {
      bm04_VINFO(G, v).lowpoint = MIN(bm04_LOWPT(G, v), bm04_VINFO(G, w).dfi);
      veci_push(&bm04_VINFO(G, w).backEdges, v);
      bm04_VINFO(G, v).leastAncestor = bm04_VINFO(G, w).dfi;
    }
  }

  veci_dest(&neis);
}

static void bm04_embed_arc(bm04_graphP G, gint v, gint c) {
  gint vc = c + bm04_VCOUNT(G);

  bm04_verrec_t *root = &bm04_VERTEX(G, vc);
  root->parent = v;
  root->index = vc;
  root->link[0] = root->link[1] = bm04_append_edge(G, vc, c);
  G->E[root->link[0]].link[0] = G->E[root->link[0]].link[1] = root->link[0];

  bm04_verrec_t *child = &bm04_VERTEX(G, c);
  child->parent = v;
  child->index = c;
  child->link[0] = child->link[1] = bm04_append_edge(G, c, vc);
  G->E[child->link[0]].link[0] = G->E[child->link[0]].link[1] = child->link[0];
}

static gint bm04_append_edge(bm04_graphP G, gint v, gint neighbour) {
  bm04_verrec_t ver = bm04_VERTEX(G, v);
  gint eid = G->M++;
  G->E[eid] = (bm04_edge_t){{ver.link[1], ver.link[0]}, neighbour, 1};
  ver.link[1] = eid;

  return eid;
}

/* static gint bm04_prepend_edge(bm04_graphP G, gint v, gint neighbour) {
  bm04_verrec_t ver = bm04_VERTEX(G, v);
  gint eid = G->M++;
  G->E[eid] = (bm04_edge_t){{ver.link[1], ver.link[0]}, neighbour, 1};
  ver.link[0] = eid;
  return eid;
} */

static gint next_edge(bm04_graphP G, gint w, gint win, gint *sin) {
  bm04_verrec_t v = bm04_VERTEX(G, w);
  gint eid = v.link[win];
  gint sn = bm04_EDGE(G, eid).neighbor;
  bm04_verrec_t s = bm04_VERTEX(G, sn);
  if (v.link[0] == v.link[1]) {
    *sin = win;
    return sn;
  } else {
    *sin = bm04_ISTWIN(s.link[0], eid) ? 1 : 0;
    return sn;
  }
}

static void walkup(bm04_graphP G, gint v, gint w) {
  bm04_VINFO(G, w).backedgeFlag = v;
  gint x = w, y = w, xin = 1, yin = 0;
  while (v != x) {
    bm04_GETVISITED(G, x) = v;
    bm04_GETVISITED(G, y) = v;

    gint zp = NIL;
    if (bm04_ISROOT(G, x))
      zp = x;
    else if (bm04_ISROOT(G, y))
      zp = y;

    if (zp != NIL) {
      gint z = bm04_VERTEX(G, zp).parent;
      if (z != v) {
        if (bm04_LOWPT(G, bm04_ROOTCHILD(G, zp)) < v)
          veci_push(&bm04_VINFO(G, z).pertinentRoots, zp);
        else
          veci_prepend(&bm04_PERTINENTROOTS(G, z), zp);
      }
      x = z, xin = 1, y = z, yin = 0;
    } else {
      x = next_edge(G, x, xin, &xin);
      y = next_edge(G, y, yin, &yin);
    }
  }
}

void bm04_walkup(bm04_graphP G, gint v, gint w) { walkup(G, v, w); }

static void sort_vertecies(bm04_graphP G) {
  vec2int buckets;
  vec2int_init(&buckets, G->N);
  for (int i = 0; i < bm04_VCOUNT(G); i++) {
    veci *bucket = vec2iget(&buckets, bm04_LOWPT(G, i));
    veci_init(bucket, 0);
    veci_push(bucket, i);
  }

  for (int i = 0; i < bm04_VCOUNT(G); i++) {
    veci *bucket = vec2iget(&buckets, i);
    for (int j = 0; j < vector_size(bucket); j++) {
      gint vi = vecget(*bucket, j);
      bm04_vertex_t *v = &bm04_VINFO(G, vi);
      veci_push(&bm04_VINFO(G, v->dfsParent).separatedDFSChildList, vi);
    }
  }

  for (int i = 0; i < bm04_VCOUNT(G); i++) {
    veci *bucket = vec2iget(&buckets, bm04_LOWPT(G, i));
    veci_dest(bucket);
  }
  vec2int_dest(&buckets);
}

#endif

#define GINT(i) ((gint)i)

int bm04_embed_graph(const igraph_t *g, embedding_t *embedding) {
#ifdef ORIGINAL_IMPLEMENTATION
  embedding_t em;
  em.N = g_vcount(g);

  veci edgelist = veci_new();
  BM_graph *G = gp_New();
  gp_InitGraph(G, (int)em.N);
  igraph_get_edgelist(g, &edgelist, true);
  for (gint i = 0; i < igraph_ecount(g); i++) {
    gp_AddEdge(G, (int)vecget(edgelist, i), 0,
               (int)vecget(edgelist, i + igraph_ecount(g)), 0);
  }
  veci_dest(&edgelist);
  if (gp_Embed(G, EMBEDFLAGS_PLANAR) || gp_SortVertices(G)) {
    gp_Free(&G);
    return -1;
  }
  em.M = 2 * G->M;
  gint C = 2 * em.N;

  em.edges = calloc(assize(em.M), sizeof(em_edge));
  em.vertices = calloc(assize(em.N), sizeof(em_vertex));

  for (int v = 0; v < em.N; v++) {
    em.vertices[v][0] = GINT(G->G[v].link[0]) - C;
    em.vertices[v][1] = GINT(G->G[v].link[1]) - C;

    G->G[G->G[v].link[0]].link[1] = G->G[v].link[1];
    G->G[G->G[v].link[1]].link[0] = G->G[v].link[0];
  }

  for (int eid = 0; eid < em.M; eid++) {
    graphNode edge = G->G[eid + C];
    int link0 = edge.link[0], link1 = edge.link[1];
    em.edges[eid] = (em_edge){{link0 - C, link1 - C}, edge.v};
  }

  *embedding = em;
  gp_Free(&G);
  return 0;
#else
  TODO("Implement this function");
  return -1;
#endif
}

void bm04_embedding_destroy(embedding_t *embedding) {
  free(embedding->edges);
  free(embedding->vertices);
  embedding->edges = 0;
  embedding->vertices = 0;
}
