# Homework 4: Multicore Programming with Cilk

This homework focuses on parallel programming using Cilk, exploring parallelization techniques, reducers, and performance analysis.

## Overview

Homework 4 covers:
- **Cilk Programming**: Parallelizing programs using `cilk_spawn` and `cilk_sync`
- **Reducer Hyperobjects**: Using Cilk reducers for thread-safe reductions
- **Performance Analysis**: Measuring speedup with different numbers of workers
- **Matrix Operations**: Parallelizing matrix transpose operations

## Setup

### Compiler
- **Cilk Compiler**: `/opt/opencilk-2/bin/clang` with `-fopencilk` flag
- **OpenMP**: `gcc` with `-fopenmp` flag (for recitation exercises)

### Environment Variables
```bash
# Set number of Cilk workers (for parallel execution)
export CILK_NWORKERS=8

# Or set for single execution with timing
{ time CILK_NWORKERS=8 ./fib 45; }
```

### Timing the Program
```bash
# Method 1: Using shell's time command (works in zsh/bash)
{ time CILK_NWORKERS=1 ./fib 45; }
{ time CILK_NWORKERS=4 ./fib 45; }
{ time CILK_NWORKERS=8 ./fib 45; }

# Method 2: Using the timing script
./time_fib.sh 1 45
./time_fib.sh 4 45
./time_fib.sh 8 45
```

### Building
```bash
cd hw4/recitation
make PARALLEL=1 fib      # Build with Cilk
make fib                  # Build serial version
```

## Recitation Exercises

### Files
- `fib.c` - Fibonacci calculation with Cilk parallelization
- `transpose.c` - Matrix transpose with OpenMP
- `qsort.c` - Quick sort (to be implemented)
- `OMP_test.c` - OpenMP test program
- `Makefile` - Build configuration

### Completed Tasks

#### ✅ Task 1: Serial Fibonacci Timing
- **Status**: Completed
- **Description**: Compiled and timed the serial `fib` program for `fib(45)`
- **Results**: 
  - Initial timing completed
  - Verified correct output: `Fibonacci of 45 is 1134903170`

#### ✅ Task 2: Parallelize Fibonacci with Cilk
- **Status**: Completed
- **Description**: Converted `fib.c` from OpenMP to Cilk using `cilk_spawn` and `cilk_sync`
- **Changes Made**:
  - Replaced `#include "omp.h"` with `#include <cilk/cilk.h>`
  - Replaced `#pragma omp task` with `cilk_spawn`
  - Replaced `#pragma omp taskwait` with `cilk_sync`
  - Removed OpenMP-specific code (omp_set_num_threads, parallel/master blocks)
- **Makefile Updates**:
  - Uses `/opt/opencilk-2/bin/clang` when `PARALLEL=1`
  - Compiles with `-fopencilk` flag
- **Verification**: Program runs correctly with 1, 4, and 8 Cilk workers

#### ✅ Checkoff Item 1: Parallelize fib and Report Runtime
- **Status**: Completed
- **Description**: Parallelize fib using `cilk_spawn` and `cilk_sync`, report runtime with 1, 4, and 8 Cilk workers
- **Results**: See Performance Results section below

### Pending Tasks

#### ⏳ Checkoff Item 2: Coarsening
- **Status**: Not Started
- **Description**: Implement coarsening to reduce spawn overhead

#### ⏳ Checkoff Item 3: Matrix Transpose
- **Status**: Not Started
- **Description**: Parallelize matrix transpose operation

#### ⏳ Homework: Reducer Hyperobjects
- **Status**: Not Started
- **Description**: Implement and test Cilk reducers for various operations

## Performance Results

### Fibonacci (fib 45)

| Workers | Real Time | User Time | Sys Time | CPU Usage | Speedup | Status |
|---------|-----------|-----------|----------|-----------|---------|--------|
| 1       | 22.100s   | 22.00s    | 0.00s    | 99%       | 1.0x    | ✅     |
| 4       | 8.338s    | 33.14s    | 0.00s    | 397%      | 2.65x   | ✅     |
| 8       | 7.796s    | 54.37s    | 0.20s    | 700%      | 2.83x   | ✅     |

**Observations:**
- **1 worker**: Serial execution, ~22 seconds wall time
- **4 workers**: 2.65x speedup, using ~4 cores (397% CPU)
- **8 workers**: 2.83x speedup, using ~7 cores (700% CPU)
- **Efficiency**: 
  - 4 workers: 66% efficiency (2.65x / 4 = 0.66)
  - 8 workers: 35% efficiency (2.83x / 8 = 0.35)
- **Note**: Speedup is limited due to spawn overhead and load balancing. The user time increases with more workers due to parallel overhead, but real time decreases.

**Test Command:**
```bash
{ time CILK_NWORKERS=1 ./fib 45; }
{ time CILK_NWORKERS=4 ./fib 45; }
{ time CILK_NWORKERS=8 ./fib 45; }
```

## Code Structure

### fib.c
```c
#include <cilk/cilk.h>

int64_t fib(int64_t n) {
    if (n < 2) return n;
    int64_t x, y;
    if (n < 19) {
        // Serial execution for small n
        x = fib(n - 1);
        y = fib(n - 2);
    } else {
        // Parallel execution for large n
        x = cilk_spawn fib(n - 1);
        y = cilk_spawn fib(n - 2);
        cilk_sync;
    }
    return (x + y);
}
```

### Key Features
- **Coarsening**: Uses serial execution for `n < 19` to reduce spawn overhead
- **Parallel Spawn**: Uses `cilk_spawn` for parallel recursive calls
- **Synchronization**: Uses `cilk_sync` to wait for spawned tasks

## Notes

- The original code used OpenMP, which was converted to Cilk for this homework
- Cilk workers are controlled via `CILK_NWORKERS` environment variable
- The Makefile automatically switches to Cilk compiler when `PARALLEL=1` is set
- Build artifacts (`.cflags`, binaries) are excluded from git

## References

- [Homework 4 PDF](hw4.pdf)
- [Cilk Documentation](https://opencilk.org/)
- [OpenCilk Installation](/opt/opencilk-2/)

---

**Last Updated**: 2025-12-19
**Status**: Checkoff Item 1 Completed ✅ - Next: Checkoff Item 2 (Coarsening)

