# mit6_172_lab

MIT OCW on performance engineering:

## Homework 2: Merge Sort Optimization
- [Merge Sort Optimization](hw2/write-up-1.md)
  - [Merge Sort - Recursive - Inline External Function Calls](hw2/write-up-2.md)
  - [Merge Sort - Iterative - Explicit Stack](hw2/write-up-3-1.md)
  - [Merge Sort - Iterative - Bottom UP](hw2/write-up-3-2.md)
  - [Merge Sort - Pointers vs Arrays](hw2/write-up-4.md)
  - [Merge Sort - Coarsening with Insert Sort](hw2/write-up-5.md)
  - [Merge Sort - Memory Scratch Space Reduction](hw2/write-up-6.md)
  - [Merge Sort - Memory Allocation Reduction](hw2/write-up-7.md)

## Homework 3: Vectorization and SIMD Optimization
- [Assembly Analysis - Countdown Loop](hw3/write-up-1.md)
- [Assembly Analysis - AVX2 Aligned Moves](hw3/write-up-2.md)
- [Assembly Analysis - Ternary Magic](hw3/write-up-3.md)
- [Assembly Analysis - `memcpy` Pattern Recognition](hw3/write-up-4.md)
- [Assembly Analysis - `-ffast-math` re-ordering vs IEEE 754](hw3/write-up-5.md)
- [Timing - Speedup Measurements](hw3/write-up-6.md)
- [Assembly Analysis - Packed Instructions](hw3/write-up-7.md)
- [Assembly Analysis - Variable vs Constant Shift](hw3/write-up-8.md)
- [Vector Packing - Data Type Impact on Performance](hw3/write-up-9.md)
- [Vector Packing - Operation Complexity Impact on Performance](hw3/write-up-10.md)
- [Vector Patterns - Termination Overhead](hw3/write-up-12.md)
- [Vector Patterns - Strided Loop Vectorization Analysis](hw3/write-up-13.md)
- [Vector Patterns - Clang Loop Pragma Vectorization](hw3/write-up-14.md)
- [Vector Patterns - Reduction Loop Vectorization Analysis](hw3/write-up-15.md)

## Homework 4: Multicore Programming with Cilk
- [Homework 4 Overview](hw4/README.md) - Cilk parallelization and reducer hyperobjects
  - ✅ Serial Fibonacci timing
  - ✅ Parallelize Fibonacci with Cilk (`cilk_spawn`, `cilk_sync`)
  - ⏳ Checkoff Item 1: Performance measurements (1, 4, 8 workers)
  - ⏳ Checkoff Item 2: Coarsening optimization
  - ⏳ Checkoff Item 3: Matrix transpose parallelization
  - ⏳ Reducer hyperobjects implementation

## Project 1: Bit-array Cyclic Rotation
- [Project 1 Overview](prj1/00-project1-introduction.md)
  - [The Inplace Mandate](prj1/00-project1-the-inplace-mandate.md)
  - [Mathematical Foundations](prj1/01-project1-mathematical-foundations.md)
  - [Rotation Algorithms](prj1/02-project1-rotation-algorithms.md)
  - [Performance Analysis](prj1/03-project1-performance-analysis.md)
  - [Bonus Methods](prj1/04-project1-bonus-methods.md)

## Project 2: Quadtree Collision Detection ⭐
- [Project 2 Overview](prj2/README.md)
  - [Introduction](prj2/00-project2-introduction.md) - Problem statement and quadtree optimization
  - [Algorithm Comparison](prj2/01-project2-algorithm-comparison.md) - Brute-force vs quadtree, design decisions
  - [Performance Analysis](prj2/02-project2-performance-analysis.md) - Speedup measurements (up to 24.45x)
  - [Correctness Verification](prj2/03-project2-correctness.md) - 100% correctness across all test cases
  - [Future Work](prj2/04-project2-future-work.md) - Optimizations and parallelization status
  - **[Parallelization with OpenCilk](prj2/MIT6_172F18-project2/README.md)** ⭐ Complete
    - All 10 phases implemented and verified
    - Thread-safe reducers and atomic operations
    - Race condition fixes and correctness verification
    - Cilkscale setup for parallelism analysis
    - See [parallelization documentation](prj2/MIT6_172F18-project2/parallelization/) for detailed phase reports

### Project 2 Highlights

**Serial Performance:**
- ✅ Quadtree faster in 7/7 test cases (100%)
- ✅ Maximum speedup: **24.45x** (koch.in, 3901 lines)
- ✅ Average speedup: **7.08x** across all test cases
- ✅ 100% correctness verified (identical collision counts)

**Parallel Performance:**
- ✅ Candidate testing parallelized (`cilk_for`)
- ✅ Query phase parallelized (`cilk_for`, race condition fixed)
- ✅ Thread-safety verified (0 races with Cilksan)
- ✅ Correctness verified (identical results serial vs parallel)
- ✅ Speedup: ~1.15-1.3x on 8 cores (limited by serial build phase)

**Key Achievements:**
- Reduced complexity from $O(n^2)$ to $O(n \log n)$ average case
- Fixed 6 critical bugs (3 O(n²) bugs eliminated)
- Complete parallelization with OpenCilk 2.0
- Comprehensive debugging and verification process


