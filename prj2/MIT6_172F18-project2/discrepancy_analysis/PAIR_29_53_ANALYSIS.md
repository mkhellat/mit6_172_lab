# Pair (29,53) Discrepancy Analysis

## Summary

**Pair**: (29, 53)  
**Status**: QT missing 6 frames, divergence starts at frame 96  
**Root Cause**: QT not testing pair in critical frames, leading to missed collisions and physics divergence

---

## Key Findings

### 1. Missing Frames

QT did NOT test pair (29,53) in the following frames:
- **Frame 13**
- **Frame 42**
- **Frame 57**
- **Frame 72**
- **Frame 89** ⚠️ (Critical - same as pair 101,105)
- **Frame 91** ⚠️ (Critical)

**Total**: 6 missing frames out of 100

### 2. Collision Detection Discrepancy

**BF found collisions**:
- Frame 98: (29,53) type=1
- Frame 100: (29,53) type=1

**QT found collisions**:
- None (QT never found a collision for this pair)

**Impact**: QT missed 2 collisions that BF found, causing physics divergence.

### 3. Divergence Timeline

- **Frames 1-95**: Physical quantities match (when both tested)
- **Frame 96**: **First divergence** detected
  - BF l1_p1.x = 0.5414192376, l1_vel.x = 0.0081724785
  - QT l1_p1.x = 0.5382672583, l1_vel.x = 0.0018685200
  - **Difference**: Position differs by ~0.003, velocity differs by ~0.006
- **Frames 97-100**: Divergence increases
- **Frame 98**: BF finds collision, QT does not (different physical states)
- **Frame 100**: BF finds collision, QT does not (different physical states)

### 4. Critical Frame Analysis: Frame 89

**BF Frame 89**:
- Tested pair (29,53): result=0 (NO_INTERSECTION)
- Line 29 collisions: (5,29), (9,29), (13,29)
- Line 53 collisions: (53,89), (53,105)

**QT Frame 89**:
- **Did NOT test pair (29,53)** ⚠️
- Line 29 collisions: (5,29), (9,29), (13,29) ✓ (matches BF)
- Line 53 collisions: (53,89), (53,105) ✓ (matches BF)

**Analysis**: 
- Both lines had collisions in frame 89, but QT did not test the pair
- This suggests the quadtree did not find lines 29 and 53 in overlapping cells
- The missing test in frame 89 may have contributed to the divergence

### 5. Collision Tally Comparison (Frames 85-95)

**Line 29 collisions**:
- BF: 15 collisions
- QT: 15 collisions (but some different pairs)
- **Status**: Similar count, but some pairs differ

**Line 53 collisions**:
- BF: 24 collisions
- QT: 24 collisions (but some different pairs)
- **Status**: Similar count, but some pairs differ

**Key Difference**: 
- QT found some collisions that BF didn't (false positives)
- BF found some collisions that QT didn't (missing)
- The **net effect** is that line 29 and line 53 had different collision histories, leading to different physical states by frame 96

---

## Root Cause Hypothesis

### Primary Issue: Quadtree Spatial Query

**Problem**: QT is not finding lines 29 and 53 in overlapping quadtree cells in certain frames (13, 42, 57, 72, 89, 91).

**Possible reasons**:
1. **AABB gap problem**: The bounding boxes of lines 29 and 53 overlap in world space, but do not overlap any common quadtree cells due to cell boundaries.
2. **Insufficient expansion**: The velocity-dependent expansion (k_rel=0.3, k_gap=0.15) may not be sufficient for this pair.
3. **Cell boundary issue**: Lines near cell boundaries may be assigned to non-overlapping cells.

### Secondary Issue: Cascading Divergence

**Problem**: Once the physical quantities diverge (starting at frame 96), subsequent collision detection gives different results.

**Impact**:
- Frame 98: BF finds collision, QT doesn't (different line states)
- Frame 100: BF finds collision, QT doesn't (different line states)
- This creates a feedback loop, increasing divergence over time

---

## Comparison with Pair (101,105)

| Aspect | Pair (101,105) | Pair (29,53) |
|--------|----------------|--------------|
| Missing frames | Frame 89 (before fix) | Frames 13, 42, 57, 72, 89, 91 |
| Divergence start | Frame 90 | Frame 96 |
| Collisions found | BF: 1, QT: 1 (after fix) | BF: 2, QT: 0 |
| Root cause | AABB gap, missing frame 89 | AABB gap, multiple missing frames |

**Similarity**: Both pairs have the same root cause (AABB gap problem), but pair (29,53) is more severe with 6 missing frames vs 1.

---

## Recommendations

1. **Investigate why frames 13, 42, 57, 72, 89, 91 are missing**:
   - Check quadtree cell assignments for lines 29 and 53 in these frames
   - Verify if AABBs overlap in world space but not in common cells
   - Check if expansion is sufficient for this pair

2. **Consider pair-specific expansion**:
   - Lines 29 and 53 may need different expansion parameters
   - Or the expansion formula needs refinement

3. **Verify cell boundary handling**:
   - Ensure lines near cell boundaries are correctly assigned to overlapping cells

---

## Files Generated

- `history_files/pair_29_53_bf.txt`: BF kinematic history (100 frames)
- `history_files/pair_29_53_qt.txt`: QT kinematic history (94 frames)
- `comparison_files/pair_29_53_comparison.txt`: Frame-by-frame comparison

---

**Analysis Date**: $(date)  
**Test Configuration**: Velocity-dependent expansion (k_rel=0.3, k_gap=0.15)  
**Input**: box.in, 100 frames

