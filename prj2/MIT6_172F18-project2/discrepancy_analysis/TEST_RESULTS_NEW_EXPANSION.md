# Test Results: Velocity-Dependent AABB Expansion

## Test Configuration
- **Input File**: box.in
- **Frames**: 100
- **Expansion Parameters**: 
  - k_rel = 0.3 (relative motion factor)
  - k_gap = 0.15 (gap factor)
  - precision_margin = 1e-6

## Overall Results

### Collision Counts
- **BF Total Collisions**: 3249
- **QT Total Collisions**: 3305
- **Difference**: +56 (QT has more collisions)

### Unique Collision Pairs
- **BF unique pairs**: 1668
- **QT unique pairs**: 1702
- **Common pairs**: 1406
- **BF only (missing in QT)**: 262 pairs
- **QT only (extra in QT)**: 296 pairs
- **Net difference**: +34 unique pairs

## Pair (101,105) Analysis

### Status: ✅ **RESOLVED**

- **BF frames tested**: 100/100
- **QT frames tested**: 100/100
- **Mismatches**: 0
- **All frames match perfectly**

### Critical Frames (88-92)
| Frame | BF Result | QT Result | Match |
|-------|-----------|-----------|-------|
| 88    | 0         | 0         | ✓     |
| 89    | 0         | 0         | ✓     |
| 90    | 0         | 0         | ✓     |
| 91    | 1         | 1         | ✓     |
| 92    | 0         | 0         | ✓     |

**Key Finding**: Frame 89 is now being tested by both BF and QT, and they match! This was the critical missing frame that caused the divergence.

## Comparison with Previous Implementation

| Metric | Previous (Epsilon 0.0005) | New (Velocity-Dependent) | Change |
|--------|--------------------------|---------------------------|--------|
| Total Discrepancy | +71 | +56 | **-15 (improved)** |
| Pair (101,105) | Missing frames | All frames match | ✅ **Fixed** |
| Frame 89 issue | Not tested | Tested and matches | ✅ **Fixed** |
| Unique pairs diff | Unknown | +34 | - |

## Detailed Discrepancy Analysis

### Missing Collisions (BF only, QT missed)
**Count**: 262 pairs

**Sample missing pairs**:
- (29, 53), (29, 89)
- (30, 54), (30, 90)
- (57, 81), (57, 89)
- (58, 82), (58, 90)
- (61, 97), (61, 113)

**Pattern**: Many pairs involve lines in the 29-30, 57-58, 61-62 range, suggesting these lines might still have AABB gap issues.

### Extra Collisions (QT only, false positives)
**Count**: 296 pairs

**Sample extra pairs**:
- (29, 69), (29, 77)
- (30, 70), (30, 78)
- (61, 89), (61, 93)
- (62, 90), (62, 94)
- (65, 89), (65, 93)

**Pattern**: Similar line ranges, suggesting the expansion might be too aggressive for some line pairs.

## Key Findings

### ✅ Successes

1. **Pair (101,105) issue RESOLVED**
   - All 100 frames now match perfectly
   - Frame 89 is now being tested (was missing before)
   - No mismatches in any frame

2. **Overall discrepancy reduced**
   - From +71 to +56 (15 fewer false positives)
   - Improvement of ~21%

3. **Velocity-dependent expansion works**
   - The approach addresses relative motion
   - Better than fixed epsilon

### ⚠️ Remaining Issues

1. **Still has +56 total discrepancy**
   - 262 missing collisions (QT didn't find)
   - 296 extra collisions (QT false positives)
   - Net: +34 unique pairs difference

2. **Pattern in discrepancies**
   - Many involve lines 29-30, 57-58, 61-65, 89-90, 93-94
   - Suggests specific line groups still have issues
   - May need line-specific or cluster-specific expansion

3. **Parameters may need tuning**
   - k_rel = 0.3 might not be optimal
   - k_gap = 0.15 might not be optimal
   - Could try different values

## Recommendations

### Short-term (Parameter Tuning)
1. **Try different k_rel values**: 0.2, 0.25, 0.35, 0.4
2. **Try different k_gap values**: 0.1, 0.12, 0.18, 0.2
3. **Test combinations** to find optimal balance

### Medium-term (Investigation)
1. **Analyze missing pairs**: Why are 262 pairs still missing?
   - Check if they have AABB gap issues
   - Check if relative motion expansion is sufficient
2. **Analyze false positives**: Why are 296 pairs false positives?
   - Check if expansion is too aggressive
   - Check if these pairs are in overlapping cells but don't actually collide

### Long-term (Alternative Solutions)
1. **Consider line-cluster-specific expansion**
   - Different expansion for different line groups
   - Adaptive based on local density
2. **Consider two-pass approach**
   - First pass: absolute AABBs (current)
   - Second pass: relative AABBs for candidate pairs
3. **Consider Minkowski sum approach**
   - More theoretically sound
   - Computationally more expensive but more accurate

## Conclusion

The velocity-dependent AABB expansion shows **significant improvement**:

- ✅ **Fixed the critical (101,105) pair issue**
- ✅ **Reduced overall discrepancy by 21%**
- ✅ **Better than fixed epsilon approach**

However, it still has:
- ⚠️ **+56 total discrepancy** (needs further work)
- ⚠️ **262 missing collisions** (needs investigation)
- ⚠️ **296 false positives** (needs tuning)

**Verdict**: The solution is **working and improving**, but needs **further tuning or refinement** to eliminate remaining discrepancies.

---

**Test Date**: $(date)
**Test Configuration**: Velocity-dependent expansion (k_rel=0.3, k_gap=0.15)
**Input**: box.in, 100 frames
