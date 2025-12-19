#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include <cilk/cilk.h>

// Coarsening threshold: execute serially for n < 25 to reduce spawn overhead
// This threshold was determined empirically to balance parallelism and overhead
#define COARSENING_THRESHOLD 25

int64_t fib(int64_t n) {
    if (n < 2) return n;
    int64_t x, y;
    if (n < COARSENING_THRESHOLD) {
        // Serial execution for small n (coarsening)
        // This reduces spawn overhead by doing more work serially
        x = fib(n - 1);
        y = fib(n - 2);
    }
    else {
        // Parallel execution for large n
        x = cilk_spawn fib(n - 1);
        y = cilk_spawn fib(n - 2);
        cilk_sync;
    }

    return (x + y);
}

int main(int argc, char* argv[]) {
    int64_t n = atoi(argv[1]);
    int64_t result;

    result = fib(n);
    printf("Fibonacci of %" PRId64 " is %" PRId64 ".\n", n, result);
}
