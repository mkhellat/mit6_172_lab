# Discrepancy Investigation Report
## Brute-Force vs Quadtree Collision Detection

**Date**: Analysis conducted on pair (101,105) and line 105  
**Input**: `box.in`  
**Frames analyzed**: 1-100  
**First divergence**: Frame 90

---

## Executive Summary

The quadtree (QT) implementation shows significant discrepancies compared to brute-force (BF) collision detection. The investigation focused on pair (101,105) and discovered that:

1. **Divergence starts at frame 90** with line 105 (l2) having different physical quantities
2. **QT missed 13 collisions** that BF found in frames 88-90
3. **QT found 2 false positives**: (105,137) in frames 89 and 90
4. **Frame 89 is critical**: QT did not test pair (101,105) in this frame

---

## 1. Pair (101,105) Physical Quantities History

### 1.1 Frame-by-Frame Comparison

The comparison file `pair_101_105_comparison.txt` shows:
- **Frames 1-88**: Physical quantities match perfectly (NO divergence)
- **Frame 89**: BF tested, QT did NOT test
- **Frame 90**: First divergence detected (YES)
- **Frames 90-99**: All show divergence

### 1.2 Physical Quantities at Divergence Point

**Frame 88 (Last matching frame):**
- BF l1_p1.x: 0.5241479610, l1_vel.x: -0.0017040779
- BF l2_p1.x: 0.5235577776, l2_vel.x: -0.0020369871
- QT l1_p1.x: 0.5241479610, l1_vel.x: -0.0017040779
- QT l2_p1.x: 0.5235577776, l2_vel.x: -0.0020369871
- **Status**: MATCH

**Frame 90 (First divergence):**
- BF l1_p1.x: 0.5239456480, l1_vel.x: -0.0021050131
- BF l2_p1.x: 0.5252171048, l2_vel.x: -0.0019351863
- QT l1_p1.x: 0.5239456480, l1_vel.x: -0.0021050131
- QT l2_p1.x: 0.5220813320, l2_vel.x: -0.0018860761
- **Status**: DIVERGED (l2_p1.x and l2_vel.x differ)

**Frame 99 (Maximum divergence):**
- BF l1_p1.x: 0.5336622681, l1_vel.x: 0.0023983180
- BF l2_p1.x: 0.5305340845, l2_vel.x: 0.0050995907
- QT l1_p1.x: 0.5300905191, l1_vel.x: 0.0014128300
- QT l2_p1.x: 0.5289743279, l2_vel.x: 0.0057589211
- **Status**: DIVERGED (all quantities differ)
- **Result**: BF finds NO_INTERSECTION (0), QT finds COLLISION (1)

### 1.3 Missing Frame Analysis

**Frame 89:**
- BF tested (101,105): result=0 (NO_INTERSECTION)
- QT did NOT test (101,105)
- This missing test is a critical issue

**Possible reasons:**
1. Array index check filtered out the pair
2. Quadtree did not find the pair in any cell
3. Pair was filtered by duplicate detection

---

## 2. Line 105 Collision History Analysis

### 2.1 Total Collision Counts

- **BF total collisions involving line 105**: 35
- **QT total collisions involving line 105**: 23
- **Difference**: QT missed 12 collisions

### 2.2 Frame-by-Frame Collision Tally (Frames 88-90)

#### Frame 88

**BF Collisions:**
- (105,145) type=1
- (105,149) type=1
- (105,161) type=1
- **Total: 3 collisions**

**QT Collisions:**
- (105,145) type=1
- (105,149) type=1
- **Total: 2 collisions**

**Analysis:**
- ✅ Matched: (105,145), (105,149)
- ❌ Missing in QT: (105,161)
- ❌ Extra in QT: None

#### Frame 89 (CRITICAL)

**BF Collisions:**
- (41,105) type=1
- (45,105) type=1
- (49,105) type=1
- (53,105) type=1
- (57,105) type=1
- (61,105) type=1
- (65,105) type=1
- (69,105) type=1
- (93,105) type=1
- **Total: 9 collisions**

