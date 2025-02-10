#include "cairo.h"
#include "igraph_layout.h"
#include "igraph_matrix.h"
#include "igraph_types.h"
#include "igraph_vector.h"
#include "renderer.h"
#include "utils.h"
#include <getopt.h>
#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

double maxd(double a, double b) { return a > b ? a : b; }
double mind(double a, double b) { return a < b ? a : b; }

int main(void) {
  igraph_t graph;
  igraph_integer_t num_vertices = 10;
  igraph_integer_t num_edges = 10;
  igraph_matrix_t resm;

  igraph_real_t temp_min = 0.1, temp_max = num_vertices,
                temp_init = sqrt(num_vertices);

  igraph_matrix_init(&resm, num_vertices, 2);

  igraph_rng_seed(igraph_rng_default(), 10);

  igThrowIfError(igraph_erdos_renyi_game_gnm(
      &graph, num_vertices, num_edges, IGRAPH_UNDIRECTED, IGRAPH_NO_LOOPS));

  igThrowIfError(igraph_layout_gem(&graph, &resm, true,
                                   40 * num_vertices * num_vertices, temp_max,
                                   temp_min, temp_init));

  igraph_vector_int_t edges;
  igraph_vector_int_init(&edges, 2 * num_edges);
  igraph_get_edgelist(&graph, &edges, true);

  double mx, Mx, my, My;
  for (int i = 0; i < num_vertices; i++) {
    double x = MATRIX(resm, i, 0);
    double y = MATRIX(resm, i, 1);

    mx = mind(mx, x);
    my = mind(my, y);
    My = maxd(My, y);
    Mx = maxd(Mx, x);
  }

  renderer_t *renderer = render_svg_create("image.svg", Mx - mx + 12, My - my + 12);
  for (int i = 0; i < num_vertices; i++) {
    double x = MATRIX(resm, i, 0);
    double y = MATRIX(resm, i, 1);
    // printf("%f %f\n", x, y);
    node_t *node = renderer_create_orb();
    node->x = x;
    node->y = y;
    node->render(renderer, node, x - mx + 6, y - my + 6);
    node->destroy(node);
  }

  for (int i = 0; i < num_edges; i++) {
    igraph_integer_t u, v;
    igraph_edge(&graph, i, &u, &v);

    double x1 = MATRIX(resm, u, 0) - mx + 6, y1 = MATRIX(resm, u, 1) - my + 6,
           x2 = MATRIX(resm, v, 0) - mx + 6, y2 = MATRIX(resm, v, 1) - my + 6;

    renderer_link(renderer, x1, y1, x2, y2);
  }
  igraph_vector_int_destroy(&edges);

  renderer_destroy(renderer);

  igraph_matrix_destroy(&resm);
  igraph_destroy(&graph);

  return EXIT_SUCCESS;
}
