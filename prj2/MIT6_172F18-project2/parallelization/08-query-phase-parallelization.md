# Phase 8: Query Phase Parallelization

## Overview

**Date:** December 2025  
**Status:** ⚠️ Partially Complete - Correctness Issue  
**Purpose:** Parallelize the quadtree query phase (`QuadTree_findCandidatePairs`) to improve performance

## Summary

Phase 8 attempts to parallelize the query phase by parallelizing the loop over lines using `cilk_for`. The implementation uses atomic operations for thread-safe access to shared data structures (`seenPairs` matrix and `candidateList`). However, there is a **correctness issue** where serial and parallel executions produce different collision counts, indicating that some candidate pairs are being missed or incorrectly filtered.

## Implementation Details

### Changes Made

1. **Added Cilk Support:**
   - Added `#include <cilk/cilk.h>` and `#include <stdatomic.h>`
   - Added reducer functions for statistics (`collisionCounter_identity`, `collisionCounter_reduce`)
   - Added reducer functions for error handling (`queryError_identity`, `queryError_reduce`)

2. **Parallelized Main Loop:**
   - Changed `for (unsigned int i = 0; i < tree->numLines; i++)` to `cilk_for`
   - Each iteration processes one line independently
   - Lines are processed in parallel to find their candidate pairs

3. **Thread-Safe seenPairs Matrix:**
   - Used `__atomic_compare_exchange_n` for atomic check-and-set operations
   - Ensures only one thread can mark a pair as seen
   - Prevents duplicate pairs from being added

4. **Thread-Safe candidateList Append:**
   - Pre-allocated capacity before parallel execution (estimated: `numLines * 200`)
   - Used `__atomic_fetch_add` for atomic index allocation
   - Each thread gets a unique index atomically

5. **Error Handling:**
   - Used reducer for error propagation (can't return from `cilk_for`)
   - Errors are collected and checked after parallel execution

### Code Changes

**Location:** `quadtree.c`

**Before (Serial):**
```c
for (unsigned int i = 0; i < tree->numLines; i++) {
  Line* line1 = tree->lines[i];
  // ... find candidates for line1 ...
  if (seenPairs[minId][maxId]) {
    continue;  // Skip duplicate
  }
  seenPairs[minId][maxId] = true;
  candidateList->pairs[candidateList->count].line1 = line1;
  candidateList->pairs[candidateList->count].line2 = line2;
  candidateList->count++;
}
```

**After (Parallel):**
```c
cilk_for (unsigned int i = 0; i < tree->numLines; i++) {
  Line* line1 = tree->lines[i];
  // ... find candidates for line1 ...
  
  // Atomic check-and-set for seenPairs
  bool expected = false;
  bool* seenPtr = &seenPairs[minId][maxId];
  if (!__atomic_compare_exchange_n(seenPtr, &expected, true, false,
                                   __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
    continue;  // Already seen by another thread
  }
  
  // Atomic append to candidateList
  unsigned int myIndex = __atomic_fetch_add(&candidateList->count, 1, __ATOMIC_ACQ_REL);
  if (myIndex >= candidateList->capacity) {
    __atomic_fetch_sub(&candidateList->count, 1, __ATOMIC_ACQ_REL);
    continue;  // Capacity exceeded
  }
  candidateList->pairs[myIndex].line1 = line1;
  candidateList->pairs[myIndex].line2 = line2;
}
```

## Correctness Issue

### Symptoms

**Test Results:**
- beaver.in: Serial=55, Parallel=51 (Mismatch)
- box.in: Serial=0, Parallel=0 (Match)
- smalllines.in: Serial=1687, Parallel=1369 (Mismatch)

**Observation:** Parallel execution finds **fewer** collisions than serial execution, indicating that some candidate pairs are being incorrectly filtered or skipped.

### Possible Causes

1. **Atomic Operations on seenPairs:**
   - The `__atomic_compare_exchange_n` might have subtle issues
   - Memory ordering might not be correct
   - Race conditions in check-and-set logic

2. **Capacity Pre-allocation:**
   - Estimated capacity might not be sufficient
   - Pairs might be skipped when capacity is exceeded
   - The capacity check happens after atomic increment, causing issues

3. **Order-Dependent Logic Removed:**
   - Removed `line2ArrayIndex <= i` check for parallel execution
   - This check ensured pairs matched brute-force order
   - Without it, pairs might be processed in wrong order

4. **Race Conditions:**
   - Multiple threads might be accessing `candidateList` simultaneously
   - The atomic operations might not be sufficient
   - Memory barriers might be needed

### Investigation Needed

1. **Verify Atomic Operations:**
   - Check if `__atomic_compare_exchange_n` is working correctly
   - Verify memory ordering semantics
   - Test with different memory orderings

2. **Check Capacity Handling:**
   - Verify pre-allocation is sufficient
   - Check if pairs are being skipped due to capacity
   - Add logging to track capacity usage

3. **Verify Pair Ordering:**
   - Ensure pairs are added in correct order
   - Check if `seenPairs` matrix is preventing valid pairs
   - Verify line ID ordering logic

4. **Debug with Cilksan:**
   - Run Cilksan to detect race conditions
   - Check for determinacy races
   - Verify thread-safety

## Performance Impact

**Not Measured Yet** (due to correctness issue)

Expected impact:
- Query phase is ~7.7% of execution time (from Phase 1 profiling)
- Parallelizing it could provide 4-6x speedup on 8 cores
- Overall speedup: ~1.3-1.4x (limited by serial portions)

## Files Modified

1. **quadtree.c:**
   - Added Cilk includes and reducer functions
   - Parallelized main query loop with `cilk_for`
   - Added atomic operations for `seenPairs` and `candidateList`
   - Pre-allocated candidate list capacity
   - Added error handling with reducer

## Next Steps

1. **Debug Correctness Issue:**
   - Investigate why parallel execution finds fewer collisions
   - Fix atomic operations or capacity handling
   - Verify pair ordering logic

2. **Test Correctness:**
   - Run all test cases and verify collision counts match
   - Use Cilksan to detect race conditions
   - Compare candidate pair lists between serial and parallel

3. **Measure Performance:**
   - Once correctness is fixed, measure speedup
   - Compare with Phase 7 results
   - Document performance improvements

## Known Issues

1. ⚠️ **Correctness:** Serial and parallel produce different collision counts
2. ⚠️ **Capacity:** Pre-allocation might not be sufficient for all cases
3. ⚠️ **Atomic Operations:** Need verification of correctness
4. ⚠️ **Order Dependencies:** Removed checks might cause issues

## Conclusion

Phase 8 parallelization infrastructure is in place, but correctness needs to be fixed before it can be used. The implementation demonstrates the approach for parallelizing the query phase, but requires debugging to ensure correct results.

**Status:** ⚠️ Incomplete - Correctness issue needs resolution

