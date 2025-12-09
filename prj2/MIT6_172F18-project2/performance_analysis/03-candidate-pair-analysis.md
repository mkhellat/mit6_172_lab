# Performance Debugging Session #3: Candidate Pair Analysis

**Date:** December 2025  
**Status:** 🔄 IN PROGRESS  
**Impact:** Critical - Quadtree generating too many candidates, limiting speedup

---

## 1. Problem Identification

### Performance Gap Analysis

**Theoretical vs Actual Speedup:**

| Input       | $n$  | Brute-Force Pairs | Theoretical Candidates | Theoretical Speedup | Actual Speedup | Gap             |
|-------------|------|-------------------|------------------------|---------------------|----------------|-----------------|
| koch.in     | 3901 | 7,606,950         | ~42,911                | **177.3x**          | 2.28x          | **77.8x worse** |
| sin_wave.in | 1181 | 696,790           | ~11,810                | **59.0x**           | 2.68x          | **22.0x worse** |

**Key Observation:**
- Quadtree is generating **WAY more candidates** than the theoretical $n \log n$
- Actual speedup is **20-80x worse** than theoretical expectations
- This suggests we're either:
  1. Generating too many candidate pairs (spatial filtering ineffective)
  2. Spending too much time in tree construction/query overhead
  3. Missing additional O(n²) operations

### Hypothesis

**Primary Hypothesis:** Quadtree is generating far more candidate pairs than $n \log n$ due to:
1. **Lines spanning too many cells** - Long lines or large bounding boxes cause lines to appear in many cells
2. **Ineffective spatial filtering** - Bounding box expansion too large, causing false positives
3. **Tree structure issues** - Tree not subdividing effectively, leading to large cells with many lines

**Secondary Hypothesis:** Significant overhead in:
1. **Tree construction** - O(n log n) but with high constant factors
2. **Query phase** - Finding overlapping cells, traversing tree
3. **Memory allocation** - Allocating tree nodes, candidate lists

---

## 2. Investigation Plan

### Step 1: Instrument Candidate Pair Generation

**Goal:** Measure actual number of candidate pairs vs theoretical

**Implementation:**
1. Enable `DEBUG_QUADTREE` flag to log candidate counts
2. Add instrumentation to count:
   - Total candidate pairs generated
   - Average candidates per line
   - Ratio: candidates / brute-force pairs
   - Expected ratio: $(n \log n) / (n(n-1)/2) \approx (2 \log n) / n$

**Expected Results:**
- For $n = 3901$: Expected ratio ≈ $(2 \times 12) / 3901 \approx 0.6\%$
- For $n = 1181$: Expected ratio ≈ $(2 \times 11) / 1181 \approx 1.9\%$

**If actual ratio >> expected:**
- Spatial filtering is ineffective
- Need to investigate bounding box expansion
- Need to investigate tree structure

### Step 2: Profile Time Breakdown

**Goal:** Understand where time is spent

**Measurements:**
1. **Tree construction time** - `QuadTree_build()` duration
2. **Query phase time** - `QuadTree_findCandidatePairs()` duration
3. **Test phase time** - Time spent calling `intersect()` on candidates
4. **Sorting time** - `qsort()` duration

