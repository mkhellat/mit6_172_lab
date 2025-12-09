# Performance Debugging Session #1: O(n²) Bugs in Query Phase

**Date:** December 2025  
**Status:** ✅ RESOLVED  
**Impact:** Critical - Made quadtree slower than brute-force in 5/7 test cases

---

## 1. Problem Identification

### Initial Observation

During performance benchmarking, we discovered that the quadtree algorithm was
**slower than brute-force in 5 out of 7 test cases**, despite having theoretical
O(n log n) complexity vs brute-force's O(n²).

**Initial Results (1000 frames, BEFORE fixes):**

| Input File | $n$ | BF Time | QT Time | Speedup | Status |
|-----------|-----|---------|---------|---------|--------|
| beaver.in | 294 | 2.44s | 3.26s | **0.75x** | ❌ Slower |
| box.in | 401 | 4.42s | 6.39s | **0.69x** | ❌ Slower |
| explosion.in | 601 | 9.61s | 19.15s | **0.50x** | ❌ Slower |
| koch.in | 3901 | 444.51s | 419.77s | **1.06x** | ✅ Faster |
| mit.in | 810 | 17.14s | 31.83s | **0.54x** | ❌ Slower |
| sin_wave.in | 1181 | 34.50s | 29.24s | **1.18x** | ✅ Faster |
| smalllines.in | 530 | 7.64s | 27.53s | **0.28x** | ❌ Slower |

**Key Observations:**
- Quadtree was slower in **71% of test cases** (5/7)
- Even when faster, speedup was modest (1.06x, 1.18x)
- Worst case: smalllines.in was **3.57x slower** (0.28x speedup)

### Hypothesis

The quadtree should theoretically be faster for large $n$ due to O(n log n) vs O(n²).
The fact that it's slower suggests:
1. **High constant factors** in quadtree overhead
2. **Implementation bugs** causing worse-than-expected complexity
3. **Collision density** making spatial filtering ineffective

We suspected implementation bugs were the primary cause.

---

## 2. Root Cause Analysis

### Code Review Process

We systematically reviewed the quadtree query phase implementation to identify
performance bottlenecks.

### Bug #1: maxVelocity Recomputation (O(n²) Overhead)

**Location:** `quadtree.c`, lines 947-957

**Problematic Code:**
```c
for (unsigned int i = 0; i < tree->numLines; i++) {
    Line* line1 = tree->lines[i];
    // ...
    
    // BUG: Computing maxVelocity for EVERY line!
    double maxVelocity = 0.0;
    for (unsigned int k = 0; k < tree->numLines; k++) {  // O(n) per line!
        if (tree->lines[k] != NULL) {
            double v_mag = Vec_length(tree->lines[k]->velocity);
            if (v_mag > maxVelocity) {
                maxVelocity = v_mag;
            }
        }
    }
    if (maxVelocity == 0.0) {
        maxVelocity = 1e-10;
    }
    
    computeLineBoundingBox(line1, tree->buildTimeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax,
                           maxVelocity, tree->config.minCellSize);
}
```

**Complexity Analysis:**
- Outer loop: $n$ iterations
- Inner loop: $n$ iterations per line
- **Total: O(n²)** - This is worse than brute-force's O(n²) for collision testing!

**Impact Calculation:**
For $n = 1000$ lines:
- Operations: $1000 \times 1000 = 1,000,000$ unnecessary maxVelocity computations
- Brute-force tests: $\frac{1000 \times 999}{2} = 499,500$ pairs
- **Overhead is 2x worse than brute-force's main work!**

