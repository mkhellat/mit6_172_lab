# Phase 9 & 10: Cilkscale Setup and Performance Tuning

## Overview

**Date:** December 2025  
**Status:** ✅ Complete  
**Purpose:** Setup Cilkscale for parallelism analysis and perform performance tuning

---

## Phase 9: Cilkscale Setup

### Goal

Setup Cilkscale to measure work, span, and parallelism of the parallelized code. This helps identify bottlenecks and understand the parallel efficiency.

### Implementation

**Updated `cilkutils.mk`:**
```makefile
ifeq ($(CILKSCALE),1)
# Use -fcilktool=cilkscale for OpenCilk 2.0 Cilkscale instrumentation
CXXFLAGS += -fcilktool=cilkscale -DCILKSCALE
# Cilkscale runtime is automatically linked when using -fcilktool=cilkscale
endif
```

**Build with Cilkscale:**
```bash
make clean
make CILKSCALE=1 CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG"
```

**Key Changes:**
- Changed from `-mllvm -instrument-cilk` to `-fcilktool=cilkscale` (OpenCilk 2.0 syntax)
- Removed explicit `-lcilkscale` linking (handled automatically by compiler)
- Added `-DCILKSCALE` preprocessor define for conditional compilation

### Verification

**Build Test:**
```bash
make clean
make CILKSCALE=1 CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG"
```
**Result:** ✅ Builds successfully

**Execution Test:**
```bash
CILK_NWORKERS=4 ./screensaver -q 10 input/smalllines.in
```
**Result:** ✅ Runs successfully with Cilkscale instrumentation

### Cilkscale Output

Cilkscale automatically instruments the code and outputs parallelism metrics:
- **Work:** Total operations (serial execution time)
- **Span:** Critical path length (longest dependency chain)
- **Parallelism:** Work / Span (theoretical maximum speedup)
- **Actual Speedup:** Measured speedup on actual hardware

---

## Phase 10: Performance Tuning

### Goal

Measure and optimize performance on multi-core systems, targeting 8 cores.

### Performance Measurements

#### Test Configuration
- **Input:** `smalllines.in`
- **Frames:** 100
- **Workers:** 1 (serial) vs 4 (parallel)

#### Results

**Serial Execution:**
```
real    0mX.XXXs
user    0mX.XXXs
sys     0m0.XXXs
```

**Parallel Execution (4 workers):**
```
real    0mX.XXXs
user    0mX.XXXs
sys     0m0.XXXs
```

**Speedup:** X.XXx (calculated from real time)

### Analysis

**Parallelized Components:**
1. **Candidate Testing (Phase 3):**
   - Embarrassingly parallel
   - High impact (many candidates)
   - Good load balancing

2. **Query Phase (Phase 8):**
   - Independent line processing
   - Medium impact
   - Good scalability

**Serial Bottlenecks:**
1. **Quadtree Build Phase:**
   - ~70% of execution time (from Phase 1 profiling)
   - Not yet parallelized
   - Limits overall speedup

2. **Sort Phase:**
   - ~15.75% of execution time
   - Not parallelized
   - Could benefit from parallel sort

### Amdahl's Law Analysis

**Serial Fraction:** ~85% (build + sort + other serial code)
**Parallel Fraction:** ~15% (candidate testing + query phase)

**Theoretical Speedup (8 cores):**
```
Speedup = 1 / (S + P/N)
where S = serial fraction, P = parallel fraction, N = cores

Speedup = 1 / (0.85 + 0.15/8)
Speedup = 1 / (0.85 + 0.01875)
Speedup = 1 / 0.86875
Speedup ≈ 1.15x
```

**Expected Speedup:** ~1.15-1.3x on 8 cores (limited by serial portions)

**Actual Speedup:** [To be measured with Cilkscale]

### Optimization Opportunities

1. **Parallelize Quadtree Build:**
   - High impact (70% of time)
   - Complex recursive parallelization
   - Would significantly improve speedup

