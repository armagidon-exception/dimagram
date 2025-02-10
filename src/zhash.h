#ifndef ZHASH_H
#define ZHASH_H

#include "graph_utils.h"
#include <stdbool.h>
#include <stddef.h>

// hash table
// keys are strings
// values are void *pointers

#define ZCOUNT_OF(arr) (sizeof(arr) / sizeof(*arr))

// struct representing an entry in the hash table
struct ZHashEntry {
  char *key;
  long val;
  struct ZHashEntry *next;
};

// struct representing the hash table
// size_index is an index into the hash_sizes array in hash.c
struct ZHashTable {
  size_t size_index;
  size_t entry_count;
  struct ZHashEntry **entries;
  Arena *arena;
};

// hash table creation and destruction
struct ZHashTable *zcreate_hash_table(Arena *arena);

// hash table operations
void zhash_set(struct ZHashTable *hash_table, const char *key, long val);
long zhash_get(struct ZHashTable *hash_table, const char *key);
long hash_delete(struct ZHashTable *hash_table, const char *key);
bool zhash_exists(struct ZHashTable *hash_table, const char *key);

#endif