**Why This Happened:**
The TODO comment on line 945 says "TODO: Store maxVelocity in tree structure to avoid recomputing"
- This was identified as a problem but not fixed
- maxVelocity is constant for all lines in a frame (doesn't change during query)
- Should be computed ONCE during build phase

### Bug #2: Array Index Linear Search (O(n² log n) Overhead)

**Location:** `quadtree.c`, lines 1012-1018

**Problematic Code:**
```c
for (int cellIdx = 0; cellIdx < numCells; cellIdx++) {
    QuadNode* cell = overlappingCells[cellIdx];
    
    for (unsigned int j = 0; j < cell->numLines; j++) {
        Line* line2 = cell->lines[j];
        // ...
        
        // BUG: Linear search for EVERY candidate pair!
        unsigned int line2ArrayIndex = 0;
        bool line2Found = false;
        for (unsigned int k = 0; k < tree->numLines; k++) {  // O(n) per candidate!
            if (tree->lines[k] == line2) {
                line2ArrayIndex = k;
                line2Found = true;
                break;
            }
        }
        // ...
    }
}
```

**Complexity Analysis:**
- Outer loop: $n$ lines
- Middle loop: ~$\log n$ cells per line (average)
- Inner loop: ~$n$ lines per cell (worst case, but typically much less)
- Array index search: $n$ operations per candidate pair
- **Total: O(n² log n)** or worse!

**Impact Calculation:**
For $n = 1000$ lines with ~$n \log n = 14,000$ candidates:
- Array index searches: $14,000 \times 1000 = 14,000,000$ operations
- Brute-force tests: $499,500$ operations
- **Overhead is 28x worse than brute-force!**

**Why This Happened:**
- Need to check if line2 appears later in array than line1 (to match brute-force order)
- Used simple linear search instead of building a lookup table
- Comment on line 889-896 shows awareness of the problem but no solution implemented

---

## 3. Impact Assessment

### Combined Overhead Analysis

For $n = 1000$ lines:

| Operation | Complexity | Operations | Brute-Force Equivalent |
|-----------|------------|------------|------------------------|
| Bug #1: maxVelocity | O(n²) | 1,000,000 | 2.0x brute-force work |
| Bug #2: Array lookup | O(n² log n) | 14,000,000 | 28.0x brute-force work |
| **Total Overhead** | **O(n² log n)** | **15,000,000** | **30.0x brute-force work** |

**Conclusion:** The quadtree was doing **30x more overhead work** than brute-force's
actual collision testing! This completely negated any benefit from spatial filtering.

### Performance Impact by Input Size

| Input Size | Bug #1 Overhead | Bug #2 Overhead | Total Overhead | Brute-Force Work | Ratio |
|------------|-----------------|-----------------|----------------|------------------|-------|
| $n = 100$ | 10,000 | ~140,000 | 150,000 | 4,950 | 30x |
| $n = 500$ | 250,000 | ~3,500,000 | 3,750,000 | 124,750 | 30x |
| $n = 1000$ | 1,000,000 | ~14,000,000 | 15,000,000 | 499,500 | 30x |
| $n = 2000$ | 4,000,000 | ~56,000,000 | 60,000,000 | 1,999,000 | 30x |

The overhead scales worse than brute-force, explaining why quadtree was slower
even at large $n$.

---

## 4. Solution Design

### Fix #1: Store maxVelocity in Tree Structure

**Approach:**
1. Compute maxVelocity once during `QuadTree_build()`
2. Store in `tree->maxVelocity` field
3. Use stored value during query phase

**Changes Required:**
1. Add `maxVelocity` field to `QuadTree` struct in `quadtree.h`
2. Compute and store during build phase (already computed, just need to store)
3. Remove recomputation loop from query phase
4. Use `tree->maxVelocity` in `computeLineBoundingBox()` call

**Complexity Improvement:**
- Before: O(n²) - computed for every line
- After: O(n) - computed once during build
- **Improvement: O(n) reduction**

### Fix #2: Build Reverse Lookup Table

**Approach:**
1. Build a hash table mapping line ID → array index once during query initialization
2. Use O(1) lookup instead of O(n) linear search

**Implementation:**
- Allocate array `lineIdToIndex[maxLineId + 1]`
- Initialize to invalid index (numLines + 1)
- Populate: `lineIdToIndex[line->id] = arrayIndex` for each line
- Lookup: `line2ArrayIndex = lineIdToIndex[line2->id]`

**Complexity Improvement:**
- Before: O(n) per candidate = O(n² log n) total
- After: O(1) per candidate = O(n log n) total
- **Improvement: O(n) reduction per candidate**

**Memory Cost:**
- Additional: O(maxLineId) space for lookup table
- Acceptable: maxLineId is typically close to n, so O(n) space
- Trade-off: Small memory cost for huge time savings

---

## 5. Implementation

### Code Changes

#### Change 1: Add maxVelocity to QuadTree struct

**File:** `quadtree.h`, line 252

```c
typedef struct {
  // ... existing fields ...
  double buildTimeStep;         // Time step from QuadTree_build()
  
  // Maximum velocity in system (computed during build, used for bounding box expansion)
  double maxVelocity;           // Maximum velocity magnitude across all lines
} QuadTree;
```

#### Change 2: Store maxVelocity during build

**File:** `quadtree.c`, line 667

```c
  // If no lines or all velocities are zero, use a small default to avoid division issues
  if (maxVelocity == 0.0) {
    maxVelocity = 1e-10;
  }
  
  // Store maxVelocity in tree for use during query phase (avoids recomputing)
  tree->maxVelocity = maxVelocity;
```

#### Change 3: Remove maxVelocity recomputation from query

**File:** `quadtree.c`, lines 941-957

**Before:**
```c
    // Compute line's bounding box
    // TODO: Store maxVelocity in tree structure to avoid recomputing
    double maxVelocity = 0.0;
    for (unsigned int k = 0; k < tree->numLines; k++) {
        if (tree->lines[k] != NULL) {
            double v_mag = Vec_length(tree->lines[k]->velocity);
            if (v_mag > maxVelocity) {
                maxVelocity = v_mag;
            }
        }
    }
    if (maxVelocity == 0.0) {
        maxVelocity = 1e-10;
    }
    computeLineBoundingBox(line1, tree->buildTimeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax,
                           maxVelocity, tree->config.minCellSize);
```

**After:**
```c
    // Compute line's bounding box
    // Use stored maxVelocity from build phase (avoids O(n) recomputation per line!)
    computeLineBoundingBox(line1, tree->buildTimeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax,
                           tree->maxVelocity, tree->config.minCellSize);
```

#### Change 4: Build reverse lookup table

**File:** `quadtree.c`, lines 881-918

**Before:**
```c
  // Find max line ID to allocate pair tracking matrix
  unsigned int maxLineId = 0;
  for (unsigned int i = 0; i < tree->numLines; i++) {
    if (tree->lines[i] != NULL && tree->lines[i]->id > maxLineId) {
      maxLineId = tree->lines[i]->id;
    }
  }
  
  // ... comments about building a map but no implementation ...
```

**After:**
```c
  // Find max line ID to allocate pair tracking matrix
  unsigned int maxLineId = 0;
  for (unsigned int i = 0; i < tree->numLines; i++) {
    if (tree->lines[i] != NULL && tree->lines[i]->id > maxLineId) {
      maxLineId = tree->lines[i]->id;
    }
  }
  
  // Build Line* to array index map using line ID as key (O(n) once, then O(1) lookups)
  unsigned int* lineIdToIndex = calloc(maxLineId + 1, sizeof(unsigned int));
  if (lineIdToIndex == NULL) {
    free(overlappingCells);
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  // Initialize to invalid index (numLines+1 as sentinel, since 0 is valid)
  for (unsigned int idx = 0; idx <= maxLineId; idx++) {
    lineIdToIndex[idx] = tree->numLines + 1;  // Invalid index
  }
  // Build the map: for each line, store its array index
  for (unsigned int idx = 0; idx < tree->numLines; idx++) {
    if (tree->lines[idx] != NULL) {
      lineIdToIndex[tree->lines[idx]->id] = idx;
    }
  }
```

#### Change 5: Use O(1) lookup instead of O(n) search

**File:** `quadtree.c`, lines 1008-1028

**Before:**
```c
        // CRITICAL: Ensure line2 appears later in tree->lines than line1
        unsigned int line2ArrayIndex = 0;
        bool line2Found = false;
        for (unsigned int k = 0; k < tree->numLines; k++) {  // O(n) search!
            if (tree->lines[k] == line2) {
                line2ArrayIndex = k;
                line2Found = true;
                break;
            }
        }
        if (!line2Found) {
            continue;
        }
```

**After:**
```c
        // CRITICAL: Ensure line2 appears later in tree->lines than line1
        // PERFORMANCE FIX: Use O(1) hash lookup instead of O(n) linear search
        unsigned int line2ArrayIndex = lineIdToIndex[line2->id];
        if (line2ArrayIndex > tree->numLines) {  // Invalid index check
            continue;
        }
```

#### Change 6: Free lookup table

**File:** `quadtree.c`, line 1101

```c
  // Free pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(lineIdToIndex);  // Free the reverse lookup table
  free(overlappingCells);
  return QUADTREE_SUCCESS;
```

---

## 6. Verification

### Correctness Check

**Test:** Run both algorithms and verify collision counts match

```bash
# Test all inputs
for input in input/*.in; do
    ./screensaver 1000 "$input" | grep "Line-Line Collisions"
    ./screensaver -q 1000 "$input" | grep "Line-Line Collisions"
done
```

**Result:** ✅ **100% correctness** - All collision counts match perfectly on all 7 test cases.

### Performance Benchmark

**Test:** Run full benchmark suite (1000 frames per input)

**Results AFTER fixes:**

| Input File | $n$ | BF Time | QT Time | Speedup | Status |
|-----------|-----|---------|---------|---------|--------|
| beaver.in | 294 | 2.05s | 1.36s | **1.51x** | ✅ Faster |
| box.in | 401 | 3.75s | 2.50s | **1.50x** | ✅ Faster |
| explosion.in | 601 | 9.13s | 8.19s | **1.11x** | ✅ Faster |
| koch.in | 3901 | 416.90s | 182.59s | **2.28x** | ✅✅ Faster |
| mit.in | 810 | 16.93s | 16.21s | **1.04x** | ✅ Faster |
| sin_wave.in | 1181 | 33.42s | 12.49s | **2.68x** | ✅✅ Faster |
| smalllines.in | 530 | 7.37s | 12.18s | **0.60x** | ❌ Slower |

### Performance Improvement Summary

| Metric | Before Fixes | After Fixes | Improvement |
|--------|---------------|-------------|-------------|
| Cases where quadtree faster | 2/7 (29%) | 6/7 (86%) | +200% |
| Maximum speedup | 1.18x | 2.68x | +127% |
| Average speedup (when faster) | 1.12x | 1.69x | +51% |
| Worst slowdown | 0.28x (3.57x slower) | 0.60x (1.67x slower) | +114% improvement |

**Key Wins:**
- ✅ koch.in: 1.06x → **2.28x** (+115% improvement)
- ✅ sin_wave.in: 1.18x → **2.68x** (+127% improvement)
- ✅ beaver.in: 0.75x → **1.51x** (was slower, now faster!)
- ✅ box.in: 0.69x → **1.50x** (was slower, now faster!)
- ✅ explosion.in: 0.50x → **1.11x** (was slower, now faster!)
- ✅ mit.in: 0.54x → **1.04x** (was slower, now faster!)

---

## 7. Lessons Learned

### Technical Lessons

1. **Theoretical complexity ≠ Actual performance**
   - O(n log n) algorithm can be slower than O(n²) if implementation has bugs
   - Always measure, don't just trust the algorithm design

2. **Profile before optimizing**
   - These bugs would have been obvious with profiling
   - Code review caught them, but profiling would have been faster

3. **TODO comments are warnings**
   - The "TODO: Store maxVelocity" comment was a red flag
   - Should have been fixed immediately, not left for later

4. **Lookup tables are worth it**
   - Building a hash table for O(1) lookup is worth the O(n) setup cost
   - Especially when you do many lookups (O(n log n) candidates)

5. **Constant factors matter**
   - Even with correct O(n log n) complexity, constant factors can kill performance
   - These bugs added 30x overhead - completely negating the algorithm's benefit

### Process Lessons

1. **Measure first, optimize second**
   - We should have benchmarked immediately after implementation
   - Would have caught these bugs before writing documentation

2. **Code review is valuable**
   - Systematic code review found both bugs
   - But profiling would have been even faster

3. **Document as you debug**
   - Creating this document helped organize the debugging process
   - Makes it easier to track what was fixed and why

### Algorithm Design Lessons

1. **Spatial data structures have overhead**
   - Tree construction, query traversal, memory allocation all cost time
   - Need large enough $n$ or low enough collision density to amortize

2. **Collision density matters**
   - High collision density (> 50%) makes spatial filtering less effective
   - But even with high density, large $n$ can still benefit (sin_wave.in: 2.68x)

3. **Implementation quality is critical**
   - A well-implemented O(n²) can beat a poorly-implemented O(n log n)
   - These bugs made quadtree 30x worse than it should have been

---

## 8. Next Steps

### Remaining Performance Issues

1. **smalllines.in still slower (0.60x)**
   - Very high collision density (55.43%)
   - May need different approach for high-density inputs
   - Or accept that brute-force is better for this case

2. **Potential further optimizations:**
   - Cache bounding box computations (currently recomputed for each line)
   - Optimize tree traversal (reduce pointer chasing)
   - Consider adaptive algorithm selection based on collision density

### Future Debugging Sessions

- Session #2: (To be documented)
- Session #3: (To be documented)

---

## Appendix: Code Diffs

### Summary of Changes

**Files Modified:**
- `quadtree.h`: Added `maxVelocity` field to `QuadTree` struct
- `quadtree.c`: 
  - Store maxVelocity during build (line 667)
  - Remove maxVelocity recomputation from query (lines 941-957)
  - Build reverse lookup table (lines 881-918)
  - Use O(1) lookup instead of O(n) search (lines 1008-1028)
  - Free lookup table (line 1101)

**Lines Changed:** ~50 lines  
**Complexity Improvement:** O(n²) → O(n log n) for query phase  
**Performance Improvement:** 2.3x better maximum speedup, 6/7 cases now faster

---

**Status:** ✅ COMPLETE  
**Date Completed:** December 2025  
**Verified By:** Benchmark results show 6/7 cases faster, correctness maintained

