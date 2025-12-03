/**
 * quadtree.c -- Spatial quadtree for optimized collision detection
 * Copyright (c) 2012 the Massachusetts Institute of Technology
 *
 * Implementation of dynamic quadtree data structure for efficiently
 * detecting potential collisions between moving line segments.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 **/

#include "./quadtree.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdbool.h>

#include "./line.h"
#include "./vec.h"

// ============================================================================
// Constants
// ============================================================================

// Default initial capacity for line arrays in nodes
#define DEFAULT_LINE_CAPACITY 4

// Default initial capacity for candidate pair lists
#define DEFAULT_CANDIDATE_CAPACITY 64

// ============================================================================
// Helper Functions (Internal)
// ============================================================================

/**
 * Check if two axis-aligned bounding boxes overlap.
 * 
 * @param xmin1, xmax1, ymin1, ymax1 Bounds of first box
 * @param xmin2, xmax2, ymin2, ymax2 Bounds of second box
 * @return true if boxes overlap, false otherwise
 */
static bool boxesOverlap(double xmin1, double xmax1,
                         double ymin1, double ymax1,
                         double xmin2, double xmax2,
                         double ymin2, double ymax2) {
  // Boxes overlap if they overlap in both X and Y dimensions
  // Two boxes don't overlap if one is completely to the left, right,
  // above, or below the other
  return !(xmax1 < xmin2 || xmin1 > xmax2 ||
           ymax1 < ymin2 || ymin1 > ymax2);
}

/**
 * Compute the minimum of two doubles.
 */
static double minDouble(double a, double b) {
  return (a < b) ? a : b;
}

/**
 * Compute the maximum of two doubles.
 */
static double maxDouble(double a, double b) {
  return (a > b) ? a : b;
}

/**
 * Compute the bounding box of a line segment, including its future
 * position after movement.
 * 
 * The bounding box includes both current and future positions to ensure
 * we don't miss collisions that occur as lines move between frames.
 * 
 * @param line Line to compute bounding box for
 * @param timeStep Time step for computing future position
 * @param xmin Output parameter for minimum X
 * @param xmax Output parameter for maximum X
 * @param ymin Output parameter for minimum Y
 * @param ymax Output parameter for maximum Y
 */
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax) {
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
  
  // CRITICAL FIX: Expand bounding box slightly to account for:
  // 1. Numerical precision errors (lines very close to cell boundaries)
  // 2. Swept parallelogram area (moving line segments sweep out area)
  // 3. Edge cases near cell boundaries (e.g., Line 105 was 0.000414 from boundary)
  // 
  // Without this expansion, lines that should be in overlapping cells
  // end up in non-overlapping cells, causing quadtree to miss candidate pairs.
  // 
  // Epsilon value: 0.0005 (half of 0.001) is approximately 0.06% of typical
  // cell size (~0.008 at depth 6). This is small enough to avoid significant
  // false positives but large enough to handle numerical precision issues.
  const double epsilon = 0.0005;
  *xmin -= epsilon;
  *xmax += epsilon;
  *ymin -= epsilon;
  *ymax += epsilon;
}

/**
 * Update debug statistics when a node is created.
 * 
 * @param tree QuadTree to update stats for
 * @param node Newly created node
 */
static void updateStatsOnNodeCreate(QuadTree* tree, QuadNode* node) {
  if (tree->stats == NULL) {
    return;
  }
  
  tree->stats->totalNodes++;
  
  if (node->isLeaf) {
    tree->stats->totalLeaves++;
  }
  
  if (node->depth > (int)tree->stats->maxDepthReached) {
    tree->stats->maxDepthReached = (unsigned int)node->depth;
  }
  
  if (node->numLines > tree->stats->maxLinesInNode) {
    tree->stats->maxLinesInNode = node->numLines;
  }
}

/**
 * Update debug statistics when a query is performed.
 * 
 * @param tree QuadTree to update stats for
 * @param cellsChecked Number of cells examined
 * @param pairsFound Number of candidate pairs found
 */
static void updateStatsOnQuery(QuadTree* tree,
                               unsigned int cellsChecked,
                               unsigned int pairsFound) {
  if (tree->stats == NULL) {
    return;
  }
  
  tree->stats->totalQueries++;
  tree->stats->totalCellsChecked += cellsChecked;
  tree->stats->totalPairsTested += pairsFound;
}

// ============================================================================
// Configuration Functions
// ============================================================================

QuadTreeConfig QuadTreeConfig_default(void) {
  QuadTreeConfig config;
  config.maxDepth = 12;
  // CRITICAL: For clustered inputs like smalllines.in, we need to INCREASE maxLinesPerNode.
  // When lines are clustered, subdividing too aggressively (small maxLinesPerNode) creates
  // many small cells that hit the minCellSize limit quickly. All clustered lines then
  // accumulate in those small cells, leading to O(n^2) behavior during query.
  // By increasing maxLinesPerNode, we allow more lines per cell before subdividing,
  // which means cells stay larger longer and can subdivide more effectively when needed.
  // Value of 32 provides good balance: allows effective subdivision for clustered inputs
  // while still maintaining spatial filtering benefits.
  config.maxLinesPerNode = 32;
  config.minCellSize = 0.001;
  config.enableDebugStats = false;
  return config;
}

