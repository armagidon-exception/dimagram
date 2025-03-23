#include "diagram.h"
#include "arena.h"
#include "cairo.h"
#include "hs.h"
#include "dynarray.h"
#include "graph_utils.h"
#include "igraph_components.h"
#include "igraph_matrix.h"
#include "igraph_operators.h"
#include "igraph_vector.h"
#include "log.h"
#include "utils.h"
#include "zhash.h"
#include <cairo-svg.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MIN_EDGE_LEN 10
#define STARTS_WITH(literal, str)                                              \
  (strncmp(literal, str, sizeof(literal) - 1) == 0)

#define NODE_TYPES_COUNT 4
typedef enum node_type_e {
  NODE_NODE,
  NODE_ENTITY,
  NODE_RELATIONSHIP,
  NODE_ATTRIBUTE
} node_type_e;
typedef cairo_surface_t *(*render_func)(const char *label);

static char *node_type_names[NODE_TYPES_COUNT] = {"node", "entity",
                                                  "relationship", "attribute"};

#define NODE_TYPE_REGEX(name) "^" #name " \\([[:alnum:]_]\\+\\) \"\\(.\\+\\)\"$"

static char *node_regex_strings[NODE_TYPES_COUNT] = {
    NODE_TYPE_REGEX(node), NODE_TYPE_REGEX(entity),
    NODE_TYPE_REGEX(relationship), NODE_TYPE_REGEX(attribute)};

typedef struct {
  gint v, w;
} node_link_t;

DEFINE_VECTOR(vertex_list, cairo_surface_t *)
DEFINE_VECTOR(link_list, node_link_t)

typedef struct {
  gint N;
  gint M;
  vertex_list_t textures;
  link_list_t links;
  Arena *arena;
  const char *errmsg;
  struct ZHashTable *vertices;
} parsing_context;

static int read_line(const char *contents, size_t N, size_t start, size_t *end);
static diagram_status_e handle_line(const char *contents, size_t start,
                                    size_t end, parsing_context *ctx);
static diagram_status_e parse_node(node_type_e node_type, parsing_context *ctx,
                                   const char *block, size_t blen);
static diagram_status_e parse_link(parsing_context *ctx, const char *block,
                                   size_t blen);
cairo_surface_t *draw_entity(const char *text);
cairo_surface_t *draw_relationship(const char *text);
cairo_surface_t *draw_attribute(const char *text);

diagram_status_e diagram_parse_diagram_from_stream(diagram_t *diagram, FILE *is,
                                                   Arena *arena) {
  parsing_context ctx = {0};
  diagram_status_e err;
  diagram->arena = arena;
  ctx.arena = arena;
  ctx.vertices = zcreate_hash_table(arena);

  long pos = ftell(is);
  fseek(is, 0, SEEK_END);
  size_t fsize = UCAST(ftell(is) - pos);
  fseek(is, pos, SEEK_SET);
  Arena local_arena = {0};
  char *contents = arena_calloc(&local_arena, sizeof(char), fsize + 1);
  fread(contents, sizeof(char), fsize, is);
  if (ferror(is)) {
    diagram->errmsg = arena_strdup(arena, strerror(errno));
    return DIAGRAM_EIO;
  }

  vertex_list_init(&ctx.textures, &local_arena);
  link_list_init(&ctx.links, &local_arena);
  size_t start_block = 0, end_block = 0;
  while (read_line(contents, fsize, start_block, &end_block) > 0) {
    if ((err = handle_line(contents, start_block, end_block, &ctx)) !=
        DIAGRAM_OK) {
      diagram->errmsg = ctx.errmsg;
      arena_free(&local_arena);
      return err;
    }

    start_block = end_block + 1;
  }
  igraph_empty(&diagram->G, SCAST(ctx.textures.N), IGRAPH_UNDIRECTED);
  diagram->textures =
      arena_calloc(arena, ctx.textures.N, sizeof(cairo_surface_t *));
  for (int v = 0; v < SCAST(ctx.textures.N); v++) {
    diagram->textures[v] = ctx.textures.array[v];
  }

  for (int eid = 0; eid < SCAST(ctx.links.N); eid++) {
    if (ctx.links.array[eid].v < 0 ||
        ctx.links.array[eid].v >= SCAST(ctx.textures.N)) {
      arena_free(&local_arena);
      diagram->errmsg = arena_sprintf(arena, "Unknown identifier %ld",
                                      ctx.links.array[eid].v);
      return DIAGRAM_EUNKIDEN;
    }

    if (ctx.links.array[eid].w < 0 ||
        ctx.links.array[eid].w >= SCAST(ctx.textures.N)) {
      arena_free(&local_arena);
      diagram->errmsg = arena_sprintf(arena, "Unknown identifier %ld",
                                      ctx.links.array[eid].v);
      return DIAGRAM_EUNKIDEN;
    }
    log_info("Linked vertices (%ld) and (%ld)", ctx.links.array[eid].v,
             ctx.links.array[eid].w);
    igraph_add_edge(&diagram->G, ctx.links.array[eid].v,
                    ctx.links.array[eid].w);
  }

  arena_free(&local_arena);

  return DIAGRAM_OK;
}

