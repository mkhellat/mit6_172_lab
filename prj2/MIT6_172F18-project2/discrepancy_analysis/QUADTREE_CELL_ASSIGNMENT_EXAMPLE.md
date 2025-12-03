# Quadtree Cell Assignment: How Parallelogram Maps to Cells

## Overview

This document explains exactly how a moving line segment's swept area (parallelogram) is used to assign the line to quadtree cells. We trace through concrete examples to understand the process step-by-step.

---

## Current Implementation: Step-by-Step Process

### Step 1: Compute Future Positions

For each line segment, we have:
- **Current endpoints**: `p1_current = (p1_x, p1_y)`, `p2_current = (p2_x, p2_y)`
- **Velocity vector**: `v = (v_x, v_y)` (same for entire line segment)
- **Time step**: `timeStep`

**Future endpoints are computed as:**
```
p1_future_x = p1_current_x + v_x * timeStep
p1_future_y = p1_current_y + v_y * timeStep

p2_future_x = p2_current_x + v_x * timeStep
p2_future_y = p2_current_y + v_y * timeStep
```

**Key Point**: Both endpoints move by the **same velocity vector**, so the line segment translates as a rigid body.

### Step 2: Form the Parallelogram

The 4 vertices of the swept parallelogram are:
1. `(p1_current_x, p1_current_y)` - Current position of endpoint 1
2. `(p2_current_x, p2_current_y)` - Current position of endpoint 2
3. `(p1_future_x, p1_future_y)` - Future position of endpoint 1
4. `(p2_future_x, p2_future_y)` - Future position of endpoint 2

**Visual representation:**
```
p1_current ──────────> p1_future
    │                      │
    │   (swept area)       │
    │                      │
p2_current ──────────> p2_future
```

This forms a **parallelogram** representing all positions the line segment occupies during the time step.

### Step 3: Compute Axis-Aligned Bounding Box (AABB)

We compute the **axis-aligned bounding box** that contains all 4 vertices:

```c
xmin = min(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
xmax = max(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
ymin = min(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
ymax = max(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
```

**Important**: This AABB is a **conservative approximation**:
- It always contains the entire parallelogram
- It may be larger than the parallelogram (if parallelogram is rotated)
- It's axis-aligned (edges parallel to X and Y axes)

### Step 4: Find Overlapping Quadtree Cells

For each quadtree cell, we check if the line's AABB overlaps the cell's bounding box:

```c
bool boxesOverlap(lineXmin, lineXmax, lineYmin, lineYmax,
                  cell->xmin, cell->xmax, cell->ymin, cell->ymax)
```

**Overlap condition**: Two axis-aligned rectangles overlap if they overlap in **both** X and Y dimensions:
- X overlap: `!(lineXmax < cellXmin || lineXmin > cellXmax)`
- Y overlap: `!(lineYmax < cellYmin || lineYmin > cellYmax)`
- Both must be true for overall overlap

### Step 5: Insert Line into All Overlapping Cells

The line is inserted into **every cell** whose bounding box overlaps with the line's AABB.

**Key Point**: A line can be in multiple cells if its AABB spans cell boundaries.

---

## Concrete Example 1: Simple Horizontal Movement

### Setup

**World bounds**: [0.0, 1.0] x [0.0, 1.0]  
**Quadtree depth**: 2 (4 cells at leaf level)  
**Cell boundaries at depth 2:**
- Cell A: [0.0, 0.5) x [0.0, 0.5)
- Cell B: [0.5, 1.0) x [0.0, 0.5)
- Cell C: [0.0, 0.5) x [0.5, 1.0)
- Cell D: [0.5, 1.0) x [0.5, 1.0)

**Time step**: `timeStep = 0.1`

### Line 1: Moving Right

**Line parameters:**
- `p1_current = (0.2, 0.3)`
- `p2_current = (0.2, 0.4)`
- `velocity = (0.5, 0.0)`  // Moving right, no vertical movement
- `timeStep = 0.1`

**Step 1: Compute future positions**
```
p1_future_x = 0.2 + 0.5 * 0.1 = 0.2 + 0.05 = 0.25
p1_future_y = 0.3 + 0.0 * 0.1 = 0.3 + 0.0 = 0.3

p2_future_x = 0.2 + 0.5 * 0.1 = 0.2 + 0.05 = 0.25
p2_future_y = 0.4 + 0.0 * 0.1 = 0.4 + 0.0 = 0.4
```

