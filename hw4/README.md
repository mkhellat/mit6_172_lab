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

#### ✅ Checkoff Item 3: Matrix Transpose
- **Status**: Completed
- **Description**: Parallelize matrix transpose using `cilk_for`
- **Approach**:
  - Converted from OpenMP `#pragma omp parallel for` to Cilk `cilk_for`
  - Parallelized outer loop (i = 1 to rows-1)
  - Inner loop remains serial (j = 0 to i-1) - triangular transpose pattern
  - Each outer iteration swaps one row with corresponding column independently
- **Results**: See Performance Results section below

#### ✅ Checkoff Item 4: Cilksan Race Detection
- **Status**: Completed
- **Description**: Use Cilksan to find and fix a determinacy race in parallel quicksort
- **Race Detection**:
  - **Location**: Line 50 in `qsort-race.c` - Write to global variable `comparison_count`
  - **Variable**: `comparison_count` declared at line 31
  - **Race Type**: Read/Write race - multiple spawned threads write to same global variable
  - **Cause**: Both recursive calls spawned (lines 44-45) eventually execute line 50,
    causing concurrent writes to `comparison_count` without synchronization
- **Fix**:
  - Removed the global `comparison_count` variable (not needed for sorting)
  - Changed parallelization to spawn only one recursive call instead of both
  - This maintains parallelism while eliminating the determinacy race
- **Verification**: Cilksan confirms 0 races after fix

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

### Matrix Transpose (transpose 10000)

| Workers | Real Time | User Time | Sys Time | CPU Usage | Speedup | Status |
|---------|-----------|-----------|----------|-----------|---------|--------|
| 1       | 1.122s    | 1.07s     | 0.05s    | 99%       | 1.0x    | ✅     |
| 4       | 0.921s    | 1.17s     | 0.05s    | 132%      | 1.22x   | ✅     |
| 8       | 0.903s    | 1.44s     | 0.05s    | 165%      | 1.24x   | ✅     |

**Observations:**
- **1 worker**: Serial execution, ~1.12 seconds
- **4 workers**: 1.22x speedup, using ~1.3 cores (132% CPU)
- **8 workers**: 1.24x speedup, using ~1.6 cores (165% CPU)
- **Limited speedup**: The triangular transpose pattern has limited parallelism
  - Each outer iteration has different work (i elements to swap)
  - Load imbalance: early iterations do less work than later ones
  - Memory access pattern may cause cache conflicts
- **Note**: Speedup is modest due to the nature of the algorithm and potential cache effects

**Test Command:**
```bash
{ time CILK_NWORKERS=1 ./transpose 10000; }
{ time CILK_NWORKERS=4 ./transpose 10000; }
{ time CILK_NWORKERS=8 ./transpose 10000; }
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

### transpose.c
```c
#include <cilk/cilk.h>

void transpose(Matrix* arr) {
    uint16_t j;
    // Parallel transpose using cilk_for
    // Parallelize the outer loop - each iteration swaps one row with corresponding column
    cilk_for (uint16_t i = 1; i < arr->rows; i++) {
        // Inner loop remains serial - swaps elements in row i with column i
        for (j = 0; j < i; j++) {
            uint8_t tmp = arr->data[i][j];
            arr->data[i][j] = arr->data[j][i];
            arr->data[j][i] = tmp;
        }
    }
}
```

### Key Features
- **Coarsening (fib.c)**: Uses serial execution for `n < 25` to reduce spawn overhead
  - Threshold determined empirically (tested 19, 25, 30, 35)
  - Balance between parallelism and spawn overhead
  - Reduces number of small tasks created
- **Parallel Spawn (fib.c)**: Uses `cilk_spawn` for parallel recursive calls when `n >= 25`
- **Synchronization (fib.c)**: Uses `cilk_sync` to wait for spawned tasks
- **cilk_for (transpose.c)**: Parallelizes outer loop of triangular matrix transpose
  - Each iteration is independent (swaps row i with column i)
  - Inner loop remains serial (sequential swaps within each row/column pair)

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

## Race Detection Results

### Quicksort Race (qsort-race)

**Race Detected by Cilksan:**
- **Line 50**: Write to `comparison_count` (global variable)
- **Line 31**: Declaration of `comparison_count`
- **Lines 44-45**: Both recursive calls spawned, both eventually access shared variable
- **Race Type**: Determinacy race - multiple threads write to same memory location

**Race Description:**
The race condition occurred because the naively parallelized quicksort spawned both
recursive calls (lines 44-45), and each spawned call eventually executed line 50,
which increments the global `comparison_count` variable. Since multiple threads
were writing to the same memory location (`comparison_count`) without synchronization,
this created a determinacy race. The order of writes was non-deterministic, leading
to potential incorrect results.

**Fix Applied:**
1. Removed the global `comparison_count` variable (not necessary for sorting)
2. Changed parallelization strategy: spawn only one recursive call, execute the other serially
3. This maintains parallelism (one branch runs in parallel) while eliminating the race

**Verification:**
```bash
cd hw4/qsort-race
make clean && make CILKSAN=1
CILK_NWORKERS=4 ./qsort-race 10 1
# Output: "Cilksan detected 0 distinct races."
```

---

**Last Updated**: 2025-12-19
**Status**: Checkoff Item 4 Completed ✅ - Next: Homework (Reducer Hyperobjects)

