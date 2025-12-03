# Next Steps for Discrepancy Investigation

## Current Status

### Key Findings

1. **False Positive (105,137) in frames 89-90:**
   - BF tested: result=0 (NO_INTERSECTION)
   - QT tested: result=1 (COLLISION)
   - **Root cause**: QT detected collision when lines were not actually colliding
   - **Impact**: Incorrect collision resolution changed line 105's state

2. **Missing 9 Collisions in Frame 89:**
   - Pairs: (41,105), (45,105), (49,105), (53,105), (57,105), (61,105), (65,105), (69,105), (93,105)
   - BF tested all 9 and found collisions (result=1)
   - QT did NOT test ANY of these 9 pairs
   - **Root cause**: Quadtree spatial query not finding these pairs in same cells
   - **Impact**: Line 105 missed 9 collision resolutions, causing state divergence

3. **Divergence Point:**
   - Frame 90: First divergence in physical quantities
   - Caused by different collision resolutions in frames 88-89
   - Line 105's velocity diverged, then position diverged

## Root Cause Hypotheses

### Hypothesis 1: Spatial Partitioning Issue
**Problem**: Lines 41-93 and line 105 are not being found in overlapping quadtree cells.

**Possible reasons**:
1. Bounding boxes computed incorrectly (too small or wrong timeStep)
2. Quadtree cells too small, lines in different cells
3. Lines span multiple cells but query not finding all overlapping cells
4. Cell boundaries causing lines to be separated incorrectly

**Investigation needed**:
- Add debug logging to track which cells each line is in
- Compare bounding boxes between BF and QT
- Verify bounding box computation uses correct timeStep
- Check if lines 41-93 and 105 share any cells

### Hypothesis 2: Array Index Filtering Issue
**Problem**: Pairs are found in same cells but filtered out by array index check.

**Possible reasons**:
1. Line 105 is at earlier array index than lines 41-93
2. When processing line1=line41, line2=line105 is skipped because line105's index < line41's index
3. Array index check logic is incorrect

**Investigation needed**:
- Add debug logging to track array indices of lines 41-93 and 105
- Log when pairs are filtered by array index check
- Verify array index check logic is correct

### Hypothesis 3: False Positive Detection Issue
**Problem**: QT detects (105,137) as collision when it's not.

**Possible reasons**:
1. Lines have different physical quantities in QT vs BF (due to earlier divergence)
2. Bounding box overlap but actual lines don't intersect
3. Intersection test using wrong line states

**Investigation needed**:
- Compare physical quantities of lines 105 and 137 in QT vs BF at frame 89
- Verify intersection test is using correct line states
- Check if false positive is due to earlier divergence

## Proposed Investigation Steps

### Step 1: Add Debug Logging to Quadtree (HIGH PRIORITY)

Add debug logging to `QuadTree_findCandidatePairs` to track:

1. **Cell Assignment Logging:**
   ```c
   #ifdef DEBUG_DISCREPANCY
   // Log which cells each line is in (for specific lines in frame 89)
   if (line1->id == 105 || line1->id == 41 || line1->id == 45 || ...) {
     fprintf(debugFile, "Frame %d: Line %u in %d cells: ", frameNum, line1->id, numCells);
     for (int c = 0; c < numCells; c++) {
       fprintf(debugFile, "[%.6f,%.6f)x[%.6f,%.6f) ", 
               overlappingCells[c]->xmin, overlappingCells[c]->xmax,
               overlappingCells[c]->ymin, overlappingCells[c]->ymax);
     }
     fprintf(debugFile, "\n");
   }
   #endif
   ```

2. **Candidate Pair Logging:**
   ```c
   #ifdef DEBUG_DISCREPANCY
   // Log when we find a candidate pair (before filtering)
   if ((line1->id == 105 && (line2->id == 41 || line2->id == 45 || ...)) ||
       ((line1->id == 41 || line1->id == 45 || ...) && line2->id == 105)) {
     fprintf(debugFile, "Frame %d: Found candidate pair (%u,%u) in cell [%.6f,%.6f)x[%.6f,%.6f)\n",
             frameNum, line1->id, line2->id, cell->xmin, cell->xmax, cell->ymin, cell->ymax);
   }
   #endif
   ```

3. **Filtering Logging:**
   ```c
   #ifdef DEBUG_DISCREPANCY
   // Log when pairs are filtered
   if ((line1->id == 105 && (line2->id == 41 || line2->id == 45 || ...)) ||
       ((line1->id == 41 || line1->id == 45 || ...) && line2->id == 105)) {
     if (line2ArrayIndex <= i) {
       fprintf(debugFile, "Frame %d: Filtered pair (%u,%u) - line2 idx=%u <= line1 idx=%u\n",
               frameNum, line1->id, line2->id, line2ArrayIndex, i);
     }
     if (line1->id >= line2->id) {
       fprintf(debugFile, "Frame %d: Filtered pair (%u,%u) - id order wrong\n",
               frameNum, line1->id, line2->id);
     }
   }
   #endif
   ```

**Challenge**: Quadtree doesn't know frame number. Need to pass it or track it.

### Step 2: Compare Bounding Boxes (MEDIUM PRIORITY)

Add debug logging to compare bounding boxes between algorithms:

1. Log bounding boxes computed in quadtree build phase
2. Log bounding boxes computed in quadtree query phase
3. Compare with bounding boxes that would be used in brute-force
4. Verify timeStep consistency

### Step 3: Analyze False Positive (105,137) (MEDIUM PRIORITY)

1. Extract physical quantities of lines 105 and 137 in QT at frame 89
2. Extract physical quantities of lines 105 and 137 in BF at frame 89
3. Manually verify if lines actually intersect
4. Check if false positive is due to different line states

### Step 4: Verify Array Index Order (LOW PRIORITY)

1. Log array indices of lines 41-93 and 105 in tree->lines
2. Verify array index check logic is correct
3. Check if filtering is happening incorrectly

## Implementation Plan

1. **Modify quadtree.c:**
   - Add frame number tracking (pass from collision_world.c)
   - Add debug logging for cell assignments
   - Add debug logging for candidate pairs
   - Add debug logging for filtering

2. **Modify collision_world.c:**
   - Pass frame number to quadtree query functions
   - Enable DEBUG_DISCREPANCY flag

3. **Run debug build:**
   - Compile with -DDEBUG_DISCREPANCY
   - Run box.in for 99 frames
   - Extract debug output

4. **Analyze results:**
   - Check which cells lines 41-93 and 105 are in
   - Check if pairs are found but filtered
   - Identify root cause

## Expected Outcomes

After investigation, we should be able to answer:

1. **Why QT didn't find the 9 missing pairs:**
   - Are lines in different cells?
   - Are pairs found but filtered?
   - Is bounding box computation wrong?

2. **Why QT found false positive (105,137):**
   - Are line states different?
   - Is intersection test wrong?
   - Is bounding box overlap causing false positive?

3. **What needs to be fixed:**
   - Spatial partitioning logic?
   - Bounding box computation?
   - Array index filtering?
   - Intersection test?

## Next Actions

1. ✅ Confirm QT didn't test the 9 missing pairs
2. ✅ Confirm QT found false positive (105,137)
3. ⏳ Add debug logging to quadtree (NEXT)
4. ⏳ Run debug build and analyze output
5. ⏳ Identify root cause and fix

