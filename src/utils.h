#include "igraph_error.h"
#include <assert.h>
#include <igraph.h>
#include <stdio.h>

static inline void igThrowIfError(igraph_error_t err) {
  if (!err)
    return;

  fprintf(stderr, "igraph returned with error: %s", igraph_strerror(err));
  assert(!err && "Igraph error");
}