**Future positions:**
- `p1_future = (0.25, 0.3)`
- `p2_future = (0.25, 0.4)`

**Step 2: Parallelogram vertices**
1. `(0.2, 0.3)` - p1_current
2. `(0.2, 0.4)` - p2_current
3. `(0.25, 0.3)` - p1_future
4. `(0.25, 0.4)` - p2_future

**Visual representation:**
```
(0.2, 0.3) ──────────> (0.25, 0.3)
    │                        │
    │    (swept area)        │
    │                        │
(0.2, 0.4) ──────────> (0.25, 0.4)
```

**Step 3: Compute AABB**
```
xmin = min(0.2, 0.2, 0.25, 0.25) = 0.2
xmax = max(0.2, 0.2, 0.25, 0.25) = 0.25
ymin = min(0.3, 0.4, 0.3, 0.4) = 0.3
ymax = max(0.3, 0.4, 0.3, 0.4) = 0.4
```

**AABB**: [0.2, 0.25] x [0.3, 0.4]

**Step 4: Check cell overlaps**

**Cell A [0.0, 0.5) x [0.0, 0.5):**
- X overlap: `!(0.25 < 0.0 || 0.2 > 0.5)` = `!(false || false)` = `true` ✓
- Y overlap: `!(0.4 < 0.0 || 0.3 > 0.5)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Cell B [0.5, 1.0) x [0.0, 0.5):**
- X overlap: `!(0.25 < 0.5 || 0.2 > 1.0)` = `!(true || false)` = `false` ✗
- **Result**: NO OVERLAP

**Cell C [0.0, 0.5) x [0.5, 1.0):**
- Y overlap: `!(0.4 < 0.5 || 0.3 > 1.0)` = `!(true || false)` = `false` ✗
- **Result**: NO OVERLAP

**Cell D [0.5, 1.0) x [0.5, 1.0):**
- X overlap: `false` ✗
- **Result**: NO OVERLAP

**Step 5: Insert line**
- Line 1 inserted into **Cell A only**

---

## Concrete Example 2: Diagonal Movement Crossing Boundaries

### Line 2: Moving Diagonally Up-Right

**Line parameters:**
- `p1_current = (0.48, 0.48)`
- `p2_current = (0.48, 0.52)`
- `velocity = (0.3, 0.2)`  // Moving diagonally
- `timeStep = 0.1`

**Step 1: Compute future positions**
```
p1_future_x = 0.48 + 0.3 * 0.1 = 0.48 + 0.03 = 0.51
p1_future_y = 0.48 + 0.2 * 0.1 = 0.48 + 0.02 = 0.50

p2_future_x = 0.48 + 0.3 * 0.1 = 0.48 + 0.03 = 0.51
p2_future_y = 0.52 + 0.2 * 0.1 = 0.52 + 0.02 = 0.54
```

**Future positions:**
- `p1_future = (0.51, 0.50)`
- `p2_future = (0.51, 0.54)`

**Step 2: Parallelogram vertices**
1. `(0.48, 0.48)` - p1_current
2. `(0.48, 0.52)` - p2_current
3. `(0.51, 0.50)` - p1_future
4. `(0.51, 0.54)` - p2_future

**Visual representation:**
```
(0.48, 0.48) ──────────> (0.51, 0.50)
    │                        │
    │    (swept area)        │
    │                        │
(0.48, 0.52) ──────────> (0.51, 0.54)
```

**Step 3: Compute AABB**
```
xmin = min(0.48, 0.48, 0.51, 0.51) = 0.48
xmax = max(0.48, 0.48, 0.51, 0.51) = 0.51
ymin = min(0.48, 0.52, 0.50, 0.54) = 0.48
ymax = max(0.48, 0.52, 0.50, 0.54) = 0.54
```

**AABB**: [0.48, 0.51] x [0.48, 0.54]

**Step 4: Check cell overlaps**

**Cell A [0.0, 0.5) x [0.0, 0.5):**
- X overlap: `!(0.51 < 0.0 || 0.48 > 0.5)` = `!(false || false)` = `true` ✓
- Y overlap: `!(0.54 < 0.0 || 0.48 > 0.5)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Cell B [0.5, 1.0) x [0.0, 0.5):**
- X overlap: `!(0.51 < 0.5 || 0.48 > 1.0)` = `!(false || false)` = `true` ✓
  - Note: 0.51 > 0.5, so AABB extends into Cell B
