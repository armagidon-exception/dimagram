#include "cairo.h"
#include "diagram.h"
#include "log.h"
#include <float.h>
#include <getopt.h>
#include <igraph.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void print_help(const char *execname) {
  printf("Usage: %s [-i input_file] [-o output_file]\n\n", execname);
  printf("Generate diagrams from text input.\n\n");
  printf("Options:\n");
  printf("  -i input_file   Read input from the specified file. Defaults to "
         "stdin if not provided.\n");
  printf("  -o output_file  Write output to the specified file. Defaults to "
         "stdout if not provided.\n");
  printf("  -h              Display this help message and exit.\n\n");
  printf("  -q              Display only error messages.\n\n");
  printf("Examples:\n");
  printf("  %s -i diagram.txt -o output.svg  # Read from diagram.txt and write "
         "to output.svg\n",
         execname);
  printf("  %s -i diagram.txt                # Read from diagram.txt and write "
         "to stdout\n",
         execname);
  printf("  %s                               # Read from stdin and write to "
         "stdout\n\n",
         execname);
  printf("For more information, visit: "
         "https://github.com/armagidon-exception/dimagram\n");
}

int main(int argc, char **argv) {
  int opt;
  FILE *input = stdin;
  FILE *output = stdout;
  // log_set_level(LOG_INFO);
  while ((opt = getopt(argc, argv, "qi:o:h")) != -1) {
    switch (opt) {
    case 'i': {
      if (input && input != stdin) {
        fclose(input);
      }
      FILE *f = fopen(optarg, "r");
      if (!f) {
        perror("Error");
        exit(EXIT_FAILURE);
      }
      input = f;
      break;
    }
    case 'o': {
      if (output && output != stdout) {
        fclose(output);
      }
      FILE *f = fopen(optarg, "w");
      if (!f) {
        perror("Error");
        exit(EXIT_FAILURE);
      }
      log_set_quiet(false);
      output = f;
      break;
    }
    case 'q': {
      log_set_level(LOG_ERROR);
        break;
    }
    default:
      print_help(argv[0]);
      exit(EXIT_FAILURE);
      break;
    }
  }

  Arena arena = {0};
  diagram_t diagram;
  diagram_status_e status =
      diagram_parse_diagram_from_stream(&diagram, input, &arena);
  if (status != DIAGRAM_OK) {
    log_error("Error: %s", diagram.errmsg);
    exit(EXIT_FAILURE);
  }

  cairo_surface_t *surf = diagram_lay_out(&diagram, output, &status);
  if (status != DIAGRAM_OK) {
    log_error("Error: %s", diagram.errmsg);
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < igraph_vcount(&diagram.G); i++) {
    cairo_surface_destroy(diagram.textures[i]);
  }

  cairo_surface_destroy(surf);
  igraph_destroy(&diagram.G);

  arena_free(&arena);
  return EXIT_SUCCESS;
}
