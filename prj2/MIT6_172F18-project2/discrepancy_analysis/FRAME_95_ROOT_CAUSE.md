# Frame 95 Root Cause Analysis: Pairs (29,89) and (29,77)

## Summary

**Root Cause Identified**: AABB gap problem and AABB approximation limitations
- **Pair (29,89)**: AABBs don't overlap in world space, but actual parallelograms do → QT misses collision
- **Pair (29,77)**: AABBs overlap, but actual parallelograms don't → QT finds false positive

---

## Investigation Results

### Cell Assignments (Frame 95)

**Line 29:**
- Cells: `[0.531250,0.562500)x[0.656250,0.687500)` and `[0.531250,0.562500)x[0.687500,0.718750)`
- AABB: `[0.537039,0.538604]x[0.684706,0.690294]`
- X-range: `[0.531250, 0.562500)`
- Y-range: `[0.656250, 0.718750)`

**Line 77:**
- Cells: `[0.531250,0.562500)x[0.656250,0.687500)` and `[0.531250,0.562500)x[0.687500,0.718750)`
- AABB: `[0.535276,0.540676]x[0.680237,0.694763]`
- X-range: `[0.531250, 0.562500)` ✓ **SAME as Line 29**
- Y-range: `[0.656250, 0.718750)` ✓ **SAME as Line 29**

**Line 89:**
- Cells: `[0.515625,0.531250)x[0.671875,0.687500)` and `[0.515625,0.531250)x[0.687500,0.703125)`
- AABB: `[0.529084,0.529913]x[0.680156,0.694844]`
- X-range: `[0.515625, 0.531250)` ❌ **DIFFERENT from Line 29**
- Y-range: `[0.671875, 0.703125)` ✓ **OVERLAPS with Line 29**

---

## Pair (29,89) - Missing Collision

### Cell Analysis

**Line 29 cells**: X-range `[0.531250, 0.562500)`
**Line 89 cells**: X-range `[0.515625, 0.531250)`

**Critical Finding**: These X-ranges are **adjacent but non-overlapping**!
- Line 29 min X: `0.531250`
- Line 89 max X: `0.531250`
- **Gap**: Exactly at the cell boundary (0.000000)

**Result**: QT doesn't find lines 29 and 89 in overlapping cells, so it never tests the pair.

### AABB Analysis

**Line 29 AABB**: X=`[0.537039, 0.538604]`, Y=`[0.684706, 0.690294]`
**Line 89 AABB**: X=`[0.529084, 0.529913]`, Y=`[0.680156, 0.694844]`

**AABB Overlap**:
- X overlap: **NO** (gap of ~0.007126)
- Y overlap: **YES**
- **Overall AABB overlap: NO**

**But BF found a collision!** This means:
- The **actual parallelograms** (swept areas) DO overlap
- But the **AABBs** (axis-aligned bounding boxes) DON'T overlap
- This is the **AABB gap problem** - AABBs are too conservative

### Root Cause

1. **AABB approximation is too conservative**:
   - The actual parallelogram swept area overlaps
   - But the AABB (which is a rectangular approximation) doesn't overlap
   - This happens when lines move at angles (not axis-aligned)

2. **Current expansion is insufficient**:
   - Velocity-dependent expansion (k_rel=0.3, k_gap=0.15) doesn't expand enough
   - The gap of ~0.007126 is larger than the expansion
   - Even with expansion, AABBs might not overlap enough to be in same cells

3. **Cell boundary issue**:
   - Lines are exactly at the cell boundary (0.531250)
   - One line is in cells ending at 0.531250, the other starts at 0.531250
   - They're adjacent but non-overlapping

---

## Pair (29,77) - False Positive

### Cell Analysis

**Line 29 cells**: X-range `[0.531250, 0.562500)`, Y-range `[0.656250, 0.718750)`
**Line 77 cells**: X-range `[0.531250, 0.562500)`, Y-range `[0.656250, 0.718750)`

**Critical Finding**: Lines 29 and 77 are in **EXACTLY THE SAME CELLS**!

**Result**: QT finds them in overlapping cells, tests the pair, and finds a collision.

### AABB Analysis

**Line 29 AABB**: X=`[0.537039, 0.538604]`, Y=`[0.684706, 0.690294]`
**Line 77 AABB**: X=`[0.535276, 0.540676]`, Y=`[0.680237, 0.694763]`

**AABB Overlap**:
- X overlap: **YES** (`[0.537039, 0.538604]` overlaps `[0.535276, 0.540676]`)
- Y overlap: **YES** (`[0.684706, 0.690294]` overlaps `[0.680237, 0.694763]`)
- **Overall AABB overlap: YES**

**But BF found NO collision!** This means:
- The **AABBs** DO overlap (so QT tests them)
- But the **actual parallelograms** DON'T overlap (so BF finds no collision)
- This is a **false positive** due to AABB approximation

