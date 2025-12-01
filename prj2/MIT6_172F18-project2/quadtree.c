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
  config.maxLinesPerNode = 4;
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
    // Check if we should subdivide
    bool shouldSubdivide = false;
    
    if (node->numLines >= tree->config.maxLinesPerNode &&
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
      for (unsigned int i = 0; i < node->numLines; i++) {
        Line* existingLine = node->lines[i];
        double exmin, exmax, eymin, eymax;
        computeLineBoundingBox(existingLine, 0.0, &exmin, &exmax,
                               &eymin, &eymax);
        
        for (int j = 0; j < 4; j++) {
          insertLineRecursive(node->children[j], existingLine,
                             exmin, exmax, eymin, eymax, tree);
        }
      }
      
      // Clear parent's line list (lines now in children)
      node->numLines = 0;
    } else {
      // Just add line to this leaf node
      QuadTreeError result = addLineToNode(node, line);
      if (result != QUADTREE_SUCCESS) {
        return result;
      }
      return QUADTREE_SUCCESS;
    }
  }
  
  // If we reach here, node is not a leaf (or was just subdivided)
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
                                          QuadTreeCandidateList* candidateList) {
  if (tree == NULL) {
    return QUADTREE_ERROR_NULL_POINTER;
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
  
  // Track statistics
  unsigned int totalCellsChecked = 0;
  unsigned int totalPairsFound = 0;
  
  // For each line, find overlapping cells and collect candidate pairs
  for (unsigned int i = 0; i < tree->numLines; i++) {
    Line* line1 = tree->lines[i];
    if (line1 == NULL) {
      continue;
    }
    
    // Compute line's bounding box
    double lineXmin, lineXmax, lineYmin, lineYmax;
    computeLineBoundingBox(line1, timeStep,
                           &lineXmin, &lineXmax, &lineYmin, &lineYmax);
    
    // Find all cells this line overlaps
    int numCells = findOverlappingCellsRecursive(tree->root,
                                                  lineXmin, lineXmax,
                                                  lineYmin, lineYmax,
                                                  overlappingCells, 0,
                                                  maxCellsPerLine);
    if (numCells < 0) {
      free(overlappingCells);
      return QUADTREE_ERROR_MALLOC_FAILED;
    }
    
    totalCellsChecked += (unsigned int)numCells;
    
    // Collect all OTHER lines from these cells
    // Use a simple set to avoid duplicate pairs (line1.id < line2.id)
    for (int cellIdx = 0; cellIdx < numCells; cellIdx++) {
      QuadNode* cell = overlappingCells[cellIdx];
      
      for (unsigned int j = 0; j < cell->numLines; j++) {
        Line* line2 = cell->lines[j];
        
        // Only consider pairs where line1.id < line2.id (avoid duplicates)
        // This matches the brute-force algorithm's approach
        if (line2 == NULL || line1->id >= line2->id) {
          continue;
        }
        
        // Check if this pair is already in candidate list
        bool alreadyAdded = false;
        for (unsigned int k = 0; k < candidateList->count; k++) {
          if ((candidateList->pairs[k].line1 == line1 &&
               candidateList->pairs[k].line2 == line2) ||
              (candidateList->pairs[k].line1 == line2 &&
               candidateList->pairs[k].line2 == line1)) {
            alreadyAdded = true;
            break;
          }
        }
        
        if (!alreadyAdded) {
          // Grow candidate list if needed
          if (candidateList->count >= candidateList->capacity) {
            unsigned int newCapacity = candidateList->capacity * 2;
            QuadTreeCandidatePair* newPairs = realloc(
                candidateList->pairs,
                newCapacity * sizeof(QuadTreeCandidatePair));
            if (newPairs == NULL) {
              free(overlappingCells);
              return QUADTREE_ERROR_MALLOC_FAILED;
            }
            candidateList->pairs = newPairs;
            candidateList->capacity = newCapacity;
          }
          
          // Add candidate pair (ensure line1.id < line2.id)
          if (line1->id < line2->id) {
            candidateList->pairs[candidateList->count].line1 = line1;
            candidateList->pairs[candidateList->count].line2 = line2;
          } else {
            candidateList->pairs[candidateList->count].line1 = line2;
            candidateList->pairs[candidateList->count].line2 = line1;
          }
          candidateList->count++;
          totalPairsFound++;
        }
      }
    }
  }
  
  // Update statistics
  updateStatsOnQuery(tree, totalCellsChecked, totalPairsFound);
  
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

