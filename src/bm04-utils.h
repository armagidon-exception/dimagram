#include "graph_utils.h"

typedef struct {
  gint dfi;
  veci pertinentRoots;
  gint visited;
  gint backedgeFlag;
  veci separatedDFSChildList;
  gint lowpoint;       // Low point of the vertex
  gint dfsParent;      // Parent vertex id
  gint leastAncestor;  // Least ancestor of the vertex
  veci directChildren; // Direct children of the vertex in DFS tree
  veci backEdges;      // Back edges to the vertex
} bm04_vertex_t;

typedef struct {
  gint link[2];
  gint neighbor;
  char sign;
} bm04_edge_t;

typedef struct {
  gint link[2];
  gint parent;
  gint index;
} bm04_verrec_t;

typedef bm04_verrec_t *bm04_verrecP;
typedef bm04_vertex_t *bm04_vertexP;
typedef bm04_edge_t *bm04_edgeP;

typedef struct {
  gint N;
  gint M;
  bm04_verrecP V;
  bm04_vertexP VI;
  bm04_edgeP E;
  istack S;
  veci dfsNumbering;
} bm04_graph;

typedef bm04_graph *bm04_graphP;

void bm04_init_graph(bm04_graph *G, const igraph_t *g);

void bm04_walkup(bm04_graphP G, gint v, gint w);

void bm04_dfs(bm04_graphP G, const igraph_t *g, gint *dfi, gint v, gint u);

#define NIL -1

#define bm04_VCOUNT(g) ((g)->N)
#define bm04_ECOUNT(g) ((g)->M)
#define bm04_GETSTACKP(g) (&((g)->S))
#define bm04_VERTEX(g, v) ((g)->V[v])
#define bm04_VINFO(g, v) ((g)->VI[v])
#define bm04_EDGE(g, e) ((g)->E[e])
#define bm04_TWIN(g, e) ((g)->E[e ^ 1])
#define bm04_ISTWIN(e1, e2) ((e1 ^ e2) == 1)
#define bm04_FROMDFI(g, dfi) VECTOR((g)->dfsNumbering)[dfi]
#define bm04_LOWPT(g, v) (bm04_VINFO(g, v).lowpoint)
#define bm04_NEIGHS(g, v) (bm04_VINFO(g, v).directChildren)
#define bm04_CHILDCOUNT(g, v) (vector_size(&bm04_NEIGHS(&G, v)))
#define bm04_DFSCHILD(g, v, i) VECTOR(bm04_VINFO(g, v).directChildren)[i]
#define bm04_ISROOT(g, v) ((v) >= bm04_VCOUNT(g))
#define bm04_UNROOT(g, v) (bm04_VERTEX(g, (v) - bm04_VCOUNT(g)).parent)
#define bm04_ROOTCHILD(g, v) ((v) - bm04_VCOUNT(g))
#define bm04_PERTINENTROOTS(g, v)                                              \
  ((g)->VI[v >= bm04_VCOUNT(g) ? bm04_UNROOT(g, v) : v].pertinentRoots)

#define bm04_GETVISITED(g, w)                                                  \
  ((g)->VI[v >= bm04_VCOUNT(g) ? bm04_UNROOT(g, v) : v].visited)
