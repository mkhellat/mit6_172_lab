# Phase 8: Query Phase Parallelization - Debugging Attempts

## Overview

**Date:** December 2025  
**Status:** ⚠️ Correctness Issue Persists  
**Purpose:** Debug and fix correctness issue in Phase 8 parallelization

## Problem

Parallel execution finds **fewer** collisions than serial execution:
- beaver.in: Serial=55, Parallel=53 (2 pairs missing)
- smalllines.in: Serial=1687, Parallel=1416 (271 pairs missing)

This indicates that some candidate pairs are being incorrectly filtered or skipped.

## Debugging Attempts

### Attempt 1: Check Capacity Before Marking as Seen

**Change:** Moved capacity check before atomic compare-and-exchange to avoid marking pairs as seen if capacity is exceeded.

**Result:** Still incorrect (beaver.in: Serial=55, Parallel=51)

**Analysis:** Capacity check didn't fix the issue. The problem might be elsewhere.

### Attempt 2: Restore Order-Dependent Check

**Change:** Restored `line2ArrayIndex <= i` check to match serial version's logic.

**Result:** Still incorrect (beaver.in: Serial=55, Parallel=53)

**Analysis:** Order check didn't fix the issue. The problem might be with atomic operations themselves.

### Attempt 3: Use atomic_exchange Instead of compare_exchange

**Change:** Changed from `__atomic_compare_exchange_n` to `__atomic_exchange_n` for simpler atomic operation.

**Result:** Still incorrect (beaver.in: Serial=55, Parallel=53)

**Analysis:** Different atomic operation didn't fix the issue. The problem might be more fundamental.

## Current Code State

**Atomic Operation:**
```c
bool oldValue = __atomic_exchange_n(seenPtr, true, __ATOMIC_ACQ_REL);
if (oldValue) {
  continue;  // Already seen
}
```

**Capacity Check:**
```c
unsigned int currentCount = __atomic_load_n(&candidateList->count, __ATOMIC_ACQUIRE);
if (currentCount >= candidateList->capacity) {
  continue;  // Skip without marking
}
```

**Order Check:**
```c
if (line2ArrayIndex <= i) {
  continue;  // Skip to match serial order
}
```

## Possible Root Causes

1. **Race Condition in Capacity Check:**
   - Capacity is checked before marking as seen
   - But between check and mark, other threads might add pairs
   - This could cause valid pairs to be skipped

2. **Atomic Operation Issue:**
   - `__atomic_exchange_n` on `bool` might have issues
   - Memory ordering might not be correct
   - The operation might not be working as expected

3. **Order Check Race:**
   - In parallel execution, `i` values are processed out of order
   - The `line2ArrayIndex <= i` check might incorrectly skip valid pairs
   - This check assumes sequential processing

4. **Memory Barrier Issues:**
   - Memory barriers might not be sufficient
   - Threads might not see consistent state
   - Cache coherency issues

## Next Steps for Investigation

1. **Add Debug Logging:**
   - Log when pairs are skipped and why
   - Compare serial vs parallel skip reasons
   - Identify which pairs are being missed

2. **Test with Cilksan:**
   - Run Cilksan to detect race conditions
   - Check for determinacy races
   - Verify thread-safety

3. **Compare Candidate Lists:**
   - Dump candidate lists from serial and parallel
   - Compare to see which pairs are missing
   - Identify pattern in missing pairs

4. **Try Different Approach:**
   - Use per-thread candidate lists and merge
   - Use locks instead of atomic operations
   - Simplify the parallelization approach

## Conclusion

The correctness issue persists despite multiple debugging attempts. The problem appears to be more fundamental than simple fixes can address. Further investigation is needed, possibly requiring a different parallelization approach or more detailed debugging with logging and race detection tools.

**Status:** ⚠️ Debugging In Progress - Correctness Issue Not Yet Resolved

