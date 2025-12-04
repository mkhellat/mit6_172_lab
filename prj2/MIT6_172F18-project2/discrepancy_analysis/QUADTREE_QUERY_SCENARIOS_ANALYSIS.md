# Quadtree Query Scenarios Analysis

## Overview

This document analyzes how the quadtree handles different scenarios where lines are in different cells but their AABBs overlap in various ways.

## Key Mechanisms

### Build Phase (`insertLineRecursive`)

**Process:**
1. For each line, compute its AABB (axis-aligned bounding box)
2. Insert the line into **ALL cells** whose bounding box overlaps with the line's AABB
3. A line can be in **multiple cells** if its AABB spans cell boundaries

**Code:**
```c
static QuadTreeError insertLineRecursive(QuadNode* node, Line* line,
                                         double lineXmin, double lineXmax,
                                         double lineYmin, double lineYmax,
                                         QuadTree* tree) {
  // Check if line overlaps this cell
  if (!boxesOverlap(lineXmin, lineXmax, lineYmin, lineYmax,
                    node->xmin, node->xmax, node->ymin, node->ymax)) {
    return QUADTREE_SUCCESS;  // Line doesn't overlap, nothing to do
  }
  
  // If leaf, add line to this cell
  if (node->isLeaf) {
    addLineToNode(node, line);
    // ... subdivision logic ...
  }
  
  // If not leaf, recursively insert into children
  for (int i = 0; i < 4; i++) {
    insertLineRecursive(node->children[i], line, ...);
  }
}
```

**Key Point:** A line is inserted into **every cell** its AABB overlaps, regardless of where the line's actual geometry is.

### Query Phase (`QuadTree_findCandidatePairs`)

**Process:**
1. For each line (line1):
   - Compute its AABB
   - Find **ALL cells** whose bounding box overlaps with line1's AABB
   - For each of these cells, collect **ALL lines** in that cell (line2)
   - These become candidate pairs

**Code:**
```c
// For each line
for (unsigned int i = 0; i < tree->numLines; i++) {
  Line* line1 = tree->lines[i];
  
  // Compute line1's AABB
  computeLineBoundingBox(line1, tree->buildTimeStep,
                         &lineXmin, &lineXmax, &lineYmin, &lineYmax);
  
  // Find all cells line1's AABB overlaps
  int numCells = findOverlappingCellsRecursive(tree->root,
                                                lineXmin, lineXmax,
                                                lineYmin, lineYmax,
                                                overlappingCells, ...);
  
  // Collect all OTHER lines from these cells
  for (int cellIdx = 0; cellIdx < numCells; cellIdx++) {
    QuadNode* cell = overlappingCells[cellIdx];
    
    for (unsigned int j = 0; j < cell->numLines; j++) {
      Line* line2 = cell->lines[j];
      // Add (line1, line2) as candidate pair
    }
  }
}
```

**Key Point:** When querying for line1, we collect all lines from all cells that line1's AABB overlaps.

---

## Scenario Analysis

### Scenario 1: l1 totally in C1, l2 totally in C2, but l2's AABB overlaps C1

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- l2's AABB: overlaps cell C1's bounding box

**Build Phase:**
1. **l1 insertion:**
   - l1's AABB overlaps C1 → l1 inserted into C1
   - l1's AABB does NOT overlap C2 → l1 NOT inserted into C2
   - **Result:** l1 is in C1 only

2. **l2 insertion:**
   - l2's AABB overlaps C2 → l2 inserted into C2
   - l2's AABB overlaps C1 → l2 ALSO inserted into C1
   - **Result:** l2 is in BOTH C1 and C2

**Query Phase:**
1. **When querying for l1:**
   - l1's AABB overlaps C1 → find C1
   - Collect all lines from C1: {l1, l2} (l2 is in C1 because its AABB overlaps C1)
   - **Result:** (l1, l2) is found as candidate pair ✓

