#pragma once
#include "cairo.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *string;
  size_t length;
} string;

#define ufree(ptr)                                                             \
  free(ptr);                                                                   \
  ptr = 0;

static inline double maxd(double a, double b) { return a > b ? a : b; }
static inline double mind(double a, double b) { return a < b ? a : b; }

static inline int maxi(int a, int b) { return a > b ? a : b; }
static inline int mini(int a, int b) { return a < b ? a : b; }

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(a) ((a) < 0 ? -(a) : (a))
// #define assize(n) UCAST((long)n)

#define UCAST(n) _Generic((n), long: long_to_ulong, int: int_to_uint)(n)
#define SCAST(n)                                                               \
  _Generic((n), uint64_t: ulong_to_long, uint32_t: uint_to_int)(n)
unsigned long long_to_ulong(long l);
unsigned int int_to_uint(int i);
long ulong_to_long(unsigned long l);
int uint_to_int(unsigned int l);

#define TODO(msg) assert(0 && "Not implemented yet: " #msg)

static inline string *str_from_cstr(const char *str) {
  size_t l = strlen(str);
  char *buf = calloc(l + 1, 1);

  string *s = calloc(1, sizeof(*s));
  s->string = buf;
  s->length = l;

  return s;
}

static inline void str_destroy(string **str) { ufree(*str); }

static inline void *ucalloc(size_t nmem, size_t size) {
  void *ptr = calloc(nmem, size);
  assert(ptr && "Buy more RAM lol");

  return ptr;
}

cairo_surface_t* cairo_tmp_svg_surface(double width, double height);