**Expected Breakdown (for ideal quadtree):**
- Tree construction: ~20-30% of total time
- Query phase: ~20-30% of total time
- Test phase: ~40-50% of total time (most candidates don't collide)
- Sorting: <5% of total time

**If construction/query >> test:**
- Overhead is too high
- Need to optimize tree operations

### Step 3: Analyze Tree Structure

**Goal:** Understand tree characteristics

**Measurements:**
1. **Tree depth** - Maximum depth reached
2. **Node count** - Total number of nodes created
3. **Average lines per leaf** - Should be close to `maxLinesPerNode`
4. **Cell overlap** - Average number of cells each line spans

**Expected:**
- Tree depth: ~$\log_4(n)$ for balanced tree
- Node count: ~$n$ nodes (one per line on average)
- Average lines per leaf: ~`maxLinesPerNode / 2` (before subdivision)
- Cell overlap: ~1-4 cells per line (most lines in 1-2 cells)

**If lines span many cells:**
- Bounding boxes too large
- Need to reduce expansion factors (`k_rel`, `k_gap`)

### Step 4: Analyze Bounding Box Expansion

**Goal:** Check if bounding boxes are too large

**Measurements:**
1. **Bounding box size** - Average width/height of bounding boxes
2. **Expansion factors** - Actual expansion vs line length
3. **Cell coverage** - How many cells each bounding box overlaps

**Current Expansion Factors:**
- `k_rel = 0.3` - Relative motion expansion
- `k_gap = 0.15` - Gap factor
- `precision_margin = 1e-6` - Numerical precision

**If bounding boxes too large:**
- Reduce expansion factors
- Trade-off: Risk missing collisions vs generating fewer candidates

---

## 3. Implementation

### Instrumentation Code

**Add to `quadtree.c`:**

```c
#ifdef DEBUG_QUADTREE_STATS
// After candidate pair generation
unsigned long long expectedPairs = ((unsigned long long)tree->numLines * 
                                   (tree->numLines - 1)) / 2;
double expectedRatio = (2.0 * log2(tree->numLines)) / tree->numLines * 100.0;
double actualRatio = (candidateList->count * 100.0) / expectedPairs;

fprintf(stderr, "QUADTREE STATS:\n");
fprintf(stderr, "  Lines: %u\n", tree->numLines);
fprintf(stderr, "  Brute-force pairs: %llu\n", expectedPairs);
fprintf(stderr, "  Candidate pairs: %u\n", candidateList->count);
fprintf(stderr, "  Expected candidates: ~%.0f (%.2f%%)\n", 
        tree->numLines * log2(tree->numLines), expectedRatio);
fprintf(stderr, "  Actual ratio: %.2f%%\n", actualRatio);
fprintf(stderr, "  Ratio gap: %.1fx\n", actualRatio / expectedRatio);
#endif
```

**Add timing instrumentation:**

```c
#ifdef DEBUG_QUADTREE_TIMING
clock_t start_build = clock();
// ... build tree ...
clock_t end_build = clock();
double build_time = ((double)(end_build - start_build)) / CLOCKS_PER_SEC;

clock_t start_query = clock();
// ... query phase ...
clock_t end_query = clock();
double query_time = ((double)(end_query - start_query)) / CLOCKS_PER_SEC;

fprintf(stderr, "TIMING BREAKDOWN:\n");
fprintf(stderr, "  Build: %.3fs (%.1f%%)\n", build_time, build_time/total_time*100);
fprintf(stderr, "  Query: %.3fs (%.1f%%)\n", query_time, query_time/total_time*100);
fprintf(stderr, "  Test: %.3fs (%.1f%%)\n", test_time, test_time/total_time*100);
#endif
```

---

## 4. Expected Findings

### Scenario A: Too Many Candidates

**Symptoms:**
- Candidate ratio >> expected (e.g., 10-50% instead of 0.6-2%)
- Lines spanning many cells (10+ cells per line)
- Large bounding boxes relative to line length

**Root Cause:**
- Bounding box expansion too aggressive
- Tree not subdividing effectively

**Fix:**
- Reduce `k_rel` and `k_gap` factors
- Increase `maxLinesPerNode` to reduce subdivision
- Optimize bounding box computation

### Scenario B: High Overhead

**Symptoms:**
- Construction/query time >> test time
- Tree operations taking most of the time

**Root Cause:**
- Tree construction too expensive
- Query traversal inefficient
- Memory allocation overhead

**Fix:**
- Optimize tree construction (reduce allocations)
- Cache tree structure across frames (incremental updates)
- Optimize cell overlap queries

### Scenario C: Both Issues

**Symptoms:**
- Too many candidates AND high overhead
- Worst of both worlds

**Root Cause:**
- Fundamental design issues
- Need major refactoring

**Fix:**
- Consider alternative spatial data structures
- Adaptive algorithm selection based on input characteristics

---

## 5. Next Steps

1. **Enable instrumentation** - Add debug flags and compile with `-DDEBUG_QUADTREE_STATS`
2. **Run benchmarks** - Collect candidate pair counts and timing data
3. **Analyze results** - Compare actual vs expected
4. **Identify bottlenecks** - Determine primary issue (candidates vs overhead)
5. **Implement fixes** - Address identified issues
6. **Re-measure** - Verify improvements

---

## 6. Success Criteria

**Target Improvements:**
- Reduce candidate pairs to <5% of brute-force pairs (currently likely 10-50%)
- Achieve speedup >10x for large inputs (currently 2-3x)
- Maintain 100% correctness

**Acceptable Trade-offs:**
- Slightly larger bounding boxes (risk missing rare collisions)
- More aggressive tree subdivision (higher memory usage)
- Adaptive parameters based on input characteristics

---

**Status:** 🔄 READY FOR INVESTIGATION  
**Priority:** HIGH - This is the key to achieving theoretical performance

