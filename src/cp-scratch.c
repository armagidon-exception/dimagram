/* #include "hs.h"
#include "graph_utils.h"
#include "igraph_constants.h"
#include "igraph_conversion.h"
#include "igraph_matrix.h"
#include "igraph_types.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

typedef igraph_matrix_t mat;
#define SET_P(m, v, x, y)                                                      \
  (MATRIX(m, v, 0) = (igraph_real_t)x, MATRIX(m, v, 1) = (igraph_real_t)y)
#define CO(v) vecget(*CO, v)
#define TNODE_NIL -1
#define NEIGHBOUR(mat, v, w) ((gint)MATRIX((mat), v, w))
#define NEXT(v) (Ls[v].rs)

typedef struct Tnode {
  gint fc;
  gint rs;
  gint xo;
  gint y;
} Tnode;

static void accumulateOffsets(gint v, gint d, mat *output, Tnode *Ls);

int chrobak_payne_layout(const igraph_t *G, veci *CO, mat *output) {
  gint vcount = g_vcount(G);
  assert(vcount >= 3 && "Number of vertices must be greater than 2.");

  if (output->ncol < 2) {
    if (igraph_matrix_add_cols(output, 2 - output->ncol))
      return -1;
  }
  if (output->nrow < vcount) {
    if (igraph_matrix_add_rows(output, vcount - output->nrow))
      return -1;
  }

  Tnode *Ls = calloc((size_t)vcount, sizeof(Tnode));
  for (gint i = 0; i < vcount; i++) {
    Ls[i].fc = TNODE_NIL;
    Ls[i].rs = TNODE_NIL;
  }
  Ls[CO(0)] = (Tnode){TNODE_NIL, CO(2), 0, 0};
  Ls[CO(1)] = (Tnode){TNODE_NIL, TNODE_NIL, 1, 0};
  Ls[CO(2)] = (Tnode){TNODE_NIL, CO(1), 1, 1};

  igraph_matrix_t adjmat;
  igraph_matrix_init(&adjmat, 0, 0);
  igraph_get_adjacency(G, &adjmat, IGRAPH_GET_ADJACENCY_BOTH, 0,
                       IGRAPH_NO_LOOPS);
  for (int k = 3; k < vcount; k++) {
    gint vk = CO(k);
    gint p = CO(0), q = TNODE_NIL, pq = TNODE_NIL;
    // Aquire neighbours on the contour

    while (!NEIGHBOUR(adjmat, p, vk)) {
      pq = p;
      p = NEXT(p);
    }
    assert(p != TNODE_NIL && "Contour does not contain neighbour of v_k");
    q = p;

    while (NEXT(q) != TNODE_NIL && NEIGHBOUR(adjmat, NEXT(q), vk)) {
      pq = q;
      q = NEXT(q);
    }

    Ls[NEXT(p)].xo++;
    Ls[q].xo++;

    gint pp = NEXT(p);
    gint dwpq = 0;
    for (gint i = pp; i != NEXT(q); i = NEXT(i))
      dwpq += Ls[i].xo;

    Ls[vk].xo = (-Ls[p].y + dwpq + Ls[q].y) >> 1;
    Ls[vk].y = (Ls[p].y + dwpq + Ls[q].y) >> 1;
    Ls[q].xo = dwpq - Ls[vk].xo;

    if (pp != q) {
      Ls[pp].xo -= Ls[vk].xo;
    }

    Ls[p].rs = vk;
    Ls[vk].rs = q;

    if (pp != q) {
      Ls[vk].fc = pp;
      Ls[pq].rs = TNODE_NIL;
    } else {
      Ls[vk].fc = TNODE_NIL;
    }
  }

  accumulateOffsets(CO(0), 0, output, Ls);

  igraph_matrix_destroy(&adjmat);
  free(Ls);

  return 0;
}

static void accumulateOffsets(gint v, gint d, mat *output, Tnode *Ls) {
  if (v != TNODE_NIL) {
    Ls[v].xo += d;
    SET_P(*output, v, Ls[v].xo, Ls[v].y);
    accumulateOffsets(Ls[v].fc, Ls[v].xo, output, Ls);
    accumulateOffsets(Ls[v].rs, Ls[v].xo, output, Ls);
  }
} */
