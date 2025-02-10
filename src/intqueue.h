#pragma once

#include "arena.h"
#include "graph_utils.h"
#include <stdbool.h>

typedef struct intqueue_t intqueue_t;

typedef struct {
  gint head;
  gint tail;
  bool present;
} intqueue_entry_t;

struct intqueue_t {
  intqueue_entry_t *queue;
  gint cap;
  gint N;
};

#define INT_QUEUE(q) ((q)->queue)

void intqueue_init(intqueue_t *queue, Arena *arena, gint N);

void intqueue_enqueue(intqueue_t *queue, gint value);

gint intqueue_dequeue(intqueue_t *queue);

gint intqueue_pull(intqueue_t *queue, gint value);

void intqueue_print(intqueue_t *queue);

gint intqueue_peek(intqueue_t *queue);

gint intqueue_rpeek(intqueue_t *queue);

intqueue_entry_t* intqueue_get(intqueue_t* queue, gint i);