static cairo_status_t cairo_write_to_stream(void *fp, const uint8_t *data,
                                            uint32_t length) {
  FILE *os = fp;
  if (!os) {
    return CAIRO_STATUS_FILE_NOT_FOUND;
  }
  if (fwrite(data, sizeof(uint8_t), length, os) == length) {
    return CAIRO_STATUS_SUCCESS;
  } else {
    if (ferror(os)) {
      return CAIRO_STATUS_WRITE_ERROR;
    } else {
      return CAIRO_STATUS_SUCCESS;
    }
  }
}

cairo_surface_t *diagram_lay_out(diagram_t *D, FILE *os,
                                 diagram_status_e *err) {
  igraph_matrix_t vmat;
  igraph_matrix_init(&vmat, igraph_vcount(&D->G), 2);

  igraph_t G_augmented;
  igraph_copy(&G_augmented, &D->G);
  hs_error_t hs_err;
  if ((hs_err = harel_sardes_layout(&G_augmented, &vmat)) != HS_OK) {
    switch (hs_err) {
    case HS_EMPTY_GRAPH:
      D->errmsg =
          arena_strdup(D->arena, "Graph induced by this diagram is empty, "
                                 "therefore cannot be renderered");
      *err = DIAGRAM_EEMPTY;
      break;
    case HS_NONPLANAR:
      D->errmsg =
          arena_strdup(D->arena, "Graph induced by this diagram is not planar, "
                                 "therefore cannot be renderered");
      *err = DIAGRAM_ENONPLANAR;
      break;
    case HS_INVALID_INPUT:
      log_fatal("Invalid input was supplied to layout function");
      abort();
      break;
    case HS_DISCONNECTED:
      igraph_vector_int_t membership = veci_new();
      igraph_connected_components(&G_augmented, &membership, 0, 0, 0);
      for (int i = 0; i < vector_size(&membership); i++) {
        log_error("vertex: %ld in component %ld", i, vecget(membership, i));
      }
      D->errmsg = arena_strdup(
          D->arena, "Graph induced by this diagram is not connected");
      *err = DIAGRAM_EDISCONNECTED;
      break;
    default:
      break;
    }
    igraph_destroy(&G_augmented);
    igraph_matrix_destroy(&vmat);
    return 0;
  }
  igraph_destroy(&G_augmented);

  igraph_real_t stretch = 1;
  for (gint v = 0; v < igraph_vcount(&D->G); v++) {
    for (gint w = 0; w < igraph_vcount(&D->G); w++) {
      if (v == w)
        continue;
      igraph_real_t x0 = MATRIX(vmat, v, 0) * stretch,
                    y0 = MATRIX(vmat, v, 1) * stretch,
                    x1 = MATRIX(vmat, w, 0) * stretch,
                    y1 = MATRIX(vmat, w, 1) * stretch;
      // log_debug("P_0=(%f,%f), P_1=(%f,%f)", x0, y0, x1, y1);

      cairo_surface_t *texture1 = D->textures[v];
      int w1 = cairo_image_surface_get_width(texture1) / 2;
      int h1 = cairo_image_surface_get_height(texture1) / 2;

      cairo_surface_t *texture2 = D->textures[v];
      int w2 = cairo_image_surface_get_width(texture2) / 2;
      int h2 = cairo_image_surface_get_height(texture2) / 2;

      // log_debug("%f", stretch);
      // log_debug("Pair %ld %ld", v, w);
      // log_debug("dx=%f, dy=%f", ABS(x0 - x1), ABS(y0 - y1));
      if (ABS(x0 - x1) <= DBL_EPSILON) {
        // log_debug("Vertical %ld %ld", v, w);
        stretch *= MAX((h1 + h2 + MIN_EDGE_LEN) / ABS(y0 - y1), 1);
        // log_debug("%f", stretch);
      } else if (ABS(y0 - y1) <= DBL_EPSILON) {
        // log_debug("Horizontal");
        stretch *= MAX((w1 + w2 + MIN_EDGE_LEN) / ABS(x1 - x0), 1);
        // log_debug("%f", stretch);
      } else {
        double minx = w1 + w2 + MIN_EDGE_LEN;
        double miny = h1 + h2 + MIN_EDGE_LEN;

        double xs = MAX(1, minx / ABS(x0 - x1));
        double ys = MAX(1, miny / ABS(y0 - y1));
        if (xs > ys) {
          stretch *= ys;
        } else {
          stretch *= xs;
        }
      }
    }
  }

  int maxRx = 0, maxRy = 0;
  double minX = DBL_MAX, minY = DBL_MAX, maxX = 0, maxY = 0;
  for (int i = 0; i < igraph_matrix_nrow(&vmat); i++) {
    cairo_surface_t *texture = D->textures[i];

    int w = cairo_image_surface_get_width(texture);
    int h = cairo_image_surface_get_height(texture);
    double x = MATRIX(vmat, i, 0) * stretch;
    double y = MATRIX(vmat, i, 1) * stretch;

    maxRx = MAX(maxRx, w / 2);
    maxRy = MAX(maxRy, h / 2);
    minX = MIN(minX, x);
    minY = MIN(minY, y);
    maxX = MAX(maxX, x);
    maxY = MAX(maxY, y);
  }

  cairo_surface_t *output = cairo_svg_surface_create_for_stream(
      cairo_write_to_stream, os, maxX - minX + 2 * maxRx,
      maxY - minY + 2 * maxRy);

  if (!output) {
    log_fatal("Could not create output stream");
    abort();
  }
  // cairo_surface_t *output = cairo_svg_surface_create(
  //     "image.svg", maxX - minX + 2 * maxRx, maxY - minY + 2 * maxRy);
  cairo_t *cr = cairo_create(output);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width(cr, 2);

  cairo_set_source_rgb(cr, 0, 0, 0);
  for (int i = 0; i < igraph_ecount(&D->G); i++) {
    gint v, w;
    igraph_edge(&D->G, i, &v, &w);

    cairo_move_to(cr, MATRIX(vmat, v, 0) * stretch + maxRx,
                  MATRIX(vmat, v, 1) * stretch + maxRy);
    cairo_line_to(cr, MATRIX(vmat, w, 0) * stretch + maxRx,
                  MATRIX(vmat, w, 1) * stretch + maxRy);
    cairo_stroke(cr);
  }

  for (int i = 0; i < igraph_matrix_nrow(&vmat); i++) {

    cairo_surface_t *texture = D->textures[i];

    int w = cairo_image_surface_get_width(texture);
    int h = cairo_image_surface_get_height(texture);

    double x = MATRIX(vmat, i, 0) * stretch + maxRx;
    double y = MATRIX(vmat, i, 1) * stretch + maxRy;

    cairo_move_to(cr, x, y);
    cairo_set_source_surface(cr, texture, x - ((double)w / 2),
                             y - ((double)h / 2));
    cairo_paint(cr);
  }

  cairo_destroy(cr);
  igraph_matrix_destroy(&vmat);
  return output;
}