- Y overlap: `!(0.54 < 0.0 || 0.48 > 0.5)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Cell C [0.0, 0.5) x [0.5, 1.0):**
- X overlap: `!(0.51 < 0.0 || 0.48 > 0.5)` = `!(false || false)` = `true` ✓
- Y overlap: `!(0.54 < 0.5 || 0.48 > 1.0)` = `!(false || false)` = `true` ✓
  - Note: 0.54 > 0.5, so AABB extends into Cell C
- **Result**: OVERLAPS

**Cell D [0.5, 1.0) x [0.5, 1.0):**
- X overlap: `!(0.51 < 0.5 || 0.48 > 1.0)` = `!(false || false)` = `true` ✓
- Y overlap: `!(0.54 < 0.5 || 0.48 > 1.0)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Step 5: Insert line**
- Line 2 inserted into **ALL 4 cells (A, B, C, D)**

**Why all 4?** The AABB [0.48, 0.51] x [0.48, 0.54] spans:
- X boundary at 0.5 (0.48-0.51 crosses it)
- Y boundary at 0.5 (0.48-0.54 crosses it)
- So it overlaps all 4 quadrants

---

## Concrete Example 3: Near Boundary (Similar to Line 105 Issue)

### Line 3: Slow Movement Near X Boundary

**Line parameters:**
- `p1_current = (0.495, 0.3)`
- `p2_current = (0.495, 0.4)`
- `velocity = (0.1, 0.0)`  // Moving slowly right
- `timeStep = 0.1`

**Step 1: Compute future positions**
```
p1_future_x = 0.495 + 0.1 * 0.1 = 0.495 + 0.01 = 0.505
p1_future_y = 0.3 + 0.0 * 0.1 = 0.3 + 0.0 = 0.3

p2_future_x = 0.495 + 0.1 * 0.1 = 0.495 + 0.01 = 0.505
p2_future_y = 0.4 + 0.0 * 0.1 = 0.4 + 0.0 = 0.4
```

**Future positions:**
- `p1_future = (0.505, 0.3)`
- `p2_future = (0.505, 0.4)`

**Step 2: Parallelogram vertices**
1. `(0.495, 0.3)` - p1_current
2. `(0.495, 0.4)` - p2_current
3. `(0.505, 0.3)` - p1_future
4. `(0.505, 0.4)` - p2_future

**Visual representation:**
```
(0.495, 0.3) ──────────> (0.505, 0.3)
    │                        │
    │    (swept area)        │
    │                        │
(0.495, 0.4) ──────────> (0.505, 0.4)
```

**Step 3: Compute AABB**
```
xmin = min(0.495, 0.495, 0.505, 0.505) = 0.495
xmax = max(0.495, 0.495, 0.505, 0.505) = 0.505
ymin = min(0.3, 0.4, 0.3, 0.4) = 0.3
ymax = max(0.3, 0.4, 0.3, 0.4) = 0.4
```

**AABB**: [0.495, 0.505] x [0.3, 0.4]

**Step 4: Check cell overlaps**

**Cell A [0.0, 0.5) x [0.0, 0.5):**
- X overlap: `!(0.505 < 0.0 || 0.495 > 0.5)` = `!(false || false)` = `true` ✓
  - Note: 0.505 > 0.5, but overlap check uses `<=` logic
  - Actually: `!(xmax < xmin_other || xmin > xmax_other)`
  - `!(0.505 < 0.0 || 0.495 > 0.5)` = `!(false || false)` = `true`
