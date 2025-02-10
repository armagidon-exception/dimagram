#pragma once

#include "renderer.h"

typedef enum { ERD_ETYPE_S, ERD_ETYPE_W, ERD_ETYPE_A } erd_etype_e;

typedef enum {
  ERD_ATYPE_P,
  ERD_ATYPE_N,
  ERD_ATYPE_D,
  ERD_ATYPE_COMPOSITE
} erd_atype_e;

typedef enum { ERD_RTYPE_S, ERD_RTYPE_I } erd_rtype_e;

node_t* create_entity_node(const char* title);
