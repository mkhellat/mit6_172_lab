# Phase 8: Continued Debugging Session

## Date
December 2025

## Current Status

After fixing beaver.in by removing the order check, smalllines.in still has correctness issues:
- **beaver.in:** Serial=55, Parallel=54 (1 collision missing, was fixed at 55/55 but regressed)
- **smalllines.in:** Serial=1687, Parallel=1415 (272 collisions missing)

## Observations

### Debug Statistics (smalllines.in, 10 frames, last frame)

**Serial:**
- Total pairs added: 13105
- Pairs skipped - capacity: 0
- Pairs skipped - already seen: 50608
- Pairs skipped - order check: 63713
- Final candidate list count: 13105
- Collisions detected: 1687

**Parallel (4 workers):**
- Total pairs added: 14315
- Pairs skipped - capacity: 0
- Pairs skipped - already seen: 48378
- Pairs skipped - order check: 60916
- Final candidate list count: 14315
- Collisions detected: 1415

### Key Findings

1. **Parallel adds MORE pairs (14315 vs 13105)** but finds **FEWER collisions (1415 vs 1687)**
   - This is counterintuitive - more pairs should lead to more collisions, not fewer
   - Suggests that extra pairs being added don't cause collisions (false positives)
   - OR some pairs that DO cause collisions are being missed

2. **Fewer pairs skipped as "already seen" in parallel (48378 vs 50608)**
   - Atomic operation appears to be working (preventing some duplicates)
   - But parallel is still adding more pairs overall

3. **No capacity warnings**
   - Capacity is never exceeded (52900 capacity, max 14315 pairs)
   - Pairs are not being lost due to capacity issues

## Investigation Attempts

### Attempt 1: Reorder Operations (Mark as Seen First)

**Change:** Mark pair as seen BEFORE checking capacity.

**Rationale:** Avoid TOCTOU race condition where capacity check passes but then capacity is exceeded before adding pair.

**Result:** Made it worse
- beaver.in: 55 → 51 collisions
- smalllines.in: 1377 → 1427 collisions

**Analysis:** Marking as seen first causes pairs to be lost if capacity is exceeded. Reverted.

### Attempt 2: Check for Duplicate Pairs

**Check:** Verify if duplicate pairs are being added despite atomic operations.

**Result:** No duplicates found in either serial or parallel execution.

**Analysis:** Atomic operations are working correctly - no duplicate pairs.

### Attempt 3: Check Capacity Exceeded Warnings

**Check:** Look for pairs being lost due to capacity exceeded.

**Result:** No capacity warnings found.

**Analysis:** Capacity is sufficient, pairs are not being lost due to capacity.

## Hypothesis

The issue might be related to **non-deterministic pair ordering** in parallel execution:

1. **Without order check:** Parallel execution adds pairs in non-deterministic order
2. **Different pair sets:** Parallel adds different pairs than serial (14315 vs 13105)
3. **Missing collisions:** Some pairs that cause collisions in serial are not added in parallel
4. **Extra pairs:** Some pairs added in parallel don't cause collisions (false positives from quadtree)

**Possible Root Cause:**
- The quadtree query phase finds pairs based on spatial proximity
- In parallel, different threads process different lines simultaneously
- This leads to different pair discovery order
- Some pairs that should be found are missed due to race conditions in spatial queries
- Some pairs that shouldn't be found are added due to different processing order

## Next Steps

1. **Compare candidate pair sets:**
   - Dump candidate pairs from serial and parallel execution
   - Identify which pairs are missing in parallel
   - Check if missing pairs correspond to missing collisions

2. **Investigate spatial query race conditions:**
   - Check if `findOverlappingCellsRecursive` has race conditions
   - Verify that quadtree structure is not modified during parallel query

3. **Consider reverting to serial query phase:**
   - Phase 8 is optional
   - Candidate testing (Phase 3) is already parallelized and working
   - Query phase parallelization may not be worth correctness issues

4. **Alternative: Use different synchronization:**
   - Consider using mutex for `seenPairs` instead of atomic operations
   - OR: Use a different data structure (e.g., hash set with mutex)

## Commands Run

```bash
# Test correctness
CILK_NWORKERS=1 ./screensaver -q 10 input/beaver.in 2>&1 | grep "Line-Line"
CILK_NWORKERS=4 ./screensaver -q 10 input/beaver.in 2>&1 | grep "Line-Line"

CILK_NWORKERS=1 ./screensaver -q 10 input/smalllines.in 2>&1 | grep "Line-Line"
CILK_NWORKERS=4 ./screensaver -q 10 input/smalllines.in 2>&1 | grep "Line-Line"

# Check debug statistics
make clean
make CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG -DDEBUG_PHASE8"
CILK_NWORKERS=4 ./screensaver -q 10 input/smalllines.in 2>&1 | grep -A 8 "PHASE 8 DEBUG"
```

## Files Modified

- `quadtree.c`: 
  - Reordered operations (mark as seen first) - reverted
  - Added capacity exceeded warnings
  - Current: Check capacity first, then mark as seen, then add pair


