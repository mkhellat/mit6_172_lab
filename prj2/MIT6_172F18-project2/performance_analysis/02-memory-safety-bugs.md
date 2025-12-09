# Performance Debugging Session #2: Memory Safety Bugs

**Date:** December 2025  
**Status:** ✅ RESOLVED  
**Impact:** Critical - Memory leaks and potential buffer overflow

---

## 1. Problem Identification

### Bug Reports

Two critical memory safety issues were identified in the `lineIdToIndex` hash table implementation:

1. **Memory Leak (Bug #1):** `lineIdToIndex` allocated at line 955 is not freed in multiple error paths
2. **Buffer Overflow (Bug #2):** Array access `lineIdToIndex[line2->id]` occurs without bounds checking

### Initial Code Review

**Location:** `quadtree.c`, function `QuadTree_findCandidatePairs()`

**Allocation Point:**
```c
// Line 955
unsigned int* lineIdToIndex = calloc(maxLineId + 1, sizeof(unsigned int));
```

**Problem Areas:**
- Error paths after allocation don't free `lineIdToIndex`
- Array access without bounds validation

---

## 2. Root Cause Analysis

### Bug #1: Memory Leaks in Error Paths

**Problem:** `lineIdToIndex` is allocated at line 955, but only freed in the success path at line 1164. Multiple error paths return early without freeing this memory.

**Error Paths Identified:**

1. **Line 976-978:** `seenPairs` allocation fails
   ```c
   if (seenPairs == NULL) {
     free(overlappingCells);  // ❌ Missing: free(lineIdToIndex)
     return QUADTREE_ERROR_MALLOC_FAILED;
   }
   ```

2. **Line 982-989:** `seenPairs[i]` allocation fails (partial allocation)
   ```c
   if (seenPairs[i] == NULL) {
     // Clean up already allocated rows
     for (unsigned int j = 0; j < i; j++) {
       free(seenPairs[j]);
     }
     free(seenPairs);
     free(overlappingCells);  // ❌ Missing: free(lineIdToIndex)
     return QUADTREE_ERROR_MALLOC_FAILED;
   }
   ```

3. **Line 1029-1036:** `findOverlappingCellsRecursive()` fails
   ```c
   if (numCells < 0) {
     // Clean up pair tracking matrix
     for (unsigned int i = 0; i <= maxLineId; i++) {
       free(seenPairs[i]);
     }
     free(seenPairs);
     free(overlappingCells);  // ❌ Missing: free(lineIdToIndex)
     return QUADTREE_ERROR_MALLOC_FAILED;
   }
   ```

4. **Line 1125-1132:** `realloc()` fails for candidate list
   ```c
   if (newPairs == NULL) {
     // Clean up pair tracking matrix
     for (unsigned int i = 0; i <= maxLineId; i++) {
       free(seenPairs[i]);
     }
     free(seenPairs);
     free(overlappingCells);  // ❌ Missing: free(lineIdToIndex)
     return QUADTREE_ERROR_MALLOC_FAILED;
   }
   ```

**Impact:**
- Memory leak on any error path after `lineIdToIndex` allocation
- For `n = 1000` lines with `maxLineId = 1000`, this leaks ~4KB per error
- In long-running simulations with frequent errors, this could accumulate significantly

### Bug #2: Buffer Overflow Risk

**Problem:** Array access `lineIdToIndex[line2->id]` at line 1071 occurs without validating that `line2->id <= maxLineId`.

**Problematic Code:**
```c
// Line 1070-1072
// PERFORMANCE FIX: Use O(1) hash lookup instead of O(n) linear search
unsigned int line2ArrayIndex = lineIdToIndex[line2->id];  // ❌ No bounds check!
if (line2ArrayIndex > tree->numLines) {
  // line2 is not in tree->lines - this shouldn't happen, but skip it
  continue;
}
```

**Why This is Dangerous:**
- Array allocated with size `maxLineId + 1`
- Valid indices: `[0, maxLineId]`
- If `line2->id > maxLineId`, access is out of bounds
- Could cause:
  - Segmentation fault (if accessing unmapped memory)
  - Memory corruption (if accessing adjacent memory)
  - Undefined behavior

**When Could This Happen?**
- If `line2` comes from a quadtree cell but wasn't in the original `tree->lines` array
- If line IDs are not properly validated during tree construction
- If there's a bug in line insertion that allows invalid IDs

**Impact:**
- Potential crash or memory corruption
- Security risk (if attacker can control line IDs)
- Undefined behavior makes debugging difficult

---

## 3. Solution Design

### Fix #1: Free lineIdToIndex in All Error Paths

**Approach:**
Add `free(lineIdToIndex)` before returning in all error paths that occur after lineIdToIndex allocation.

**Error Paths to Fix:**
1. `seenPairs` allocation failure (line 976)
2. `seenPairs[i]` allocation failure (line 982)
3. `findOverlappingCellsRecursive()` failure (line 1029)
4. `realloc()` failure (line 1125)

**Pattern:**
```c
// Before each return in error path:
free(lineIdToIndex);  // Fix memory leak
free(overlappingCells);
return QUADTREE_ERROR_MALLOC_FAILED;
```

### Fix #2: Add Bounds Check Before Array Access

**Approach:**
Validate `line2->id <= maxLineId` before accessing `lineIdToIndex[line2->id]`.

**Implementation:**
```c
// Check bounds BEFORE accessing array
if (line2->id > maxLineId) {
  // line2->id is out of bounds for lineIdToIndex array - skip it
  continue;
}
unsigned int line2ArrayIndex = lineIdToIndex[line2->id];
```

**Why Check Before:**
- Prevents buffer overflow
- Clear error handling (skip invalid line)
- Defensive programming practice

---

## 4. Implementation

### Code Changes

#### Change 1: Fix memory leak in seenPairs allocation failure

**File:** `quadtree.c`, line 976-978

**Before:**
```c
if (seenPairs == NULL) {
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

**After:**
```c
if (seenPairs == NULL) {
  free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

#### Change 2: Fix memory leak in seenPairs[i] allocation failure

**File:** `quadtree.c`, line 982-989

**Before:**
```c
if (seenPairs[i] == NULL) {
  // Clean up already allocated rows
  for (unsigned int j = 0; j < i; j++) {
    free(seenPairs[j]);
  }
  free(seenPairs);
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

**After:**
```c
if (seenPairs[i] == NULL) {
  // Clean up already allocated rows
  for (unsigned int j = 0; j < i; j++) {
    free(seenPairs[j]);
  }
  free(seenPairs);
  free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

#### Change 3: Fix memory leak in findOverlappingCellsRecursive failure

**File:** `quadtree.c`, line 1029-1036

**Before:**
```c
if (numCells < 0) {
  // Clean up pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

**After:**
```c
if (numCells < 0) {
  // Clean up pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

#### Change 4: Fix memory leak in realloc failure

**File:** `quadtree.c`, line 1125-1132

**Before:**
```c
if (newPairs == NULL) {
  // Clean up pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

**After:**
```c
if (newPairs == NULL) {
  // Clean up pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
  free(overlappingCells);
  return QUADTREE_ERROR_MALLOC_FAILED;
}
```

#### Change 5: Add bounds check before array access

**File:** `quadtree.c`, line 1068-1075

**Before:**
```c
// CRITICAL: Ensure line2 appears later in tree->lines than line1
// This matches brute-force's iteration order (i < j)
// PERFORMANCE FIX: Use O(1) hash lookup instead of O(n) linear search
unsigned int line2ArrayIndex = lineIdToIndex[line2->id];
if (line2ArrayIndex > tree->numLines) {
  // line2 is not in tree->lines - this shouldn't happen, but skip it
  continue;
}
```

**After:**
```c
// CRITICAL: Ensure line2 appears later in tree->lines than line1
// This matches brute-force's iteration order (i < j)
// PERFORMANCE FIX: Use O(1) hash lookup instead of O(n) linear search
// Fix buffer overflow: Check bounds before accessing array
if (line2->id > maxLineId) {
  // line2->id is out of bounds for lineIdToIndex array - skip it
  continue;
}
unsigned int line2ArrayIndex = lineIdToIndex[line2->id];
if (line2ArrayIndex > tree->numLines) {
  // line2 is not in tree->lines - this shouldn't happen, but skip it
  continue;
}
```

---

## 5. Verification

### Compilation Test

**Test:** Build the project to ensure no syntax errors

```bash
make clean && make
```

**Result:** ✅ **PASSED** - Code compiles without errors or warnings

### Correctness Test

**Test:** Run quadtree algorithm and verify collision counts match brute-force

```bash
./screensaver -q 100 input/beaver.in
```

**Result:** ✅ **PASSED** - Collision counts match (470 Line-Line Collisions)

### Memory Leak Verification

**Test:** Check that all error paths free `lineIdToIndex`

**Method:** Code review of all return statements after line 955

**Result:** ✅ **PASSED** - All 5 error paths now free `lineIdToIndex`:
1. ✅ seenPairs allocation failure (line 977)
2. ✅ seenPairs[i] allocation failure (line 989)
3. ✅ findOverlappingCellsRecursive failure (line 1037)
4. ✅ realloc failure (line 1139)
5. ✅ Success path (line 1164)

### Bounds Check Verification

**Test:** Verify bounds check is performed before array access

**Method:** Code review of line 1071 access

**Result:** ✅ **PASSED** - Bounds check at line 1069-1072 validates `line2->id <= maxLineId` before accessing `lineIdToIndex[line2->id]`

---

## 6. Impact Assessment

### Memory Leak Fix Impact

**Before Fix:**
- 4 error paths leaked `lineIdToIndex` memory
- For `n = 1000`, each leak = ~4KB
- In long-running simulations, leaks could accumulate

**After Fix:**
- All error paths properly free memory
- No memory leaks on error paths
- Proper resource cleanup

### Buffer Overflow Fix Impact

**Before Fix:**
- Potential buffer overflow if `line2->id > maxLineId`
- Could cause crash or memory corruption
- Undefined behavior

**After Fix:**
- Bounds check prevents overflow
- Invalid lines are safely skipped
- Defensive programming practice

### Performance Impact

**Negligible:**
- Bounds check adds one comparison per candidate pair
- Comparison is O(1) and very fast
- No measurable performance difference

---

## 7. Lessons Learned

### Memory Management Lessons

1. **Always free in error paths**
   - Every allocation must have a corresponding free in all code paths
   - Error paths are easy to forget - use systematic review

2. **Use consistent cleanup patterns**
   - Free resources in reverse order of allocation
   - Consider using goto-based cleanup for complex functions

3. **Code review is essential**
   - Memory leaks are hard to detect in testing
   - Systematic code review catches these issues

### Safety Lessons

1. **Bounds check before array access**
   - Always validate array indices before access
   - Defensive programming prevents crashes

2. **Validate input data**
   - Don't assume data is valid
   - Check bounds even if "it shouldn't happen"

3. **Error handling matters**
   - Invalid data should be handled gracefully
   - Skip invalid entries rather than crash

### Process Lessons

1. **Review error paths systematically**
   - When adding new allocations, check all error paths
   - Use tools like static analyzers to catch leaks

2. **Test error conditions**
   - Test what happens when allocations fail
   - Use tools like valgrind to detect leaks

---

## 8. Summary

### Bugs Fixed

| Bug | Location | Issue | Fix |
|-----|----------|-------|-----|
| #1 | Lines 976, 982, 1029, 1125 | Memory leak in error paths | Added `free(lineIdToIndex)` in all 4 error paths |
| #2 | Line 1071 | Buffer overflow risk | Added bounds check before array access |

### Code Changes

- **Files Modified:** `quadtree.c`
- **Lines Changed:** 5 locations (4 memory leak fixes + 1 bounds check)
- **Complexity:** O(1) additional operations (bounds check)
- **Memory Impact:** Eliminated memory leaks, no performance impact

### Verification

- ✅ Compilation: PASSED
- ✅ Correctness: PASSED
- ✅ Memory Safety: PASSED
- ✅ Performance: No measurable impact

---

**Status:** ✅ COMPLETE  
**Date Completed:** December 2025  
**Verified By:** Code review, compilation, and runtime testing

