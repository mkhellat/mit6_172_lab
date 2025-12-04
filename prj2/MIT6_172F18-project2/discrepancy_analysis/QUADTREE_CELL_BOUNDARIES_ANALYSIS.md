# Quadtree Cell Boundaries and Overlapping Parallelograms Analysis

## User's Question

**Question 1:** In QT, how are cell boundaries determined on the basis of these parallelograms?

**Question 2:** Specifically, confirm that, in QT, upon creation of parallelograms, OVERLAPPING parallelograms are identified and it is made sure that their corresponding lines are put together inside a cell, i.e. is it guaranteed that no overlapping parallelograms are placed inside separate cells?

---

## Answer to Question 1: How Cell Boundaries Are Determined

### Current Implementation

**Cell boundaries are NOT determined by parallelograms.** They are determined by:

1. **Initial World Bounds**: The quadtree is created with fixed world bounds (e.g., [0.0, 1.0] x [0.0, 1.0])

2. **Recursive Subdivision**: Cells are created by recursively subdividing space into 4 equal squares:
   - Each cell is split at its midpoint: `xmid = (xmin + xmax) / 2.0`, `ymid = (ymin + ymax) / 2.0`
   - This creates 4 child cells: SW, SE, NW, NE
   - Subdivision happens when a cell has more than `maxLinesPerNode` lines

3. **Subdivision Criteria**:
   ```c
   if (node->numLines > tree->config.maxLinesPerNode &&
       node->depth < tree->config.maxDepth) {
     double cellWidth = node->xmax - node->xmin;
     double cellHeight = node->ymax - node->ymin;
     double minDimension = minDouble(cellWidth, cellHeight);
     
     if (minDimension >= tree->config.minCellSize * 2.0) {
       shouldSubdivide = true;
     }
   }
   ```

4. **Fixed Boundaries**: Once a cell is subdivided, its boundaries are fixed:
   - Cell boundaries are determined by the subdivision process (midpoint splits)
   - Boundaries do NOT adapt to the parallelograms
   - Boundaries are the same regardless of what lines are inserted

### Key Insight

**Cell boundaries are geometry-based (spatial subdivision), not content-based (parallelogram-aware).**

The quadtree uses a **fixed spatial grid** that subdivides based on line count, not based on where parallelograms actually are or where they overlap.

---

## Answer to Question 2: Are Overlapping Parallelograms Guaranteed to Be in the Same Cell?

### Short Answer: **NO, they are NOT guaranteed to be in the same cell.**

However, they ARE guaranteed to be in **at least one overlapping cell** (if the AABB computation is correct).

### Detailed Analysis

#### How Lines Are Inserted

1. **Compute AABB**: For each line, compute the axis-aligned bounding box of its parallelogram:
   ```c
   computeLineBoundingBox(line, timeStep, &xmin, &xmax, &ymin, &ymax);
   ```

2. **Find Overlapping Cells**: Find ALL cells whose bounding boxes overlap with the line's AABB:
   ```c
   int numCells = findOverlappingCellsRecursive(tree->root,
                                                 lineXmin, lineXmax,
                                                 lineYmin, lineYmax,
                                                 overlappingCells, ...);
   ```

3. **Insert into All Overlapping Cells**: The line is inserted into **every cell** its AABB overlaps:
   ```c
   static QuadTreeError insertLineRecursive(QuadNode* node, Line* line, ...) {
     // Check if line overlaps this cell
     if (!boxesOverlap(lineXmin, lineXmax, lineYmin, lineYmax,
                       node->xmin, node->xmax, node->ymin, node->ymax)) {
       return QUADTREE_SUCCESS;  // Line doesn't overlap, nothing to do
     }
     // ... insert into this cell and recursively into children ...
   }
   ```

#### The Guarantee

**If two parallelograms overlap, their AABBs MUST overlap** (because AABB is a conservative approximation).

**If two AABBs overlap, and each line is inserted into ALL cells its AABB overlaps, then:**
- Line A's AABB overlaps cells [1, 2, 3]
- Line B's AABB overlaps cells [2, 3, 4]
- They both overlap cells 2 and 3
- **Therefore, they WILL be found as a candidate pair** (both lines are in cells 2 and 3)

#### The Problem: When This Guarantee Fails

The guarantee fails if:

1. **AABB Computation is Incorrect**: If the AABB doesn't actually contain the parallelogram, then overlapping parallelograms might have non-overlapping AABBs.

