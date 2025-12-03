# Debug Logging Findings - Frame 89 Analysis

## Root Cause Identified: Non-Overlapping Cells

### Key Discovery

**The quadtree is not finding the 9 missing pairs because the lines are in NON-OVERLAPPING cells!**

### Cell Assignments (Frame 89)

**Lines 41, 45, 49, 53, 57, 61, 65, 69, 93:**
- All in cells: `[0.523438,0.531250)x[0.679688,0.687500)` and `[0.523438,0.531250)x[0.687500,0.695312)`
- Array indices: 41, 45, 49, 53, 57, 61, 65, 69, 93
- Bounding boxes: All in range `[0.525212,0.529351]x[0.680000,0.693125]`

**Line 105:**
- In cells: `[0.515625,0.523438)x[0.671875,0.679688)`, `[0.515625,0.523438)x[0.679688,0.687500)`, `[0.515625,0.523438)x[0.687500,0.695312)`, `[0.515625,0.523438)x[0.695312,0.703125)`
- Array index: 105
- Bounding box: `[0.522491,0.523024]x[0.679063,0.695937]`

**Line 137:**
- In cells: 8 cells spanning both `[0.515625,0.523438)` and `[0.523438,0.531250)` ranges
- Array index: 137
- Bounding box: `[0.522401,0.524674]x[0.676562,0.698438]`

### Critical Finding

**Lines 41-93 are in X-range `[0.523438,0.531250)`**
**Line 105 is in X-range `[0.515625,0.523438)`**

**These X-ranges DO NOT OVERLAP!**

- Line 105's X-range: `[0.515625, 0.523438)` (max = 0.523438)
- Lines 41-93's X-range: `[0.523438, 0.531250)` (min = 0.523438)

The cells are adjacent but non-overlapping. The quadtree spatial query only finds pairs in the SAME cells, so it never finds (41,105), (45,105), etc. as candidate pairs.

### Why This Is Wrong

If brute-force found collisions between line 105 and lines 41-93, the lines MUST be close enough to collide. The fact that they're in non-overlapping cells suggests:

1. **Bounding box computation is too small** - The bounding boxes don't include the full extent of the lines' movement
2. **Cell boundaries are too strict** - Lines that should be in overlapping cells are separated by cell boundaries
3. **timeStep inconsistency** - The bounding boxes might be computed with wrong timeStep

### Candidate Pairs Found

**Only (105,137) was found:**
- Found in 4 cells (line1=105) and 4 cells (line1=137)
- Both lines share overlapping cells: `[0.515625,0.523438)x[...]`
- This is why QT tested (105,137) and found a collision (false positive)

### Filtered Pairs

**Pair (137,105) was filtered:**
- When processing line1=137 (idx=137), found line2=105 (idx=105)
- Filtered because line2 array idx (105) <= line1 array idx (137)
- This is correct filtering logic

### Analysis

**Why QT didn't find the 9 missing pairs:**
- Lines 41-93 are in cells `[0.523438,0.531250)x[...]`
- Line 105 is in cells `[0.515625,0.523438)x[...]`
- These cells don't overlap, so quadtree never finds them together
- Root cause: Bounding boxes are too small or computed incorrectly

**Why QT found false positive (105,137):**
- Both lines share overlapping cells `[0.515625,0.523438)x[...]`
- Quadtree correctly found them as candidate pair
- But the actual lines don't intersect (BF found NO_INTERSECTION)
- This suggests the bounding boxes overlap but actual lines don't

### Next Steps

1. **Verify bounding box computation:**
   - Check if bounding boxes include full line extent
   - Verify timeStep is used correctly
   - Compare bounding boxes between build and query phases

2. **Check cell boundary handling:**
   - Lines near cell boundaries might be in wrong cells
   - Consider expanding bounding boxes slightly to account for cell boundaries

3. **Investigate false positive (105,137):**
   - Why do bounding boxes overlap but lines don't intersect?
   - Check if this is due to different line states between algorithms

### Conclusion

The root cause is clear: **Bounding boxes are too small or computed incorrectly, causing lines that should be in overlapping cells to be in non-overlapping cells.** This prevents the quadtree from finding candidate pairs that brute-force correctly identifies.

The fix will likely involve:
- Expanding bounding boxes to include full line extent
- Ensuring timeStep consistency
- Possibly adjusting cell boundary handling

