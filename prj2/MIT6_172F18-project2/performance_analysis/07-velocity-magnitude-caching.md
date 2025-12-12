# Optimization #3: Velocity Magnitude Caching

## Overview

**Date:** December 2025  
**Status:** ✅ Implemented  
**Impact:** Reduces expensive sqrt() operations in bounding box calculations

## Problem

The `computeLineBoundingBox()` function computes velocity magnitude for every line during bounding box calculation using:

```c
double velocity_magnitude = Vec_length(line->velocity);
```

**Issues:**
1. `Vec_length()` calls `hypot()` which performs an expensive `sqrt()` operation
2. Velocity magnitude is computed **once per line per bounding box computation**
3. **Velocity doesn't change during a frame** - only position changes, so velocity magnitude is constant
4. Bounding boxes are computed multiple times:
   - Once per line during build phase (for bounds expansion)
   - Once per line during query phase (for candidate pair generation)
   - For n lines: **2n sqrt() calls per frame** just for velocity magnitude

**Example Impact:**
- koch.in (3901 lines): 2 × 3901 = **7,802 sqrt() calls per frame** for velocity magnitude
- Over 100 frames = **780,200 redundant sqrt() operations**

## Solution

**Precompute and cache velocity magnitude:**
1. Add `cachedVelocityMagnitude` field to `Line` struct
2. Compute magnitude once when line is created (`line_demo.c`)
3. Recompute magnitude once per frame in `CollisionWorld_updatePosition()` (ensures correctness)
4. Use cached value in `computeLineBoundingBox()` instead of computing on-the-fly

## Implementation

### 1. Added Cached Velocity Magnitude Field (`line.h`)

```c
struct Line {
  Vec p1;
  Vec p2;
  Vec velocity;
  Color color;
  unsigned int id;
  double cachedLength;
  double cachedVelocityMagnitude;  // NEW: Cached velocity magnitude
};
```

### 2. Initialize on Line Creation (`line_demo.c`)

```c
// Initialize cached velocity magnitude (optimization: precompute to avoid expensive sqrt in bounding box calculations)
line->cachedVelocityMagnitude = Vec_length(line->velocity);
```

### 3. Update After Position Changes (`collision_world.c`)

```c
void CollisionWorld_updatePosition(CollisionWorld* collisionWorld) {
  double t = collisionWorld->timeStep;
  for (int i = 0; i < collisionWorld->numOfLines; i++) {
    Line *line = collisionWorld->lines[i];
    line->p1 = Vec_add(line->p1, Vec_multiply(line->velocity, t));
    line->p2 = Vec_add(line->p2, Vec_multiply(line->velocity, t));
    line->cachedLength = Vec_length(Vec_subtract(line->p1, line->p2));
    // Precompute velocity magnitude once per frame (used in bounding box calculations)
    line->cachedVelocityMagnitude = Vec_length(line->velocity);
  }
}
```

### 4. Use Cached Value in Bounding Box Calculation (`quadtree.c`)

```c
// BEFORE:
double velocity_magnitude = Vec_length(line->velocity);

// AFTER:
double velocity_magnitude = line->cachedVelocityMagnitude;
```

## Performance Analysis

### Expected Impact

**Before:**
- 1 `Vec_length()` call per line per bounding box computation
- Each `Vec_length()` = 1 `hypot()` call = 1 `sqrt()` operation
- Bounding boxes computed twice per frame (build + query phases)
- Total: **2n sqrt() calls per frame** (where n = number of lines)

**After:**
- 1 `Vec_length()` call per line per frame (in `updatePosition`)
- 0 `Vec_length()` calls per bounding box computation (uses cached value)
- Total: **n sqrt() calls per frame** (where n = number of lines)

**Savings:**
- For n lines: **Eliminates n sqrt() calls per frame**
- Example: koch.in (3901 lines)
  - Before: 7,802 sqrt() calls/frame (2 × 3901)
  - After: 3,901 sqrt() calls/frame (1 × 3901)
  - **Savings: 3,901 sqrt() calls/frame (50% reduction)**

### Complexity Analysis

**Before:**
- Build phase: O(n) bounding box computations, each with O(1) but expensive (1 sqrt())
- Query phase: O(n) bounding box computations, each with O(1) but expensive (1 sqrt())
- Total: O(n) with 2n expensive sqrt() operations

**After:**
- Position update: O(n) where n = number of lines (1 sqrt() per line)
- Build phase: O(n) bounding box computations with O(1) per computation (no sqrt())
- Query phase: O(n) bounding box computations with O(1) per computation (no sqrt())
- **Total: O(n) with n sqrt() operations (50% reduction)**

## Correctness

**Guarantee:** Velocity magnitude is always up-to-date because:
1. Magnitude is computed when line is created
2. Magnitude is recomputed after every position update
3. Bounding box calculations use cached value (computed in same frame)

**Note:** Since velocity doesn't change during a frame (only position changes), velocity magnitude is constant per frame. However, recomputing after position updates ensures correctness even if the implementation changes in the future.

## Testing

**Verification Steps:**
1. ✅ Code compiles without errors
2. ✅ Program runs correctly (tested with beaver.in)
3. ✅ Correctness verified (collision counts match brute-force)
4. ⏳ Benchmark performance improvement
5. ⏳ Profile to verify sqrt() reduction

## Related Optimizations

This optimization is similar to:
- **Line length caching (Optimization #1):** Precomputed value used instead of recomputing
- **maxVelocity caching (Session #4):** Precomputed value used instead of recomputing

All three optimizations share the same principle: identify expensive operations (sqrt()) that are computed repeatedly with the same inputs, and cache the results.

## Lessons Learned

1. **Identify repeated expensive operations:** sqrt() is expensive, computing it thousands of times is wasteful
2. **Cache constant values:** Velocity magnitude is constant per frame (velocity doesn't change)
3. **Precompute in update phase:** Computing once per frame in `updatePosition` is more efficient than computing per bounding box
4. **Simple optimizations can have big impact:** This is a simple change but eliminates thousands of expensive operations

## Files Modified

1. **line.h**
   - Added `cachedVelocityMagnitude` field to Line struct
   - Added documentation explaining purpose and usage

2. **collision_world.c**
   - Modified `CollisionWorld_updatePosition()` to recompute `cachedVelocityMagnitude`
   - Ensures cached value is always up-to-date

3. **line_demo.c**
   - Added initialization of `cachedVelocityMagnitude` when lines are created
   - Ensures cached value is valid from the start

4. **quadtree.c**
   - Modified `computeLineBoundingBox()` to use `cachedVelocityMagnitude` instead of `Vec_length(line->velocity)`
   - Eliminates expensive sqrt() operations during bounding box calculations

5. **performance_analysis/07-velocity-magnitude-caching.md**
   - Created comprehensive documentation of optimization
   - Includes problem analysis, solution design, implementation details, performance analysis, and correctness guarantees

6. **performance_analysis/INDEX.md**
   - Added entry for Optimization #3: Velocity Magnitude Caching
   - Updated performance analysis index

## Next Steps

1. Run benchmarks to measure actual performance improvement
2. Profile with `perf` to verify sqrt() reduction
3. Document results in main performance analysis document