### Root Cause

1. **AABB expansion may be too aggressive**:
   - The velocity-dependent expansion (k_rel=0.3, k_gap=0.15) expands AABBs
   - Expanded AABBs overlap, so QT tests the pair
   - But the actual parallelograms don't overlap, so it's a false positive

2. **AABB approximation limitations**:
   - AABBs are rectangular approximations of parallelograms
   - They can overlap even when parallelograms don't
   - This is the inverse of the (29,89) problem

3. **Possible earlier divergence**:
   - If line 29's state diverged earlier (due to other mismatches), its AABB might be different
   - This could cause AABBs to overlap when they shouldn't

---

## Root Cause Chain

### The First Ring of the Chain

**The fundamental issue**: **AABB approximation is imperfect**

1. **AABBs are rectangular approximations of parallelograms**:
   - Parallelograms can overlap when AABBs don't → Missing collisions
   - AABBs can overlap when parallelograms don't → False positives

2. **Current expansion solution is insufficient**:
   - Velocity-dependent expansion (k_rel=0.3, k_gap=0.15) helps but doesn't solve the problem
   - It's a compromise: too small → missing collisions, too large → false positives

3. **Cell boundary issues**:
   - Lines near cell boundaries can be in non-overlapping cells
   - Even if AABBs overlap slightly, they might not overlap enough to be in same cells

### The Chain of Events

```
Frame 95:
  └─> AABB gap problem for (29,89)
      └─> AABBs don't overlap → Lines in non-overlapping cells
      └─> QT doesn't test (29,89) → Misses collision
      
  └─> AABB approximation issue for (29,77)
      └─> AABBs overlap (possibly due to expansion)
      └─> QT tests (29,77) → Finds false positive

Frame 96:
  └─> Different collision resolution for line 29
      └─> Line 29's velocity changes differently
      └─> Physical quantities diverge
      
Frames 97-100:
  └─> Cascading divergence
      └─> More mismatches due to different line states
```

---

## Comparison with Frame 89 (Pair 101,105)

| Aspect | Frame 89 (101,105) | Frame 95 (29,89) |
|--------|---------------------|------------------|
| Issue type | Missing frame (not tested) | Missing collision (tested but not found) |
| Cell overlap | Non-overlapping X-ranges | Non-overlapping X-ranges |
| AABB overlap | Unknown | NO (gap ~0.007) |
| Root cause | AABB gap problem | AABB gap problem |
| Solution | Velocity-dependent expansion | Same (but insufficient) |

**Similarity**: Both have the same root cause - AABB gap problem where AABBs don't overlap enough to be in same cells.

---

## Key Insights

1. **AABB approximation is the fundamental limitation**:
   - Rectangular AABBs can't perfectly represent parallelogram swept areas
   - This causes both missing collisions and false positives

2. **Expansion is a compromise**:
   - Too small → missing collisions (like 29,89)
   - Too large → false positives (like 29,77)
   - Current parameters (k_rel=0.3, k_gap=0.15) are a compromise

3. **Cell boundaries are problematic**:
   - Lines near boundaries can be separated even when close
   - The gap of 0.007126 for (29,89) is larger than typical expansion

4. **The first ring of the chain**:
   - The fundamental issue is AABB approximation
   - All other problems stem from this
   - To fix it, we need a better approximation or a different approach

---

## Recommendations

1. **Investigate AABB computation**:
   - Verify that AABBs correctly represent the parallelogram swept area
   - Check if the 4-point computation (current + future positions) is correct
   - Consider if we need to include more points or use a different method

2. **Consider alternative approaches**:
   - **Minkowski sum**: More theoretically sound but computationally expensive
   - **Two-pass approach**: First pass with AABBs, second pass with actual parallelograms
   - **Adaptive expansion**: Different expansion for different line pairs based on velocity angle

3. **Tune expansion parameters**:
   - For (29,89): Need larger expansion to bridge the ~0.007 gap
   - For (29,77): Need smaller expansion to reduce false positives
   - This suggests we need **pair-specific or adaptive expansion**

4. **Verify earlier divergence**:
   - Check if line 29's state diverged before frame 95
   - If so, this could explain why AABBs are different between BF and QT

---

## Files Reference

- `debug_qt_cells.txt`: Cell assignments for lines 29, 77, 89 in frame 95
- `debug_qt_candidates.txt`: Candidate pairs found (empty for frame 95 tracked pairs)
- `PAIR_29_53_ROOT_CAUSE.md`: Initial root cause analysis (frame 95 collision mismatch)

---

**Analysis Date**: $(date)
**Frame**: 95
**Pairs Investigated**: (29,89) missing, (29,77) false positive
**Root Cause**: AABB gap problem and AABB approximation limitations
**First Ring**: AABB approximation is imperfect - rectangular AABBs can't perfectly represent parallelograms