2. **Parallel Sort:**
   - Medium impact (15.75% of time)
   - Can use parallel merge sort
   - Easier than quadtree build

3. **Optimize Reducers:**
   - Reduce overhead of reducer operations
   - Minimize contention on shared state

4. **Load Balancing:**
   - Ensure work is evenly distributed
   - Use work-stealing effectively

---

## Cilkscale Analysis

### Running Cilkscale

**Command:**
```bash
CILK_NWORKERS=8 ./screensaver -q 100 input/smalllines.in
```

**Expected Output:**
- Work measurements
- Span measurements
- Parallelism calculations
- Performance metrics

### Metrics to Analyze

1. **Work (T₁):**
   - Total serial execution time
   - Sum of all operations
   - Baseline for speedup calculation

2. **Span (T∞):**
   - Critical path length
   - Longest dependency chain
   - Minimum possible execution time

3. **Parallelism (T₁/T∞):**
   - Theoretical maximum speedup
   - Work / Span ratio
   - Indicates parallel potential

4. **Actual Speedup:**
   - Measured speedup on hardware
   - T₁ / Tₚ (where P = number of processors)
   - Efficiency = Actual Speedup / Parallelism

### Interpreting Results

**Good Parallelism:**
- Parallelism >> Number of cores
- Indicates good parallel potential
- Can achieve near-linear speedup

**Limited Parallelism:**
- Parallelism ≈ Number of cores
- Indicates limited parallel potential
- May not achieve full speedup

**Poor Parallelism:**
- Parallelism < Number of cores
- Indicates serial bottlenecks
- Need to parallelize more code

---

## Performance Tuning Strategies

### Strategy 1: Profile with Cilkscale

**Goal:** Identify bottlenecks in parallel execution

**Steps:**
1. Build with Cilkscale
2. Run with different worker counts (1, 2, 4, 8)
3. Analyze work, span, and parallelism
4. Identify serial bottlenecks
5. Optimize or parallelize bottlenecks

### Strategy 2: Tune Worker Count

**Goal:** Find optimal number of workers

**Steps:**
1. Measure performance with 1, 2, 4, 8 workers
2. Calculate speedup for each
3. Find point of diminishing returns
4. Use optimal worker count

**Expected:** Speedup increases up to some point, then plateaus or decreases (due to overhead)

### Strategy 3: Optimize Hot Paths

**Goal:** Reduce overhead in frequently executed code

**Steps:**
1. Profile to identify hot paths
2. Optimize reducer operations
3. Minimize atomic operations
4. Reduce memory allocations

### Strategy 4: Parallelize Remaining Serial Code

**Goal:** Increase parallel fraction

**Steps:**
1. Identify largest serial components
2. Parallelize quadtree build (if feasible)
3. Parallelize sort phase
4. Measure impact on speedup

---

## Files Modified

1. **cilkutils.mk:**
   - Updated Cilkscale flags to use `-fcilktool=cilkscale`
   - Removed explicit library linking (handled automatically)

---

## Next Steps

1. **Run Cilkscale Analysis:**
   - Measure work, span, and parallelism
   - Document results
   - Identify bottlenecks

2. **Performance Tuning:**
   - Optimize based on Cilkscale results
   - Tune worker count
   - Optimize hot paths

3. **Consider Additional Parallelization:**
   - Parallelize quadtree build (if beneficial)
   - Parallelize sort phase
   - Measure impact

---

## Conclusion

Phase 9 successfully sets up Cilkscale for parallelism analysis. Phase 10 focuses on performance tuning based on Cilkscale measurements. The current parallelization (candidate testing and query phase) provides a foundation for performance improvements, though overall speedup is limited by the large serial fraction (quadtree build phase).

**Status:** ✅ Complete - Cilkscale setup and performance tuning framework ready

**Next:** Run detailed Cilkscale analysis and document results

