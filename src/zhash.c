#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "./zhash.h"
#include "graph_utils.h"

// helper macros and functions, declarations
#define ZCOUNT_OF(arr) (sizeof(arr) / sizeof(*arr))

static struct ZHashEntry *zcreate_entry(Arena* arena, char *key, long val);
static size_t zgenerate_hash(struct ZHashTable *hash_table, char *key);
static void zhash_rehash(struct ZHashTable *hash_table, size_t size_index);
static size_t znext_size_index(size_t size_index);
static size_t zprevious_size_index(size_t size_index);
static struct ZHashTable *zcreate_hash_table_with_size(Arena* arena, size_t size_index);

// possible sizes for hash table; must be prime numbers
static const size_t hash_sizes[] = {
  53, 101, 211, 503, 1553, 3407, 6803, 12503, 25013, 50261,
  104729, 250007, 500009, 1000003, 2000029, 4000037, 10000019,
  25000009, 50000047, 104395301, 217645177, 512927357, 1000000007
};

// functions declared in zhash.h
struct ZHashTable *zcreate_hash_table(Arena* arena)
{
  return zcreate_hash_table_with_size(arena,0);
}

void zhash_set(struct ZHashTable *hash_table, const char *key, long val)
{
  size_t size, hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry) {
    if (strcmp(key, entry->key) == 0) {
      entry->val = val;
      return;
    }
    entry = entry->next;
  }

  entry = zcreate_entry(hash_table->arena, key, val);

  entry->next = hash_table->entries[hash];
  hash_table->entries[hash] = entry;
  hash_table->entry_count++;

  size = hash_sizes[hash_table->size_index];

  if (hash_table->entry_count > size / 2) {
    zhash_rehash(hash_table, znext_size_index(hash_table->size_index));
  }
}

long zhash_get(struct ZHashTable *hash_table, const char *key)
{
  size_t hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry && strcmp(key, entry->key) != 0) entry = entry->next;

  return entry ? entry->val : 0;
}

long zhash_delete(struct ZHashTable *hash_table, char *key)
{
  size_t size, hash;
  struct ZHashEntry *entry;
  long val;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  if (entry && strcmp(key, entry->key) == 0) {
    hash_table->entries[hash] = entry->next;
  } else {
    while (entry) {
      if (entry->next && strcmp(key, entry->next->key) == 0) {
        struct ZHashEntry *deleted_entry;

        deleted_entry = entry->next;
        entry->next = entry->next->next;
        entry = deleted_entry;
        break;
      }
      entry = entry->next;
    }
  }

  if (!entry) return 0;

  val = entry->val;
  hash_table->entry_count--;

  size = hash_sizes[hash_table->size_index];

  if (hash_table->entry_count < size / 8) {
    zhash_rehash(hash_table, zprevious_size_index(hash_table->size_index));
  }

  return val;
}

bool zhash_exists(struct ZHashTable *hash_table, const char *key)
{
  size_t hash;
  struct ZHashEntry *entry;

  hash = zgenerate_hash(hash_table, key);
  entry = hash_table->entries[hash];

  while (entry && strcmp(key, entry->key) != 0) entry = entry->next;

  return entry ? true : false;
}

// helper functions, definitions
static struct ZHashTable *zcreate_hash_table_with_size(Arena* arena, size_t size_index)
{
  struct ZHashTable *hash_table;

  hash_table = (struct ZHashTable *) arena_calloc(arena, 1, sizeof(struct ZHashTable));

  hash_table->size_index = size_index;
  hash_table->entry_count = 0;
  hash_table->entries = arena_calloc(arena, hash_sizes[size_index], sizeof(void*));
  hash_table->arena = arena;

  return hash_table;
}

static struct ZHashEntry *zcreate_entry(Arena* arena, char *key, long val)
{
  struct ZHashEntry *entry;
  char *key_cpy;

  key_cpy = (char *) arena_calloc(arena, (strlen(key)+1), sizeof(char)); 
  entry = (struct ZHashEntry *) arena_calloc(arena, 1, sizeof(struct ZHashEntry));

  strcpy(key_cpy, key);
  entry->key = key_cpy;
  entry->val = val;

  return entry;
}

static size_t zgenerate_hash(struct ZHashTable *hash_table, char *key)
{
  size_t size, hash;
  char ch;

  size = hash_sizes[hash_table->size_index];
  hash = 0;

  while ((ch = *key++)) hash = (17 * hash + ch) % size;

  return hash;
}

static void zhash_rehash(struct ZHashTable *hash_table, size_t size_index)
{
  size_t hash, size, ii;
  struct ZHashEntry **entries;

  if (size_index == hash_table->size_index) return;

  size = hash_sizes[hash_table->size_index];
  entries = hash_table->entries;

  hash_table->size_index = size_index;
  hash_table->entries = arena_calloc(hash_table->arena, hash_sizes[size_index], sizeof(void*));

  for (ii = 0; ii < size; ii++) {
    struct ZHashEntry *entry;

    entry = entries[ii];
    while (entry) {
      struct ZHashEntry *next_entry;

      hash = zgenerate_hash(hash_table, entry->key);
      next_entry = entry->next;
      entry->next = hash_table->entries[hash];
      hash_table->entries[hash] = entry;

      entry = next_entry;
    }
  }
}

static size_t znext_size_index(size_t size_index)
{
  if (size_index == ZCOUNT_OF(hash_sizes)) return size_index;

  return size_index + 1;
}

static size_t zprevious_size_index(size_t size_index)
{
  if (size_index == 0) return size_index;

  return size_index - 1;
}