static void regex_err_print(regex_t *regex, int err) {
  size_t errbuf_size = regerror(err, regex, (char *)NULL, (size_t)0);
  char *errbuf = ucalloc(errbuf_size, sizeof(char));
  regerror(err, regex, errbuf, errbuf_size);
  log_error("Error regex: %s", errbuf);
  free(errbuf);
}

static bool is_blank(const char *line, int N) {
  for (int i = 0; i < N; i++) {
    if (!isblank(line[i])) {
      return false;
    }
  }

  return true;
}

static diagram_status_e handle_line(const char *contents, size_t start,
                                    size_t end, parsing_context *ctx) {
  size_t len = end - start;
  Arena local_arena = {0};
  char *block = arena_calloc(&local_arena, len + 1, sizeof(char));
  strncpy(block, contents + start, len);
  diagram_status_e status = DIAGRAM_OK;

  if (!strncmp(";", block, 1)) {
    arena_free(&local_arena);
  } else if (STARTS_WITH("link", block)) {
    status = parse_link(ctx, block, len);
  } else if (STARTS_WITH("node", block)) {
    status = parse_node(NODE_NODE, ctx, block, len);
  } else if (STARTS_WITH("entity", block)) {
    status = parse_node(NODE_ENTITY, ctx, block, len);
  } else if (STARTS_WITH("relationship", block)) {
    status = parse_node(NODE_RELATIONSHIP, ctx, block, len);
  } else if (STARTS_WITH("attribute", block)) {
    status = parse_node(NODE_ATTRIBUTE, ctx, block, len);
  } else if (STARTS_WITH(";", block)) {
    status = DIAGRAM_OK;
  } else if (is_blank(block, (int)SCAST(len))) {
    status = DIAGRAM_OK;
  } else {
    ctx->errmsg = arena_sprintf(ctx->arena, "Unknown command: %s", block);
    status = DIAGRAM_ESYNTAX;
  }

  arena_free(&local_arena);

  return status;
}