**QT Collisions:**
- (105,137) type=1
- **Total: 1 collision**

**Analysis:**
- ❌ Missing in QT: ALL 9 BF collisions
  - (41,105), (45,105), (49,105), (53,105), (57,105)
  - (61,105), (65,105), (69,105), (93,105)
- ❌ Extra in QT: (105,137) - **FALSE POSITIVE** (BF did not find this)
- **Impact**: Line 105's velocity changed differently in QT vs BF

#### Frame 90

**BF Collisions:**
- (65,105) type=1
- (73,105) type=1
- (97,105) type=1
- (105,129) type=1
- **Total: 4 collisions**

**QT Collisions:**
- (105,137) type=1
- **Total: 1 collision**

**Analysis:**
- ❌ Missing in QT: ALL 4 BF collisions
  - (65,105), (73,105), (97,105), (105,129)
- ❌ Extra in QT: (105,137) - **FALSE POSITIVE** (BF did not find this)
- **Impact**: Line 105's state continues to diverge

### 2.3 Cumulative Impact

**Frames 88-90 Summary:**
- BF total collisions: 16
- QT total collisions: 4
- QT missed: 13 collisions
- QT false positives: 2 collisions

**Physics Impact:**
Each collision resolution changes line velocities. With 13 missed collisions and 2 false positives, line 105's velocity vector diverged significantly, causing:
- Different position updates
- Different future collision trajectories
- Cascading divergence in subsequent frames

---

## 3. Root Cause Analysis

### 3.1 Why QT Missed Collisions

**Frame 89 - Missing 9 collisions:**
Possible reasons:
1. **Quadtree spatial partitioning**: Lines might be in different cells than expected
2. **Array index check**: The check to match BF's iteration order might be too restrictive
3. **Bounding box computation**: Lines spanning multiple cells might not be properly handled
4. **Candidate pair filtering**: Pairs might be filtered out incorrectly

**Frame 88 - Missing (105,161):**
- Similar issues as frame 89
- One specific pair not found by quadtree

### 3.2 Why QT Found False Positives

**False positive: (105,137) in frames 89-90**

Possible reasons:
1. **Incorrect bounding box**: Line 105's bounding box might include line 137 when it shouldn't
2. **Time step mismatch**: Bounding box computed with wrong timeStep
3. **Cell overlap**: Lines in adjacent cells incorrectly considered as candidates
4. **Array order issue**: Pair might be tested in wrong order, leading to incorrect collision detection

### 3.3 Why Frame 89 Pair (101,105) Was Not Tested

Possible reasons:
1. **Array index check**: The check `line2ArrayIndex <= i` might have filtered it out
2. **Quadtree query**: The quadtree might not have found this pair in any cell
3. **Duplicate detection**: The pair might have been incorrectly marked as duplicate

---

## 4. Physics Quantities Analysis

### 4.1 Velocity Changes

**Line 105 velocity evolution:**

**Frame 88:**
- BF: l2_vel.x = -0.0020369871
- QT: l2_vel.x = -0.0020369871
- **Status**: MATCH

