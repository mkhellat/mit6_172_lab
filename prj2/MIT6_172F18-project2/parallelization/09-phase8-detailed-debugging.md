# Phase 8: Detailed Debugging Session

## Date
December 2025

## Problem Summary
Parallel execution finds **fewer** collisions than serial execution:
- beaver.in: Serial=55, Parallel=53 (2 collisions missing)
- smalllines.in: Serial=1687, Parallel=1416 (271 collisions missing)

## Debugging Session Results

### Debug Statistics (beaver.in, 10 frames, last frame)

**Serial:**
- Total pairs added: 3769
- Pairs skipped - capacity: 0
- Pairs skipped - already seen: 140
- Pairs skipped - order check: 3909
- Pairs skipped - bounds: 0
- Final candidate list count: 3769
- Collisions detected: 55

**Parallel (4 workers):**
- Total pairs added: 3793
- Pairs skipped - capacity: 0
- Pairs skipped - already seen: 122
- Pairs skipped - order check: 3896
- Pairs skipped - bounds: 0
- Final candidate list count: 3793
- Collisions detected: 53

### Key Observations

1. **Parallel adds MORE pairs (3793 vs 3769)** but finds **FEWER collisions (53 vs 55)**
   - This suggests the issue is NOT simply "missing pairs"
   - The parallel version is generating a DIFFERENT set of candidate pairs

2. **Fewer pairs skipped as "already seen" in parallel (122 vs 140)**
   - This suggests the atomic operation on `seenPairs` might not be working correctly
   - OR the parallel version is processing pairs in a different order, leading to different duplicate detection

3. **Different order check skips (3896 vs 3909)**
   - The order check (`line2ArrayIndex <= i`) is filtering different pairs in parallel
   - This is expected since parallel execution processes lines in parallel, not sequentially

### Code Changes Made During Debugging

1. **Added DEBUG_PHASE8 flag** with detailed statistics:
   - Tracks pairs added, skipped (capacity, already seen, order check, bounds)
   - Logs each pair being added with indices

2. **Changed `bool**` to `_Atomic bool**` for seenPairs matrix**
   - Attempted to fix atomic operations on bool type
   - Used `atomic_exchange_explicit` with `memory_order_acq_rel`
   - Issue persists, suggesting atomic operations are working but logic is incorrect

3. **Verified atomic operations are being used correctly**
   - `atomic_exchange_explicit` returns old value atomically
   - Logic: if oldValue is true, pair was already seen; if false, pair is new

### Root Cause Hypothesis

The issue appears to be with the **order check** (`line2ArrayIndex <= i`). In parallel execution:

1. **Serial behavior:** Lines are processed sequentially (i=0, 1, 2, ...), so the order check ensures we only add pairs where `line2ArrayIndex > i`, matching brute-force's `i < j` pattern.

2. **Parallel behavior:** Lines are processed in parallel, so multiple threads might process different `i` values simultaneously. The order check `line2ArrayIndex <= i` might incorrectly filter out valid pairs because:
   - Thread 1 processes i=10, finds line2 at index 5 → skips (5 <= 10 is false, but should be tested)
   - Thread 2 processes i=3, finds line2 at index 5 → adds (5 > 3, correct)
   - But if Thread 1 processes first, it might mark the pair as seen before Thread 2 can add it

3. **The atomic `seenPairs` check happens AFTER the order check**, so:
   - If Thread 1 skips due to order check, it never marks the pair as seen
   - Thread 2 can add it correctly
   - BUT: If Thread 1 processes a pair that should be skipped, and Thread 2 processes the same pair correctly, there might be a race condition

### Resolution

### Fix Applied: Remove Order Check

**Change:** Removed the order-dependent check (`line2ArrayIndex <= i`) and rely solely on `seenPairs` matrix for duplicate prevention.

**Rationale:**
- The order check was designed for serial execution where lines are processed sequentially
- In parallel execution, lines are processed concurrently, making the order check incorrect
- The `seenPairs` matrix already prevents duplicates using atomic operations

**Results:**
- ✅ **beaver.in:** Serial=55, Parallel=55 (FIXED - now matches!)
- ⚠️ **smalllines.in:** Serial=1687, Parallel=1468 (still has mismatch - 219 collisions missing)

### Remaining Issue: smalllines.in

For smalllines.in (larger input):
- Serial: 13105 pairs added, 50608 skipped (already seen)
- Parallel: 14285 pairs added, 46618 skipped (already seen)

**Observation:** Parallel adds MORE pairs (14285 vs 13105) but finds FEWER collisions (1468 vs 1687). This suggests:
1. Parallel version is adding duplicate or incorrect pairs (atomic operation might have race conditions)
2. OR parallel version is missing pairs that should cause collisions

**Possible Causes:**
1. **Race condition in atomic operation:** For larger inputs, more threads compete for the same `seenPairs` entries, potentially causing issues
2. **Memory ordering:** The `memory_order_acq_rel` might not be sufficient for all cases
3. **Capacity pre-allocation:** The pre-allocated capacity might not be sufficient for larger inputs, causing pairs to be skipped

### Next Steps

1. **Investigate smalllines.in issue:**
   - Check if capacity is being exceeded (debug output shows 0, so this is unlikely)
   - Verify atomic operations are working correctly for larger inputs
   - Consider using a different synchronization mechanism (e.g., mutex) for `seenPairs`

2. **Consider reverting to serial query phase for correctness:**
   - Phase 8 is marked as "optional" in the plan
   - The candidate testing phase (Phase 3) is already parallelized and working correctly
   - Query phase parallelization provides limited benefit if it causes correctness issues

3. **Alternative approach:**
   - Use a different data structure for `seenPairs` (e.g., hash set with mutex)
   - OR: Accept the correctness issue and document it as a known limitation

## Commands Run

```bash
# Compile with debug flags
make clean
make CXXFLAGS="-std=gnu99 -Wall -fopencilk -O3 -DNDEBUG -DDEBUG_PHASE8"

# Test serial
CILK_NWORKERS=1 ./screensaver -q 10 input/beaver.in 2>&1 | grep -A 8 "PHASE 8 DEBUG"

# Test parallel
CILK_NWORKERS=4 ./screensaver -q 10 input/beaver.in 2>&1 | grep -A 8 "PHASE 8 DEBUG"
```

## Files Modified

- `quadtree.c`: Added DEBUG_PHASE8 statistics, changed `bool**` to `_Atomic bool**`, used `atomic_exchange_explicit`