static int read_line(const char *contents, size_t N, size_t start,
                     size_t *end) {
  if (start >= N)
    return 0;
  size_t i;
  for (i = start; i < N; i++) {
    if (contents[i] == '\n') {
      break;
    }
  }

  *end = i;

  return 1;
}

cairo_surface_t *draw_entity(const char *text) {
  const int padding = 20;
  const int lw = 5;
  const int font_size = 72;

  cairo_surface_t *test =
      cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 10, 10);
  cairo_t *cr = cairo_create(test);
  cairo_set_font_size(cr, font_size);
  cairo_text_extents_t te;
  cairo_font_extents_t fe;
  cairo_text_extents(cr, text, &te);
  cairo_font_extents(cr, &fe);
  cairo_destroy(cr);
  cairo_surface_destroy(test);
  int height = (int)ceil(te.height + fe.descent);
  int width = (int)ceil(te.width);

  cairo_surface_t *surf = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, (int)ceil(width + 2 * padding + 4 * lw),
      (int)ceil(height + 2 * padding + 4 * lw));

  cr = cairo_create(surf);
  cairo_set_line_width(cr, lw);
  cairo_set_font_size(cr, font_size);
  cairo_set_source_rgb(cr, 0, 0, 0);

  // cairo_move_to(cr, lw, lw);
  cairo_rectangle(cr, lw, lw, width + 2 * padding, height + 2 * padding);
  cairo_stroke_preserve(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill(cr);

  cairo_move_to(cr, padding, height + padding + lw);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_show_text(cr, text);
  cairo_destroy(cr);

  return surf;
}

