# Pair (29,53) Root Cause Analysis: Line 29 Collision History

## Summary

**Root Cause Identified**: Frame 95 collision mismatch for line 29
- QT **MISSED** collision (29,89) that BF found
- QT **FOUND** false positive (29,77) that BF didn't find
- This mismatch caused line 29's velocity to diverge, leading to position divergence at frame 96

---

## Investigation Methodology

Following the user's guidance, we investigated the collision history of the divergent line (line 29) before the kinematic divergence point (frame 96) to find the root cause.

### Steps:
1. Identified line 29 as the divergent line (l1 in pair 29,53)
2. Extracted collision history for line 29 from frames 1-95
3. Compared BF vs QT collision history frame-by-frame
4. Found first mismatch at frame 95
5. Verified physical quantities match until frame 95, then diverge at frame 96

---

## Key Findings

### 1. Collision History for Line 29 (Frames 85-95)

**Direct collisions involving line 29** (not 229, 329, etc.):

| Frame | BF Collisions | QT Collisions | Match |
|-------|---------------|---------------|-------|
| 85 | None | None | ✓ |
| 86 | None | None | ✓ |
| 87 | None | None | ✓ |
| 88 | (29,93) | (29,93) | ✓ |
| 89 | (5,29), (9,29), (13,29) | (5,29), (9,29), (13,29) | ✓ |
| 90 | (29,49) | (29,49) | ✓ |
| 91 | (29,33) | (29,33) | ✓ |
| 92 | None | None | ✓ |
| 93 | None | None | ✓ |
| 94 | None | None | ✓ |
| 95 | **(29,85), (29,89)** | **(29,77), (29,85)** | ❌ **MISMATCH** |

### 2. First Mismatch: Frame 95

**BF found**:
- (29,85) type=1
- (29,89) type=1

**QT found**:
- (29,77) type=1 ⚠️ **FALSE POSITIVE** (BF didn't find this)
- (29,85) type=1 ✓

**QT missing**:
- (29,89) type=1 ⚠️ **MISSING** (BF found this)

**Impact**: 
- QT missed collision with line 89
- QT found false positive collision with line 77
- This caused different collision resolution for line 29

### 3. Physical Quantities Timeline

**Line 29 (l1) physical quantities**:

| Frame | BF p1.x | BF vel.x | QT p1.x | QT vel.x | Match |
|-------|---------|----------|---------|----------|-------|
| 85 | 0.5347457627 | -0.0021186441 | 0.5347457627 | -0.0021186441 | ✓ |
| 86 | 0.5336864407 | -0.0021186441 | 0.5336864407 | -0.0021186441 | ✓ |
| 87 | 0.5326271186 | -0.0021186441 | 0.5326271186 | -0.0021186441 | ✓ |
| 88 | 0.5315677966 | -0.0021186441 | 0.5315677966 | -0.0021186441 | ✓ |
| 90 | 0.5308976301 | -0.0018772311 | 0.5308976301 | -0.0018772311 | ✓ |
| 92 | 0.5344020374 | 0.0019539740 | 0.5344020374 | 0.0019539740 | ✓ |
| 93 | 0.5353790244 | 0.0019539740 | 0.5353790244 | 0.0019539740 | ✓ |
| 94 | 0.5363560113 | 0.0019539740 | 0.5363560113 | 0.0019539740 | ✓ |
| 95 | 0.5373329983 | 0.0019539740 | 0.5373329983 | 0.0019539740 | ✓ |
| 96 | 0.5414192376 | **0.0081724785** | 0.5382672583 | **0.0018685200** | ❌ **DIVERGED** |

**Key Observation**:
- Physical quantities match perfectly until frame 95
- **Divergence starts at frame 96** (velocity differs: 0.00817 vs 0.00187)
- This is **AFTER** the collision mismatch at frame 95

### 4. Root Cause Chain

```
Frame 95:
  └─> QT misses collision (29,89) that BF found
  └─> QT finds false positive (29,77) that BF didn't find
  └─> Different collision resolution for line 29
  └─> Line 29's velocity changes differently in QT vs BF

Frame 96:
  └─> Different velocity from frame 95 causes position divergence
  └─> Line 29's physical quantities now differ
  └─> Cascading effect: subsequent collisions give different results
```

---

## Root Cause Analysis

### Why did QT miss (29,89) in frame 95?

**Hypothesis**: Same AABB gap problem as pair (101,105)
- Lines 29 and 89 have overlapping bounding boxes in world space
- But their AABBs do not overlap any common quadtree cells
- Quadtree spatial query doesn't find them as candidate pairs
- Result: QT doesn't test the pair, misses the collision

### Why did QT find false positive (29,77) in frame 95?

**Hypothesis**: Over-aggressive AABB expansion
- The velocity-dependent expansion (k_rel=0.3, k_gap=0.15) may be too large
- Lines 29 and 77 have AABBs that overlap in quadtree cells
- But they don't actually collide (false positive)
- Result: QT tests the pair, finds collision incorrectly

### Why does this cause divergence?

**Collision resolution**:
- When a collision is detected, the collision resolution algorithm changes the velocities of both lines
- Different collisions = different velocity changes
- Different velocities = different future positions
- This creates a cascading effect where subsequent frames diverge more

---

## Comparison with Pair (101,105)

| Aspect | Pair (101,105) | Pair (29,53) |
|--------|----------------|--------------|
| Divergent line | Line 105 | Line 29 |
| Root cause frame | Frame 89 | Frame 95 |
| Issue type | Missing frame (not tested) | Collision mismatch (wrong collisions) |
| Missing collision | Multiple in frame 89 | (29,89) in frame 95 |
| False positive | (105,137) in frame 89 | (29,77) in frame 95 |
| Divergence start | Frame 90 | Frame 96 |

**Similarity**: Both have the same underlying root cause (AABB gap problem), but manifest differently:
- Pair (101,105): QT didn't test the pair at all (missing frame)
- Pair (29,53): QT tested but got wrong collisions (mismatch)

---

## Recommendations

1. **Investigate frame 95 quadtree cell assignments**:
   - Check which cells lines 29, 77, and 89 are assigned to
   - Verify if AABBs overlap in world space but not in common cells
   - Check if expansion is sufficient for these pairs

2. **Verify false positive (29,77)**:
   - Check if lines 29 and 77 actually collide in frame 95
   - Verify if this is a real false positive or if BF missed it
   - Check bounding box overlap vs actual collision

3. **Check missing collision (29,89)**:
   - Verify if lines 29 and 89 should collide in frame 95
   - Check if their AABBs overlap in world space
   - Verify if quadtree found them in overlapping cells

4. **Consider pair-specific expansion**:
   - Lines 29, 77, 89 may need different expansion parameters
   - Or the expansion formula needs refinement

---

## Files Reference

- `history_files/pair_29_53_bf.txt`: BF kinematic history
- `history_files/pair_29_53_qt.txt`: QT kinematic history
- `comparison_files/pair_29_53_comparison.txt`: Frame-by-frame comparison
- `PAIR_29_53_ANALYSIS.md`: Initial analysis (symptom identification)
- `PAIR_29_53_ROOT_CAUSE.md`: This document (root cause analysis)

---

**Analysis Date**: $(date)
**Root Cause Frame**: Frame 95
**Divergence Frame**: Frame 96
**Divergent Line**: Line 29
**Issue**: QT missed (29,89), found false positive (29,77)

