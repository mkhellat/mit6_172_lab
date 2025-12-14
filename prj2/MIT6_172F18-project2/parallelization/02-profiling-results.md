# Phase 1: Serial Program Profiling Results

## Profiling Setup

**Date:** December 2025  
**Input:** `input/koch.in` (3901 lines)  
**Frames:** 100  
**Tool:** `perf record` with call-graph (dwarf)  
**Command:** `perf record -g --call-graph dwarf ./screensaver -q 100 input/koch.in`

## Overall Statistics

- **Total Samples:** 7,000 samples
- **Event Count:** ~3,070,527,735 cycles
- **Lost Samples:** 0
- **Execution Time:** ~2.045 seconds

## Top Hotspots (by Overhead)

### 1. Sorting: `compareCandidatePairs` - 16.48%

**Location:** `collision_world.c:40` (comparison function for qsort)

**Analysis:**
- Called millions of times during candidate pair sorting
- Part of O(n log n) sorting operation
- High overhead due to:
  - Multiple pointer dereferences
  - Multiple `compareLines()` calls per comparison
  - Cache misses from random memory access patterns

**Parallelization Potential:**
- **Difficulty:** Medium
- **Approach:** Use parallel sort (e.g., Cilk parallel qsort)
- **Benefit:** Moderate (16% of time, but sorting is inherently sequential)

### 2. Intersection Testing: `intersectLines` + `intersect` - ~21.8%

**Breakdown:**
- `intersectLines`: 13.62%
- `intersect`: 8.18%

**Location:** `intersection_detection.c`

**Analysis:**
- Called once per candidate pair
- Independent operations (no dependencies between pairs)
- Embarrassingly parallel workload
- High percentage of total execution time

**Parallelization Potential:**
- **Difficulty:** Easy (embarrassingly parallel)
- **Approach:** `cilk_for` over candidate pairs
- **Benefit:** High (22% of time, perfect for parallelization)
- **Expected Speedup:** Near-linear (4-8x on 8 cores)

### 3. Query Phase: `QuadTree_findCandidatePairs` - 7.70%

**Location:** `quadtree.c:979` (QuadTree_findCandidatePairs)

**Analysis:**
- Finds candidate pairs for each line
- Iterates over all lines
- Each line's query is independent

**Parallelization Potential:**
- **Difficulty:** Easy-Medium
- **Approach:** `cilk_for` over lines
- **Benefit:** Moderate (7.7% of time)
- **Expected Speedup:** Good (4-6x on 8 cores)

### 4. Query Helper: `findOverlappingCellsRecursive` - 4.50%

**Location:** `quadtree.c` (recursive cell finding)

**Analysis:**
- Called from `QuadTree_findCandidatePairs`
- Recursive tree traversal
- Part of query phase overhead

**Parallelization Potential:**
- **Difficulty:** Medium (recursive, but can parallelize at line level)
- **Approach:** Parallelized as part of query phase
- **Benefit:** Included in query phase parallelization

### 5. Build Phase: `insertLineRecursive` - 4.90%

**Location:** `quadtree.c` (recursive line insertion)

**Analysis:**
- Called during quadtree construction
- Recursive tree building
- Lower percentage than expected (was 70% before optimizations)

**Parallelization Potential:**
- **Difficulty:** High (complex recursive parallelization)
- **Approach:** Recursive `cilk_spawn` for tree construction
- **Benefit:** Low-Medium (4.9% of time, complex to implement)
- **Priority:** Low (defer to later if needed)

## Summary of Parallelization Targets

### High Priority (Easy, High Impact)

1. **Candidate Pair Testing** (~22% of time)
   - **Function:** `intersect()` calls in candidate testing loop
   - **Parallelism:** Embarrassingly parallel
   - **Implementation:** `cilk_for` over candidate pairs
   - **Expected Impact:** 4-8x speedup for this phase

### Medium Priority (Moderate Complexity)

2. **Query Phase** (~12% of time)
   - **Function:** `QuadTree_findCandidatePairs` loop over lines
   - **Parallelism:** Independent lines
   - **Implementation:** `cilk_for` over lines
   - **Expected Impact:** 4-6x speedup for this phase

### Low Priority (Complex, Lower Impact)

3. **Sorting** (16% of time)
   - **Function:** `qsort` with `compareCandidatePairs`
   - **Parallelism:** Can use parallel sort
   - **Implementation:** Replace with parallel sort algorithm
   - **Expected Impact:** Moderate (sorting has sequential dependencies)

4. **Build Phase** (4.9% of time)
   - **Function:** `insertLineRecursive` in `QuadTree_build`
   - **Parallelism:** Recursive tree construction
   - **Implementation:** Complex recursive parallelization
   - **Expected Impact:** Low-Medium (small percentage, complex)

## Recommended Implementation Order

1. **Phase 1:** Implement reducers (required for all parallelization)
2. **Phase 2:** Parallelize candidate testing (easiest, highest impact)
3. **Phase 3:** Parallelize query phase (moderate complexity, good impact)
4. **Phase 4:** (Optional) Optimize sorting or build phase if needed

## Expected Overall Speedup

**Conservative Estimate (8 cores):**
- Candidate testing: 22% → 22%/8 = 2.75% (saves 19.25%)
- Query phase: 12% → 12%/6 = 2% (saves 10%)
- **Total savings:** ~29% of execution time
- **Overall speedup:** ~1.4x

**Optimistic Estimate (8 cores, near-linear):**
- Candidate testing: 22% → 22%/8 = 2.75% (saves 19.25%)
- Query phase: 12% → 12%/8 = 1.5% (saves 10.5%)
- **Total savings:** ~30% of execution time
- **Overall speedup:** ~1.43x

**Note:** These estimates assume perfect parallelization. Actual speedup may be lower due to:
- Overhead from spawn/sync
- Load imbalance
- Serial sections (sorting, build phase)
- Memory bandwidth limitations

## Next Steps

1. ✅ Profile serial program (completed)
2. ⏳ Implement reducers for thread-safe state
3. ⏳ Parallelize candidate testing
4. ⏳ Measure actual speedup
5. ⏳ Parallelize query phase if needed

