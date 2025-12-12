# Optimization #1: Line Length Caching

## Overview

**Date:** December 2025  
**Status:** ✅ Implemented  
**Impact:** Reduces expensive `sqrt()` operations in collision solver

## Problem

The collision solver (`CollisionWorld_collisionSolver`) computes line segment length for every collision using:

```c
double m1 = Vec_length(Vec_subtract(l1->p1, l1->p2));
double m2 = Vec_length(Vec_subtract(l2->p1, l2->p2));
```

**Issues:**
1. `Vec_length()` calls `hypot()` which performs an expensive `sqrt()` operation
2. Line length is computed **twice per collision** (once for each line)
3. **Line length never changes** - lines are rigid bodies, so the distance between p1 and p2 is constant
4. For high collision density inputs, this results in thousands of redundant `sqrt()` calls per frame

**Example Impact:**
- `sin_wave.in`: 279,712 collisions over 100 frames = ~2,797 collisions/frame
- Each collision computes 2 line lengths = **5,594 sqrt() calls per frame**
- Over 100 frames = **559,400 redundant sqrt() operations**

## Solution

**Precompute and cache line length:**
1. Add `cachedLength` field to `Line` struct
2. Compute length once when line is created (`line_demo.c`)
3. Recompute length once per frame in `CollisionWorld_updatePosition()` (ensures correctness even if line shape changes)
4. Use cached value in `CollisionWorld_collisionSolver()` instead of computing on-the-fly

## Implementation

### 1. Added Cached Length Field (`line.h`)

```c
struct Line {
  Vec p1;
  Vec p2;
  Vec velocity;
  Color color;
  unsigned int id;
  double cachedLength;  // NEW: Cached line segment length
};
```

### 2. Initialize on Line Creation (`line_demo.c`)

```c
// Initialize cached line length (optimization: precompute to avoid expensive sqrt in collision solver)
line->cachedLength = Vec_length(Vec_subtract(line->p1, line->p2));
```

### 3. Update After Position Changes (`collision_world.c`)

```c
void CollisionWorld_updatePosition(CollisionWorld* collisionWorld) {
  double t = collisionWorld->timeStep;
  for (int i = 0; i < collisionWorld->numOfLines; i++) {
    Line *line = collisionWorld->lines[i];
    line->p1 = Vec_add(line->p1, Vec_multiply(line->velocity, t));
    line->p2 = Vec_add(line->p2, Vec_multiply(line->velocity, t));
    // Precompute line length once per frame (used in collision solver)
    line->cachedLength = Vec_length(Vec_subtract(line->p1, line->p2));
  }
}
```

### 4. Use Cached Value in Collision Solver (`collision_world.c`)

```c
// BEFORE:
double m1 = Vec_length(Vec_subtract(l1->p1, l1->p2));
double m2 = Vec_length(Vec_subtract(l2->p1, l2->p2));

// AFTER:
double m1 = l1->cachedLength;
double m2 = l2->cachedLength;
```

## Performance Analysis

### Expected Impact

**Before:**
- 2 `Vec_length()` calls per collision
- Each `Vec_length()` = 1 `hypot()` call = 1 `sqrt()` operation
- Total: **2 sqrt() calls per collision**

**After:**
- 1 `Vec_length()` call per line per frame (in `updatePosition`)
- 0 `Vec_length()` calls per collision (uses cached value)
- Total: **n sqrt() calls per frame** (where n = number of lines)

**Savings:**
- For inputs with many collisions: **Eliminates thousands of sqrt() calls per frame**
- Example: `sin_wave.in` with 2,797 collisions/frame
  - Before: 5,594 sqrt() calls/frame
  - After: 1,181 sqrt() calls/frame (one per line)
  - **Savings: 4,413 sqrt() calls/frame (79% reduction)**

### Complexity Analysis

**Before:**
- Collision solver: O(c) where c = number of collisions
- Each collision: O(1) but expensive (2 sqrt() calls)

**After:**
- Position update: O(n) where n = number of lines (1 sqrt() per line)
- Collision solver: O(c) with O(1) per collision (no sqrt())
- **Total: O(n + c)** instead of O(c) with expensive operations

## Correctness

**Guarantee:** Line length is always up-to-date because:
1. Length is computed when line is created
2. Length is recomputed after every position update
3. Collision solver uses cached value (computed in same frame)

**Note:** Since lines are rigid bodies, length is constant. However, recomputing after position updates ensures correctness even if the implementation changes in the future.

## Testing

**Verification Steps:**
1. ✅ Code compiles without errors
2. ⏳ Run correctness tests (ensure collision counts match)
3. ⏳ Benchmark performance improvement
4. ⏳ Profile to verify sqrt() reduction

## Next Steps

1. Run benchmarks to measure actual performance improvement
2. Profile with `perf` to verify sqrt() reduction
3. Document results in main performance analysis document

## Related Optimizations

This optimization is similar to:
- **maxVelocity caching** (Session #4): Precomputed value used instead of recomputing
- **Bounding box caching** (Session #3): Precomputed values reused

## Lessons Learned

1. **Identify repeated expensive operations:** `sqrt()` is expensive, computing it thousands of times is wasteful
2. **Cache constant values:** Line length is constant for rigid bodies
3. **Precompute in update phase:** Computing once per frame in `updatePosition` is more efficient than computing per collision
4. **Simple optimizations can have big impact:** This is a simple change but eliminates thousands of expensive operations

