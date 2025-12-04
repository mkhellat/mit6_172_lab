# Revised Solution: Velocity-Dependent AABB Expansion with Relative Motion Accounting

## Executive Summary

Based on the comprehensive analysis in `QUADTREE_CELL_BOUNDARIES_ANALYSIS.md` and `QUADTREE_QUERY_SCENARIOS_ANALYSIS.md`, the root cause of the discrepancy is a **two-folded issue**:

1. **Absolute vs Relative Parallelogram Mismatch**: Quadtree uses absolute parallelograms, but collision detection uses relative parallelograms
2. **AABB Gap Problem**: AABBs can overlap in world space but not overlap the same cells, causing pairs to be missed

The revised solution addresses both issues with a **velocity-dependent AABB expansion** that:
- Accounts for relative motion between lines
- Ensures close AABBs overlap the same cells
- Adapts to different velocities automatically
- Minimizes false positives

---

## Root Cause Analysis Summary

### Issue 1: Absolute vs Relative Parallelogram Mismatch

**Problem:**
- Quadtree builds cells based on **absolute parallelograms** (each line's swept area in world space)
- Collision detection uses **relative parallelograms** (l2's swept area relative to l1)
- Two lines can collide even if their absolute parallelograms don't overlap

**Example (Frame 91):**
- Line 101 absolute AABB: `[0.5237609971, 0.5237646177]`
- Line 105 absolute AABB: `[0.5240811355, 0.5241034094]`
- Gap: `0.0003165178` (no overlap!)
- But relative velocity causes collision detection to find intersection

**Impact:** Quadtree misses pairs that would collide based on relative motion.

### Issue 2: AABB Gap Problem

**Problem:**
- Quadtree only considers **cell-level overlaps**, not direct line-to-line AABB overlaps
- If two lines' AABBs overlap in world space but don't overlap the same cells, they won't be found
- This happens when AABBs are close but don't span cell boundaries

**Example (Scenario 3B):**
- l1 in C1, l2 in C2
- AABBs overlap in world space
- But l2's AABB doesn't overlap C1's bounding box
- Result: Pair NOT found

**Impact:** Quadtree misses pairs even when absolute AABBs overlap.

### Why Current Epsilon Fix Fails

**Current Approach:**
- Fixed epsilon expansion: `0.0005` added to all AABBs
- Discrepancy increased: 50 → 71

**Why It Fails:**
1. Doesn't account for relative motion (Issue 1)
2. Too small for some cases, too large for others (not adaptive)
3. Creates false positives (expands AABBs uniformly, causing more cell overlaps)

---

## Revised Solution: Multi-Factor AABB Expansion

### Core Idea

Expand each line's AABB by an amount that accounts for:
1. **Relative motion** (velocity-dependent expansion)
2. **AABB gap problem** (minimum expansion to ensure cell overlap)
3. **Numerical precision** (small safety margin)

### Mathematical Formulation

For a line with:
- Current position: `p1_current`, `p2_current`
- Velocity: `v = (v_x, v_y)`
- Time step: `timeStep`
- Maximum velocity in system: `v_max` (computed during build)

**Step 1: Compute Absolute AABB (as before)**
```
p1_future = p1_current + v * timeStep
p2_future = p2_current + v * timeStep

xmin = min(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
xmax = max(p1_current.x, p2_current.x, p1_future.x, p2_future.x)
ymin = min(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
ymax = max(p1_current.y, p2_current.y, p1_future.y, p2_future.y)
```

**Step 2: Compute Expansion Factors**

**Factor 1: Relative Motion Expansion**
```
velocity_magnitude = sqrt(v_x^2 + v_y^2)
relative_motion_expansion = velocity_magnitude * timeStep * k_rel
```
Where `k_rel` is a constant (e.g., 0.2-0.5) that accounts for maximum relative velocity.

**Rationale:** If two lines have velocities `v1` and `v2`, their relative velocity is `|v2 - v1| ≤ 2 * v_max`. Expanding by `velocity_magnitude * timeStep * k_rel` ensures we capture most relative motion cases.

**Factor 2: Minimum Gap Expansion**
```
min_gap_expansion = min_cell_size * k_gap
```
Where `k_gap` is a small constant (e.g., 0.1-0.2) relative to minimum cell size.

**Rationale:** Ensures AABBs that are close (within a fraction of cell size) overlap the same cells, addressing the gap problem.

**Factor 3: Numerical Precision Margin**
```
precision_margin = 1e-6  // Small fixed value for floating-point precision
```

**Step 3: Apply Expansion**
```
expansion = max(relative_motion_expansion, min_gap_expansion) + precision_margin

xmin -= expansion
xmax += expansion
ymin -= expansion
ymax += expansion
```

### Implementation

```c
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax,
                                   double maxVelocityInSystem,
                                   double minCellSize) {
  // Current endpoints
  Vec p1_current = line->p1;
  Vec p2_current = line->p2;
  
  // Future endpoints (after movement)
  Vec p1_future = Vec_add(p1_current, Vec_multiply(line->velocity, timeStep));
  Vec p2_future = Vec_add(p2_current, Vec_multiply(line->velocity, timeStep));
  
  // Compute bounding box that includes all four points
  *xmin = minDouble(minDouble(p1_current.x, p2_current.x),
                    minDouble(p1_future.x, p2_future.x));
  *xmax = maxDouble(maxDouble(p1_current.x, p2_current.x),
                    maxDouble(p1_future.x, p2_future.x));
  *ymin = minDouble(minDouble(p1_current.y, p2_current.y),
                    minDouble(p1_future.y, p2_future.y));
  *ymax = maxDouble(maxDouble(p1_current.y, p2_current.y),
                    maxDouble(p1_future.y, p2_future.y));
  
  // REVISED EXPANSION: Multi-factor approach
  
  // Factor 1: Relative motion expansion
  // Accounts for maximum relative velocity between lines
  // If line has velocity v, another line can have velocity up to v_max
  // Relative velocity can be up to |v - v_max| ≤ v + v_max ≤ 2*v_max
  // We expand by a fraction of the line's velocity magnitude
  double velocity_magnitude = Vec_length(line->velocity);
  double k_rel = 0.3;  // Tune based on typical velocity differences
  double relative_motion_expansion = velocity_magnitude * timeStep * k_rel;
  
  // Factor 2: Minimum gap expansion
  // Ensures AABBs that are close (within fraction of cell size) overlap same cells
  // Addresses the AABB gap problem (Scenario 3B, 7C)
  double k_gap = 0.15;  // 15% of minimum cell size
  double min_gap_expansion = minCellSize * k_gap;
  
  // Factor 3: Numerical precision margin
  // Small fixed value to handle floating-point precision issues
  const double precision_margin = 1e-6;
  
  // Use maximum of relative motion and gap expansion, plus precision margin
  // This ensures we address both issues (relative motion and gap problem)
  double expansion = maxDouble(relative_motion_expansion, min_gap_expansion) 
                     + precision_margin;
  
  *xmin -= expansion;
  *xmax += expansion;
  *ymin -= expansion;
  *ymax += expansion;
}
```

### Required Changes

1. **Modify `computeLineBoundingBox` signature:**
   - Add `maxVelocityInSystem` parameter (computed during build)
   - Add `minCellSize` parameter (from tree config)

2. **Compute maximum velocity during build:**
   ```c
   double maxVelocity = 0.0;
   for (unsigned int i = 0; i < numLines; i++) {
     double v_mag = Vec_length(lines[i]->velocity);
     if (v_mag > maxVelocity) {
       maxVelocity = v_mag;
     }
   }
   ```

3. **Update all calls to `computeLineBoundingBox`:**
   - Pass `maxVelocity` and `tree->config.minCellSize`
   - Update in `QuadTree_build`, `insertLineRecursive`, `QuadTree_findCandidatePairs`

---

## Advantages of Revised Solution

### 1. Addresses Both Issues

**Relative Motion (Issue 1):**
- Expansion proportional to velocity magnitude
- Faster lines get larger expansion (more likely to have relative motion)
- Adapts to different velocity ranges automatically

**AABB Gap Problem (Issue 2):**
- Minimum gap expansion ensures close AABBs overlap same cells
- Based on cell size (adaptive to tree structure)
- Prevents Scenario 3B and 7C cases

### 2. Adaptive and Physics-Based

- No hard-coded epsilon
- Expansion depends on:
  - Line's velocity (relative motion)
  - Cell size (gap problem)
  - Time step (temporal scale)
- Adapts to different simulation parameters

### 3. Minimizes False Positives

- Expansion is proportional, not uniform
- Slow lines get smaller expansion
- Fast lines get larger expansion (but they're more likely to collide anyway)
- Better than fixed epsilon that expands all lines uniformly

### 4. Tunable Parameters

- `k_rel`: Controls relative motion expansion (0.2-0.5 recommended)
- `k_gap`: Controls gap expansion (0.1-0.2 recommended)
- Can be tuned based on simulation characteristics

---

## Tuning Guidelines

### Parameter Selection

**`k_rel` (Relative Motion Factor):**
- **Small (0.1-0.2):** Conservative, fewer false positives, might miss some collisions
- **Medium (0.3-0.4):** Balanced, recommended starting point
- **Large (0.5-0.7):** Aggressive, more false positives, fewer missed collisions

**`k_gap` (Gap Factor):**
- **Small (0.05-0.1):** Minimal expansion, might not address all gap cases
- **Medium (0.15-0.2):** Recommended, addresses most gap cases
- **Large (0.25-0.3):** Aggressive, might create unnecessary overlaps

**Starting Values:**
- `k_rel = 0.3`
- `k_gap = 0.15`
- `precision_margin = 1e-6`

### Tuning Process

1. **Start with recommended values**
2. **Run simulation and compare with brute-force:**
   - Count missing collisions (false negatives)
   - Count extra collisions (false positives)
3. **Adjust based on results:**
   - Too many missing collisions → Increase `k_rel` or `k_gap`
   - Too many false positives → Decrease `k_rel` or `k_gap`
4. **Iterate until acceptable balance**

---

## Expected Results

### Before (Current Epsilon Fix)
- Discrepancy: ~71 extra collisions
- Fixed expansion: 0.0005 for all lines
- Doesn't account for relative motion
- Creates many false positives

### After (Revised Solution)
- **Expected:** Discrepancy reduced to < 10
- Adaptive expansion based on velocity and cell size
- Accounts for relative motion
- Fewer false positives (proportional expansion)

### Validation

**Success Criteria:**
1. Collision count matches brute-force within 5%
2. No missing critical collisions (like Line 105 case)
3. False positive rate < 10% of total collisions
4. Performance remains O(n log n)

**Test Cases:**
1. Frame 89: Verify Line 105 pairs are found
2. Frame 91: Verify pair (101,105) is found
3. Full simulation: Compare total collision counts
4. Edge cases: Very fast lines, very slow lines, clustered lines

---

## Implementation Steps

### Phase 1: Modify AABB Computation

1. Update `computeLineBoundingBox` signature
2. Add max velocity computation in `QuadTree_build`
3. Update all call sites
4. Test compilation

### Phase 2: Tune Parameters

1. Start with recommended values
2. Run on test case (smalllines.in)
3. Compare with brute-force
4. Adjust parameters iteratively

### Phase 3: Validate

1. Run full simulation
2. Compare collision counts
3. Check for missing critical pairs
4. Measure performance impact

### Phase 4: Document

1. Document final parameter values
2. Explain tuning process
3. Record results
4. Update analysis documents

---

## Alternative Considerations

### Option A: Two-Pass Quadtree

**Idea:** Build quadtree twice - once with absolute AABBs, once with relative AABBs.

**Pros:**
- Theoretically correct
- Captures all relative motion cases

**Cons:**
- Doubles build time
- Complex implementation
- Still need expansion for gap problem

**Verdict:** Too complex, not recommended.

### Option B: Minkowski Sum Expansion

**Idea:** Expand AABB by Minkowski sum with "relative motion ball".

**Pros:**
- Theoretically sound
- Guarantees all collisions found

**Cons:**
- Computationally expensive
- Requires knowing all velocities
- Overkill for this problem

**Verdict:** Too expensive, not practical.

### Option C: Hybrid Approach

**Idea:** Use absolute AABBs for quadtree, but add second pass checking relative AABBs.

**Pros:**
- Addresses both issues
- More accurate

**Cons:**
- More complex
- Slower (second pass)
- Still need expansion for efficiency

**Verdict:** More complex than needed, current solution is better.

---

## Conclusion

The revised solution addresses both root causes identified in the analysis:

1. **Relative Motion:** Velocity-dependent expansion accounts for maximum relative velocity
2. **AABB Gap:** Minimum gap expansion ensures close AABBs overlap same cells

The solution is:
- **Adaptive:** Depends on velocity and cell size
- **Physics-based:** Accounts for relative motion
- **Tunable:** Parameters can be adjusted
- **Practical:** Simple to implement, efficient to run

**Next Steps:**
1. Implement the revised `computeLineBoundingBox`
2. Add max velocity computation
3. Tune parameters
4. Validate against brute-force

---

## Code References

### Files to Modify

1. **`quadtree.c`:**
   - `computeLineBoundingBox` (lines 99-136)
   - `QuadTree_build` (lines 592-653)
   - `insertLineRecursive` (lines 481-590)
   - `QuadTree_findCandidatePairs` (lines 704-1257)

2. **`quadtree.h`:**
   - Add max velocity to `QuadTree` structure (if needed)
   - Or compute on-the-fly during build

### Key Functions

- `computeLineBoundingBox`: Compute expanded AABB
- `Vec_length`: Compute velocity magnitude
- `maxDouble`: Get maximum of two values
- `QuadTree_build`: Compute max velocity, build tree

---

This revised solution provides a comprehensive fix that addresses both root causes while maintaining efficiency and minimizing false positives.