- Y overlap: `!(0.4 < 0.0 || 0.3 > 0.5)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Cell B [0.5, 1.0) x [0.0, 0.5):**
- X overlap: `!(0.505 < 0.5 || 0.495 > 1.0)` = `!(false || false)` = `true` ✓
  - Note: 0.505 > 0.5, so AABB extends into Cell B
- Y overlap: `!(0.4 < 0.0 || 0.3 > 0.5)` = `!(false || false)` = `true` ✓
- **Result**: OVERLAPS

**Cell C [0.0, 0.5) x [0.5, 1.0):**
- Y overlap: `!(0.4 < 0.5 || 0.3 > 1.0)` = `!(true || false)` = `false` ✗
- **Result**: NO OVERLAP

**Cell D [0.5, 1.0) x [0.5, 1.0):**
- Y overlap: `false` ✗
- **Result**: NO OVERLAP

**Step 5: Insert line**
- Line 3 inserted into **Cell A and Cell B** (spans X boundary at 0.5)

---

## The Line 105 Problem: Real-World Case

### Actual Data from Frame 89

**Line 105:**
- Bounding box: [0.522491, 0.523024] x [0.679063, 0.695937]
- Cell boundary at X = 0.523438
- Line 105 max X = 0.523024
- **Distance to boundary**: 0.523438 - 0.523024 = 0.000414

**Lines 41-93:**
- Bounding box: [0.528405, 0.529351] x [0.684063, 0.690937]
- In cells: [0.523438, 0.531250) x [...]
- Line 41 min X = 0.528405

**The Problem:**
- Line 105 AABB: [0.522491, 0.523024]
- Cell boundary: 0.523438
- Line 105 max (0.523024) < Cell boundary (0.523438)
- So Line 105 is in cells [0.515625, 0.523438) only
- Lines 41-93 are in cells [0.523438, 0.531250)
- **No overlap** → quadtree never finds them as candidate pairs

**But brute-force found collisions!** This means the lines actually intersect, but the quadtree's spatial partitioning separated them.

---

## Key Observations

### Observation 1: AABB is Conservative

The axis-aligned bounding box is a **conservative approximation**:
- ✅ Always contains the entire parallelogram
- ⚠️ May be larger than necessary (if parallelogram is rotated)
- ⚠️ May miss cases where parallelogram extends beyond AABB in certain directions

**For a parallelogram formed by translating a line segment:**
- The AABB of the 4 vertices **should** contain the entire parallelogram
- This is mathematically guaranteed for any convex shape

### Observation 2: Boundary Cases

When a line's AABB spans a cell boundary:
- The line is inserted into **all overlapping cells**
- This is correct - the line's swept area does cross the boundary
- Example: Line 3 spans X=0.5 boundary, goes into both Cell A and B

### Observation 3: The Line 105 Issue

**The critical question**: Why was Line 105's bounding box [0.522491, 0.523024] ending at 0.523024 when it should have extended to at least 0.523438?

**Possible explanations:**
1. **Bounding box computation is correct** - The 4 vertices actually produce this AABB
2. **Numerical precision** - Floating-point errors in calculations
3. **The swept area extends beyond AABB** - But this shouldn't happen for a parallelogram
4. **Something else** - Need to investigate actual line positions and velocities

### Observation 4: Why Epsilon "Fixed" It (But Not Properly)

With epsilon expansion of 0.0005:
- Line 105 AABB becomes [0.521991, 0.523524]
- Now 0.523524 > 0.523438 (cell boundary)
- So Line 105 overlaps cells [0.523438, 0.531250) as well
- Now quadtree finds the pairs

**But this is a band-aid!** The real question is: why was the original AABB [0.522491, 0.523024] computed? Was it correct based on the actual line positions, or is there an error in the computation?

---

## Mathematical Verification

### For a Parallelogram

Given 4 vertices of a parallelogram:
- `v1 = (x1, y1)`
- `v2 = (x2, y2)`
- `v3 = (x3, y3)`
- `v4 = (x4, y4)`

The AABB is:
```
xmin = min(x1, x2, x3, x4)
xmax = max(x1, x2, x3, x4)
ymin = min(y1, y2, y3, y4)
ymax = max(y1, y2, y3, y4)
```

**This AABB is guaranteed to contain the entire parallelogram** because:
- A parallelogram is a convex shape
- The bounding box of any convex shape's vertices contains the entire shape
- This is a fundamental property of convex hulls

### For Our Case

Our parallelogram is formed by:
- `v1 = p1_current`
- `v2 = p2_current`
- `v3 = p1_future = p1_current + velocity * timeStep`
- `v4 = p2_future = p2_current + velocity * timeStep`

**The AABB of these 4 points MUST contain the entire parallelogram.**

So if the AABB is [0.522491, 0.523024], then the parallelogram is entirely within this range. The question is: **Is this the correct AABB for the actual line positions?**

---

## Next Steps for Investigation

1. **Verify actual line positions** in frame 89:
   - What are the exact p1_current, p2_current for Line 105?
   - What is the exact velocity for Line 105?
   - Compute p1_future, p2_future manually
   - Verify the AABB computation

2. **Compare with brute-force**:
   - If brute-force found collisions, the lines must be close
   - Check actual line positions when brute-force tested the pairs
   - See if there's a discrepancy

3. **Investigate the root cause**:
   - Is the AABB computation correct?
   - Are the line positions correct?
   - Is there a timing issue (when are positions sampled)?

This will help us determine if the issue is:
- A bug in bounding box computation
- A timing/sampling issue
- A fundamental limitation of the AABB approach
- Something else entirely