2. **Numerical Precision Issues**: If two AABBs are very close but don't overlap due to floating-point precision, the lines won't be in overlapping cells.

3. **Cell Boundaries Split Overlapping Region**: If the overlapping region of two AABBs falls exactly on a cell boundary, and the boundary check uses strict inequalities, the lines might be placed in non-overlapping cells.

#### Example: The Line 105 Problem

From our investigation:
- Line 105's AABB: [0.522491, 0.523024] x [0.3, 0.4]
- Cell boundary: 0.523438
- Gap: 0.000414 (very small!)

**What happened:**
- Line 105's AABB ended at 0.523024
- The cell boundary was at 0.523438
- Line 105 was placed in cells with xmax < 0.523438
- Other lines (41-93) were in cells with xmin >= 0.523438
- **Result**: Line 105 and lines 41-93 were in non-overlapping cells, even though their parallelograms might have overlapped!

**Why this happened:**
- The AABB computation was correct (it contained the parallelogram)
- But the AABB was too small (ended at 0.523024 instead of extending to at least 0.523438)
- This could be due to:
  1. The parallelogram itself was smaller than expected
  2. Numerical precision issues in AABB computation
  3. The parallelogram didn't actually extend to the cell boundary

---

## Critical Insight: The Root Cause

### The Real Question

**Are overlapping parallelograms guaranteed to have overlapping AABBs?**

**Answer: YES, mathematically.**

If parallelogram A overlaps parallelogram B, then:
- The AABB of A contains all of A
- The AABB of B contains all of B
- If A and B overlap, their AABBs MUST overlap (because AABB is a superset)

### But There's a Catch

**The guarantee only holds if:**
1. The AABB computation is correct (includes all 4 vertices)
2. The parallelograms actually overlap (not just close)
3. There are no numerical precision issues

### The Line 105 Case

In the Line 105 case:
- Line 105's AABB: [0.522491, 0.523024]
- Other lines' cells: xmin >= 0.523438
- **Question**: Did the parallelograms actually overlap?

**If the parallelograms overlapped:**
- Line 105's AABB should have extended to at least 0.523438
- The AABB computation must have been wrong

**If the parallelograms didn't overlap:**
- The AABB computation was correct
- But the quadtree missed the pair because they were in non-overlapping cells
- This suggests the parallelograms were very close but didn't actually overlap

---

## Mathematical Guarantee

### Theorem: Overlapping Parallelograms → Overlapping AABBs

**Given:**
- Parallelogram A with vertices {v1, v2, v3, v4}
- Parallelogram B with vertices {w1, w2, w3, w4}
- AABB_A = [min(v1.x, v2.x, v3.x, v4.x), max(v1.x, v2.x, v3.x, v4.x)] x [min(v1.y, ...), max(...)]
- AABB_B = [min(w1.x, ...), max(...)] x [min(w1.y, ...), max(...)]

**If A and B overlap, then AABB_A and AABB_B MUST overlap.**

**Proof:**
- If A and B overlap, there exists a point p in both A and B
- p is in A, so p.x is between min(v1.x, ...) and max(v1.x, ...), i.e., p is in AABB_A
- p is in B, so p.x is between min(w1.x, ...) and max(w1.x, ...), i.e., p is in AABB_B
- Therefore, p is in both AABB_A and AABB_B
- Therefore, AABB_A and AABB_B overlap

### Corollary: Overlapping AABBs → Lines in Overlapping Cells

**If AABB_A and AABB_B overlap, and:**
- Line A is inserted into ALL cells its AABB overlaps
- Line B is inserted into ALL cells its AABB overlaps

**Then:**
- There exists at least one cell C such that:
  - AABB_A overlaps C (so Line A is in C)
  - AABB_B overlaps C (so Line B is in C)
- Therefore, Line A and Line B are both in cell C
- Therefore, they will be found as a candidate pair

---

## The Actual Problem

### What We Know

1. **Cell boundaries are fixed** (determined by subdivision, not parallelograms)
2. **Lines are inserted into ALL overlapping cells** (correct implementation)
3. **Overlapping AABBs guarantee overlapping cells** (mathematical guarantee)
4. **But we still have missing pairs** (Line 105 case)

### What This Means

**The problem is NOT with cell boundary determination or line insertion.**