**Frame 89:**
- BF: l2_vel.x = 0.0052538406 (changed due to 9 collisions)
- QT: l2_vel.x = ? (not tested, but line had 1 collision)
- **Status**: UNKNOWN (QT didn't test pair, but line had different collisions)

**Frame 90:**
- BF: l2_vel.x = -0.0019351863
- QT: l2_vel.x = -0.0018860761
- **Status**: DIVERGED

**Analysis:**
The velocity divergence is directly caused by different collision resolutions:
- BF resolved 9 collisions in frame 89, changing velocity significantly
- QT resolved only 1 collision (false positive), changing velocity differently
- This velocity difference propagates to position differences

### 4.2 Position Evolution

**Line 105 position (l2_p1.x) evolution:**

**Frame 88:**
- BF: 0.5235577776
- QT: 0.5235577776
- **Status**: MATCH

**Frame 90:**
- BF: 0.5252171048
- QT: 0.5220813320
- **Difference**: 0.0031357728
- **Status**: DIVERGED

**Frame 99:**
- BF: 0.5305340845
- QT: 0.5289743279
- **Difference**: 0.0015597566
- **Status**: DIVERGED

**Analysis:**
Position divergence starts at frame 90 and continues. The difference is due to:
1. Different velocity updates from frame 89 collisions
2. Different collision resolutions in subsequent frames
3. Cascading effects of initial divergence

---

## 5. Statistics Summary

### 5.1 Pair (101,105) Testing

- **BF tested in**: 100 frames (frames 1-100)
- **QT tested in**: 98 frames (missing frames 89 and 91)
- **Frames with matching quantities**: 88 frames (1-88)
- **Frames with divergence**: 11 frames (90-100)

### 5.2 Line 105 Collisions

- **BF total collisions**: 35
- **QT total collisions**: 23
- **Missing in QT**: 12 collisions
- **False positives in QT**: 2 collisions
- **Match rate**: 65.7% (23/35, excluding false positives)

### 5.3 Critical Frames

- **Frame 88**: Last frame with matching quantities
- **Frame 89**: Critical frame with massive collision discrepancy
- **Frame 90**: First divergence detected
- **Frame 99**: Maximum divergence with false positive collision

---

## 6. Conclusions

### 6.1 Primary Issues

1. **QT misses many collisions** that BF finds, especially in frame 89
2. **QT finds false positives** that BF does not find
3. **QT does not test all pairs** that BF tests (missing frames 89, 91)
4. **Physics divergence** starts at frame 90 due to different collision resolutions

### 6.2 Impact

The discrepancies cause:
- Different line velocities after collision resolution
- Different line positions in subsequent frames
- Cascading divergence throughout the simulation
- Incorrect collision detection in later frames

### 6.3 Next Steps

1. **Investigate quadtree spatial partitioning**:
   - Why are lines 41, 45, 49, 53, 57, 61, 65, 69, 93 not found with line 105 in frame 89?
   - Check bounding box computation for these lines

2. **Investigate false positive (105,137)**:
   - Why does QT find this collision when BF doesn't?
   - Check if bounding boxes incorrectly overlap
   - Verify timeStep consistency

3. **Fix array index check**:
   - Why is pair (101,105) not tested in frame 89?
   - Review the array index filtering logic

4. **Verify quadtree cell assignment**:
   - Ensure lines are placed in correct cells
   - Check for lines spanning multiple cells

---

## 7. Files Reference

### Data Files
- `pair_101_105_bf.txt`: BF raw data for pair (101,105)
- `pair_101_105_qt.txt`: QT raw data for pair (101,105)
- `line_105_collisions_bf.txt`: All BF collisions involving line 105
- `line_105_collisions_qt.txt`: All QT collisions involving line 105

### History Files
- `history_files/pair_101_105_bf_history.txt`: Complete BF history
- `history_files/pair_101_105_qt_history.txt`: Complete QT history

### Comparison Files
- `comparison_files/pair_101_105_comparison.txt`: Frame-by-frame comparison of physical quantities
- `comparison_files/line_105_collision_comparison.txt`: Detailed collision comparison

### Analysis Files
- `line_105_analysis.txt`: Summary analysis
- `STATISTICS.txt`: Summary statistics

### Scripts
- `scripts/create_comparison.py`: Python script for comparing physical quantities
- `scripts/extract_history.sh`: Shell script for extracting history
- `scripts/extract_comparison.sh`: Shell script for extracting comparisons
- `scripts/debug_discrepancy.sh`: Automated discrepancy debugging
- See `scripts/README.md` for complete script documentation

### Debug Logs
- `debug_logs/debug_bf_pairs.txt`: All pairs tested by BF (718MB)
- `debug_logs/debug_bf_collisions.txt`: All collisions found by BF
- `debug_logs/debug_qt_pairs.txt`: All pairs tested by QT (44MB)
- `debug_logs/debug_qt_collisions.txt`: All collisions found by QT

---

**Report Generated**: Analysis of discrepancy between BF and QT collision detection  
**Focus**: Physics quantities and collision tallies  
**Status**: Investigation ongoing

