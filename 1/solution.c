#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

/**
 * A function, called from inside of coroutines recursively. Just to demonstrate
 * the example. You can split your code into multiple functions, that usually
 * helps to keep the individual code blocks simple.
 */
static void
other_function(const char *name, int depth) {
  printf("%s: entered function, depth = %d\n", name, depth);
  coro_yield();
  if (depth < 3)
    other_function(name, depth + 1);
}

int get_rnd() {
  return rand();
}

void swap(int *a, int *b) {
  int c = *a;
  *a = *b;
  *b = c;
}

int *
quick_sort(int *a, int sz) {
  if (sz <= 1) {
    return a;
  }
  int pivot = get_rnd() % sz;
  int hi = sz - 1;
  int lo = 0;

  for (; hi > pivot || lo < pivot;) {
    for (; hi > pivot && a[hi] >= a[pivot];)
      --hi;
    for (; lo < pivot && a[lo] <= a[pivot];)
      ++lo;
    if (hi > pivot) {
      if (lo < pivot) {
        swap(&a[lo], &a[hi]);
        ++lo;
        --hi;
      } else {
        swap(&a[pivot], &a[pivot + 1]);
        if (hi != pivot + 1) {
          swap(&a[hi], &a[pivot]);
        }
        ++pivot;
      }
    } else if (lo < pivot) {
      swap(&a[pivot], &a[pivot - 1]);
      if (lo != pivot - 1) {
        swap(&a[lo], &a[pivot]);
      }
      --pivot;
    }
  }

  coro_yield();
  quick_sort(a, pivot);
  coro_yield();
  quick_sort(a + pivot, sz - pivot);
  return a;
}

struct sorting_ctx {
  int *data;
  int sz;
  int step;
  char *name;
};

void delete_ctx(struct sorting_ctx *ctx) {
  free(ctx->data);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context) {
  /* IMPLEMENT SORTING OF INDIVIDUAL FILES HERE. */

  struct coro *this = coro_this();
  char *name = context;
  printf("Started coroutine %s\n", name);
  printf("%s: switch count %lld\n", name, coro_switch_count(this));
  printf("%s: yield\n", name);
  coro_yield();

  printf("%s: switch count %lld\n", name, coro_switch_count(this));
  printf("%s: yield\n", name);
  coro_yield();

  printf("%s: switch count %lld\n", name, coro_switch_count(this));
  other_function(name, 1);
  printf("%s: switch count after other function %lld\n", name,
         coro_switch_count(this));
  free(name);
  /* This will be returned from coro_status(). */
  return 0;
}

static int
coroutine_sort_f(void *context) {
  struct sorting_ctx *ctx = (struct sorting_ctx *) context;
  FILE *in = fopen(ctx->name, "r");

  int actual_size = 8;
  ctx->data = calloc(sizeof(int), actual_size);
  ctx->sz = 0;
  int num;

  while (fscanf(in, "%d", &num) > 0) {
    ctx->sz++;
    if (ctx->sz > actual_size) {
      actual_size *= 2;
      ctx->data = realloc(ctx->data, sizeof(int) * actual_size);
      if (ctx->data == NULL)
        exit(EXIT_FAILURE);
    }
    ctx->data[ctx->sz - 1] = num;
  }
  memset(ctx->data + ctx->sz, 0, sizeof(int) * (actual_size - ctx->sz));

  ctx->data = quick_sort(ctx->data, ctx->sz);
  fclose(in);
  return ctx->sz;
}

int
main(int argc, char **argv) {
  /* Initialize our coroutine global cooperative scheduler. */
  coro_sched_init();

  struct sorting_ctx *ctxs = malloc(sizeof(struct sorting_ctx) * (argc - 1));

  for (int i = 0; i < argc - 1; ++i) {
    ctxs[i].data = NULL;
    ctxs[i].sz = 0;
    ctxs[i].name = argv[i + 1];
    ctxs[i].step = 0;
    coro_new(coroutine_sort_f, (void *) &ctxs[i]);
  }

  /* Wait for all the coroutines to end. */
  struct coro *c;
  while ((c = coro_sched_wait()) != NULL) {
    /*
     * Each 'wait' returns a finished coroutine with which you can
     * do anything you want. Like check its exit status, for
     * example. Don't forget to free the coroutine afterwards.
     */
    printf("Finished %d\n", coro_status(c));
    printf("Switched: %lld\n", coro_switch_count(c));
    coro_delete(c);
  }
  /* All coroutines have finished. */

  /* IMPLEMENT MERGING OF THE SORTED ARRAYS HERE. */
  int common_size = 0;
  for (int i = 0; i < argc - 1; ++i) {
    common_size += ctxs[i].sz;
  }
  FILE *output = fopen("result.txt", "w");

  for (int i = 0; i < common_size; ++i) {
    int min_num = -1;
    for (int j = 0; j < argc - 1; ++j) {
      if (ctxs[j].step == ctxs[j].sz) continue;
      if (min_num == -1 || ctxs[j].data[ctxs[j].step] < ctxs[min_num].data[ctxs[min_num].step]) {
        min_num = j;
      }
    }
    fprintf(output, "%d ", ctxs[min_num].data[ctxs[min_num].step++]);
  }
  for (int i = 0; i < argc - 1; ++i) {
    delete_ctx(&ctxs[i]);
  }
  free(ctxs);
  fclose(output);
  return 0;
}
