#include "intqueue.h"
#include "log.h"
#include "utils.h"
#include <stdlib.h>
#define STRICT_BOUNDS


#define QUEUE_TAIL(q) ((q)->queue[(q)->cap + 1].head)
#define QUEUE_HEAD(q) ((q)->queue[0].tail)

void intqueue_init(intqueue_t *queue, Arena *arena, gint N) {
  assert(queue && "Queue is null");
  assert(arena && "Arena is null");
  assert(N > 0 && "Queue size must be positive");

  intqueue_entry_t *array =
      arena_calloc(arena, UCAST(N) + 2, sizeof(intqueue_entry_t));
  assert(array && "Buy more RAM!");

  queue->queue = array;
  queue->cap = N;
  queue->N = 0;
  queue->queue[N + 1] = (intqueue_entry_t){0, -1, false};
  queue->queue[0] = (intqueue_entry_t){-1, N + 1, false};
}

void intqueue_enqueue(intqueue_t *queue, gint value) {
  assert(queue && "Queue is null");
  assert(queue->cap + 2 > value && "Index out out bounds");
#ifndef STRICT_BOUNDS
  if (INT_QUEUE(queue)[value].present) {
    return;
  }
#else
  if (queue->queue[value + 1].present) {
    log_error("Value %ld is already present", value);
    abort();
  }
  assert(!INT_QUEUE(queue)[value + 1].present && "Value is already present");
#endif

  gint tail = QUEUE_TAIL(queue);
  queue->queue[value + 1].head = tail;
  queue->queue[value + 1].tail = queue->cap + 1;
  queue->queue[value + 1].present = true;
  queue->queue[queue->cap + 1].head = value + 1;
  queue->queue[tail].tail = value + 1;
  queue->N++;
}

gint intqueue_dequeue(intqueue_t *queue) {
  assert(queue && "Queue is null");
#ifdef STRICT_BOUNDS
  // assert(queue->queue[QUEUE_HEAD(queue)].present && "Not present");
  assert(queue->N && "Queue is empty");
#else
  if (queue->N)
    return -1;
#endif
  gint output = QUEUE_HEAD(queue) - 1;

  return intqueue_pull(queue, output);
}

gint intqueue_pull(intqueue_t *queue, gint value) {
  assert(queue->cap + 2 > value + 1 && "Index out out bounds");
#ifdef STRICT_BOUNDS
  gint index = value + 1;

  if (!queue->queue[index].present) {
    log_debug("Element %ld is not present", value);
    abort();
  }
  // assert(queue->queue[value + 1].present && "Element is not present");
#else
  if (!INT_QUEUE(queue)[value].present)
    return -1;
#endif

  queue->queue[index].present = false;
  queue->N--;

  gint headlink = INT_QUEUE(queue)[index].head,
       taillink = INT_QUEUE(queue)[index].tail;

  queue->queue[index].tail = queue->queue[index].head = -1;
  queue->queue[taillink].head = headlink;
  queue->queue[headlink].tail = taillink;

  return value;
}

void intqueue_print(intqueue_t *queue) {
  log_debug("head=%ld, tail=%ld", intqueue_peek(queue),
            intqueue_rpeek(queue));
  for (gint cur = QUEUE_HEAD(queue); cur != queue->cap + 1;
       cur = queue->queue[cur].tail) {
    if (!queue->queue[cur].present) {
      log_error("queue corruption at element %ld", cur - 1);
    }
    log_debug("queue=%ld", cur - 1);
  }
}

gint intqueue_peek(intqueue_t *queue) { return queue->queue[0].tail - 1; }

gint intqueue_rpeek(intqueue_t *queue) {
  return queue->queue[queue->cap + 1].head - 1;
}


intqueue_entry_t* intqueue_get(intqueue_t* queue, gint i) {
  return &queue->queue[i + 1];
}
