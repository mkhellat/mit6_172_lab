# Optimization #5: Incremental Quadtree Updates - Design Document

## Overview

**Date:** December 2025  
**Status:** 🔄 Design Phase  
**Complexity:** High  
**Priority:** Medium (would provide benefit but implementation is complex)

## Problem

**Current Approach:**
- Quadtree is created, built, used, and destroyed **every frame**
- Full O(n log n) rebuild cost every frame
- Even if only a few lines moved, we rebuild everything
- Memory allocation/deallocation overhead per frame

**Inefficiency:**
- For n lines: O(n log n) rebuild cost every frame
- Most lines move only slightly between frames
- Only lines that moved significantly need tree updates
- Full rebuild is wasteful when most lines haven't moved much

## Solution: Incremental Updates

**Strategy:**
1. Keep quadtree alive across frames (don't destroy each frame)
2. Track old positions for each line
3. Determine which lines moved significantly
4. Update only moved lines: remove from old cells, insert into new cells
5. Fall back to full rebuild if too many lines moved or structure changes

**Benefits:**
- O(log n) per moved line vs O(n log n) rebuild
- Significant speedup when few lines move
- Reduces memory allocation overhead

**Challenges:**
- Correctness critical - must handle all edge cases
- Need to track old positions
- Complex implementation (cell changes, subdivision, merging)
- Must handle lines spanning multiple cells correctly

## Design

### 1. Track Old Positions

**Option A: Store in CollisionWorld**
```c
struct CollisionWorld {
  // ... existing fields ...
  Vec* oldLinePositions;  // Array of old positions (p1, p2) for each line
  bool* lineMoved;        // Flag for each line indicating if it moved significantly
};
```

**Option B: Store in QuadTree**
```c
typedef struct {
  // ... existing fields ...
  Vec* oldLinePositions;  // Array of old positions for tracking
  unsigned int* lineCellCount;  // Number of cells each line is in (for removal)
} QuadTree;
```

**Decision:** Store in CollisionWorld (lines are owned by CollisionWorld, positions are line properties)

### 2. Movement Threshold

**Determine if line moved significantly:**
```c
bool lineMovedSignificantly(Line* line, Vec oldP1, Vec oldP2, double threshold) {
  Vec currentP1 = line->p1;
  Vec currentP2 = line->p2;
  
  double dist1 = Vec_length(Vec_subtract(currentP1, oldP1));
  double dist2 = Vec_length(Vec_subtract(currentP2, oldP2));
  
  // Line moved significantly if either endpoint moved more than threshold
  return (dist1 > threshold || dist2 > threshold);
}
```

**Threshold Selection:**
- Too small: Too many lines considered "moved", negates benefit
- Too large: Lines might change cells without being updated, correctness issue
- **Recommended:** Fraction of cell size (e.g., 0.1 * averageCellSize)

### 3. Incremental Update Algorithm

```c
QuadTreeError QuadTree_updateIncremental(QuadTree* tree,
                                         Line** lines,
                                         unsigned int numLines,
                                         Vec* oldPositions,  // Array of old (p1, p2) positions
                                         double timeStep,
                                         double movementThreshold) {
  // Step 1: Identify lines that moved significantly
  unsigned int movedCount = 0;
  bool* moved = malloc(numLines * sizeof(bool));
  
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) continue;
    
    Vec oldP1 = oldPositions[i * 2];
    Vec oldP2 = oldPositions[i * 2 + 1];
    
    moved[i] = lineMovedSignificantly(lines[i], oldP1, oldP2, movementThreshold);
    if (moved[i]) {
      movedCount++;
    }
  }
  
  // Step 2: If too many lines moved, fall back to full rebuild
  double movedRatio = (double)movedCount / numLines;
  if (movedRatio > 0.5) {  // If more than 50% moved, rebuild is faster
    free(moved);
    return QuadTree_build(tree, lines, numLines, timeStep);
  }
  
  // Step 3: For each moved line, remove from old cells and insert into new cells
  for (unsigned int i = 0; i < numLines; i++) {
    if (!moved[i] || lines[i] == NULL) continue;
    
    Vec oldP1 = oldPositions[i * 2];
    Vec oldP2 = oldPositions[i * 2 + 1];
    
    // Remove from old cells (using old bounding box)
    double oldXmin, oldXmax, oldYmin, oldYmax;
    computeLineBoundingBoxOld(lines[i], oldP1, oldP2, timeStep, 
                              &oldXmin, &oldXmax, &oldYmin, &oldYmax,
                              tree->maxVelocity, tree->config.minCellSize);
    
    removeLineFromTree(tree, lines[i], oldXmin, oldXmax, oldYmin, oldYmax);
    
    // Insert into new cells (using current bounding box)
    double newXmin, newXmax, newYmin, newYmax;
    computeLineBoundingBox(lines[i], timeStep,
                          &newXmin, &newXmax, &newYmin, &newYmax,
                          tree->maxVelocity, tree->config.minCellSize);
    
    insertLineRecursive(tree->root, lines[i],
                       newXmin, newXmax, newYmin, newYmax, tree);
  }
  
  // Step 4: Update old positions for next frame
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) continue;
    oldPositions[i * 2] = lines[i]->p1;
    oldPositions[i * 2 + 1] = lines[i]->p2;
  }
  
  free(moved);
  return QUADTREE_SUCCESS;
}
```

### 4. Remove Line from Tree

**Challenge:** Lines can be in multiple cells (spanning cell boundaries)

**Algorithm:**
```c
void removeLineFromTree(QuadTree* tree, Line* line,
                       double xmin, double xmax, double ymin, double ymax) {
  // Find all cells that overlap with old bounding box
  // Remove line from those cells
  removeLineRecursive(tree->root, line, xmin, xmax, ymin, ymax);
}

void removeLineRecursive(QuadNode* node, Line* line,
                        double xmin, double xmax, double ymin, double ymax) {
  // Check if node overlaps with bounding box
  if (!boxesOverlap(node->xmin, node->xmax, node->ymin, node->ymax,
                    xmin, xmax, ymin, ymax)) {
    return;  // No overlap, skip
  }
  
  if (node->isLeaf) {
    // Remove line from this cell
    removeLineFromNode(node, line);
  } else {
    // Recursively remove from children
    for (int i = 0; i < 4; i++) {
      if (node->children[i] != NULL) {
        removeLineRecursive(node->children[i], line, xmin, xmax, ymin, ymax);
      }
    }
  }
}
```

### 5. Integration with CollisionWorld

**Modify CollisionWorld to support incremental updates:**
```c
struct CollisionWorld {
  // ... existing fields ...
  QuadTree* persistentTree;  // Keep tree alive across frames
  Vec* oldLinePositions;     // Track old positions for incremental updates
  bool useIncrementalUpdates;  // Flag to enable/disable incremental updates
};
```

**Modify detectIntersection to use incremental updates:**
```c
if (collisionWorld->useIncrementalUpdates && collisionWorld->persistentTree != NULL) {
  // Use incremental update
  QuadTree_updateIncremental(collisionWorld->persistentTree, ...);
} else {
  // Full rebuild (current approach)
  QuadTree* tree = QuadTree_create(...);
  QuadTree_build(tree, ...);
  // ... use tree ...
  if (collisionWorld->useIncrementalUpdates) {
    collisionWorld->persistentTree = tree;  // Keep for next frame
  } else {
    QuadTree_destroy(tree);
  }
}
```

## Implementation Complexity

### High Complexity Areas

1. **Line Removal:**
   - Lines can be in multiple cells
   - Must remove from all cells that old bounding box overlapped
   - Need to track which cells each line is in (or search for them)

2. **Cell Subdivision/Merging:**
   - If cell structure changes (subdivision or merging), incremental update becomes complex
   - May need to fall back to rebuild if structure changes significantly

3. **Correctness:**
   - Must ensure no lines are missed or duplicated
   - Must handle edge cases (lines on boundaries, empty cells, etc.)

4. **Memory Management:**
   - Need to track old positions
   - Need to handle tree structure changes

### Simpler Approach: Hybrid Strategy

**Compromise:**
1. Keep tree alive across frames
2. Track old positions
3. If few lines moved (< 30%): Use incremental update
4. If many lines moved (>= 30%): Fall back to rebuild
5. Always rebuild if tree structure changed significantly

**Benefits:**
- Simpler than full incremental implementation
- Still provides benefit when few lines move
- Falls back to proven rebuild approach when needed
- Lower risk of correctness issues

## Performance Analysis

### Expected Impact

**Scenario 1: Few Lines Move (10% moved)**
- Current: O(n log n) rebuild
- Incremental: O(0.1n * log n) = O(n log n) but 10x fewer operations
- **Benefit: ~10x speedup for build phase**

**Scenario 2: Many Lines Move (80% moved)**
- Current: O(n log n) rebuild
- Incremental: O(0.8n * log n) + overhead ≈ O(n log n)
- **Benefit: Minimal (fall back to rebuild)**

**Scenario 3: All Lines Move (100% moved)**
- Current: O(n log n) rebuild
- Incremental: Falls back to rebuild
- **Benefit: None (same as current)**

### When Incremental Updates Help

- **Small time steps:** Lines move less, fewer updates needed
- **Low collision density:** Fewer collisions mean fewer velocity changes
- **Stable simulation:** Lines maintain similar velocities

### When Incremental Updates Don't Help

- **Large time steps:** Many lines move significantly
- **High collision density:** Many velocity changes, many lines move
- **Chaotic simulation:** Lines change velocities frequently

## Correctness Considerations

### Critical Requirements

1. **No Missed Collisions:**
   - Lines must be in correct cells after update
   - Bounding boxes must be up-to-date
   - Query phase must find all candidates

2. **No Duplicate Candidates:**
   - Lines must not appear in cells they don't belong to
   - Duplicate elimination must still work

3. **Consistent State:**
   - Tree structure must be consistent
   - Old positions must be tracked correctly
   - Movement threshold must be appropriate

### Testing Strategy

1. **Correctness Tests:**
   - Compare collision counts with brute-force
   - Verify no regressions
   - Test edge cases (boundary lines, spanning cells, etc.)

2. **Performance Tests:**
   - Measure build time with incremental vs rebuild
   - Test with different movement ratios
   - Verify fallback to rebuild works correctly

## Implementation Plan

### Phase 1: Basic Infrastructure (Current)
- ✅ Design document
- ⏳ Add old position tracking to CollisionWorld
- ⏳ Add movement threshold calculation
- ⏳ Add flag to enable/disable incremental updates

### Phase 2: Core Functionality
- ⏳ Implement `removeLineFromTree()`
- ⏳ Implement `QuadTree_updateIncremental()`
- ⏳ Integrate with CollisionWorld

### Phase 3: Optimization and Testing
- ⏳ Tune movement threshold
- ⏳ Optimize removal algorithm
- ⏳ Comprehensive testing
- ⏳ Performance benchmarking

## Recommendation

**For Now:** Document the design and provide a framework for future implementation.

**Rationale:**
- Current rebuild approach works well (build phase is ~70% of time, but acceptable)
- Incremental updates are complex and correctness-critical
- Risk of introducing bugs may outweigh benefits
- Can be implemented later if needed

**Future Work:**
- Implement when build phase becomes a bottleneck
- Start with hybrid approach (incremental for few moves, rebuild for many)
- Comprehensive testing before enabling by default

