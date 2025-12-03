# Bounding Box Analysis

## Problem Summary

Quadtree is missing collisions because bounding boxes don't overlap, even though lines actually intersect.

## Key Findings

### Bounding Box Gap (Frame 89)

**Line 105:**
- Bounding box: `[0.522491, 0.523024]x[0.679063, 0.695937]`
- Max X: 0.523024
- Cell boundary: 0.523438
- Distance to boundary: **0.000414** (very close!)

**Line 41:**
- Bounding box: `[0.528405, 0.529351]x[0.684063, 0.690937]`
- Min X: 0.528405
- Cell boundary: 0.523438
- Distance from boundary: 0.004967

**Gap between bounding boxes:** 0.005381

### Critical Observation

Line 105 is only **0.000414** away from the cell boundary at 0.523438. This is extremely close, suggesting:
1. Numerical precision issues
2. Bounding box might be slightly too small
3. Need for small expansion to account for swept area

## Root Cause Hypothesis

### Hypothesis 1: Bounding Box Too Small

The current bounding box computation uses only 4 corner points:
- p1_current, p2_current
- p1_future, p2_future

But a moving line segment sweeps out a **parallelogram**, and the bounding box of just 4 points might not fully account for:
1. The actual swept area
2. Numerical precision errors
3. Edge cases near cell boundaries

### Hypothesis 2: Intersection Test Uses Relative Velocity

The intersection test (`intersect()`) uses:
- Relative velocity: `l2->velocity - l1->velocity`
- Parallelogram: `l2->p1 + relative_vel*time` to `l2->p2 + relative_vel*time`

But bounding box computation uses:
- Absolute velocity: `l1->velocity`
- Future position: `p1 + velocity*time`

These are different! The intersection test considers relative motion, but bounding box considers absolute motion.

### Hypothesis 3: Need for Expansion

Even if bounding box computation is correct, we might need a small expansion to:
1. Account for numerical precision
2. Ensure we don't miss pairs near cell boundaries
3. Account for the actual swept parallelogram area

## Proposed Solution

### Option 1: Expand Bounding Boxes Slightly

Add a small epsilon expansion to bounding boxes:

```c
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax) {
  // ... existing computation ...
  
  // Expand slightly to account for:
  // 1. Numerical precision
  // 2. Swept parallelogram area
  // 3. Cell boundary edge cases
  double epsilon = 0.001;  // ~0.1% of typical cell size
  *xmin -= epsilon;
  *xmax += epsilon;
  *ymin -= epsilon;
  *ymax += epsilon;
}
```

**Pros:**
- Simple fix
- Accounts for numerical precision
- Ensures pairs near boundaries are found

**Cons:**
- Might find more false positives
- Adds small overhead

### Option 2: Compute Bounding Box of Swept Parallelogram

Compute the actual bounding box of the swept parallelogram:

```c
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax) {
  Vec p1_current = line->p1;
  Vec p2_current = line->p2;
  Vec p1_future = Vec_add(p1_current, Vec_multiply(line->velocity, timeStep));
  Vec p2_future = Vec_add(p2_current, Vec_multiply(line->velocity, timeStep));
  
  // The swept area is a parallelogram with vertices:
  // p1_current, p2_current, p1_future, p2_future
  // The bounding box of these 4 points should contain the parallelogram
  // But we need to ensure we account for the full extent
  
  // Current implementation is correct for axis-aligned cases
  // But might need expansion for non-axis-aligned movement
  *xmin = minDouble(minDouble(p1_current.x, p2_current.x),
                    minDouble(p1_future.x, p2_future.x));
  *xmax = maxDouble(maxDouble(p1_current.x, p2_current.x),
                    maxDouble(p1_future.x, p2_future.x));
  *ymin = minDouble(minDouble(p1_current.y, p2_current.y),
                    minDouble(p1_future.y, p2_future.y));
  *ymax = maxDouble(maxDouble(p1_current.y, p2_current.y),
                    maxDouble(p1_future.y, p2_future.y));
  
  // Add small expansion for numerical precision and edge cases
  double epsilon = 0.0005;  // Half of 0.001
  *xmin -= epsilon;
  *xmax += epsilon;
  *ymin -= epsilon;
  *ymax += epsilon;
}
```

**Pros:**
- More accurate
- Accounts for swept area
- Handles edge cases

**Cons:**
- More complex
- Still might need epsilon

### Option 3: Use Relative Velocity in Bounding Box

This is probably wrong - bounding boxes should use absolute positions, not relative.

## Recommended Approach

**Start with Option 1 (small expansion):**

1. Add epsilon expansion of 0.0005 to 0.001
2. Test if it fixes the missing pairs
3. Verify no increase in false positives
4. If needed, refine epsilon value

**Rationale:**
- Line 105 is only 0.000414 from cell boundary
- Small expansion (0.0005-0.001) should fix this
- Simple and low risk
- Can be refined based on testing

## Testing Plan

1. **Add epsilon expansion** to `computeLineBoundingBox()`
2. **Re-run debug build** with frame 89
3. **Check cell assignments:**
   - Verify lines 41-93 and 105 are now in overlapping cells
   - Check if candidate pairs are found
4. **Verify correctness:**
   - Run full simulation
   - Compare collision counts with brute-force
   - Check for new false positives
5. **Refine epsilon** if needed based on results

## Expected Outcome

After adding epsilon expansion:
- Lines 41-93 and 105 should be in overlapping cells
- Quadtree should find the 9 missing pairs
- Collision counts should match brute-force
- No significant increase in false positives