QuadTreeConfig QuadTreeConfig_create(unsigned int maxDepth,
                                     unsigned int maxLinesPerNode,
                                     double minCellSize,
                                     bool enableDebugStats) {
  QuadTreeConfig config;
  config.maxDepth = maxDepth;
  config.maxLinesPerNode = maxLinesPerNode;
  config.minCellSize = minCellSize;
  config.enableDebugStats = enableDebugStats;
  return config;
}

QuadTreeError QuadTreeConfig_validate(const QuadTreeConfig* config) {
  if (config == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (config->maxDepth == 0) {
    return QUADTREE_ERROR_INVALID_CONFIG;
  }
  
  if (config->maxLinesPerNode == 0) {
    return QUADTREE_ERROR_INVALID_CONFIG;
  }
  
  if (config->minCellSize <= 0.0) {
    return QUADTREE_ERROR_INVALID_CONFIG;
  }
  
  return QUADTREE_SUCCESS;
}

// ============================================================================
// Error Handling
// ============================================================================

const char* QuadTree_errorString(QuadTreeError error) {
  switch (error) {
    case QUADTREE_SUCCESS:
      return "Success";
    case QUADTREE_ERROR_NULL_POINTER:
      return "NULL pointer argument";
    case QUADTREE_ERROR_INVALID_BOUNDS:
      return "Invalid bounding box (xmin >= xmax or ymin >= ymax)";
    case QUADTREE_ERROR_MALLOC_FAILED:
      return "Memory allocation failed";
    case QUADTREE_ERROR_INVALID_CONFIG:
      return "Invalid configuration parameters";
    case QUADTREE_ERROR_EMPTY_TREE:
      return "Operation on empty tree";
    default:
      return "Unknown error";
  }
}

// ============================================================================
// Node Creation and Destruction (Internal)
// ============================================================================

/**
 * Create a new quadtree node with the specified bounds.
 * 
 * @param xmin, xmax, ymin, ymax Bounding box of the node (must be square)
 * @param depth Depth of this node in the tree (0 = root)
 * @return New node, or NULL on allocation failure
 */
static QuadNode* createQuadNode(double xmin, double xmax,
                                double ymin, double ymax,
                                int depth) {
  QuadNode* node = malloc(sizeof(QuadNode));
  if (node == NULL) {
    return NULL;
  }
  
  node->xmin = xmin;
  node->xmax = xmax;
  node->ymin = ymin;
  node->ymax = ymax;
  
  node->lines = malloc(DEFAULT_LINE_CAPACITY * sizeof(Line*));
  if (node->lines == NULL) {
    free(node);
    return NULL;
  }
  
  node->numLines = 0;
  node->capacity = DEFAULT_LINE_CAPACITY;
  
  node->children[0] = NULL;  // SW
  node->children[1] = NULL;  // SE
  node->children[2] = NULL;  // NW
  node->children[3] = NULL;  // NE
  
  node->isLeaf = true;
  node->depth = depth;
  
  return node;
}

/**
 * Destroy a quadtree node and all its children recursively.
 * 
 * @param node Node to destroy (can be NULL)
 */
static void destroyQuadNode(QuadNode* node) {
  if (node == NULL) {
    return;
  }
  
  // Recursively destroy children
  if (!node->isLeaf) {
    for (int i = 0; i < 4; i++) {
      destroyQuadNode(node->children[i]);
    }
  }
  
  // Free line array and node itself
  free(node->lines);
  free(node);
}

/**
 * Add a line to a node's line array, growing if necessary.
 * 
 * @param node Node to add line to
 * @param line Line to add
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
static QuadTreeError addLineToNode(QuadNode* node, Line* line) {
  if (node == NULL || line == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  // Grow array if needed
  if (node->numLines >= node->capacity) {
    unsigned int newCapacity = node->capacity * 2;
    Line** newLines = realloc(node->lines, newCapacity * sizeof(Line*));
    if (newLines == NULL) {
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
    node->lines = newLines;
    node->capacity = newCapacity;
  }
  
  // Add line to array
  node->lines[node->numLines] = line;
  node->numLines++;
  
  return QUADTREE_SUCCESS;
}

// ============================================================================
// Tree Creation and Destruction
// ============================================================================

QuadTree* QuadTree_create(double xmin, double xmax,
                          double ymin, double ymax,
                          const QuadTreeConfig* config,
                          QuadTreeError* error) {
  // Validate bounds
  if (xmin >= xmax || ymin >= ymax) {
    if (error != NULL) {
      *error = QUADTREE_ERROR_INVALID_BOUNDS;
    }
    return NULL;
  }
  
  // Use default config if none provided
  QuadTreeConfig actualConfig;
  if (config == NULL) {
    actualConfig = QuadTreeConfig_default();
  } else {
    actualConfig = *config;
  }
  
  // Validate configuration
  QuadTreeError configError = QuadTreeConfig_validate(&actualConfig);
  if (configError != QUADTREE_SUCCESS) {
    if (error != NULL) {
      *error = configError;
    }
    return NULL;
  }
  
  // Allocate tree structure
  QuadTree* tree = malloc(sizeof(QuadTree));
  if (tree == NULL) {
    if (error != NULL) {
      *error = QUADTREE_ERROR_MALLOC_FAILED;
    }
    return NULL;
  }
  
  // Ensure world bounds form a square (take larger dimension)
  double width = xmax - xmin;
  double height = ymax - ymin;
  double size = maxDouble(width, height);
  
  // Center the square
  double centerX = (xmin + xmax) / 2.0;
  double centerY = (ymin + ymax) / 2.0;
  
  tree->worldXmin = centerX - size / 2.0;
  tree->worldXmax = centerX + size / 2.0;
  tree->worldYmin = centerY - size / 2.0;
  tree->worldYmax = centerY + size / 2.0;
  
  // Create root node
  tree->root = createQuadNode(tree->worldXmin, tree->worldXmax,
                               tree->worldYmin, tree->worldYmax, 0);
  if (tree->root == NULL) {
    free(tree);
    if (error != NULL) {
      *error = QUADTREE_ERROR_MALLOC_FAILED;
    }
    return NULL;
  }
  
  // Store configuration
  tree->config = actualConfig;
  
  // Initialize lines array (will be set during build)
  tree->lines = NULL;
  tree->numLines = 0;
  
  // Allocate debug statistics if enabled
  if (actualConfig.enableDebugStats) {
    tree->stats = malloc(sizeof(QuadTreeDebugStats));
    if (tree->stats == NULL) {
      destroyQuadNode(tree->root);
      free(tree);
      if (error != NULL) {
        *error = QUADTREE_ERROR_MALLOC_FAILED;
      }
      return NULL;
    }
    memset(tree->stats, 0, sizeof(QuadTreeDebugStats));
  } else {
    tree->stats = NULL;
  }
  
  // Update statistics
  updateStatsOnNodeCreate(tree, tree->root);
  
  if (error != NULL) {
    *error = QUADTREE_SUCCESS;
  }
  return tree;
}

void QuadTree_destroy(QuadTree* tree) {
  if (tree == NULL) {
    return;
  }
  
  destroyQuadNode(tree->root);
  
  if (tree->stats != NULL) {
    free(tree->stats);
  }
  
  free(tree);
}

// ============================================================================
// Line Insertion and Tree Building
// ============================================================================

/**
 * Recursively insert a line into the quadtree.
 * 
 * @param node Current node to insert into
 * @param line Line to insert
 * @param lineXmin, lineXmax, lineYmin, lineYmax Bounding box of line
 * @param tree QuadTree (for config and stats)
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
static QuadTreeError insertLineRecursive(QuadNode* node,
                                         Line* line,
                                         double lineXmin, double lineXmax,
                                         double lineYmin, double lineYmax,
                                         QuadTree* tree) {
  // Check if line overlaps this cell
  if (!boxesOverlap(lineXmin, lineXmax, lineYmin, lineYmax,
                    node->xmin, node->xmax, node->ymin, node->ymax)) {
    return QUADTREE_SUCCESS;  // Line doesn't overlap, nothing to do
  }
  
  // If this is a leaf node
  if (node->isLeaf) {
    // CRITICAL FIX: Add line FIRST, then check if we need to subdivide
    // This matches the standard quadtree insertion algorithm
    QuadTreeError result = addLineToNode(node, line);
    if (result != QUADTREE_SUCCESS) {
      return result;
    }
    
    // Now check if we should subdivide (AFTER adding the line)
    bool shouldSubdivide = false;
    
    if (node->numLines > tree->config.maxLinesPerNode &&
        node->depth < (int)tree->config.maxDepth) {
      // Check if cell is large enough to subdivide
      double cellWidth = node->xmax - node->xmin;
      double cellHeight = node->ymax - node->ymin;
      double minDimension = minDouble(cellWidth, cellHeight);
      
      if (minDimension >= tree->config.minCellSize * 2.0) {
        shouldSubdivide = true;
      }
    }
    
    if (shouldSubdivide) {
      // Subdivide this node
      double xmid = (node->xmin + node->xmax) / 2.0;
      double ymid = (node->ymin + node->ymax) / 2.0;
      
      // Create 4 child nodes (squares)
      node->children[0] = createQuadNode(node->xmin, xmid,
                                         node->ymin, ymid,
                                         node->depth + 1);  // SW
      node->children[1] = createQuadNode(xmid, node->xmax,
                                         node->ymin, ymid,
                                         node->depth + 1);  // SE
      node->children[2] = createQuadNode(node->xmin, xmid,
                                         ymid, node->ymax,
                                         node->depth + 1);  // NW
      node->children[3] = createQuadNode(xmid, node->xmax,
                                         ymid, node->ymax,
                                         node->depth + 1);  // NE
      
      // Check for allocation failures
      for (int i = 0; i < 4; i++) {
        if (node->children[i] == NULL) {
          // Clean up already-allocated children
          for (int j = 0; j < i; j++) {
            destroyQuadNode(node->children[j]);
            node->children[j] = NULL;
          }
          return QUADTREE_ERROR_MALLOC_FAILED;
        }
        updateStatsOnNodeCreate(tree, node->children[i]);
      }
      
      // Mark as non-leaf
      node->isLeaf = false;
      
      // Redistribute existing lines to children
      // CRITICAL: Use the same timeStep that was used during initial build
      // This ensures bounding boxes are computed consistently
      for (unsigned int i = 0; i < node->numLines; i++) {
        Line* existingLine = node->lines[i];
        double exmin, exmax, eymin, eymax;
        // Use the timeStep from build phase (stored in tree structure)
        computeLineBoundingBox(existingLine, tree->buildTimeStep, &exmin, &exmax,
                               &eymin, &eymax);
        
        for (int j = 0; j < 4; j++) {
          insertLineRecursive(node->children[j], existingLine,
                             exmin, exmax, eymin, eymax, tree);
        }
      }
      
      // Clear parent's line list (lines now in children, including the new one)
      node->numLines = 0;
      
      // All lines (including the new one) have been redistributed to children
      return QUADTREE_SUCCESS;
    } else {
      // Line already added above, no subdivision needed
      return QUADTREE_SUCCESS;
    }
  }
  
  // If we reach here, node is not a leaf (and was NOT just subdivided)
  // Recursively insert into children
  for (int i = 0; i < 4; i++) {
    QuadTreeError result = insertLineRecursive(node->children[i], line,
                                               lineXmin, lineXmax,
                                               lineYmin, lineYmax, tree);
    if (result != QUADTREE_SUCCESS) {
      return result;
    }
  }
  
  return QUADTREE_SUCCESS;
}

QuadTreeError QuadTree_build(QuadTree* tree,
                             Line** lines,
                             unsigned int numLines,
                             double timeStep) {
  if (tree == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (lines == NULL && numLines > 0) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  // Store lines array for query phase (we don't own these, just reference)
  tree->lines = lines;
  tree->numLines = numLines;
  tree->buildTimeStep = timeStep;  // Store for use during subdivision
  
  // Reset statistics if enabled
  if (tree->stats != NULL) {
    memset(tree->stats, 0, sizeof(QuadTreeDebugStats));
    // Re-count root node
    tree->stats->totalNodes = 1;
    tree->stats->totalLeaves = 1;
  }
  
  // Insert each line
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) {
      continue;  // Skip NULL lines
    }
    
    // Compute line's bounding box
    double lineXmin, lineXmax, lineYmin, lineYmax;
    computeLineBoundingBox(lines[i], timeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax);
    
    // Insert into tree
    QuadTreeError result = insertLineRecursive(tree->root, lines[i],
                                               lineXmin, lineXmax,
                                               lineYmin, lineYmax, tree);
    if (result != QUADTREE_SUCCESS) {
      return result;
    }
    
    // Update statistics for lines in multiple cells
    if (tree->stats != NULL) {
      // Simple heuristic: if bounding box spans multiple potential cells,
      // it's likely in multiple cells
      double worldWidth = tree->worldXmax - tree->worldXmin;
      double worldHeight = tree->worldYmax - tree->worldYmin;
      double lineWidth = lineXmax - lineXmin;
      double lineHeight = lineYmax - lineYmin;
      
      // If line is large relative to world, it likely spans cells
      if (lineWidth > worldWidth * 0.1 || lineHeight > worldHeight * 0.1) {
        tree->stats->linesInMultipleCells++;
      }
    }
  }
  
  return QUADTREE_SUCCESS;
}

// ============================================================================
// Query Functions
// ============================================================================

/**
 * Recursively find all cells that a line overlaps.
 * 
 * @param node Current node to check
 * @param lineXmin, lineXmax, lineYmin, lineYmax Bounding box of line
 * @param cells Array to collect overlapping cells (must be pre-allocated)
 * @param cellCount Current count in cells array
 * @param cellCapacity Capacity of cells array
 * @return Updated cellCount, or -1 on error
 */
static int findOverlappingCellsRecursive(QuadNode* node,
                                         double lineXmin, double lineXmax,
                                         double lineYmin, double lineYmax,
                                         QuadNode** cells,
                                         int cellCount,
                                         int cellCapacity) {
  // Check if line overlaps this cell
  if (!boxesOverlap(lineXmin, lineXmax, lineYmin, lineYmax,
                    node->xmin, node->xmax, node->ymin, node->ymax)) {
    return cellCount;  // No overlap, return current count
  }
  
  // If leaf, add to list
  if (node->isLeaf) {
    if (cellCount >= cellCapacity) {
      return -1;  // Array full (shouldn't happen with proper sizing)
    }
    cells[cellCount] = node;
    return cellCount + 1;
  }
  
  // If not leaf, recursively check children
  for (int i = 0; i < 4; i++) {
    cellCount = findOverlappingCellsRecursive(node->children[i],
                                              lineXmin, lineXmax,
                                              lineYmin, lineYmax,
                                              cells, cellCount, cellCapacity);
    if (cellCount < 0) {
      return -1;  // Error
    }
  }
  
  return cellCount;
}

QuadTreeError QuadTree_findCandidatePairs(QuadTree* tree,
                                          double timeStep,
                                          QuadTreeCandidateList* candidateList,
                                          int frameNumber) {
  if (tree == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  // CRITICAL: Verify timeStep matches buildTimeStep for consistency
  // The query phase must use the same timeStep as the build phase
  // to ensure bounding boxes are computed consistently
  if (fabs(timeStep - tree->buildTimeStep) > 1e-10) {
    #ifdef DEBUG_QUADTREE
    fprintf(stderr, "WARNING: timeStep mismatch! buildTimeStep=%.10f, query timeStep=%.10f\n",
            tree->buildTimeStep, timeStep);
    #endif
    // Use buildTimeStep to ensure consistency
    // (The parameter is kept for API compatibility, but we use buildTimeStep internally)
  }
  
  if (candidateList == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (tree->root == NULL) {
    return QUADTREE_ERROR_EMPTY_TREE;
  }
  
  if (tree->lines == NULL || tree->numLines == 0) {
    // Tree was built but has no lines
    candidateList->count = 0;
    return QUADTREE_SUCCESS;
  }
  
  // Allocate temporary storage for overlapping cells per line
  // Worst case: line spans all cells at max depth
  unsigned int maxCellsPerLine = (1u << tree->config.maxDepth);
  QuadNode** overlappingCells = malloc(maxCellsPerLine * sizeof(QuadNode*));
  if (overlappingCells == NULL) {
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  
  // Find max line ID to allocate pair tracking matrix
  unsigned int maxLineId = 0;
  for (unsigned int i = 0; i < tree->numLines; i++) {
    if (tree->lines[i] != NULL && tree->lines[i]->id > maxLineId) {
      maxLineId = tree->lines[i]->id;
    }
  }
  
  // Build a map from Line* to array index for O(1) lookup
  // This allows us to quickly check if line2 appears later in array than line1
  // We use a hash-like approach: since lines are pointers, we can't easily hash them,
  // but we can do a linear search once per line2 (still O(n) per line2, but cached)
  // Actually, let's build a proper map: array of (Line*, index) pairs, sorted by pointer
  // Or simpler: just do the lookup, but cache results per line2
  // Actually simplest: build an array indexed by line pointer... but pointers aren't small integers
  // So we'll stick with the linear search but optimize it
  
  // Allocate a 2D boolean matrix to track all pairs we've seen globally
  // This ensures we never add the same pair twice, regardless of processing order
  // Matrix is upper triangular (only pairs where id1 < id2 are stored)
  // Access pattern: seenPairs[minId][maxId] for pair (minId, maxId)
  bool** seenPairs = calloc(maxLineId + 1, sizeof(bool*));
  if (seenPairs == NULL) {
    free(overlappingCells);
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  for (unsigned int i = 0; i <= maxLineId; i++) {
    seenPairs[i] = calloc(maxLineId + 1, sizeof(bool));
    if (seenPairs[i] == NULL) {
      // Clean up already allocated rows
      for (unsigned int j = 0; j < i; j++) {
        free(seenPairs[j]);
      }
      free(seenPairs);
      free(overlappingCells);
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
  }
  
  // Track statistics
  unsigned int totalCellsChecked = 0;
  unsigned int totalPairsFound = 0;
  
  // For each line, find overlapping cells and collect candidate pairs
  // NOTE: We assume tree->lines has no duplicates (lines are inserted once during build)
  // If duplicates exist, they would be caught by the seenPairs matrix anyway
  for (unsigned int i = 0; i < tree->numLines; i++) {
    Line* line1 = tree->lines[i];
    if (line1 == NULL) {
      continue;
    }
    
    // CRITICAL FIX: Removed O(n^2) duplicate line check
    // The previous implementation checked all previous lines for each line,
    // resulting in O(n^2) complexity that negated quadtree benefits.
    // We trust that tree->lines has no duplicates (lines inserted once during build),
    // and the seenPairs matrix will catch any duplicate pairs anyway.
    
    // No need to reset - we use a global matrix that persists across iterations
    
    // Compute line's bounding box
    // CRITICAL: Use buildTimeStep to match the timeStep used during build phase
    // This ensures bounding boxes are computed consistently between build and query
    double lineXmin, lineXmax, lineYmin, lineYmax;
    computeLineBoundingBox(line1, tree->buildTimeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax);
    
    // Find all cells this line overlaps
    int numCells = findOverlappingCellsRecursive(tree->root,
                                                  lineXmin, lineXmax,
                                                  lineYmin, lineYmax,
                                                  overlappingCells, 0,
                                                  maxCellsPerLine);
    if (numCells < 0) {
      // Clean up pair tracking matrix
      for (unsigned int i = 0; i <= maxLineId; i++) {
        free(seenPairs[i]);
      }
      free(seenPairs);
      free(overlappingCells);
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
    
    totalCellsChecked += (unsigned int)numCells;
    
    #ifdef DEBUG_DISCREPANCY
    // Debug: Log cell assignments for specific lines in frame 89
    // Lines we care about: 41, 45, 49, 53, 57, 61, 65, 69, 93, 105, 137
    static FILE* qtCellFile = NULL;
    static bool qtCellFileOpened = false;
    if (!qtCellFileOpened) {
      qtCellFile = fopen("debug_qt_cells.txt", "w");
      if (qtCellFile != NULL) {
        fprintf(qtCellFile, "=== Quadtree Cell Assignments ===\n");
      }
      qtCellFileOpened = true;
    }
    
    // Check if this is one of the lines we're tracking
    unsigned int line1Id = line1->id;
    bool isTrackedLine = (line1Id == 41 || line1Id == 45 || line1Id == 49 ||
                          line1Id == 53 || line1Id == 57 || line1Id == 61 ||
                          line1Id == 65 || line1Id == 69 || line1Id == 93 ||
                          line1Id == 105 || line1Id == 137);
    
    if (isTrackedLine && frameNumber == 89 && qtCellFile != NULL) {
      fprintf(qtCellFile, "Frame %d: Line %u (array idx=%u) in %d cells: ",
              frameNumber, line1Id, i, numCells);
      for (int c = 0; c < numCells; c++) {
        QuadNode* cell = overlappingCells[c];
        fprintf(qtCellFile, "[%.6f,%.6f)x[%.6f,%.6f)@depth%d ",
                cell->xmin, cell->xmax, cell->ymin, cell->ymax, cell->depth);
      }
      fprintf(qtCellFile, "bbox=[%.6f,%.6f]x[%.6f,%.6f]\n",
              lineXmin, lineXmax, lineYmin, lineYmax);
      fflush(qtCellFile);
    }
    #endif
    
    // Collect all OTHER lines from these cells
    // CRITICAL FIX: Only consider pairs that brute-force would also test.
    // Brute-force tests all pairs (i,j) where i < j (array indices), then swaps.
    // To match brute-force exactly, we need to ensure line2 comes from a later
    // position in the array than line1, OR if line2 comes from an earlier position,
    // we should have already tested it when line2 was line1.
    // 
    // However, since we're iterating through tree->lines in array order (i from 0 to numLines-1),
    // and we only add pairs where line1->id < line2->id, we need to ensure that
    // line2 actually appears later in the array than line1, OR that we've already
    // processed line2 as line1 in a previous iteration.
    //
    // The simplest fix: Only add pairs where line2 appears later in tree->lines
    // than line1. This ensures we test the same pairs as brute-force.
    for (int cellIdx = 0; cellIdx < numCells; cellIdx++) {
      QuadNode* cell = overlappingCells[cellIdx];
      
      for (unsigned int j = 0; j < cell->numLines; j++) {
        Line* line2 = cell->lines[j];
        
        // Skip NULL lines and self
        if (line2 == NULL || line2 == line1) {
          continue;
        }
        
        #ifdef DEBUG_DISCREPANCY
        // Debug: Log candidate pairs found (before filtering) for specific lines in frame 89
        static FILE* qtCandidatesFile = NULL;
        static bool qtCandidatesFileOpened = false;
        if (!qtCandidatesFileOpened) {
          qtCandidatesFile = fopen("debug_qt_candidates.txt", "w");
          if (qtCandidatesFile != NULL) {
            fprintf(qtCandidatesFile, "=== Quadtree Candidate Pairs (Before Filtering) ===\n");
          }
          qtCandidatesFileOpened = true;
        }
        
        unsigned int line1Id = line1->id;
        unsigned int line2Id = line2->id;
        
        // Check if this is one of the pairs we're tracking in frame 89
        // Missing pairs: (41,105), (45,105), (49,105), (53,105), (57,105),
        //                (61,105), (65,105), (69,105), (93,105)
        // False positive: (105,137)
        bool isTrackedPair = false;
        if (frameNumber == 89) {
          if ((line1Id == 41 && line2Id == 105) || (line1Id == 105 && line2Id == 41) ||
              (line1Id == 45 && line2Id == 105) || (line1Id == 105 && line2Id == 45) ||
              (line1Id == 49 && line2Id == 105) || (line1Id == 105 && line2Id == 49) ||
              (line1Id == 53 && line2Id == 105) || (line1Id == 105 && line2Id == 53) ||
              (line1Id == 57 && line2Id == 105) || (line1Id == 105 && line2Id == 57) ||
              (line1Id == 61 && line2Id == 105) || (line1Id == 105 && line2Id == 61) ||
              (line1Id == 65 && line2Id == 105) || (line1Id == 105 && line2Id == 65) ||
              (line1Id == 69 && line2Id == 105) || (line1Id == 105 && line2Id == 69) ||
              (line1Id == 93 && line2Id == 105) || (line1Id == 105 && line2Id == 93) ||
              (line1Id == 105 && line2Id == 137) || (line1Id == 137 && line2Id == 105)) {
            isTrackedPair = true;
          }
        }
        
        if (isTrackedPair && qtCandidatesFile != NULL) {
          fprintf(qtCandidatesFile, "Frame %d: Found candidate pair (line1 id=%u at idx=%u, line2 id=%u) in cell [%.6f,%.6f)x[%.6f,%.6f)@depth%d\n",
                  frameNumber, line1Id, i, line2Id, cell->xmin, cell->xmax, cell->ymin, cell->ymax, cell->depth);
          fflush(qtCandidatesFile);
        }
        #endif
        
        // CRITICAL: Ensure line2 appears later in tree->lines than line1
        // This matches brute-force's iteration order (i < j)
        unsigned int line2ArrayIndex = 0;
        bool line2Found = false;
        for (unsigned int k = 0; k < tree->numLines; k++) {
          if (tree->lines[k] == line2) {
            line2ArrayIndex = k;
            line2Found = true;
            break;
          }
        }
        if (!line2Found) {
          // line2 is not in tree->lines - this shouldn't happen, but skip it
          #ifdef DEBUG_DISCREPANCY
          fprintf(stderr, "WARNING: line2 (id=%u) not found in tree->lines!\n", line2->id);
          #endif
          continue;
        }
        if (line2ArrayIndex <= i) {
          // line2 appears before/at line1's position in the array
          // Brute-force would test this as (line2, line1) when j < i, not (line1, line2)
          // So we should skip it here to avoid double-testing
          #ifdef DEBUG_DISCREPANCY
          static FILE* qtFilteredFile = NULL;
          static bool qtFilteredFileOpened = false;
          if (!qtFilteredFileOpened) {
            qtFilteredFile = fopen("debug_qt_filtered.txt", "w");
            if (qtFilteredFile != NULL) {
              fprintf(qtFilteredFile, "=== Quadtree Filtered Pairs ===\n");
            }
            qtFilteredFileOpened = true;
          }
          
          unsigned int line1Id = line1->id;
          unsigned int line2Id = line2->id;
          
          // Log filtering for tracked pairs in frame 89
          bool isTrackedPair = false;
          if (frameNumber == 89) {
            if ((line1Id == 41 && line2Id == 105) || (line1Id == 105 && line2Id == 41) ||
                (line1Id == 45 && line2Id == 105) || (line1Id == 105 && line2Id == 45) ||
                (line1Id == 49 && line2Id == 105) || (line1Id == 105 && line2Id == 49) ||
                (line1Id == 53 && line2Id == 105) || (line1Id == 105 && line2Id == 53) ||
                (line1Id == 57 && line2Id == 105) || (line1Id == 105 && line2Id == 57) ||
                (line1Id == 61 && line2Id == 105) || (line1Id == 105 && line2Id == 61) ||
                (line1Id == 65 && line2Id == 105) || (line1Id == 105 && line2Id == 65) ||
                (line1Id == 69 && line2Id == 105) || (line1Id == 105 && line2Id == 69) ||
                (line1Id == 93 && line2Id == 105) || (line1Id == 105 && line2Id == 93) ||
                (line1Id == 105 && line2Id == 137) || (line1Id == 137 && line2Id == 105)) {
              isTrackedPair = true;
            }
          }
          
          if (isTrackedPair && qtFilteredFile != NULL) {
            fprintf(qtFilteredFile, "Frame %d: Filtered pair (line1 id=%u at idx=%u, line2 id=%u at idx=%u) - REASON: line2 array idx <= line1 array idx\n",
                    frameNumber, line1Id, i, line2Id, line2ArrayIndex);
            fflush(qtFilteredFile);
          }
          #endif
          continue;
        }
        
        // Now ensure line1->id < line2->id (matches brute-force after swap)
        if (line1->id >= line2->id) {
          #ifdef DEBUG_DISCREPANCY
          static FILE* qtFilteredFile2 = NULL;
          static bool qtFilteredFile2Opened = false;
          if (!qtFilteredFile2Opened) {
            qtFilteredFile2 = fopen("debug_qt_filtered.txt", "a");
            qtFilteredFile2Opened = true;
          }
          
          unsigned int line1Id = line1->id;
          unsigned int line2Id = line2->id;
          
          // Log filtering for tracked pairs in frame 89
          bool isTrackedPair = false;
          if (frameNumber == 89) {
            if ((line1Id == 41 && line2Id == 105) || (line1Id == 105 && line2Id == 41) ||
                (line1Id == 45 && line2Id == 105) || (line1Id == 105 && line2Id == 45) ||
                (line1Id == 49 && line2Id == 105) || (line1Id == 105 && line2Id == 49) ||
                (line1Id == 53 && line2Id == 105) || (line1Id == 105 && line2Id == 53) ||
                (line1Id == 57 && line2Id == 105) || (line1Id == 105 && line2Id == 57) ||
                (line1Id == 61 && line2Id == 105) || (line1Id == 105 && line2Id == 61) ||
                (line1Id == 65 && line2Id == 105) || (line1Id == 105 && line2Id == 65) ||
                (line1Id == 69 && line2Id == 105) || (line1Id == 105 && line2Id == 69) ||
                (line1Id == 93 && line2Id == 105) || (line1Id == 105 && line2Id == 93) ||
                (line1Id == 105 && line2Id == 137) || (line1Id == 137 && line2Id == 105)) {
              isTrackedPair = true;
            }
          }
          
          if (isTrackedPair && qtFilteredFile2 != NULL) {
            fprintf(qtFilteredFile2, "Frame %d: Filtered pair (line1 id=%u at idx=%u, line2 id=%u at idx=%u) - REASON: line1->id >= line2->id (wrong ID order)\n",
                    frameNumber, line1Id, i, line2Id, line2ArrayIndex);
            fflush(qtFilteredFile2);
          }
          #endif
          continue;
        }
        
        // Check if we've already added this pair globally
        // (line2 might appear in multiple cells, or we might process lines in different order)
        // Use min/max IDs to access upper triangular matrix
        unsigned int minId = line1->id;
        unsigned int maxId = line2->id;
        if (maxId > maxLineId) {
          // This shouldn't happen, but handle gracefully
          continue;
        }
        if (seenPairs[minId][maxId]) {
          // DEBUG: This should never happen if our logic is correct
          #ifdef DEBUG_QUADTREE
          fprintf(stderr, "WARNING: Duplicate pair detected! (%u, %u) - line1 id=%u, line2 id=%u, cellIdx=%d\n", 
                  minId, maxId, line1->id, line2->id, cellIdx);
          #endif
          continue;  // Already added this pair
        }
        
        // Mark as seen BEFORE adding to list (critical for correctness)
        seenPairs[minId][maxId] = true;
        
        #ifdef DEBUG_QUADTREE
        // Verify the matrix was actually set
        if (!seenPairs[minId][maxId]) {
          fprintf(stderr, "ERROR: Matrix not set correctly! (%u, %u)\n", minId, maxId);
        }
        #endif
        
        // Grow candidate list if needed
        if (candidateList->count >= candidateList->capacity) {
          unsigned int newCapacity = candidateList->capacity * 2;
          if (newCapacity == 0) {
            newCapacity = DEFAULT_CANDIDATE_CAPACITY;
          }
          QuadTreeCandidatePair* newPairs = realloc(
              candidateList->pairs,
              newCapacity * sizeof(QuadTreeCandidatePair));
          if (newPairs == NULL) {
            // Clean up pair tracking matrix
            for (unsigned int i = 0; i <= maxLineId; i++) {
              free(seenPairs[i]);
            }
            free(seenPairs);
            free(overlappingCells);
            return QUADTREE_ERROR_MALLOC_FAILED;
          }
          candidateList->pairs = newPairs;
          candidateList->capacity = newCapacity;
        }
        
        // Add candidate pair (line1.id < line2.id is guaranteed by check above)
        candidateList->pairs[candidateList->count].line1 = line1;
        candidateList->pairs[candidateList->count].line2 = line2;
        candidateList->count++;
        totalPairsFound++;
      }
    }
  }
  
  // Update statistics
  updateStatsOnQuery(tree, totalCellsChecked, totalPairsFound);
  
  // Free pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(overlappingCells);
  return QUADTREE_SUCCESS;
}

// ============================================================================
// Candidate List Functions
// ============================================================================

QuadTreeError QuadTreeCandidateList_init(QuadTreeCandidateList* list,
                                        unsigned int initialCapacity) {
  if (list == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  unsigned int capacity = (initialCapacity > 0) ? initialCapacity
                                                 : DEFAULT_CANDIDATE_CAPACITY;
  
  list->pairs = malloc(capacity * sizeof(QuadTreeCandidatePair));
  if (list->pairs == NULL) {
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  
  list->count = 0;
  list->capacity = capacity;
  
  return QUADTREE_SUCCESS;
}

void QuadTreeCandidateList_destroy(QuadTreeCandidateList* list) {
  if (list == NULL) {
    return;
  }
  
  if (list->pairs != NULL) {
    free(list->pairs);
    list->pairs = NULL;
  }
  
  list->count = 0;
  list->capacity = 0;
}

// ============================================================================
// Utility Functions
// ============================================================================

QuadTreeError QuadTree_getDebugStats(const QuadTree* tree,
                                     QuadTreeDebugStats* stats) {
  if (tree == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (stats == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (tree->stats == NULL) {
    return QUADTREE_ERROR_INVALID_CONFIG;  // Stats not enabled
  }
  
  *stats = *(tree->stats);
  return QUADTREE_SUCCESS;
}

void QuadTree_printDebugStats(const QuadTree* tree) {
  if (tree == NULL || tree->stats == NULL) {
    printf("Debug statistics not available.\n");
    return;
  }
  
  QuadTreeDebugStats* stats = tree->stats;
  
  printf("===== QuadTree Debug Statistics =====\n");
  printf("Tree Structure:\n");
  printf("  Total nodes: %u\n", stats->totalNodes);
  printf("  Leaf nodes: %u\n", stats->totalLeaves);
  printf("  Max depth reached: %u\n", stats->maxDepthReached);
  printf("  Max lines in node: %u\n", stats->maxLinesInNode);
  printf("\n");
  printf("Query Performance:\n");
  printf("  Total queries: %u\n", stats->totalQueries);
  printf("  Total cells checked: %u\n", stats->totalCellsChecked);
  printf("  Total pairs tested: %u\n", stats->totalPairsTested);
  printf("\n");
  printf("Optimization Hints:\n");
  printf("  Lines in multiple cells: %u\n", stats->linesInMultipleCells);
  printf("  Empty cells: %u\n", stats->emptyCells);
  printf("=====================================\n");
}

QuadTreeError QuadTree_resetDebugStats(QuadTree* tree) {
  if (tree == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (tree->stats == NULL) {
    return QUADTREE_ERROR_INVALID_CONFIG;  // Stats not enabled
  }
  
  memset(tree->stats, 0, sizeof(QuadTreeDebugStats));
  return QUADTREE_SUCCESS;
}

QuadTreeError QuadTree_isEmpty(const QuadTree* tree, bool* isEmpty) {
  if (tree == NULL || isEmpty == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  if (tree->root == NULL) {
    *isEmpty = true;
    return QUADTREE_SUCCESS;
  }
  
  *isEmpty = (tree->root->numLines == 0 && tree->root->isLeaf);
  return QUADTREE_SUCCESS;
}

/**
 * Recursively count lines in a node and its children.
 * Note: This counts duplicates if lines span multiple cells.
 */
static unsigned int countLinesRecursive(QuadNode* node) {
  if (node == NULL) {
    return 0;
  }
  
  unsigned int count = 0;
  
  if (node->isLeaf) {
    count = node->numLines;
  } else {
    // Count in children
    for (int i = 0; i < 4; i++) {
      count += countLinesRecursive(node->children[i]);
    }
  }
  
  return count;
}

QuadTreeError QuadTree_getLineCount(const QuadTree* tree, unsigned int* count) {
  if (tree == NULL || count == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
  }
  
  // Use stored line count if available (more accurate - no duplicates)
  if (tree->lines != NULL) {
    *count = tree->numLines;
    return QUADTREE_SUCCESS;
  }
  
  // Otherwise, recursively count from tree structure
  // Note: This counts duplicates if lines span multiple cells
  *count = countLinesRecursive(tree->root);
  return QUADTREE_SUCCESS;
}

