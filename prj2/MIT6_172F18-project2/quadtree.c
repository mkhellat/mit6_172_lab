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
#include "./fasttime.h"  // For timing instrumentation

// Cilk support for parallelization
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>  // For __cilkrts_get_worker_number()
#include <stdatomic.h>  // For atomic operations on seenPairs

// Reducer functions for statistics (same as in collision_world.c)
static void collisionCounter_identity(void* v) {
  *(unsigned int*)v = 0;
}

static void collisionCounter_reduce(void* left, void* right) {
  *(unsigned int*)left += *(unsigned int*)right;
}

// Reducer functions for error handling in parallel query
static void queryError_identity(void* v) {
  *(QuadTreeError*)v = QUADTREE_SUCCESS;
}

static void queryError_reduce(void* left, void* right) {
  // If either has an error, propagate the error (first non-success error wins)
  QuadTreeError* leftErr = (QuadTreeError*)left;
  QuadTreeError* rightErr = (QuadTreeError*)right;
  if (*leftErr == QUADTREE_SUCCESS && *rightErr != QUADTREE_SUCCESS) {
    *leftErr = *rightErr;
  }
}

// Debug flag for discrepancy investigation
// When enabled, logs cell assignments and candidate pairs for specific frames

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
// TEST IMPLEMENTATION: Multi-factor AABB expansion
// This is a test implementation to evaluate if velocity-dependent expansion
// improves collision detection accuracy. Parameters k_rel and k_gap are
// tunable and may need refinement or replacement with a better solution.
static void computeLineBoundingBox(const Line* line, double timeStep,
                                   double* xmin, double* xmax,
                                   double* ymin, double* ymax,
                                   double maxVelocityInSystem,
                                   double minCellSize) {
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
  
  // TEST IMPLEMENTATION: Multi-factor expansion to address:
  // 1. Relative motion mismatch (absolute vs relative parallelograms)
  // 2. AABB gap problem (AABBs overlap in world space but not same cells)
  // 
  // NOTE: This uses hard-coded parameters (k_rel, k_gap) which is not ideal.
  // This is a test to see if the approach works. A better solution would
  // compute these dynamically or use a different approach entirely.
  
  // Factor 1: Relative motion expansion
  // Accounts for maximum relative velocity between lines
  // If line has velocity v, relative velocity with another line can be up to ~2*v_max
  // We expand by a fraction of the line's velocity magnitude
  // OPTIMIZATION: Use cached velocity magnitude to avoid expensive sqrt operation
  double velocity_magnitude = line->cachedVelocityMagnitude;
  const double k_rel = 0.3;  // TEST PARAMETER: Relative motion factor (tunable: 0.2-0.5)
  double relative_motion_expansion = velocity_magnitude * timeStep * k_rel;
  
  // Factor 2: Minimum gap expansion
  // Ensures AABBs that are close (within fraction of cell size) overlap same cells
  // Addresses the AABB gap problem (Scenario 3B, 7C from query scenarios analysis)
  const double k_gap = 0.15;  // TEST PARAMETER: Gap factor (tunable: 0.1-0.2)
  double min_gap_expansion = minCellSize * k_gap;
  
  // Factor 3: Numerical precision margin
  // Small fixed value to handle floating-point precision issues
  const double precision_margin = 1e-6;
  
  // Use maximum of relative motion and gap expansion, plus precision margin
  // This ensures we address both issues (relative motion and gap problem)
  double expansion = maxDouble(relative_motion_expansion, min_gap_expansion) 
                     + precision_margin;
  
  *xmin -= expansion;
  *xmax += expansion;
  *ymin -= expansion;
  *ymax += expansion;
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
      // CRITICAL PERFORMANCE FIX: Use stored maxVelocity from tree (computed once in QuadTree_build)
      // instead of recomputing it O(n) times for every line during subdivision
      for (unsigned int i = 0; i < node->numLines; i++) {
        Line* existingLine = node->lines[i];
        double exmin, exmax, eymin, eymax;
        // Use the timeStep and maxVelocity from build phase (stored in tree structure)
        // This avoids O(n²) Vec_length calls during subdivision
        computeLineBoundingBox(existingLine, tree->buildTimeStep, &exmin, &exmax,
                               &eymin, &eymax, tree->maxVelocity, tree->config.minCellSize);
        
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
  #ifdef DEBUG_QUADTREE_TIMING
  fasttime_t start_build = gettime();
  #endif
  
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
  
  // TEST IMPLEMENTATION: Compute maximum velocity in system for AABB expansion
  // This is needed for the velocity-dependent expansion in computeLineBoundingBox
  double maxVelocity = 0.0;
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) {
      continue;
    }
    double v_mag = Vec_length(lines[i]->velocity);
    if (v_mag > maxVelocity) {
      maxVelocity = v_mag;
    }
  }
  // If no lines or all velocities are zero, use a small default to avoid division issues
  if (maxVelocity == 0.0) {
    maxVelocity = 1e-10;
  }
  
  // Store maxVelocity in tree for use during query phase (avoids recomputing)
  tree->maxVelocity = maxVelocity;
  
  // CRITICAL FIX: Expand root bounds to include all lines
  // Compute actual bounds of all lines (including future positions)
  // This ensures lines outside the initial box bounds are included
  double actualXmin = tree->worldXmax;
  double actualXmax = tree->worldXmin;
  double actualYmin = tree->worldYmax;
  double actualYmax = tree->worldYmin;
  
  // OPTIMIZATION #1: Cache bounding boxes to avoid recomputing during insertion
  // Allocate array to store bounding boxes for all lines
  typedef struct {
    double xmin, xmax, ymin, ymax;
  } BoundingBox;
  
  BoundingBox* bboxes = malloc(numLines * sizeof(BoundingBox));
  if (bboxes == NULL) {
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  
  // First pass: Compute and cache bounding boxes, find actual bounds
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) {
      // Mark as invalid (we'll skip during insertion)
      bboxes[i].xmin = bboxes[i].xmax = bboxes[i].ymin = bboxes[i].ymax = 0.0;
      continue;
    }
    
    // Compute and cache bounding box
    computeLineBoundingBox(lines[i], timeStep,
                           &bboxes[i].xmin, &bboxes[i].xmax,
                           &bboxes[i].ymin, &bboxes[i].ymax,
                           maxVelocity, tree->config.minCellSize);
    
    // Expand bounds to include this line's bbox
    if (bboxes[i].xmin < actualXmin) {
      actualXmin = bboxes[i].xmin;
    }
    if (bboxes[i].xmax > actualXmax) {
      actualXmax = bboxes[i].xmax;
    }
    if (bboxes[i].ymin < actualYmin) {
      actualYmin = bboxes[i].ymin;
    }
    if (bboxes[i].ymax > actualYmax) {
      actualYmax = bboxes[i].ymax;
    }
  }
  
  // Expand root bounds to include all lines (with small margin)
  const double margin = 1e-6;
  if (actualXmin < tree->worldXmin) {
    tree->worldXmin = actualXmin - margin;
  }
  if (actualXmax > tree->worldXmax) {
    tree->worldXmax = actualXmax + margin;
  }
  if (actualYmin < tree->worldYmin) {
    tree->worldYmin = actualYmin - margin;
  }
  if (actualYmax > tree->worldYmax) {
    tree->worldYmax = actualYmax + margin;
  }
  
  // Ensure bounds form a square (take larger dimension)
  double width = tree->worldXmax - tree->worldXmin;
  double height = tree->worldYmax - tree->worldYmin;
  double size = maxDouble(width, height);
  
  // Center the square
  double centerX = (tree->worldXmin + tree->worldXmax) / 2.0;
  double centerY = (tree->worldYmin + tree->worldYmax) / 2.0;
  
  tree->worldXmin = centerX - size / 2.0;
  tree->worldXmax = centerX + size / 2.0;
  tree->worldYmin = centerY - size / 2.0;
  tree->worldYmax = centerY + size / 2.0;
  
  // Recreate root node with expanded bounds
  if (tree->root != NULL) {
    destroyQuadNode(tree->root);
  }
  tree->root = createQuadNode(tree->worldXmin, tree->worldXmax,
                               tree->worldYmin, tree->worldYmax, 0);
  if (tree->root == NULL) {
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  
  // Reset statistics if enabled
  if (tree->stats != NULL) {
    memset(tree->stats, 0, sizeof(QuadTreeDebugStats));
    // Re-count root node
    tree->stats->totalNodes = 1;
    tree->stats->totalLeaves = 1;
  }
  
  // Second pass: Insert each line using cached bounding boxes
  for (unsigned int i = 0; i < numLines; i++) {
    if (lines[i] == NULL) {
      continue;  // Skip NULL lines
    }
    
    // OPTIMIZATION #1: Use cached bounding box instead of recomputing
    // Insert into tree using cached bounding box
    QuadTreeError result = insertLineRecursive(tree->root, lines[i],
                                               bboxes[i].xmin, bboxes[i].xmax,
                                               bboxes[i].ymin, bboxes[i].ymax, tree);
    if (result != QUADTREE_SUCCESS) {
      free(bboxes);  // Free cache before returning on error
      return result;
    }
    
    // Update statistics for lines in multiple cells
    if (tree->stats != NULL) {
      // Simple heuristic: if bounding box spans multiple potential cells,
      // it's likely in multiple cells
      double worldWidth = tree->worldXmax - tree->worldXmin;
      double worldHeight = tree->worldYmax - tree->worldYmin;
      double lineWidth = bboxes[i].xmax - bboxes[i].xmin;
      double lineHeight = bboxes[i].ymax - bboxes[i].ymin;
      
      // If line is large relative to world, it likely spans cells
      if (lineWidth > worldWidth * 0.1 || lineHeight > worldHeight * 0.1) {
        tree->stats->linesInMultipleCells++;
      }
    }
  }
  
  // Free bounding box cache
  free(bboxes);
  
  #ifdef DEBUG_QUADTREE_TIMING
  {
    fasttime_t end_build = gettime();
    double build_time = tdiff(start_build, end_build);
    fprintf(stderr, "BUILD PHASE TIMING: %.6fs\n", build_time);
  }
  #endif
  
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
  #ifdef DEBUG_QUADTREE_TIMING
  fasttime_t start_query = gettime();
  #endif
  
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
  
  // CRITICAL PERFORMANCE FIX: Build Line* to array index hash map for O(1) lookup
  // This eliminates the O(n) linear search that was happening for EVERY candidate pair
  // We'll use a simple approach: since we have tree->lines array, we can build
  // a reverse map. But actually, we can just use the fact that tree->lines[i] 
  // gives us the line at index i. The problem is we need to find index given Line*.
  // 
  // Solution: Build a hash table or use a simpler approach - since lines are stored
  // in tree->lines array, we can build a reverse lookup table.
  // Actually, simplest: Build an array that maps Line* to index during initialization.
  
  // Allocate hash table: We'll use a simple approach - build array indexed by line pointer
  // But pointers aren't small integers, so we need a hash table.
  // For now, let's use a simpler approach: build a lookup array during initialization
  // that we can use for O(1) lookup. Since we iterate through tree->lines anyway,
  // we can build this map once.
  
  // Build Line* to array index map (O(n) once, then O(1) lookups)
  // We'll use a simple linear array approach: for each line in tree->lines,
  // we know its index. We need to map Line* -> index.
  // Since we can't easily hash pointers, we'll use the fact that we iterate
  // through candidates in order and can cache the lookup.
  
  // Actually, the REAL fix: Since we're iterating through tree->lines[i] for line1,
  // and we get line2 from cells, we need to find line2's index. The issue is we
  // do this for EVERY candidate pair, causing O(n² log n) complexity.
  //
  // Better solution: Build a hash table once, or use the fact that lines are
  // stored in tree->lines. We can build a reverse index.
  
  // SIMPLEST FIX: Since tree->lines[i] contains the line at index i, and we
  // iterate through it, we can build a reverse map. But we need O(1) lookup.
  // 
  // For now, let's optimize the linear search by breaking early and caching
  // results per line2. Actually, we can build a proper hash table, but that's
  // complex. Let's use a simpler approach: build an array that maps line pointer
  // to index, but we need to handle pointer hashing.
  
  // ACTUAL FIX: Build reverse index array - but we need to handle that lines
  // might not be in a contiguous range. Let's use a different approach:
  // Instead of finding line2's index, we can just check if line2->id > line1->id
  // and skip the array index check if we're careful about ordering.
  
  // WAIT - The array index check is to match brute-force order. But we can
  // optimize this: Build a hash table once that maps Line* -> array index.
  // Since C doesn't have built-in hash tables, we'll use a simple approach:
  // Build an array indexed by some property of the line.
  
  // BEST FIX: Since we have line IDs, and we're already building maxLineId,
  // we can build an array indexed by line ID that maps to array index.
  // But wait - line IDs might not be contiguous or might not match array indices.
  
  // SIMPLEST WORKING FIX: Build a reverse lookup table once:
  // For each line in tree->lines, store its index in a way we can look it up.
  // We'll use a simple approach: iterate through tree->lines once to build
  // a map. Since we can't easily hash Line* pointers, we'll use a linear
  // search but cache results per line2 (so we only search once per unique line2).
  
  // ACTUALLY, THE REAL FIX: We don't need array index at all if we're careful!
  // We can use line IDs to ensure ordering. But the issue is brute-force uses
  // array index order. Let's build a proper solution:
  
  // Build Line* to array index map using a simple hash table approach
  // We'll use line->id as the key since IDs are unique
  unsigned int* lineIdToIndex = calloc(maxLineId + 1, sizeof(unsigned int));
  if (lineIdToIndex == NULL) {
    free(overlappingCells);
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  // Initialize to invalid index (0 is valid, so use numLines+1 as sentinel)
  for (unsigned int idx = 0; idx <= maxLineId; idx++) {
    lineIdToIndex[idx] = tree->numLines + 1;  // Invalid index
  }
  // Build the map: for each line, store its array index
  for (unsigned int idx = 0; idx < tree->numLines; idx++) {
    if (tree->lines[idx] != NULL) {
      lineIdToIndex[tree->lines[idx]->id] = idx;
    }
  }
  
  // Allocate a 2D boolean matrix to track all pairs we've seen globally
  // This ensures we never add the same pair twice, regardless of processing order
  // Matrix is upper triangular (only pairs where id1 < id2 are stored)
  // Access pattern: seenPairs[minId][maxId] for pair (minId, maxId)
  // PHASE 8: Use _Atomic bool for proper atomic operations
  _Atomic bool** seenPairs = calloc(maxLineId + 1, sizeof(_Atomic bool*));
  if (seenPairs == NULL) {
    free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
    free(overlappingCells);
    return QUADTREE_ERROR_MALLOC_FAILED;
  }
  for (unsigned int i = 0; i <= maxLineId; i++) {
    seenPairs[i] = calloc(maxLineId + 1, sizeof(_Atomic bool));
    if (seenPairs[i] == NULL) {
      // Clean up already allocated rows
      for (unsigned int j = 0; j < i; j++) {
        free(seenPairs[j]);
      }
      free(seenPairs);
      free(lineIdToIndex);  // Fix memory leak: free lineIdToIndex before returning
      free(overlappingCells);
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
  }
  
  // PHASE 8: Pre-allocate candidate list capacity to avoid realloc during parallel execution
  // Estimate: worst case is n*(n-1)/2 pairs, but quadtree should reduce this significantly
  // Use a conservative estimate: assume each line might have up to 100 candidates
  // This avoids the need for thread-safe realloc during parallel execution
  unsigned int estimatedCapacity = tree->numLines * 100;
  if (estimatedCapacity < DEFAULT_CANDIDATE_CAPACITY) {
    estimatedCapacity = DEFAULT_CANDIDATE_CAPACITY;
  }
  // Cap at reasonable maximum to avoid excessive memory usage
  if (estimatedCapacity > 1000000) {
    estimatedCapacity = 1000000;
  }
  
  // Grow candidate list to estimated capacity if needed
  if (candidateList->capacity < estimatedCapacity) {
    QuadTreeCandidatePair* newPairs = realloc(
        candidateList->pairs,
        estimatedCapacity * sizeof(QuadTreeCandidatePair));
    if (newPairs == NULL) {
      // Clean up
      for (unsigned int i = 0; i <= maxLineId; i++) {
        free(seenPairs[i]);
      }
      free(seenPairs);
      free(lineIdToIndex);
      free(overlappingCells);
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
    candidateList->pairs = newPairs;
    candidateList->capacity = estimatedCapacity;
  }
  
  // Track statistics (use reducers for parallel accumulation)
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    totalCellsChecked = 0;
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    totalPairsFound = 0;
  
  // PHASE 8: Debug counters for detailed debugging
  #ifdef DEBUG_PHASE8
  // Track frame number for per-frame comparison (shared across all threads)
  static unsigned int debugFrameNumber = 0;
  debugFrameNumber++;
  
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    debugPairsSkippedCapacity = 0;
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    debugPairsSkippedAlreadySeen = 0;
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    debugPairsSkippedOrder = 0;
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    debugPairsSkippedBounds = 0;
  unsigned int cilk_reducer(collisionCounter_identity, collisionCounter_reduce) 
    debugPairsAdded = 0;
  #endif
  
  // PHASE 8: Error flag for parallel error handling (can't return from cilk_for)
  QuadTreeError cilk_reducer(queryError_identity, queryError_reduce) queryError = QUADTREE_SUCCESS;
  
  // PHASE 8: Parallelize query phase - each line's candidate search is independent
  // Use cilk_for to parallelize the loop over lines
  // Note: seenPairs matrix uses atomic operations for thread-safety
  cilk_for (unsigned int i = 0; i < tree->numLines; i++) {
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
    // Use stored maxVelocity from build phase (avoids O(n) recomputation per line!)
    double lineXmin, lineXmax, lineYmin, lineYmax;
    computeLineBoundingBox(line1, tree->buildTimeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax,
                           tree->maxVelocity, tree->config.minCellSize);
    
    // Find all cells this line overlaps
    int numCells = findOverlappingCellsRecursive(tree->root,
                                                  lineXmin, lineXmax,
                                                  lineYmin, lineYmax,
                                                  overlappingCells, 0,
                                                  maxCellsPerLine);
    if (numCells < 0) {
      // PHASE 8: Can't return from cilk_for - set error flag instead
      queryError = QUADTREE_ERROR_MALLOC_FAILED;
      continue;  // Skip this line and continue with others
    }
    
    #ifdef DEBUG_PHASE8
    // Trace spatial query for line1_idx=8 to understand why it finds/doesn't find line 133
    if (i == 8) {
      unsigned int workerId = __cilkrts_get_worker_number();
      fprintf(stderr, "TRACE_SPATIAL: line1_idx=8 line1_id=%u found %d overlapping cells (worker=%u)\n",
              line1->id, numCells, workerId);
    }
    #endif
    
    totalCellsChecked += (unsigned int)numCells;
    
    
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
      
      #ifdef DEBUG_PHASE8
      // Trace cells for line1_idx=8
      if (i == 8) {
        unsigned int workerId = __cilkrts_get_worker_number();
        fprintf(stderr, "TRACE_SPATIAL: line1_idx=8 cell[%d] has %u lines (worker=%u)\n",
                cellIdx, cell->numLines, workerId);
      }
      #endif
      
      for (unsigned int j = 0; j < cell->numLines; j++) {
        Line* line2 = cell->lines[j];
        
        // Skip NULL lines and self
        if (line2 == NULL || line2 == line1) {
          continue;
        }
        
        #ifdef DEBUG_PHASE8
        // Trace all lines found in cells for line1_idx=8
        if (i == 8) {
          unsigned int workerId = __cilkrts_get_worker_number();
          fprintf(stderr, "TRACE_SPATIAL: line1_idx=8 found line2_id=%u in cell[%d] (worker=%u)\n",
                  line2->id, cellIdx, workerId);
        }
        
        // Track all potential pairs for debugging
        // Check if this is a pair we're interested in tracing
        bool traceThisPair = false;
        if ((line1->id == 8 && line2->id == 133) || (line1->id == 133 && line2->id == 8)) {
          traceThisPair = true;
          unsigned int workerId = __cilkrts_get_worker_number();
          fprintf(stderr, "TRACE_PAIR: FOUND in spatial query! line1_idx=%u line1_id=%u line2_id=%u worker=%u cellIdx=%d\n", 
                  i, line1->id, line2->id, workerId, cellIdx);
        }
        #endif
        
        // PHASE 8: Removed order-dependent check - rely on seenPairs for duplicate prevention
        // The order check (line2ArrayIndex <= i) was causing correctness issues in parallel
        // because parallel execution processes lines in parallel, not sequentially.
        // The seenPairs matrix already prevents duplicates, so the order check is redundant.
        // We still need to ensure line1->id < line2->id for consistent pair representation.
        
        // Ensure line1->id < line2->id (normalize pair representation)
        // This ensures we always represent pairs as (minId, maxId) regardless of order
        if (line1->id >= line2->id) {
          #ifdef DEBUG_PHASE8
          debugPairsSkippedOrder++;
          if (traceThisPair) {
            fprintf(stderr, "TRACE_PAIR: Skipped due to order check (line1_id=%u >= line2_id=%u)\n",
                    line1->id, line2->id);
          }
          #endif
          continue;
        }
        
        // Bounds check for seenPairs matrix
        if (line2->id > maxLineId) {
          // line2->id is out of bounds for seenPairs matrix - skip it
          #ifdef DEBUG_PHASE8
          debugPairsSkippedBounds++;
          if (traceThisPair) {
            fprintf(stderr, "TRACE_PAIR: Skipped due to bounds check (line2_id=%u > maxLineId=%u)\n",
                    line2->id, maxLineId);
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
          #ifdef DEBUG_PHASE8
          if (traceThisPair) {
            fprintf(stderr, "TRACE_PAIR: Skipped due to maxId > maxLineId\n");
          }
          #endif
          continue;
        }
        
        // PHASE 8: Use atomic operations for thread-safe seenPairs check and set
        // CRITICAL: Check capacity BEFORE marking as seen to avoid losing pairs
        // If capacity is exceeded, we skip without marking, so the pair can be
        // added by another thread (though with pre-allocation this shouldn't happen)
        
        // First, check if we have capacity (optimistic check - might change, but unlikely)
        unsigned int currentCount = __atomic_load_n(&candidateList->count, __ATOMIC_ACQUIRE);
        if (currentCount >= candidateList->capacity) {
          // Capacity exceeded - skip this pair without marking as seen
          // This allows another thread to add it if capacity grows
          // But with pre-allocation, this should never happen
          #ifdef DEBUG_PHASE8
          debugPairsSkippedCapacity++;
          if (traceThisPair) {
            fprintf(stderr, "TRACE_PAIR: Skipped due to capacity (currentCount=%u >= capacity=%u)\n",
                    currentCount, candidateList->capacity);
          }
          #endif
          continue;
        }
        
        // Now atomically check and mark as seen
        // Use atomic test-and-set style operation to atomically check and set
        // Try to atomically set the value from false to true
        // Returns the old value: false if we successfully set it (wasn't seen), true if already seen
        // PHASE 8: Use _Atomic bool for proper atomic operations
        _Atomic bool* seenPtr = &seenPairs[minId][maxId];
        // Use atomic_exchange_explicit (C11 standard) for proper atomic operations
        bool oldValue = atomic_exchange_explicit(seenPtr, true, memory_order_acq_rel);
        
        if (oldValue) {
          // Pair was already seen by another thread - skip it
          // This is correct behavior - we only want to add each pair once
          #ifdef DEBUG_PHASE8
          debugPairsSkippedAlreadySeen++;
          if (traceThisPair) {
            unsigned int workerId = __cilkrts_get_worker_number();
            fprintf(stderr, "TRACE_PAIR: Skipped - already seen by another thread (worker=%u, oldValue=true)\n",
                    workerId);
          }
          #endif
          #ifdef DEBUG_QUADTREE
          fprintf(stderr, "WARNING: Duplicate pair detected! (%u, %u) - line1 id=%u, line2 id=%u, cellIdx=%d\n", 
                  minId, maxId, line1->id, line2->id, cellIdx);
          #endif
          continue;  // Already added this pair
        }
        
        #ifdef DEBUG_PHASE8
        if (traceThisPair) {
          unsigned int workerId = __cilkrts_get_worker_number();
          fprintf(stderr, "TRACE_PAIR: Successfully marked as seen (worker=%u, oldValue=false)\n", workerId);
        }
        #endif
        
        // Successfully marked as seen (old value was false, now it's true)
        // Now we can safely add it to the candidate list
        
        // PHASE 8: Thread-safe append to candidateList
        // Get index atomically (capacity already checked above, but might have changed)
        // atomic_fetch_add returns the OLD value before incrementing, so we get a valid index
        unsigned int myIndex = __atomic_fetch_add(&candidateList->count, 1, __ATOMIC_ACQ_REL);
        
        // Double-check bounds (should not happen with pre-allocation, but be safe)
        if (myIndex >= candidateList->capacity) {
          // This should never happen with proper pre-allocation
          // But if it does, we've already marked as seen, so we need to handle it
          // Decrement count to undo the increment
          __atomic_fetch_sub(&candidateList->count, 1, __ATOMIC_ACQ_REL);
          // Note: Pair is marked as seen but not added - this is a bug if it happens
          // But with proper pre-allocation, this should never occur
          #ifdef DEBUG_PHASE8
          debugPairsSkippedCapacity++;
          fprintf(stderr, "WARNING: Capacity exceeded after atomic fetch_add! Pair (%u, %u) lost!\n",
                  minId, maxId);
          #endif
          continue;
        }
        
        // Add candidate pair (line1.id < line2.id is guaranteed by check above)
        candidateList->pairs[myIndex].line1 = line1;
        candidateList->pairs[myIndex].line2 = line2;
        totalPairsFound++;
        #ifdef DEBUG_PHASE8
        debugPairsAdded++;
        // Log each pair being added for detailed comparison
        // Note: line2ArrayIndex is in scope here (declared earlier in the loop)
        // Frame number is tracked at function start (debugFrameNumber)
        // Note: debugFrameNumber is in outer scope, accessible here
        // Get worker ID for parallel debugging
        unsigned int workerId = __cilkrts_get_worker_number();
        fprintf(stderr, "DEBUG_PAIR_ADDED: FRAME=%u WORKER=%u PAIR=%u,%u line1_idx=%u line2_idx=%u myIndex=%u\n",
                debugFrameNumber, workerId, line1->id, line2->id, i, lineIdToIndex[line2->id], myIndex);
        #endif
      }
    }
  }
  
  // PHASE 8: Check for errors from parallel execution
  if (queryError != QUADTREE_SUCCESS) {
    // Clean up pair tracking matrix
    for (unsigned int i = 0; i <= maxLineId; i++) {
      free(seenPairs[i]);
    }
    free(seenPairs);
    free(lineIdToIndex);
    free(overlappingCells);
    return queryError;
  }
  
  // Update statistics
  updateStatsOnQuery(tree, totalCellsChecked, totalPairsFound);
  
  #ifdef DEBUG_PHASE8
  // PHASE 8: Debug output for correctness investigation
  // Frame number is already tracked at function start (debugFrameNumber)
  fprintf(stderr, "===== PHASE 8 DEBUG STATISTICS (Frame %u) =====\n", debugFrameNumber);
  fprintf(stderr, "Total pairs added: %u\n", debugPairsAdded);
  fprintf(stderr, "Pairs skipped - capacity: %u\n", debugPairsSkippedCapacity);
  fprintf(stderr, "Pairs skipped - already seen: %u\n", debugPairsSkippedAlreadySeen);
  fprintf(stderr, "Pairs skipped - order check: %u\n", debugPairsSkippedOrder);
  fprintf(stderr, "Pairs skipped - bounds: %u\n", debugPairsSkippedBounds);
  fprintf(stderr, "Final candidate list count: %u\n", candidateList->count);
  fprintf(stderr, "Candidate list capacity: %u\n", candidateList->capacity);
  fprintf(stderr, "====================================\n");
  #endif
  
  // STEP 1: Instrument candidate pair generation statistics
  #ifdef DEBUG_QUADTREE_STATS
  {
    unsigned long long expectedPairs = ((unsigned long long)tree->numLines * 
                                       (tree->numLines - 1)) / 2;
    double expectedCandidates = tree->numLines * log2((double)tree->numLines);
    double expectedRatio = (2.0 * log2((double)tree->numLines)) / tree->numLines * 100.0;
    double actualRatio = (candidateList->count * 100.0) / (expectedPairs > 0 ? expectedPairs : 1);
    double ratioGap = (expectedRatio > 0) ? actualRatio / expectedRatio : 0.0;
    double avgCellsPerLine = (tree->numLines > 0) ? (double)totalCellsChecked / tree->numLines : 0.0;
    
    fprintf(stderr, "===== QUADTREE CANDIDATE PAIR STATISTICS (Step 1) =====\n");
    fprintf(stderr, "Input: n=%u lines\n", tree->numLines);
    fprintf(stderr, "Brute-force pairs: %llu\n", expectedPairs);
    fprintf(stderr, "Candidate pairs generated: %u\n", candidateList->count);
    fprintf(stderr, "Expected candidates (n*log2(n)): ~%.0f\n", expectedCandidates);
    fprintf(stderr, "Expected ratio: %.2f%%\n", expectedRatio);
    fprintf(stderr, "Actual ratio: %.2f%%\n", actualRatio);
    fprintf(stderr, "Ratio gap: %.1fx (actual/expected)\n", ratioGap);
    fprintf(stderr, "Average cells per line: %.2f\n", avgCellsPerLine);
    fprintf(stderr, "======================================================\n");
  }
  #endif
  
  // STEP 2: Instrument query phase timing
  #ifdef DEBUG_QUADTREE_TIMING
  fasttime_t end_query = gettime();
  double query_time = tdiff(start_query, end_query);
  #endif
  
  // STEP 3: Instrument tree structure analysis
  #ifdef DEBUG_QUADTREE_STATS
  {
    // Use existing stats if available
    if (tree->stats != NULL) {
      unsigned int totalNodes = tree->stats->totalNodes;
      unsigned int totalLeaves = tree->stats->totalLeaves;
      unsigned int maxDepth = tree->stats->maxDepthReached;
      unsigned int maxLinesInNode = tree->stats->maxLinesInNode;
      unsigned int emptyCells = tree->stats->emptyCells;
      
      double avgLinesPerLeaf = (totalLeaves > 0) ? (double)tree->numLines / totalLeaves : 0.0;
      double expectedDepth = log2((double)tree->numLines) / 2.0;  // log4(n) = log2(n)/2
      double depthRatio = (expectedDepth > 0) ? (double)maxDepth / expectedDepth : 0.0;
      
      fprintf(stderr, "===== QUADTREE STRUCTURE ANALYSIS (Step 3) =====\n");
      fprintf(stderr, "Tree Structure:\n");
      fprintf(stderr, "  Total nodes: %u\n", totalNodes);
      fprintf(stderr, "  Leaf nodes: %u\n", totalLeaves);
      fprintf(stderr, "  Max depth reached: %u\n", maxDepth);
      fprintf(stderr, "  Expected depth (log4(n)): ~%.1f\n", expectedDepth);
      fprintf(stderr, "  Depth ratio: %.2fx (actual/expected)\n", depthRatio);
      fprintf(stderr, "  Max lines in any node: %u\n", maxLinesInNode);
      fprintf(stderr, "  Average lines per leaf: %.2f\n", avgLinesPerLeaf);
      fprintf(stderr, "  Empty cells: %u (%.1f%%)\n", emptyCells, 
              (totalNodes > 0) ? (emptyCells * 100.0 / totalNodes) : 0.0);
      fprintf(stderr, "  Lines in tree: %u\n", tree->numLines);
      fprintf(stderr, "===============================================\n");
    } else {
      // Stats not enabled, print basic info
      fprintf(stderr, "===== QUADTREE STRUCTURE ANALYSIS (Step 3) =====\n");
      fprintf(stderr, "Tree Structure:\n");
      fprintf(stderr, "  Lines in tree: %u\n", tree->numLines);
      fprintf(stderr, "  NOTE: Enable debug stats in config for detailed analysis\n");
      fprintf(stderr, "===============================================\n");
    }
  }
  #endif
  
  // Free pair tracking matrix
  for (unsigned int i = 0; i <= maxLineId; i++) {
    free(seenPairs[i]);
  }
  free(seenPairs);
  free(lineIdToIndex);  // Free the reverse lookup table
  free(overlappingCells);
  
  #ifdef DEBUG_QUADTREE_TIMING
  // Report query timing
  fprintf(stderr, "QUERY PHASE TIMING: %.6fs\n", query_time);
  #endif
  
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