cairo_surface_t *draw_relationship(const char *text) {
  if (!strlen(text)) {
    return 0;
  }
  const int lw = 5;
  const int font_size = 72;

  cairo_surface_t *test = cairo_tmp_svg_surface(1, 1);
  cairo_t *cr = cairo_create(test);
  cairo_set_font_size(cr, font_size);
  cairo_text_extents_t te;
  cairo_font_extents_t fe;
  cairo_text_extents(cr, text, &te);
  cairo_font_extents(cr, &fe);
  cairo_destroy(cr);
  cairo_surface_destroy(test);

  double h = te.height + fe.descent;
  double w = te.width;
  double a = sqrt((w * h) / 8);
  double b = (w * h) / (4 * a);

  double whalf = w / 2;
  double hhalf = h / 2;

  cairo_surface_t *surf = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, (int)ceil(2 * b + w), (int)ceil(2 * a + h));

  cr = cairo_create(surf);
  cairo_translate(cr, b + whalf, a + hhalf);

  cairo_set_line_width(cr, lw);
  cairo_set_font_size(cr, font_size);
  cairo_set_source_rgb(cr, 0, 0, 0);
  // cairo_paint(cr);

  cairo_move_to(cr, -whalf - b, 0);
  cairo_line_to(cr, 0, hhalf + a);
  cairo_line_to(cr, whalf + b, 0);
  cairo_line_to(cr, 0, -hhalf - a);
  cairo_line_to(cr, -whalf - b, 0);
  cairo_stroke_preserve(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill(cr);

  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, -whalf, hhalf);
  cairo_show_text(cr, text);
  cairo_destroy(cr);

  return surf;
}

cairo_surface_t *draw_attribute(const char *text) {
  if (!strlen(text)) {
    return 0;
  }
  const int lw = 5;
  const int font_size = 72;
  const double k = 30;

  cairo_surface_t *test = cairo_tmp_svg_surface(1, 1);
  cairo_t *cr = cairo_create(test);
  cairo_set_font_size(cr, font_size);
  cairo_text_extents_t te;
  cairo_font_extents_t fe;
  cairo_text_extents(cr, text, &te);
  cairo_font_extents(cr, &fe);
  cairo_destroy(cr);
  cairo_surface_destroy(test);

  double h = te.height + fe.descent;
  double w = te.width;
  double whalf = w / 2;
  double hhalf = h / 2;
  double c =
      sqrt((h * h) * (2 * k + w) * (2 * k + w) / (k * (k + w))) / 4 - hhalf;
  double d = ((2 * w * c + w * h) / (4 * sqrt(c * c + h * c))) - whalf;

  double img_width = ceil(2 * d + w + 2 * lw);
  double img_height = ceil(2 * c + h + 2 * lw);
  cairo_surface_t *surf = cairo_image_surface_create(
      CAIRO_FORMAT_ARGB32, (int)img_width, (int)img_height);
  cr = cairo_create(surf);

  cairo_translate(cr, d + whalf + lw, c + hhalf + lw);

  // cairo_set_source_rgb(cr, 1, 1, 1);
  // cairo_paint(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  {
    cairo_matrix_t save_matrix;
    cairo_get_matrix(cr, &save_matrix);
    cairo_scale(cr, d + whalf, c + hhalf);
    cairo_new_path(cr);
    cairo_arc(
        /*cr =*/cr,
        /*xc =*/0,
        /*yc =*/0,
        /*radius =*/1,
        /*angle1 =*/0,
        /*angle2 =*/2 * M_PI);
    cairo_set_matrix(cr, &save_matrix);
  }
  cairo_set_line_width(cr, lw);
  cairo_stroke_preserve(cr);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_fill(cr);

  cairo_set_font_size(cr, font_size);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_move_to(cr, -whalf, hhalf - 2 * lw);
  cairo_show_text(cr, text);
  cairo_destroy(cr);
  return surf;
}

