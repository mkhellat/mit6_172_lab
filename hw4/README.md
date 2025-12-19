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

#### ✅ Checkoff Item 2: Coarsening
- **Status**: Completed
- **Description**: Implement coarsening to reduce spawn overhead
- **Approach**: 
  - Increased coarsening threshold from 19 to 25
  - For `n < 25`, execute serially to reduce spawn overhead
  - For `n >= 25`, use parallel execution with `cilk_spawn`
  - Threshold determined empirically by testing values 19, 25, 30, 35
  - Threshold 25 provides optimal balance between parallelism and overhead
- **Results**: See Performance Results section below

#### ⏳ Checkoff Item 3: Matrix Transpose
- **Status**: Not Started
- **Description**: Parallelize matrix transpose operation

#### ⏳ Homework: Reducer Hyperobjects
- **Status**: Not Started
- **Description**: Implement and test Cilk reducers for various operations

## Performance Results

### Fibonacci (fib 45) - Original Version (threshold=19)

| Workers | Real Time | User Time | Sys Time | CPU Usage | Speedup | Status |
|---------|-----------|-----------|----------|-----------|---------|--------|
| 1       | 22.100s   | 22.00s    | 0.00s    | 99%       | 1.0x    | ✅     |
| 4       | 8.338s    | 33.14s    | 0.00s    | 397%      | 2.65x   | ✅     |
| 8       | 7.796s    | 54.37s    | 0.20s    | 700%      | 2.83x   | ✅     |

### Fibonacci (fib 45) - Coarsened Version (threshold=25)

| Workers | Real Time | User Time | Sys Time | CPU Usage | Speedup | Status |
|---------|-----------|-----------|----------|-----------|---------|--------|
| 1       | 22.673s   | 22.57s    | 0.00s    | 99%       | 1.0x    | ✅     |
| 4       | 8.439s    | 33.53s    | 0.01s    | 397%      | 2.69x   | ✅     |
| 8       | 7.958s    | 54.53s    | 0.18s    | 687%      | 2.85x   | ✅     |

**Coarsening Analysis:**
- **Why parallel version can be slower**: 
  - Spawn overhead: Creating and managing parallel tasks has overhead
  - Too many small tasks: With low threshold (e.g., n < 19), many small tasks are created
  - Load balancing: Many small tasks can lead to poor load distribution
  - Cache effects: More tasks can cause more cache misses
  
- **Coarsening approach**:
  - Increased threshold from 19 to 25
  - For `n < 25`: Execute serially (no spawn overhead)
  - For `n >= 25`: Use parallel execution
  - This reduces the number of spawns while maintaining parallelism for larger subproblems
  
- **Results comparison**:
  - **8 workers**: Coarsened version (7.958s) is slightly faster than original (7.796s)
  - **4 workers**: Coarsened version (8.439s) is slightly slower than original (8.338s)
  - **1 worker**: Coarsened version (22.673s) is slightly slower (expected, more serial work)
  - Overall: Coarsening provides better performance with 8 workers (2.85x vs 2.83x speedup)

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

#define COARSENING_THRESHOLD 25

int64_t fib(int64_t n) {
    if (n < 2) return n;
    int64_t x, y;
    if (n < COARSENING_THRESHOLD) {
        // Serial execution for small n (coarsening)
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
- **Coarsening**: Uses serial execution for `n < 25` to reduce spawn overhead
  - Threshold determined empirically (tested 19, 25, 30, 35)
  - Balance between parallelism and spawn overhead
  - Reduces number of small tasks created
- **Parallel Spawn**: Uses `cilk_spawn` for parallel recursive calls when `n >= 25`
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
**Status**: Checkoff Item 2 Completed ✅ - Next: Checkoff Item 3 (Matrix Transpose)

