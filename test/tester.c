#include "tester.h"
#include "dynarray.h"
#include "log.h"
#include "utils.h"
#include <bits/time.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

static Arena tester_arena = {0};
static bool initialized = false;
static volatile int tests_passed = 0;
static volatile int tests_failed = 0;

// static logger_state lg;

DEFINE_VECTOR(testcases, testcase_t)

static testcases_t testcases;

void do_backtrace(pid_t child);

void register_test(const char *name, tester_func function) {
  if (!initialized) {
    testcases_init(&testcases, &tester_arena);
    initialized = true;
  }
  testcase_t testcase = {
      arena_strdup(&tester_arena, name), function, 0, 0, STDOUT_FILENO, false};

  testcases_push(&testcases, testcase);
}

void run_tests(void) {
  for (int i = 0; i < testcases.N; i++) {
    run_test(&testcases.array[i], 0, 0);
  }

  printf("\n=== Test Summary ===\n");
  printf(GREEN "Passed: %d\n" RESET, tests_passed);
  printf(RED "Failed: %d\n" RESET, tests_failed);
}

void tester_log(testcase_state_t *state, const char *fmt, ...) {
  va_list va;
  va_start(va, fmt);
  // Length with \0 character
  vdprintf(state->write_pipe, fmt, va);
  va_end(va);
}

static void print_test_log(int log_fd, const char *name) {
  char buffer[BUFSIZ + 1] = {0};
  ssize_t len;
  printf("[" BLUE "LOG" RESET "]: Log of %s\n", name);
  printf("================================\n\n");
  while ((len = read(log_fd, buffer, BUFSIZ)) > 0) {
    write(STDOUT_FILENO, buffer, (size_t)len);
  }
  printf("================================\n\n");
}

void run_test(testcase_t *testcase, void *data, size_t datalen) {
  int pid, status;
  int log_pipe_fd[2];
  struct timespec begin = {0}, end = {0};
  clock_gettime(CLOCK_MONOTONIC, &begin);

  if (pipe(log_pipe_fd) == -1) {
    perror("pipe failed");
    exit(EXIT_FAILURE);
  }

  if ((pid = fork()) < 0) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }

  if (pid == 0) {
    if (!testcase->silent)
      printf(EXECUTING_MSG, testcase->name);
    testcase_state_t state = {log_pipe_fd[1]};

    close(log_pipe_fd[0]);
    if (dup2(log_pipe_fd[1], STDOUT_FILENO) < 0) {
      perror("dup2 (stdout)");
      exit(EXIT_FAILURE);
    }

    if (dup2(log_pipe_fd[1], STDERR_FILENO) < 0) {
      perror("dup2 (stderr)");
      exit(EXIT_FAILURE);
    }

    testcase->func(&state, data, datalen);
    close(log_pipe_fd[1]);

    exit(EXIT_SUCCESS);
  } else {

    waitpid(pid, &status, 0);
    close(log_pipe_fd[1]);
    clock_gettime(CLOCK_MONOTONIC, &end);

    if (WIFSIGNALED(status)) {
      if (WTERMSIG(status) == SIGSEGV) {
        printf(SEGV_MSG, testcase->name);
      } else {
        printf(SIGNAL_MSG, testcase->name, WTERMSIG(status),
               WEXITSTATUS(status));
      }
      tests_failed++;
      print_test_log(log_pipe_fd[0], testcase->name);
      close(log_pipe_fd[0]);
      exit(EXIT_FAILURE);
    } else if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
      tests_passed++;
      if (!testcase->silent)
      {
        printf(TEST_PASS_MSG, testcase->name,
               1000.0 * (double)(end.tv_sec - begin.tv_sec) +
               (double)(end.tv_nsec - begin.tv_nsec) / 1e6);
      }
      close(log_pipe_fd[0]);
    } else {
      tests_failed++;
      printf(FAIL_PREFIX"\n");
      print_test_log(log_pipe_fd[0], testcase->name);
      close(log_pipe_fd[0]);
      exit(EXIT_FAILURE);
    }
  }
}

#define BACKTRACE_SIZ 64

void do_backtrace(pid_t pid) {
  void *array[BACKTRACE_SIZ];
  int i;
  int size;
  char **strings;

  ptrace(PTRACE_ATTACH, pid);
  size = backtrace(array, BACKTRACE_SIZ);
  strings = backtrace_symbols(array, size);
  ptrace(PTRACE_DETACH, pid);

  for (i = 0; i < size; i++) {
    printf("%s\n", strings[i]);
  }

  free(strings); // malloced by backtrace_symbols
}