cairo_surface_t *draw_from_file(const char *label) {
  return cairo_image_surface_create_from_png(label);
}

static diagram_status_e parse_node(node_type_e type, parsing_context *ctx,
                                   const char *block, size_t blen) {

  static regex_t node_regexes[NODE_TYPES_COUNT] = {0};
  static bool node_regex_initialized[NODE_TYPES_COUNT] = {0};
  static render_func render_functions[NODE_TYPES_COUNT] = {
      draw_from_file, draw_entity, draw_relationship, draw_attribute};

  int err;
  const int capture_groups = 3;
  regmatch_t matches[capture_groups];
  if (!node_regex_initialized[type]) {
    err = regcomp(&node_regexes[type], node_regex_strings[type], 0);
    node_regex_initialized[type] = true;
  }

  err = regexec(&node_regexes[type], block, 3, matches, 0);
  if (err != REG_NOERROR) {
    ctx->errmsg = arena_sprintf(ctx->arena, "Invalid syntax in %s", block);
    return DIAGRAM_ESYNTAX;
  }

  int id_len = matches[1].rm_eo - matches[1].rm_so;
  int label_len = matches[2].rm_eo - matches[2].rm_so;
  const char *id = block + matches[1].rm_so;
  const char *label = block + matches[2].rm_so;
  id = strndup(id, UCAST(id_len));
  if (zhash_exists(ctx->vertices, id)) {
    ctx->errmsg = arena_sprintf(ctx->arena, "Duplicate node with id %s", id);
    free((char *)id);
    return DIAGRAM_EDUP;
  }
  label = strndup(label, UCAST(label_len));

  log_info("Created %s %s with label %s", node_type_names[type], id, label);
  zhash_set(ctx->vertices, id, SCAST(ctx->textures.N));
  cairo_surface_t *node_surf = render_functions[type](label);
  vertex_list_push(&ctx->textures, node_surf);

  return DIAGRAM_OK;
}

static diagram_status_e parse_link(parsing_context *ctx, const char *block,
                                   size_t blen) {
  static regex_t link_regex;
  static bool regex_initialized = false;
  static const char *link_regex_str =
      "^link \\([[:alnum:]_]\\+\\) and \\([[:alnum:]_]\\+\\)$";

  int err;
  regmatch_t matches[3];
  if (!regex_initialized) {
    err = regcomp(&link_regex, link_regex_str, 0);
    regex_initialized = true;
  }
  err = regexec(&link_regex, block, 3, matches, 0);
  if (err != REG_NOERROR) {
    ctx->errmsg = arena_sprintf(ctx->arena, "Invalid syntax in %s", block);
    return DIAGRAM_ESYNTAX;
  }

  int first_len = matches[1].rm_eo - matches[1].rm_so;
  int second_len = matches[2].rm_eo - matches[2].rm_so;
  char *first = strndup(block + matches[1].rm_so, UCAST(first_len));
  char *second = strndup(block + matches[2].rm_so, UCAST(second_len));

  gint v, w;
  if (!zhash_exists(ctx->vertices, first)) {
    ctx->errmsg = arena_sprintf(ctx->arena, "Unknown identifier %s", first);
    free(first);
    free(second);
    return DIAGRAM_EUNKIDEN;
  }
  if (!zhash_exists(ctx->vertices, second)) {
    ctx->errmsg = arena_sprintf(ctx->arena, "Unknown identifier %s", second);
    free(first);
    free(second);
    return DIAGRAM_EUNKIDEN;
  }

  v = zhash_get(ctx->vertices, first);
  w = zhash_get(ctx->vertices, second);

  node_link_t link = {v, w};
  link_list_push(&ctx->links, link);
  free(first);
  free(second);
  return DIAGRAM_OK;
}