2. **When querying for l2:**
   - l2's AABB overlaps C1 and C2 → find both C1 and C2
   - Collect all lines from C1: {l1, l2}
   - Collect all lines from C2: {l2, ...}
   - **Result:** (l2, l1) would be found, but filtered by array index check

**Conclusion:** ✓ **The pair (l1, l2) IS found** because l2 is inserted into C1 (since its AABB overlaps C1), and when querying for l1, we collect all lines from C1.

---

### Scenario 2: l1 partially in C1 and C2, l2 totally in C2, but l2's AABB overlaps C1

**Setup:**
- l1's geometry: partially in C1, partially in C2
- l2's geometry: completely inside cell C2
- l2's AABB: overlaps cell C1's bounding box

**Build Phase:**
1. **l1 insertion:**
   - l1's AABB overlaps C1 → l1 inserted into C1
   - l1's AABB overlaps C2 → l1 ALSO inserted into C2
   - **Result:** l1 is in BOTH C1 and C2

2. **l2 insertion:**
   - l2's AABB overlaps C2 → l2 inserted into C2
   - l2's AABB overlaps C1 → l2 ALSO inserted into C1
   - **Result:** l2 is in BOTH C1 and C2

**Query Phase:**
1. **When querying for l1:**
   - l1's AABB overlaps C1 and C2 → find both C1 and C2
   - Collect all lines from C1: {l1, l2} (both are in C1)
   - Collect all lines from C2: {l1, l2, ...} (both are in C2)
   - **Result:** (l1, l2) is found as candidate pair ✓

2. **When querying for l2:**
   - l2's AABB overlaps C1 and C2 → find both C1 and C2
   - Collect all lines from C1: {l1, l2}
   - Collect all lines from C2: {l1, l2, ...}
   - **Result:** (l2, l1) would be found, but filtered by array index check

**Conclusion:** ✓ **The pair (l1, l2) IS found** because both lines are in both cells (since their AABBs overlap both cells).

---