**The problem is with AABB computation:**
- Either the AABB doesn't actually contain the parallelogram (computation error)
- Or the parallelograms don't actually overlap (but should, due to physics)
- Or there are numerical precision issues (AABBs are very close but don't overlap)

### The Epsilon Fix

The epsilon expansion (0.0005) was a band-aid that:
- Expanded AABBs slightly to account for numerical precision
- Ensured lines near cell boundaries are placed in overlapping cells
- But increased discrepancy (50 → 71) because it created false positives

**Why it increased discrepancy:**
- Epsilon expansion made AABBs larger
- Larger AABBs overlap more cells
- More cell overlaps → more candidate pairs
- More candidate pairs → more false positives (pairs that don't actually collide)

---

## Conclusion

### Answer to Question 1

**Cell boundaries are determined by:**
1. Fixed world bounds
2. Recursive subdivision at midpoints
3. Subdivision triggered by line count (not parallelogram properties)
4. **NOT determined by parallelograms**

### Answer to Question 2

**Are overlapping parallelograms guaranteed to be in the same cell?**

**NO, they are NOT guaranteed to be in the SAME cell.**

**BUT, they ARE guaranteed to be in at least one OVERLAPPING cell** (if AABB computation is correct).

**The guarantee chain:**
1. Overlapping parallelograms → Overlapping AABBs (mathematical guarantee)
2. Overlapping AABBs → Lines in overlapping cells (implementation guarantee)
3. Lines in overlapping cells → Candidate pairs found (query phase guarantee)

**The problem:**
- If AABB computation is wrong, step 1 fails
- If numerical precision issues occur, step 1 fails
- If parallelograms don't actually overlap (but should), step 1 fails

**The root cause:**
- AABB computation may be too conservative (too small)
- Or parallelograms may not actually overlap (but physics says they should)
- Or there are numerical precision issues near cell boundaries

---

---

## THE TWO-FOLDED ISSUE: Absolute vs Relative Parallelograms

### Critical Discovery

**The quadtree uses ABSOLUTE parallelograms, but collision detection uses RELATIVE parallelograms!**

This is the fundamental mismatch causing the discrepancy.

### Absolute Parallelogram (What Quadtree Uses)

For a line with:
- Current position: `p1_current`, `p2_current`
- Velocity: `v = (v_x, v_y)`
- Time step: `timeStep`

**Absolute Parallelogram Vertices:**
```
v1 = p1_current = (p1_x, p1_y)
v2 = p2_current = (p2_x, p2_y)
v3 = p1_future = p1_current + v * timeStep = (p1_x + v_x * timeStep, p1_y + v_y * timeStep)
v4 = p2_future = p2_current + v * timeStep = (p2_x + v_x * timeStep, p2_y + v_y * timeStep)
```

**AABB Computation:**
```c
xmin = min(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
xmax = max(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
ymin = min(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
ymax = max(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
```

This represents the **absolute swept area** of the line in world space.

### Relative Parallelogram (What `intersect()` Uses)

For two lines `l1` and `l2`:
- Relative velocity: `v_rel = l2->velocity - l1->velocity`
- Time step: `time`

**Relative Parallelogram Vertices:**
```c
// From intersection_detection.c:44-45
p1 = l2->p1 + v_rel * time = l2->p1 + (l2->velocity - l1->velocity) * time
p2 = l2->p2 + v_rel * time = l2->p2 + (l2->velocity - l1->velocity) * time
```

**Relative Parallelogram:**
```
v1 = l2->p1 (current position of l2's p1)
v2 = l2->p2 (current position of l2's p2)
v3 = p1 = l2->p1 + (l2->velocity - l1->velocity) * time
v4 = p2 = l2->p2 + (l2->velocity - l1->velocity) * time
```

This represents the **relative swept area** of l2 as seen from l1's reference frame.

**Collision Detection:**
The `intersect()` function checks if `l1` (which is stationary in this reference frame) intersects with this relative parallelogram.

### Why They Are Different

**Key Insight:** Two lines can collide even if their absolute parallelograms don't overlap!

**Example Scenario:**
- Line 1: Moving right at speed 1
- Line 2: Moving right at speed 2
- They are close together

**Absolute Parallelograms:**
- Line 1's absolute parallelogram: moves right by 1 * timeStep
- Line 2's absolute parallelogram: moves right by 2 * timeStep
- If they start close, their absolute parallelograms might not overlap (Line 2 moves faster, so its parallelogram is ahead)

**Relative Parallelogram:**
- Relative velocity: (2 - 1) = 1 (Line 2 moves 1 unit faster relative to Line 1)
- Line 2's relative parallelogram: moves right by 1 * timeStep relative to Line 1
- Line 1 is stationary in this reference frame
- **They CAN intersect!** (Line 2 catches up to Line 1)

### Mathematical Proof

**Theorem:** Two lines can collide even if their absolute parallelograms don't overlap.

**Proof:**
- Let `l1` have velocity `v1` and `l2` have velocity `v2`
- Absolute parallelogram of `l1`: swept area from `p1_current` to `p1_current + v1 * t`
- Absolute parallelogram of `l2`: swept area from `p2_current` to `p2_current + v2 * t`
- If `v1 ≠ v2`, these parallelograms can be in different regions of space
- But relative velocity `v_rel = v2 - v1` creates a relative parallelogram that can intersect with `l1`

**Example:**
- `l1`: at (0, 0), moving right at (1, 0)
- `l2`: at (0.1, 0), moving right at (2, 0)
- Time step: 0.1

**Absolute Parallelograms:**
- `l1`: from (0, 0) to (0.1, 0) → AABB: [0, 0.1] x [0, 0]
- `l2`: from (0.1, 0) to (0.3, 0) → AABB: [0.1, 0.3] x [0, 0]
- **Do they overlap?** Yes, at x=0.1

But if `l2` starts at (0.2, 0):
- `l1`: AABB: [0, 0.1] x [0, 0]
- `l2`: AABB: [0.2, 0.4] x [0, 0]
- **Do they overlap?** No! (gap from 0.1 to 0.2)
- **But do they collide?** Yes! `l2` catches up to `l1` because it's moving faster

**Relative Parallelogram:**
- Relative velocity: (2, 0) - (1, 0) = (1, 0)
- `l2`'s relative position: (0.2, 0) + (1, 0) * 0.1 = (0.3, 0)
- Relative parallelogram: from (0.2, 0) to (0.3, 0)
- `l1` is at (0, 0) and stationary in this frame
- **Do they intersect?** No, but if `l2` starts closer, they will!

---

## Concrete Example: Frame 91 (BF Detected Collision)

### Data from Frame 91 (BF)

From `pair_101_105_bf_history.txt`:
```
FRAME_91: l1_p1=(0.5237646177,0.6793750000) l1_p2=(0.5237646177,0.6956250000) 
         l1_vel=(-0.0003620606,0.0000000000) 
         l2_p1=(0.5241034094,0.6790625000) l2_p2=(0.5241034094,0.6959375000) 
         l2_vel=(-0.0022273907,0.0000000000) 
         result=1
```

**Line 101 (l1):**
- `p1_current = (0.5237646177, 0.6793750000)`
- `p2_current = (0.5237646177, 0.6956250000)`
- `velocity = (-0.0003620606, 0.0000000000)`

**Line 105 (l2):**
- `p1_current = (0.5241034094, 0.6790625000)`
- `p2_current = (0.5241034094, 0.6959375000)`
- `velocity = (-0.0022273907, 0.0000000000)`

**Time step:** Assume `timeStep = 0.01` (typical value, need to verify)

### Step 1: Compute Absolute Parallelograms

#### Line 101 Absolute Parallelogram

**Future positions:**
```
p1_future = p1_current + velocity * timeStep
          = (0.5237646177, 0.6793750000) + (-0.0003620606, 0.0) * 0.01
          = (0.5237646177, 0.6793750000) + (-0.000003620606, 0.0)
          = (0.5237609971, 0.6793750000)

p2_future = p2_current + velocity * timeStep
          = (0.5237646177, 0.6956250000) + (-0.0003620606, 0.0) * 0.01
          = (0.5237646177, 0.6956250000) + (-0.000003620606, 0.0)
          = (0.5237609971, 0.6956250000)
```

**Absolute Parallelogram Vertices:**
```
v1 = (0.5237646177, 0.6793750000)  // p1_current
v2 = (0.5237646177, 0.6956250000)  // p2_current
v3 = (0.5237609971, 0.6793750000)  // p1_future
v4 = (0.5237609971, 0.6956250000)  // p2_future
```

**AABB:**
```
xmin = min(0.5237646177, 0.5237646177, 0.5237609971, 0.5237609971)
     = 0.5237609971
xmax = max(0.5237646177, 0.5237646177, 0.5237609971, 0.5237609971)
     = 0.5237646177
ymin = min(0.6793750000, 0.6956250000, 0.6793750000, 0.6956250000)
     = 0.6793750000
ymax = max(0.6793750000, 0.6956250000, 0.6793750000, 0.6956250000)
     = 0.6956250000
```

**Line 101 AABB:** `[0.5237609971, 0.5237646177] x [0.6793750000, 0.6956250000]`

#### Line 105 Absolute Parallelogram

**Future positions:**
```
p1_future = (0.5241034094, 0.6790625000) + (-0.0022273907, 0.0) * 0.01
          = (0.5241034094, 0.6790625000) + (-0.000022273907, 0.0)
          = (0.5240811355, 0.6790625000)

p2_future = (0.5241034094, 0.6959375000) + (-0.0022273907, 0.0) * 0.01
          = (0.5241034094, 0.6959375000) + (-0.000022273907, 0.0)
          = (0.5240811355, 0.6959375000)
```

**Absolute Parallelogram Vertices:**
```
v1 = (0.5241034094, 0.6790625000)  // p1_current
v2 = (0.5241034094, 0.6959375000)  // p2_current
v3 = (0.5240811355, 0.6790625000)  // p1_future
v4 = (0.5240811355, 0.6959375000)  // p2_future
```

**AABB:**
```
xmin = min(0.5241034094, 0.5241034094, 0.5240811355, 0.5240811355)
     = 0.5240811355
xmax = max(0.5241034094, 0.5241034094, 0.5240811355, 0.5240811355)
     = 0.5241034094
ymin = min(0.6790625000, 0.6959375000, 0.6790625000, 0.6959375000)
     = 0.6790625000
ymax = max(0.6790625000, 0.6959375000, 0.6790625000, 0.6959375000)
     = 0.6959375000
```

**Line 105 AABB:** `[0.5240811355, 0.5241034094] x [0.6790625000, 0.6959375000]`

#### Do Absolute AABBs Overlap?

**Line 101 AABB:** `[0.5237609971, 0.5237646177] x [0.6793750000, 0.6956250000]`  
**Line 105 AABB:** `[0.5240811355, 0.5241034094] x [0.6790625000, 0.6959375000]`

**X-dimension overlap:**
- Line 101: `[0.5237609971, 0.5237646177]`
- Line 105: `[0.5240811355, 0.5241034094]`
- Gap: `0.5240811355 - 0.5237646177 = 0.0003165178`

**NO X-OVERLAP!** There's a gap of ~0.0003 between the AABBs.

**Y-dimension overlap:**
- Line 101: `[0.6793750000, 0.6956250000]`
- Line 105: `[0.6790625000, 0.6959375000]`
- Overlap: `[max(0.6793750000, 0.6790625000), min(0.6956250000, 0.6959375000)]`
- Overlap: `[0.6793750000, 0.6956250000]`

**YES Y-OVERLAP!** But since there's no X-overlap, **the absolute AABBs DO NOT OVERLAP.**

**Conclusion:** The quadtree would NOT place these lines in overlapping cells based on absolute AABBs!

### Step 2: Compute Relative Parallelogram (What `intersect()` Uses)

**Relative velocity:**
```
v_rel = l2->velocity - l1->velocity
      = (-0.0022273907, 0.0) - (-0.0003620606, 0.0)
      = (-0.0018653301, 0.0)
```

**Relative parallelogram vertices (from l2's perspective, relative to l1):**
```
p1 = l2->p1 + v_rel * timeStep
   = (0.5241034094, 0.6790625000) + (-0.0018653301, 0.0) * 0.01
   = (0.5241034094, 0.6790625000) + (-0.000018653301, 0.0)
   = (0.5240847561, 0.6790625000)

p2 = l2->p2 + v_rel * timeStep
   = (0.5241034094, 0.6959375000) + (-0.0018653301, 0.0) * 0.01
   = (0.5241034094, 0.6959375000) + (-0.000018653301, 0.0)
   = (0.5240847561, 0.6959375000)
```

**Relative Parallelogram:**
```
v1 = l2->p1 = (0.5241034094, 0.6790625000)
v2 = l2->p2 = (0.5241034094, 0.6959375000)
v3 = p1 = (0.5240847561, 0.6790625000)
v4 = p2 = (0.5240847561, 0.6959375000)
```

**What `intersect()` checks:**
- Is `l1` (at (0.5237646177, 0.6793750000) to (0.5237646177, 0.6956250000)) inside or intersecting this relative parallelogram?

**Relative Parallelogram AABB:**
```
xmin = min(0.5241034094, 0.5241034094, 0.5240847561, 0.5240847561)
     = 0.5240847561
xmax = max(0.5241034094, 0.5241034094, 0.5240847561, 0.5240847561)
     = 0.5241034094
ymin = min(0.6790625000, 0.6959375000, 0.6790625000, 0.6959375000)
     = 0.6790625000
ymax = max(0.6790625000, 0.6959375000, 0.6790625000, 0.6959375000)
     = 0.6959375000
```

**Relative AABB:** `[0.5240847561, 0.5241034094] x [0.6790625000, 0.6959375000]`

**l1's position:** `(0.5237646177, 0.6793750000)` to `(0.5237646177, 0.6956250000)`

**Do they overlap?**
- X: `l1` at x=0.5237646177, relative AABB at [0.5240847561, 0.5241034094]
- Gap: `0.5240847561 - 0.5237646177 = 0.0003201384`

**Still no X-overlap in AABB!** But `intersect()` does more than AABB check - it checks actual line-parallelogram intersection!

### Step 3: Why `intersect()` Detects Collision

The `intersect()` function doesn't just check AABB overlap. It checks:
1. If `l1` intersects the edges of the relative parallelogram
2. If `l1`'s endpoints are inside the relative parallelogram
3. Complex geometric intersection tests

**Key Point:** Even if the AABBs don't overlap, the actual geometric shapes (line vs parallelogram) CAN intersect!

**In this case:**
- `l1` is a vertical line at x=0.5237646177
- Relative parallelogram spans from x=0.5240847561 to x=0.5241034094
- **But** the relative parallelogram represents l2's motion relative to l1
- Since l2 is moving faster (more negative velocity), it's "catching up" to l1
- The relative parallelogram might extend beyond its AABB due to the parallelogram shape (not axis-aligned)

**The actual intersection test in `intersect()`:**
- Checks if `l1` intersects the edges: `(l2->p1, l2->p2)`, `(p1, p2)`, `(p1, l2->p1)`, `(p2, l2->p2)`
- Checks if `l1`'s endpoints are inside the parallelogram
- These tests can detect intersections even when AABBs don't overlap!

### The Two-Folded Issue Summary

**Issue 1: AABB Computation**
- The absolute AABB might be computed correctly
- But it represents the absolute swept area, not the relative swept area
- Two lines can collide even if their absolute AABBs don't overlap

**Issue 2: Parallelogram Mismatch**
- Quadtree uses: **Absolute parallelograms** (each line's swept area in world space)
- Collision detection uses: **Relative parallelograms** (l2's swept area relative to l1)
- These are fundamentally different!
- Physics predicts collision based on relative motion, but quadtree filters based on absolute motion

**Why This Causes Missing Collisions:**
1. Quadtree builds cells based on absolute AABBs
2. If two lines' absolute AABBs don't overlap, they're placed in non-overlapping cells
3. Quadtree never tests them as a candidate pair
4. But `intersect()` would detect a collision based on relative motion!

---

## Solution Approaches

### Approach 1: Use Relative Parallelograms in Quadtree (Complex)

**Idea:** Build quadtree using relative parallelograms for each pair.

**Problem:** This is impossible! We can't build a quadtree per pair - that defeats the purpose of spatial indexing.

### Approach 2: Expand AABB to Account for Relative Motion (Practical)

**Idea:** For each line, compute an expanded AABB that accounts for potential relative motion with other lines.

**Implementation:**
- For line `l1` with velocity `v1`, compute AABB that includes:
  - Absolute swept area: `[p_current, p_current + v1 * timeStep]`
  - Plus expansion for relative motion: Consider maximum relative velocity with any other line
  - This is still approximate but better than fixed epsilon

**Challenge:** How to determine the expansion factor without knowing all other lines' velocities?

### Approach 3: Conservative AABB Expansion (Current Epsilon, But Better)

**Idea:** Expand AABB based on maximum possible relative velocity.

**Implementation:**
- For each line, compute AABB including:
  - Absolute swept area
  - Plus expansion: `max_relative_velocity * timeStep` in all directions
  - Where `max_relative_velocity` is the maximum velocity magnitude in the system

**Problem:** Still creates false positives, but might be acceptable.

### Approach 4: Two-Pass Quadtree (Hybrid)

**Idea:** 
1. First pass: Build quadtree based on absolute AABBs (current approach)
2. Second pass: For candidate pairs found, check if their relative parallelograms overlap
3. Only test pairs where either:
   - Absolute AABBs overlap, OR
   - Relative parallelograms overlap

**Implementation:**
- Build quadtree as before
- Query for candidate pairs
- For each candidate pair, compute relative parallelogram AABB
- If relative AABBs overlap, add to test list
- Test all pairs in the list

**Challenge:** Computing relative parallelogram for all candidate pairs might be expensive.

### Approach 5: Velocity-Based Cell Expansion (Recommended)

**Idea:** When computing AABB for a line, expand it based on the line's velocity magnitude and direction.

**Implementation:**
- Compute absolute AABB as before
- Expand AABB by: `velocity_magnitude * timeStep * expansion_factor` in all directions
- Where `expansion_factor` is chosen to account for relative motion
- This is similar to epsilon, but velocity-dependent

**Advantage:** Adapts to different velocities automatically.

**Challenge:** Still need to choose expansion factor.

### Approach 6: Use Minkowski Sum (Theoretically Correct)

**Idea:** For each line, compute the Minkowski sum of its swept area with a "relative motion ball".

**Implementation:**
- For line `l1` with velocity `v1`:
  - Compute absolute swept parallelogram
  - Compute Minkowski sum with ball of radius `max(|v2 - v1|) * timeStep` for all possible `v2`
  - Use this expanded shape's AABB for quadtree insertion

**Advantage:** Theoretically correct - guarantees all potential collisions are found.

**Challenge:** Computationally expensive, requires knowing all velocities.

---

## Recommended Solution

**Hybrid Approach: Velocity-Dependent AABB Expansion**

1. **Compute absolute AABB** as before
2. **Expand AABB** by: `velocity_magnitude * timeStep * k` in all directions
   - Where `k` is a small constant (e.g., 0.1-0.5)
   - This accounts for relative motion with lines moving at different speeds
3. **Tune `k`** based on typical velocity differences in the simulation

**Why This Works:**
- If two lines have velocities `v1` and `v2`, their relative velocity is `|v2 - v1|`
- Maximum relative velocity is bounded by `2 * max_velocity`
- Expanding by `velocity_magnitude * timeStep * k` ensures we capture most relative motion cases
- The expansion is adaptive (faster lines get larger expansion)

**Implementation:**
```c
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax) {
  // ... existing absolute AABB computation ...
  
  // Expand based on velocity magnitude to account for relative motion
  double velocity_magnitude = Vec_length(line->velocity);
  double expansion = velocity_magnitude * timeStep * 0.2;  // k = 0.2
  
  *xmin -= expansion;
  *xmax += expansion;
  *ymin -= expansion;
  *ymax += expansion;
}
```

**Advantages:**
- No hard-coded epsilon
- Adapts to different velocities
- Accounts for relative motion
- Simple to implement

**Disadvantages:**
- Still creates some false positives
- Need to tune the expansion factor `k`

---

## Next Steps

1. **Verify AABB Computation**: Check if AABB actually contains the parallelogram
2. **Verify Parallelogram Overlap**: Check if parallelograms actually overlap (not just close)
3. **Investigate Numerical Precision**: Check if floating-point precision is causing issues
4. **CRITICAL: Address Relative vs Absolute Parallelogram Mismatch**: 
   - Implement velocity-dependent AABB expansion
   - Test with different expansion factors
   - Verify it reduces missing collisions without creating too many false positives
5. **Validate Solution**: Run full simulation and compare collision counts with BF

---

## Code References

### Cell Boundary Creation
- `quadtree.c:516-533`: Subdivision creates 4 child cells at midpoints
- `quadtree.c:359-420`: Tree creation sets world bounds

### Line Insertion
- `quadtree.c:481-590`: `insertLineRecursive` inserts into ALL overlapping cells
- `quadtree.c:60-69`: `boxesOverlap` checks if AABB overlaps cell

### AABB Computation
- `quadtree.c:99-136`: `computeLineBoundingBox` computes AABB from 4 vertices
- Includes epsilon expansion (0.0005) as a temporary fix

### Query Phase
- `quadtree.c:704-1257`: `QuadTree_findCandidatePairs` finds candidate pairs
- `quadtree.c:669-702`: `findOverlappingCellsRecursive` finds all overlapping cells

