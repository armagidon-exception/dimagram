#include "boyer/graph.h"
#include "cp.h"
#include "graph_utils.h"
#include "igraph_matrix.h"
#include "igraph_operators.h"
#include "log.h"
#include "utils.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/internal/new_asserts.h>
#include <criterion/new/assert.h>

#define HS_TEST_ENV
#include "cp.c"

igraph_t G;
void setup(void) { igraph_empty(&G, 0, IGRAPH_UNDIRECTED); }

void teardown(void) { igraph_destroy(&G); }

// Test(test_canonical_ordering, test_boyer, .init = setup, .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 6, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 3, 4);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 5, 0);
//   igraph_add_edge(Gp, 1, 3);
//   igraph_add_edge(Gp, 1, 4);
//
//   veci edgelist = veci_new();
//   BM_graph *bmg = gp_New();
//   gp_InitGraph(bmg, (int)6);
//   igraph_get_edgelist(Gp, &edgelist, true);
//   for (gint i = 0; i < igraph_ecount(Gp); i++) {
//     gp_AddEdge(bmg, (int)vecget(edgelist, i), 0,
//                (int)vecget(edgelist, i + igraph_ecount(Gp)), 0);
//   }
//   veci_dest(&edgelist);
//   if (gp_Embed(bmg, EMBEDFLAGS_PLANAR)) {
//   EMBEDDING_ERROR:
//     gp_Free(&bmg);
//     return;
//   }
//
//   // gp_Write(bmg, "stdout", WRITE_DEBUGINFO);
//
//   if (gp_SortVertices(bmg)) {
//     goto EMBEDDING_ERROR;
//   }
//   // log_debug("counterclockwise=%d clockwise=%d", bmg->G[1].link[0],
//   //           bmg->G[1].link[1]);
//   // log_debug("counterclockwise=%d clockwise=%d", bmg->G[bmg->G[1].link[0]].v,
//   //           bmg->G[bmg->G[1].link[1]].v);
//
//   // for (int i = 1; i < 2; i++) {
//   //   printf("%d:", i);
//   //
//   //   int J = bmg->G[i].link[1];
//   //   while (J >= bmg->N) {
//   //     printf(" %d", bmg->G[J].v);
//   //     J = bmg->G[J].link[1];
//   //   }
//   //   printf(" %d\n", NIL);
//   // }
//
//   // gp_Write(bmg, "stdout", WRITE_DEBUGINFO);
// }
//
// Test(test_canonical_ordering, test_embedding, .init = setup, .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 6, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 3, 4);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 5, 0);
//
//   Arena test_arena = {0};
//   Arena *arenaP = &test_arena;
//
//   embedding_t em;
//   em.N = igraph_vcount(Gp);
//   em.M = 2 * igraph_ecount(Gp);
//   em.edges = arena_calloc(arenaP, UCAST(em.M), sizeof(em_edge));
//   em.vertices = arena_calloc(arenaP, UCAST(em.N), sizeof(em_vertex));
//   em.degrees = arena_calloc(arenaP, UCAST(em.N), sizeof(int));
//
//   cr_expect(eq(i64, init_embedding(Gp, &em, arenaP), 0));
//
//   for (int i = 0; i < 6; i++) {
//     cr_expect(eq(i64, em.degrees[i], 2));
//   }
//
//   for (int i = 0; i < 6; i++) {
//     cr_expect(eq(i64, EDGE_TARGET(em, em.degrees[i]), 2));
//     cr_expect(eq(i64, EDGE_TARGET(em, em.vertices[i][0]), (i + 5) % 6));
//     cr_expect(eq(i64, EDGE_TARGET(em, em.vertices[i][1]), (i + 1) % 6));
//   }
// }
//
// Test(test_canonical_ordering, test_right_hand_walk, .init = setup,
//      .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 6, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 3, 4);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 5, 0);
//
//   Arena test_arena = {0};
//   Arena *arenaP = &test_arena;
//
//   co_state state;
//   embedding_t em;
//   em.N = igraph_vcount(Gp);
//   em.M = 2 * igraph_ecount(Gp);
//   em.edges = arena_calloc(arenaP, UCAST(em.M), sizeof(em_edge));
//   em.vertices = arena_calloc(arenaP, UCAST(em.N), sizeof(em_vertex));
//   em.degrees = arena_calloc(arenaP, UCAST(em.N), sizeof(int));
//   state.inGk = arena_calloc(arenaP, UCAST(em.N), sizeof(bool));
//
//   cr_expect(eq(i64, init_embedding(Gp, &em, arenaP), 0));
//
//   rhw_result_init(arenaP, em.M, &state.rhw);
//
//   rightHandWalk(&em, &state.rhw, arenaP);
//   cr_expect(eq(u64, state.rhw.faces.N, 2));
//
//   // for (int fi = 0; fi < state.rhw.faces.N; fi++) {
//   for (int i = 0; i < SCAST(state.rhw.faces.N); i++) {
//     face_edge_t *fe = &AS_FACEEDGE(state.rhw, FACE(state.rhw.faces, 0).array[i]);
//     gint v = EDGE_TARGET(em, fe->eid);
//     gint vi = EDGE_SOURCE(em, fe->eid);
//     gint wi = EDGE_TARGET(em, fe->next);
//
//     cr_expect(le(i64, fe->leftface, 2));
//
//     // log_debug("%ld -> %ld -> %ld", vi, v, wi);
//     cr_expect(eq(i64, vi, (v + 5) % 6));
//     cr_expect(eq(i64, wi, (v + 1) % 6));
//     // printf("%ld -> ", v);
//   }
//   // printf("\n");
//
//   for (int i = 0; i < SCAST(FACE(state.rhw.faces, 1).N); i++) {
//     face_edge_t *fe =
//         &AS_FACEEDGE(state.rhw, FACE(state.rhw.faces, 1).array[i]);
//     gint v = EDGE_TARGET(em, fe->eid);
//     gint vi = EDGE_SOURCE(em, fe->eid);
//     gint wi = EDGE_TARGET(em, fe->next);
//
//     // log_debug("%ld fe", fe->leftface);
//     cr_expect(le(i64, fe->leftface, 2));
//     cr_expect(eq(i64, wi, (v + 5) % 6));
//     cr_expect(eq(i64, vi, (v + 1) % 6));
//   }
// }
//
// Test(test_canonical_ordering, test_canonical_ordering, .init = setup,
//      .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 6, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 3, 4);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 5, 0);
//
//   Arena test_arena = {0};
//   co_t co;
//   embedding_t em;
//   co_init(&co, &test_arena);
//   canonical_ordering(Gp, &em, &co);
//
//   for (int i = 0; i < SCAST(co.N); i++) {
//     log_debug("%d: %d", i, co.array[i]);
//   }
// }
//
//
// Test(test_canonical_ordering, test_canonical_ordering1, .init = setup,
//      .fini = teardown) {
//   igraph_t *Gp = &G;
// igraph_add_vertices(Gp, 7, 0);
// igraph_add_edge(Gp, 0, 1);
// igraph_add_edge(Gp, 1, 2);
// igraph_add_edge(Gp, 2, 3);
// igraph_add_edge(Gp, 3, 4);
// igraph_add_edge(Gp, 4, 5);
// igraph_add_edge(Gp, 5, 6);
// igraph_add_edge(Gp, 6, 0);
// igraph_add_edge(Gp, 6, 3);
//
//   Arena test_arena = {0};
//   co_t co;
//   embedding_t em;
//   co_init(&co, &test_arena);
//   canonical_ordering(Gp, &em, &co);
//
//   for (int i = 0; i < SCAST(co.N); i++) {
//     log_debug("%d: %d", i, co.array[i]);
//   }
// }
//
// Test(test_canonical_ordering, test_layout, .init = setup, .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 6, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 3, 4);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 5, 0);
//
//   mat output;
//   igraph_matrix_init(&output, 0, 0);
//   // harel_sardes_layout(Gp, &output);
// }
//
// Test(test_canonical_ordering, test_embedding1, .init = setup,
//      .fini = teardown) {
//   igraph_t *Gp = &G;
//   igraph_add_vertices(Gp, 9, 0);
//   igraph_add_edge(Gp, 0, 1);
//   igraph_add_edge(Gp, 0, 2);
//   igraph_add_edge(Gp, 0, 4);
//   igraph_add_edge(Gp, 0, 5);
//   igraph_add_edge(Gp, 0, 7);
//   igraph_add_edge(Gp, 0, 8);
//   igraph_add_edge(Gp, 1, 2);
//   igraph_add_edge(Gp, 1, 3);
//   igraph_add_edge(Gp, 1, 8);
//   igraph_add_edge(Gp, 2, 3);
//   igraph_add_edge(Gp, 2, 4);
//   igraph_add_edge(Gp, 2, 6);
//   igraph_add_edge(Gp, 3, 6);
//   igraph_add_edge(Gp, 3, 7);
//   igraph_add_edge(Gp, 3, 8);
//   igraph_add_edge(Gp, 4, 7);
//   igraph_add_edge(Gp, 4, 5);
//   igraph_add_edge(Gp, 4, 6);
//   igraph_add_edge(Gp, 5, 7);
//   igraph_add_edge(Gp, 6, 7);
//   igraph_add_edge(Gp, 7, 8);
//
//   Arena test_arena = {0};
//   Arena *arenaP = &test_arena;
//
//   embedding_t em;
//   em.N = igraph_vcount(Gp);
//   em.M = 2 * igraph_ecount(Gp);
//   em.edges = arena_calloc(arenaP, UCAST(em.M), sizeof(em_edge));
//   em.vertices = arena_calloc(arenaP, UCAST(em.N), sizeof(em_vertex));
//   em.degrees = arena_calloc(arenaP, UCAST(em.N), sizeof(int));
//
//   cr_expect(eq(i64, init_embedding(Gp, &em, arenaP), 0));
//   rhw_result_t result;
//   rhw_result_init(arenaP, em.M, &result);
//   rightHandWalk(&em, &result, &test_arena);
//
//   // for (int i = 0; i < SCAST(result.faces.N); i++) {
//   //   printFace(&result.faces, result.edges, &em, i);
//   // }
//
//   gint current = em.vertices[0][0];
//   do {
//     printFace(&result.faces, result.edges, &em,
//               result.edges[current ^ 1].leftface);
//     current = em.edges[current].link[0];
//   } while (current != em.vertices[0][0]);
//
//   arena_free(&test_arena);
// }