### Scenario 3: l1 totally in C1, l2 totally in C2, but l2's AABB overlaps l1's AABB

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- l2's AABB: overlaps l1's AABB (but may not overlap C1's bounding box)

**Critical Question:** Does l2's AABB overlap C1's bounding box?

**Case 3A: l2's AABB overlaps C1's bounding box**
- This is the same as Scenario 1
- **Result:** ✓ (l1, l2) IS found

**Case 3B: l2's AABB does NOT overlap C1's bounding box**
- **Build Phase:**
  - l1's AABB overlaps C1 → l1 inserted into C1
  - l1's AABB does NOT overlap C2 → l1 NOT inserted into C2
  - l2's AABB overlaps C2 → l2 inserted into C2
  - l2's AABB does NOT overlap C1 → l2 NOT inserted into C1
  - **Result:** l1 is in C1 only, l2 is in C2 only

- **Query Phase:**
  - When querying for l1: l1's AABB overlaps C1 → find C1 → collect lines from C1: {l1} only (l2 is NOT in C1)
  - When querying for l2: l2's AABB overlaps C2 → find C2 → collect lines from C2: {l2, ...} (l1 is NOT in C2)
  - **Result:** ✗ (l1, l2) is NOT found!

**Conclusion:** 
- ✓ If l2's AABB overlaps C1's bounding box → pair IS found
- ✗ If l2's AABB does NOT overlap C1's bounding box → pair is NOT found, even though their AABBs overlap!

**This is a potential issue!** Two lines can have overlapping AABBs but be in non-overlapping cells, causing the quadtree to miss the pair.

---

### Scenario 4: l1 totally in C1, l2 totally in C2, but l2's AABB overlaps C1 and l1's AABB overlaps C2

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- l2's AABB: overlaps cell C1's bounding box
- l1's AABB: overlaps cell C2's bounding box

**Build Phase:**
1. **l1 insertion:**
   - l1's AABB overlaps C1 → l1 inserted into C1
   - l1's AABB overlaps C2 → l1 ALSO inserted into C2
   - **Result:** l1 is in BOTH C1 and C2

2. **l2 insertion:**
   - l2's AABB overlaps C2 → l2 inserted into C2
   - l2's AABB overlaps C1 → l2 ALSO inserted into C1
   - **Result:** l2 is in BOTH C1 and C2

**Query Phase:**
1. **When querying for l1:**
   - l1's AABB overlaps C1 and C2 → find both C1 and C2
   - Collect all lines from C1: {l1, l2} (both are in C1)
   - Collect all lines from C2: {l1, l2, ...} (both are in C2)
   - **Result:** (l1, l2) is found as candidate pair ✓

2. **When querying for l2:**
   - l2's AABB overlaps C1 and C2 → find both C1 and C2
   - Collect all lines from C1: {l1, l2}
   - Collect all lines from C2: {l1, l2, ...}
   - **Result:** (l2, l1) would be found, but filtered by array index check

**Conclusion:** ✓ **The pair (l1, l2) IS found** because both lines are in both cells (since their AABBs overlap both cells).

---

### Scenario 5: l1 totally in C1, l2 totally in C2, but l2's AABB overlaps C3

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- C3: not a first-neighbor of C1 or C2
- l2's AABB: overlaps cell C3's bounding box

**Build Phase:**
1. **l1 insertion:**
   - l1's AABB overlaps C1 → l1 inserted into C1
   - l1's AABB does NOT overlap C2 or C3 → l1 NOT inserted into C2 or C3
   - **Result:** l1 is in C1 only

2. **l2 insertion:**
   - l2's AABB overlaps C2 → l2 inserted into C2
   - l2's AABB overlaps C3 → l2 ALSO inserted into C3
   - l2's AABB does NOT overlap C1 → l2 NOT inserted into C1
   - **Result:** l2 is in BOTH C2 and C3 (but NOT in C1)

**Query Phase:**
1. **When querying for l1:**
   - l1's AABB overlaps C1 → find C1
   - Collect all lines from C1: {l1, ...} (l2 is NOT in C1)
   - **Result:** ✗ (l1, l2) is NOT found!

2. **When querying for l2:**
   - l2's AABB overlaps C2 and C3 → find both C2 and C3
   - Collect all lines from C2: {l2, ...} (l1 is NOT in C2)
   - Collect all lines from C3: {l2, ...} (l1 is NOT in C3)
   - **Result:** ✗ (l2, l1) is NOT found!

**Conclusion:** ✗ **The pair (l1, l2) is NOT found** because:
- l2's AABB overlaps C3, so l2 is inserted into C3
- But l1's AABB does NOT overlap C3, so l1 is NOT in C3
- When querying for l1, we only check C1 (which l2 is not in)
- When querying for l2, we check C2 and C3 (which l1 is not in)

**This is expected behavior** - if two lines' AABBs don't overlap the same cells, they won't be found as a candidate pair, even if their AABBs overlap in world space.

---

### Scenario 6: l1 totally in C1, l2 totally in C2, but l2's AABB overlaps a line's AABB within C3

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- l3: a line totally inside cell C3
- l2's AABB: overlaps l3's AABB (but may not overlap C3's bounding box)

**Critical Question:** Does l2's AABB overlap C3's bounding box?

**Case 6A: l2's AABB overlaps C3's bounding box**
- This is similar to Scenario 5
- l2 is inserted into C3
- But l1 is NOT in C3
- **Result:** ✗ (l1, l2) is NOT found

**Case 6B: l2's AABB does NOT overlap C3's bounding box**
- l2 is NOT inserted into C3
- **Result:** ✗ (l1, l2) is NOT found (same as Scenario 3B)

**Conclusion:** ✗ **The pair (l1, l2) is NOT found** regardless of whether l2's AABB overlaps l3's AABB, because:
- The quadtree only checks cell-level overlaps, not line-level AABB overlaps
- If l2's AABB doesn't overlap C1's bounding box, l2 won't be in C1
- If l1's AABB doesn't overlap C2's bounding box, l1 won't be in C2
- Therefore, they won't be found as a candidate pair

**This highlights a limitation:** The quadtree only considers cell-level overlaps, not direct line-to-line AABB overlaps.

---

### Scenario 7: l1 totally in C1, l2 totally in C2, l3 totally in C3, but AABBs of l1, l2, and l3 all overlap

**Setup:**
- l1's geometry: completely inside cell C1
- l2's geometry: completely inside cell C2
- l3's geometry: completely inside cell C3
- AABBs of l1, l2, and l3: all overlap each other

**Critical Question:** Which cells do each line's AABB overlap?

**Case 7A: All AABBs overlap all three cells (C1, C2, C3)**
- **Build Phase:**
  - l1's AABB overlaps C1, C2, C3 → l1 inserted into all three
  - l2's AABB overlaps C1, C2, C3 → l2 inserted into all three
  - l3's AABB overlaps C1, C2, C3 → l3 inserted into all three
  - **Result:** All three lines are in all three cells

- **Query Phase:**
  - When querying for l1: finds C1, C2, C3 → collects {l1, l2, l3} from all cells
  - When querying for l2: finds C1, C2, C3 → collects {l1, l2, l3} from all cells
  - When querying for l3: finds C1, C2, C3 → collects {l1, l2, l3} from all cells
  - **Result:** ✓ All pairs (l1, l2), (l1, l3), (l2, l3) ARE found

**Case 7B: l1's AABB overlaps C1 and C2, l2's AABB overlaps C2 and C3, l3's AABB overlaps C1 and C3**
- **Build Phase:**
  - l1's AABB overlaps C1, C2 → l1 inserted into C1 and C2
  - l2's AABB overlaps C2, C3 → l2 inserted into C2 and C3
  - l3's AABB overlaps C1, C3 → l3 inserted into C1 and C3
  - **Result:** 
    - C1 contains: {l1, l3}
    - C2 contains: {l1, l2}
    - C3 contains: {l2, l3}

- **Query Phase:**
  - When querying for l1: finds C1, C2 → collects {l1, l3} from C1, {l1, l2} from C2
    - **Result:** (l1, l2) and (l1, l3) ARE found ✓
  - When querying for l2: finds C2, C3 → collects {l1, l2} from C2, {l2, l3} from C3
    - **Result:** (l2, l1) and (l2, l3) would be found, but (l2, l1) filtered by array index
  - When querying for l3: finds C1, C3 → collects {l1, l3} from C1, {l2, l3} from C3
    - **Result:** (l3, l1) and (l3, l2) would be found, but filtered by array index
  - **Result:** ✓ All pairs ARE found

**Case 7C: l1's AABB overlaps only C1, l2's AABB overlaps only C2, l3's AABB overlaps only C3**
- **Build Phase:**
  - l1's AABB overlaps C1 only → l1 inserted into C1 only
  - l2's AABB overlaps C2 only → l2 inserted into C2 only
  - l3's AABB overlaps C3 only → l3 inserted into C3 only
  - **Result:** Each line is in its own cell only

- **Query Phase:**
  - When querying for l1: finds C1 only → collects {l1, ...} (l2 and l3 are NOT in C1)
  - When querying for l2: finds C2 only → collects {l2, ...} (l1 and l3 are NOT in C2)
  - When querying for l3: finds C3 only → collects {l3, ...} (l1 and l2 are NOT in C3)
  - **Result:** ✗ NO pairs are found!

**Conclusion:**
- ✓ If AABBs overlap the same cells → pairs ARE found
- ✗ If AABBs don't overlap the same cells → pairs are NOT found, even if AABBs overlap in world space

---

## Key Insights

### 1. Cell-Level vs Line-Level Overlaps

**The quadtree only considers cell-level overlaps, not direct line-to-line AABB overlaps.**

- A line is inserted into a cell if its AABB overlaps the cell's bounding box
- When querying, we collect lines from cells that the query line's AABB overlaps
- **If two lines' AABBs overlap in world space but don't overlap the same cells, they won't be found as a candidate pair.**

### 2. The "Gap" Problem

**Scenario 3B and 7C illustrate a critical issue:**

- Two lines can have overlapping AABBs
- But if their AABBs don't overlap the same cells, they won't be found
- This happens when:
  - The AABBs are close but don't span cell boundaries
  - Cell boundaries fall between the AABBs
  - The AABBs are small relative to cell size

### 3. Why Epsilon Expansion Helps (But Creates False Positives)

**The epsilon expansion (0.0005) addresses the gap problem by:**
- Expanding AABBs slightly
- Ensuring AABBs that are close overlap the same cells
- But this creates false positives (pairs that don't actually collide)

### 4. The Relative vs Absolute Parallelogram Issue

**Even if AABBs overlap the same cells, there's another issue:**
- Quadtree uses absolute parallelograms (each line's swept area)
- Collision detection uses relative parallelograms (l2's swept area relative to l1)
- Two lines can collide even if their absolute AABBs don't overlap the same cells

---

## Summary Table

| Scenario | l1's AABB overlaps | l2's AABB overlaps | l1 in cells | l2 in cells | Pair Found? |
|----------|-------------------|-------------------|------------|------------|-------------|
| 1 | C1 | C1, C2 | C1 | C1, C2 | ✓ Yes |
| 2 | C1, C2 | C1, C2 | C1, C2 | C1, C2 | ✓ Yes |
| 3A | C1 | C1, C2 | C1 | C1, C2 | ✓ Yes |
| 3B | C1 | C2 only | C1 | C2 | ✗ No |
| 4 | C1, C2 | C1, C2 | C1, C2 | C1, C2 | ✓ Yes |
| 5 | C1 | C2, C3 | C1 | C2, C3 | ✗ No |
| 6 | C1 | C2 (not C3) | C1 | C2 | ✗ No |
| 7A | C1, C2, C3 | C1, C2, C3 | All | All | ✓ Yes |
| 7B | C1, C2 | C2, C3 | C1, C2 | C2, C3 | ✓ Yes |
| 7C | C1 only | C2 only | C1 | C2 | ✗ No |

**Key Pattern:** A pair is found if and only if there exists at least one cell that both lines' AABBs overlap.

---

## Implications for the Discrepancy

### Why Missing Collisions Occur

1. **AABB Gap Problem (Scenarios 3B, 7C):**
   - Lines' AABBs overlap in world space
   - But AABBs don't overlap the same cells
   - Quadtree doesn't find them as candidate pairs
   - **Solution:** Expand AABBs (epsilon or velocity-dependent)

2. **Relative vs Absolute Parallelogram (All Scenarios):**
   - Even if AABBs overlap the same cells
   - Absolute parallelograms might not overlap
   - But relative parallelograms do overlap (collision detected)
   - **Solution:** Expand AABBs to account for relative motion

### Why False Positives Occur

1. **Epsilon Expansion:**
   - Expanded AABBs overlap more cells
   - More cell overlaps → more candidate pairs
   - Some candidate pairs don't actually collide
   - **Solution:** Use velocity-dependent expansion instead of fixed epsilon

---

## Code References

### Build Phase
- `quadtree.c:481-590`: `insertLineRecursive` - inserts line into all overlapping cells
- `quadtree.c:592-653`: `QuadTree_build` - builds tree from all lines

### Query Phase
- `quadtree.c:704-1257`: `QuadTree_findCandidatePairs` - finds candidate pairs
- `quadtree.c:669-702`: `findOverlappingCellsRecursive` - finds cells a line's AABB overlaps
- `quadtree.c:60-69`: `boxesOverlap` - checks if two AABBs overlap

### AABB Computation
- `quadtree.c:99-136`: `computeLineBoundingBox` - computes AABB from 4 parallelogram vertices

