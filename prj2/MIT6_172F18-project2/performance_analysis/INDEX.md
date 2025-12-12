# Performance Analysis Index

This directory contains documentation of all performance debugging and optimization sessions for the quadtree collision detection implementation.

## Sessions

### ✅ Session #1: O(n²) Bugs in Query Phase
**File:** [01-initial-performance-bugs.md](01-initial-performance-bugs.md)  
**Date:** December 2025  
**Status:** RESOLVED  
**Summary:** Fixed two critical O(n²) bugs that made quadtree slower than brute-force:
- Bug #1: maxVelocity recomputation (O(n²) overhead)
- Bug #2: Array index linear search (O(n² log n) overhead)

**Result:** Quadtree transformed from slower in 5/7 cases to faster in 6/7 cases, with maximum speedup improving from 1.18x to 2.68x.

### ✅ Session #2: Memory Safety Bugs
**File:** [02-memory-safety-bugs.md](02-memory-safety-bugs.md)  
**Date:** December 2025  
**Status:** RESOLVED  
**Summary:** Fixed three critical memory safety issues:
- Bug #1: Memory leaks in error paths (lineIdToIndex not freed in 4 error paths)
- Bug #2: Buffer overflow risk (array access without bounds checking)
- Bug #3: Incorrect bounds check operator (`>` instead of `>=`)

**Result:** Eliminated memory leaks, prevented buffer overflow, and fixed bounds check. No performance impact.

### ✅ Session #3: Build Phase Analysis and Optimization
**File:** [03-candidate-pair-analysis.md](03-candidate-pair-analysis.md)  
**Date:** December 2025  
**Status:** RESOLVED  
**Summary:** Identified build phase as primary bottleneck (84-91% of time) and implemented bounding box caching:
- Fixed build phase timing instrumentation
- Implemented Optimization #1: Cache bounding boxes to eliminate redundant computation
- Discovered candidate generation is near-optimal (0.9x-1.2x ratio gap)
- Tree structure is appropriate (depth ratios 0.78x-1.01x)

**Result:** Mixed results from bounding box caching (build time increased slightly), but identified the real bottleneck was O(n²) bug in insertLineRecursive.

### ✅ Session #4: Critical O(n²) Bug in insertLineRecursive
**File:** [04-critical-on2-bug-fix.md](04-critical-on2-bug-fix.md)  
**Date:** December 2025  
**Status:** RESOLVED  
**Summary:** Fixed critical O(n²) bug where maxVelocity was recomputed for every line during subdivision:
- Bug: insertLineRecursive recomputed maxVelocity O(n) times per line during subdivision
- Fix: Use precomputed tree->maxVelocity instead
- Impact: Eliminated 75% performance overhead (hypot was 75.16% of time)

**Result:** **10.3x improvement, 25.7x speedup vs brute-force!** Quadtree now faster in 7/7 cases (100%).

### ✅ Optimization #1: Line Length Caching
**File:** [05-line-length-caching.md](05-line-length-caching.md)  
**Date:** December 2025  
**Status:** ✅ Implemented  
**Summary:** Precompute and cache line segment length to eliminate expensive sqrt() operations in collision solver:
- Problem: Line length computed twice per collision using expensive `Vec_length()` (calls `hypot()`)
- Solution: Add `cachedLength` field to Line struct, compute once per frame in `updatePosition()`
- Impact: Eliminates thousands of redundant sqrt() calls per frame for high collision density inputs

**Expected Result:** Significant reduction in sqrt() operations, especially for inputs with many collisions (e.g., sin_wave.in with 279,712 collisions).

### ✅ Optimization #2: Maximum Quadtree Depth Tuning
**File:** [06-maxdepth-tuning.md](06-maxdepth-tuning.md)  
**Date:** December 2025  
**Status:** ✅ Implemented and Analyzed  
**Summary:** Analyzed optimal maxDepth values across different input sizes:
- Problem: Fixed maxDepth=12 may not be optimal for all input sizes
- Solution: Added environment variable support (`QUADTREE_MAXDEPTH`) and benchmarked different values
- Impact: Current default (12) is optimal for large inputs; small/medium inputs could benefit from 10-18

**Key Finding:** Current default maxDepth=12 is optimal for large inputs (koch.in, 3901 lines), which are most performance-critical. Small inputs benefit from maxDepth=10 (18% faster), but overall default is well-chosen.

---

## Performance Timeline

### Before Any Fixes
- Quadtree faster: 2/7 cases (29%)
- Maximum speedup: 1.18x
- Worst case: 0.28x (3.57x slower)

### After Session #1 Fixes
- Quadtree faster: 6/7 cases (86%)
- Maximum speedup: 2.68x
- Worst case: 0.60x (1.67x slower)

### After Session #4 Fixes (Critical O(n²) Bug)
- Quadtree faster: **7/7 cases (100%)** ✅
- Maximum speedup: **24.45x** (koch.in)
- Minimum speedup: **1.99x** (box.in)
- Average speedup: **7.2x**
- **Achieved theoretical performance!**

---

## Quick Reference: All Bugs Fixed

| Bug | Location | Complexity | Impact | Fix |
|-----|----------|------------|--------|-----|
| maxVelocity recomputation (query) | quadtree.c:947-957 | O(n²) | 1M ops for n=1000 | Store in tree |
| Array index linear search | quadtree.c:1012-1018 | O(n² log n) | 14M ops for n=1000 | Build hash table |
| maxVelocity recomputation (build) | quadtree.c:591-598 | O(n²) | 75% overhead, millions of hypot calls | Use tree->maxVelocity |
| Memory leak (error paths) | quadtree.c:976,982,1029,1125 | Memory leak | 4KB per error | Free in all paths |
| Buffer overflow risk | quadtree.c:1075 | Undefined behavior | Potential crash | Bounds check |
| Incorrect bounds check | quadtree.c:1080 | Out-of-bounds access | Potential crash | Change `>` to `>=` |

---

## Methodology

Each debugging session follows this process:

1. **Problem Identification** - Observe performance issue
2. **Root Cause Analysis** - Find the bug (profiling, code review)
3. **Impact Assessment** - Measure how bad it is
4. **Solution Design** - Plan the fix
5. **Implementation** - Write the code
6. **Verification** - Test correctness and performance
7. **Documentation** - Record what was learned

---

## Tools Used

- **Benchmarking:** `scripts/benchmark.sh` - Automated performance testing
- **Profiling:** `perf` - CPU profiling to identify bottlenecks (Session #4)
- **Code Analysis:** Manual code review, complexity analysis
- **Verification:** Correctness checks, performance benchmarks

---

## Key Metrics Tracked

- **Correctness:** Collision counts must match between algorithms
- **Speedup:** Ratio of brute-force time to quadtree time
- **Complexity:** Theoretical vs actual complexity
- **Overhead:** Unnecessary operations identified and eliminated

---

Last Updated: December 2025

