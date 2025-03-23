#pragma once

#include "arena.h"
#include "collections.h"
#include "log.h"
#include <stdlib.h>

#define RED "\x1B[31m"
#define BLUE "\e[0;94m"
#define GREEN "\x1B[32m"
#define RESET "\x1B[0m"

#define FAIL_PREFIX "[" RED "FAIL" RESET "]"
#define FAIL_PREFIX_DEBUG FAIL_PREFIX "%s:%d: "
#define TEST_PASS_MSG                                                          \
  "[" GREEN "PASS" RESET "] Test %s " GREEN "PASSED" RESET " in [%.3lf]ms!\n"
#define EXECUTING_MSG "[" BLUE "RUNNING" RESET "] Running test %s\n"
#define SEGV_MSG FAIL_PREFIX " Test %s failed due to segmentation fault!\n"
#define SIGNAL_MSG                                                             \
  FAIL_PREFIX " Test %s failed with signal" RED " %d " RESET "and status " RED \
              "%d" RESET "\n"

#define ASSERT_EQ_FORMATTER(value)                                             \
  _Generic((value),                                                            \
      _Bool: FAIL_PREFIX_DEBUG "Expected [%hhd] got [%hhd]\n",                 \
      char: FAIL_PREFIX_DEBUG "Expected [%hhd] got [%hhd]\n",                  \
      unsigned char: FAIL_PREFIX_DEBUG "Expected [%hhu] got [%hhu]\n",         \
      short: FAIL_PREFIX_DEBUG "Expected [%hd] got [%hd]\n",                   \
      unsigned short: FAIL_PREFIX_DEBUG "Expected [%hu] got [%hu]\n",          \
      int: FAIL_PREFIX_DEBUG "Expected [%d] got [%d]\n",                       \
      unsigned int: FAIL_PREFIX_DEBUG "Expected [%u] got [%u]\n",              \
      long: FAIL_PREFIX_DEBUG "Expected [%ld] got [%ld]\n",                    \
      unsigned long: FAIL_PREFIX_DEBUG "Expected [%lu] got [%lu]\n",           \
      long long: FAIL_PREFIX_DEBUG "Expected [%lld] got [%lld]\n",             \
      unsigned long long: FAIL_PREFIX_DEBUG "Expected [%llu] got [%llu]\n",    \
      default: -6666)

typedef struct {
  int write_pipe;
} testcase_state_t;
typedef void (*tester_func)(testcase_state_t *state, void *data,
                            size_t datalen);

typedef struct {
  const char *name;
  tester_func func;
  int logger_fd;
  int passed_subtests;
  int failed_subtests;
  bool silent;
} testcase_t;

#define Test(test_name)                                                        \
  void test_name(testcase_state_t *state, void *data, size_t datalen);         \
  __attribute__((constructor)) void register_##test_name(void) {               \
    register_test(#test_name, test_name);                                      \
  }                                                                            \
  void test_name(testcase_state_t *state, void *data, size_t datalen)

#define assert_eq(actual, expected)                                            \
  do {                                                                         \
    if ((actual) != (expected)) {                                              \
      printf(ASSERT_EQ_FORMATTER(actual), __FILE__, __LINE__,                  \
             (__typeof__(actual))expected, actual);                            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

void register_test(const char *name, tester_func function);

void run_test(testcase_t *testcase, void *data, size_t len);

void run_tests(void);

void tester_log(testcase_state_t *state, const char *fmt, ...);
