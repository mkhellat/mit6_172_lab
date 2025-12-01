/**
 * quadtree.h -- Spatial quadtree for optimized collision detection
 * Copyright (c) 2012 the Massachusetts Institute of Technology
 *
 * This module implements a dynamic quadtree data structure for efficiently
 * detecting potential collisions between moving line segments. The quadtree
 * subdivides 2D space hierarchically to reduce the number of pairwise
 * collision tests from O(n²) to O(n log n) average case.
 *
 * QUADTREE ALGORITHM OVERVIEW:
 * ----------------------------
 * The quadtree divides the collision world into square cells (not rectangles).
 * Each cell is a perfect square: width == height. The algorithm works as
 * follows:
 *
 * 1. BUILD PHASE (QuadTree_build):
 *    - Start with world bounds as root cell (a square)
 *    - For each cell, if it contains >= maxLinesPerNode lines:
 *      * Subdivide into 4 equal-sized square sub-cells (NW, NE, SW, SE)
 *      * Recursively distribute lines to child cells
 *    - Continue until maxDepth reached or cells are too small
 *    - Result: Hierarchical spatial index of lines
 *
 * 2. QUERY PHASE (QuadTree_findCandidatePairs):
 *    - For each line, find all cells it overlaps (spatial filtering)
 *    - Collect all OTHER lines from those cells
 *    - These are "candidate pairs" - lines that are spatially close
 *    - Result: List of candidate pairs (much fewer than n²)
 *
 * 3. TEST PHASE (external, uses existing intersect() function):
 *    - For each candidate pair, call intersect(line1, line2, timeStep)
 *    - This uses the existing collision detection logic
 *    - Result: Actual collision events
 *
 * DESIGN DECISIONS:
 * -----------------
 * 
 * Standalone Module (not embedded in CollisionWorld):
 *   The QuadTree is a standalone module that CollisionWorld uses as a tool.
 *   This design choice offers:
 *   
 *   Benefits:
 *   - Clear separation of concerns (spatial indexing vs collision world)
 *   - Modularity: Quadtree can be reused for other spatial queries
 *   - Testability: Can test quadtree independently
 *   - Readability: Clear ownership (CollisionWorld creates/destroys tree)
 *   
 *   Performance Considerations:
 *   - Memory overhead: Tree allocated each frame (~O(n) nodes)
 *   - Allocation cost: malloc/free per frame (minimal for typical n)
 *   - Cache effects: Tree structure may have cache misses (acceptable)
 *   
 *   Alternative (Embedded in CollisionWorld):
 *   - Would require complex update/destroy logic when switching algorithms
 *   - Tighter coupling between modules
 *   - Harder to test quadtree in isolation
 *   - Performance difference is negligible (tree is temporary anyway)
 *   
 *   Conclusion: Standalone design preferred for code quality. Performance
 *   difference is minimal since tree is rebuilt each frame regardless.
 *
 * Separation of Query and Test Phases:
 *   The quadtree provides candidate pairs (spatial filtering), while the
 *   actual collision testing uses the existing intersect() function. This
 *   separation offers:
 *   
 *   Benefits:
 *   - Clear separation: Spatial filtering vs collision logic
 *   - Reusability: Can test candidates with different algorithms
 *   - Debuggability: Can inspect candidate list before testing
 *   - Parallelization: Can test candidates in parallel (Phase 2)
 *   - Maintainability: Collision logic stays in intersection_detection.c
 *   
 *   The query functions (QuadTree_findCandidatePairs) are NOT redundant
 *   with intersect(). They serve different purposes:
 *   - Query: Finds which lines are spatially close (O(log n) per line)
 *   - intersect(): Tests if two specific lines collide (O(1) per pair)
 *   Together they achieve O(n log n) instead of O(n²).
 *
 * Separation of Create and Build:
 *   QuadTree_create() allocates structure and sets bounds.
 *   QuadTree_build() populates the tree with lines.
 *   
 *   Benefits:
 *   - Separation of memory management from algorithm logic
 *   - Can create once, rebuild many times (if needed for optimization)
 *   - Easier testing (can test create separately from build)
 *   - Clearer error handling (create fails = memory, build fails = algorithm)
 *   - Foundation vs construction analogy: create = foundation, build = house
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

#ifndef QUADTREE_H_
#define QUADTREE_H_

#include <stdbool.h>
#include <stddef.h>

// Include line.h since we use Line types in the public API
#include "./line.h"

/**
 * Error codes returned by quadtree functions.
 */
typedef enum {
  QUADTREE_SUCCESS = 0,            // Operation succeeded
  QUADTREE_ERROR_NULL_POINTER,     // NULL pointer argument
  QUADTREE_ERROR_INVALID_BOUNDS,   // Invalid bounding box (xmin >= xmax, etc.)
  QUADTREE_ERROR_MALLOC_FAILED,    // Memory allocation failed
  QUADTREE_ERROR_INVALID_CONFIG,   // Invalid configuration parameters
  QUADTREE_ERROR_EMPTY_TREE        // Operation on empty tree
} QuadTreeError;

/**
 * Configuration parameters for quadtree construction and behavior.
 * These control when and how the tree subdivides space.
 */
typedef struct {
  // Maximum depth of the quadtree. Prevents excessive subdivision.
  // Typical values: 10-15. Higher = more memory, finer granularity.
  unsigned int maxDepth;

  // Maximum number of lines per node before subdividing.
  // Typical values: 4-8. Lower = more subdivision, better performance
  // for dense regions, but more memory overhead.
  unsigned int maxLinesPerNode;

  // Minimum cell size (in each dimension). Prevents subdivision when
  // cells become too small (avoids floating-point precision issues).
  // Typical values: 0.001 - 0.0001. Must be > 0.
  double minCellSize;

  // Enable debug statistics collection (slight performance overhead).
  // When true, tree tracks node counts, line distributions, etc.
  bool enableDebugStats;
} QuadTreeConfig;

/**
 * Debug statistics collected during quadtree operations.
 * Only populated if enableDebugStats is true in config.
 */
typedef struct {
  // Tree structure statistics
  unsigned int totalNodes;            // Total nodes created
  unsigned int totalLeaves;           // Leaf nodes (no children)
  unsigned int maxDepthReached;       // Deepest node in tree
  unsigned int maxLinesInNode;        // Maximum lines in any single node
				      
  // Query statistics		      
  unsigned int totalQueries;          // Number of line queries performed
  unsigned int totalCellsChecked;     // Total cells examined during queries
  unsigned int totalPairsTested;      // Total line pairs tested for collision

  // Performance hints
  unsigned int linesInMultipleCells;  // Lines that span cell boundaries
  unsigned int emptyCells;            // Cells with no lines
} QuadTreeDebugStats;

/**
 * Internal quadtree node structure.
 * Represents a square region of space that may contain line segments.
 * 
 * IMPORTANT: All cells are perfect squares (width == height). The quadtree
 * algorithm subdivides space into equal-sized square quadrants at each level.
 * Even if the world bounds are rectangular, all subdivisions are squares.
 */
typedef struct QuadNode {
  // Bounding box of this cell (always a square: xmax-xmin == ymax-ymin)
  double xmin, xmax;  // X-axis bounds [xmin, xmax)
  double ymin, ymax;  // Y-axis bounds [ymin, ymax)

  // Lines stored in this node (array of pointers, not copies)
  Line** lines;              // Array of Line* pointers
  unsigned int numLines;     // Current number of lines
  unsigned int capacity;     // Allocated capacity of lines array

  // Child nodes (NULL if this is a leaf node)
  // Index 0: Southwest (SW) quadrant
  // Index 1: Southeast (SE) quadrant
  // Index 2: Northwest (NW) quadrant
  // Index 3: Northeast (NE) quadrant
  struct QuadNode* children[4];

  // Node metadata
  bool isLeaf;    // True if this node has no children (leaf node)
  int depth;      // Depth of this node in the tree (0 = root)
} QuadNode;

/**
 * Quadtree data structure.
 * Hierarchical spatial index for efficient collision detection.
 * 
 * RELATIONSHIP TO CollisionWorld:
 * --------------------------------
 * The QuadTree is a standalone module used BY CollisionWorld, not embedded
 * in it. The relationship is:
 * 
 *   CollisionWorld (persistent, owns lines)
 *       ↓ (contains)
 *   Line* lines[] (array of line pointers)
 *       ↓ (temporary structure built FROM these lines each frame)
 *   QuadTree (temporary spatial index, built/destroyed per frame)
 *       ↓ (used for)
 *   Collision detection algorithm
 *       ↓ (produces)
 *   IntersectionEventList
 *       ↓ (fed to)
 *   CollisionWorld_collisionSolver()
 * 
 * The quadtree is created by CollisionWorld_detectIntersection() when
 * useQuadtree flag is true, used for one frame's collision detection, then
 * destroyed. It is rebuilt each frame because lines move between frames.
 * 
 * This standalone design provides better modularity and testability compared
 * to embedding the tree in CollisionWorld. See module-level documentation
 * for detailed performance comparison.
 */
typedef struct {
  QuadNode* root;              // Root node of the tree
  QuadTreeConfig config;       // Configuration parameters
  QuadTreeDebugStats* stats;   // Debug statistics (NULL if disabled)

  // Bounding box of entire tree (world bounds, must be square or will be
  // made square by taking the larger dimension)
  double worldXmin, worldXmax;
  double worldYmin, worldYmax;
  
  // Lines stored in tree (for query phase - need to iterate over all lines)
  // This is set during QuadTree_build() and used during query phase
  Line** lines;                 // Array of all lines (not owned by tree)
  unsigned int numLines;        // Number of lines
} QuadTree;

/**
 * Candidate pair structure for collision testing.
 * Represents two lines that may potentially collide (based on spatial
 * proximity in the quadtree).
 * 
 * PURPOSE IN ALGORITHM:
 * ---------------------
 * The quadtree query phase (QuadTree_findCandidatePairs) performs spatial
 * filtering: it finds which lines are in the same or nearby cells. These
 * are "candidates" - lines that are spatially close and MIGHT collide.
 * 
 * The actual collision testing (using the existing intersect() function)
 * happens AFTER querying. This separation provides:
 * 
 * 1. Spatial Filtering (Query Phase):
 *    - Input: Line L, QuadTree
 *    - Output: List of lines near L (candidates)
 *    - Complexity: O(log n) per line
 *    - Purpose: Reduce n² pairs to ~n log n candidates
 *    - Does NOT test for collisions - just finds neighbors
 * 
 * 2. Collision Testing (Test Phase, external):
 *    - Input: Two specific lines (L1, L2) from candidate list
 *    - Output: IntersectionType (from existing intersect() function)
 *    - Complexity: O(1) per pair
 *    - Purpose: Actually determine if collision occurs
 *    - Uses existing collision detection logic
 * 
 * This separation is NOT redundant. The query functions find WHICH pairs
 * to test (spatial filtering), while intersect() tests IF they collide
 * (collision logic). Together they achieve O(n log n) instead of O(n²).
 * 
 * Example without quadtree: Test ALL pairs = n(n-1)/2 calls to intersect()
 * Example with quadtree: Query finds ~n log n candidates, then test those
 */
typedef struct {
  Line* line1;   // First line (guaranteed: line1->id < line2->id)
  Line* line2;   // Second line
} QuadTreeCandidatePair;

/**
 * Array of candidate pairs for batch collision testing.
 * 
 * This structure collects all candidate pairs found during the query phase.
 * The pairs are then tested using the existing intersect() function in a
 * separate test phase. This separation enables:
 * - Parallel testing of candidates (Phase 2: parallelization)
 * - Debugging: Can inspect candidate list before testing
 * - Flexibility: Can test with different algorithms
 */
typedef struct {
  QuadTreeCandidatePair* pairs;  // Array of candidate pairs
  unsigned int count;            // Number of pairs
  unsigned int capacity;         // Allocated capacity
} QuadTreeCandidateList;

// ============================================================================
// Configuration Functions
// ============================================================================

/**
 * Create a default quadtree configuration with reasonable values.
 * Returns a config suitable for most use cases.
 *
 * @return Default configuration structure
 */
QuadTreeConfig QuadTreeConfig_default(void);

/**
 * Create a custom quadtree configuration.
 *
 * @param maxDepth Maximum tree depth (must be > 0)
 * @param maxLinesPerNode Lines per node before subdividing (must be > 0)
 * @param minCellSize Minimum cell size (must be > 0)
 * @param enableDebugStats Whether to collect debug statistics
 * @return Configuration structure
 */
QuadTreeConfig QuadTreeConfig_create(unsigned int maxDepth,
                                     unsigned int maxLinesPerNode,
                                     double minCellSize,
                                     bool enableDebugStats);

/**
 * Validate a quadtree configuration.
 *
 * @param config Configuration to validate
 * @return QUADTREE_SUCCESS if valid, error code otherwise
 */
QuadTreeError QuadTreeConfig_validate(const QuadTreeConfig* config);

// ============================================================================
// Tree Construction and Destruction
// ============================================================================

/**
 * Create a new quadtree with the specified world bounds and configuration.
 * 
 * This function allocates memory and initializes the quadtree structure,
 * but does NOT populate it with lines. Use QuadTree_build() to populate
 * the tree after creation.
 * 
 * SEPARATION OF CREATE AND BUILD:
 * --------------------------------
 * This separation (create vs build) is intentional and provides:
 * 
 * 1. Clear separation of concerns:
 *    - Create: Memory management and structure initialization
 *    - Build: Algorithm logic (inserting lines, subdividing)
 * 
 * 2. Flexibility:
 *    - Can create once, rebuild many times (if optimizing for reuse)
 *    - Can test creation separately from building
 * 
 * 3. Error handling clarity:
 *    - Create fails = memory allocation issue
 *    - Build fails = algorithm/data issue
 * 
 * 4. Foundation analogy:
 *    - Create = laying foundation (structure ready)
 *    - Build = constructing building (populating structure)
 * 
 * @param xmin Minimum X coordinate of world bounds
 * @param xmax Maximum X coordinate of world bounds
 * @param ymin Minimum Y coordinate of world bounds
 * @param ymax Maximum Y coordinate of world bounds
 * @param config Configuration parameters (NULL uses default config)
 * @param error Output parameter for error code (can be NULL)
 * @return New quadtree, or NULL on error (check error code)
 */
QuadTree* QuadTree_create(double xmin, double xmax,
                          double ymin, double ymax,
                          const QuadTreeConfig* config,
                          QuadTreeError* error);

/**
 * Destroy a quadtree and free all associated memory.
 *
 * @param tree Quadtree to destroy (can be NULL, no-op)
 */
void QuadTree_destroy(QuadTree* tree);

/**
 * Build a quadtree from an array of lines.
 * Inserts all lines into the tree, subdividing as needed.
 * 
 * This function populates an existing quadtree (created with QuadTree_create)
 * with lines. It performs the actual quadtree construction algorithm:
 * 
 * 1. For each line, compute its bounding box (current + future position)
 * 2. Insert line into all cells it overlaps
 * 3. If a cell exceeds maxLinesPerNode, subdivide into 4 square children
 * 4. Recursively distribute lines to child cells
 * 5. Continue until maxDepth reached or cells too small
 * 
 * SEPARATION FROM CREATE:
 * -----------------------
 * This is separate from QuadTree_create() to maintain clear separation
 * between memory management (create) and algorithm logic (build). See
 * QuadTree_create() documentation for detailed rationale.
 * 
 * @param tree Quadtree to build (must be created with QuadTree_create)
 * @param lines Array of Line pointers
 * @param numLines Number of lines in array
 * @param timeStep Time step for computing future line positions
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
QuadTreeError QuadTree_build(QuadTree* tree,
                             Line** lines,
                             unsigned int numLines,
                             double timeStep);

// ============================================================================
// Query Functions
// ============================================================================

/**
 * Find all candidate line pairs that may potentially collide.
 * Uses spatial queries to reduce the number of pairs tested.
 * 
 * QUERY PHASE - SPATIAL FILTERING:
 * ---------------------------------
 * This function performs the QUERY phase of the collision detection
 * algorithm. It does NOT test for actual collisions - it performs spatial
 * filtering to find which lines are close enough to potentially collide.
 * 
 * Algorithm:
 * 1. For each line L:
 *    a. Compute L's bounding box (current + future position)
 *    b. Find all quadtree cells that L overlaps
 *    c. Collect all OTHER lines from those cells
 *    d. These are "candidates" - lines spatially close to L
 * 2. Store as (L, candidate) pairs in candidateList
 * 
 * SEPARATION FROM COLLISION TESTING:
 * ----------------------------------
 * This function is NOT redundant with the existing intersect() function.
 * They serve complementary roles:
 * 
 * - QuadTree_findCandidatePairs (this function):
 *   * Purpose: Spatial filtering - find WHICH pairs to test
 *   * Input: QuadTree, all lines
 *   * Output: List of candidate pairs (spatially close lines)
 *   * Complexity: O(n log n) - reduces n² pairs to ~n log n candidates
 *   * Does NOT test collisions - just finds neighbors
 * 
 * - intersect() (in intersection_detection.c):
 *   * Purpose: Collision testing - test IF two lines collide
 *   * Input: Two specific lines (L1, L2)
 *   * Output: IntersectionType (collision or not)
 *   * Complexity: O(1) per pair
 *   * Does NOT do spatial queries - just tests two lines
 * 
 * Together they achieve O(n log n) instead of O(n²):
 * - Without quadtree: Test ALL pairs = n(n-1)/2 calls to intersect()
 * - With quadtree: Query finds ~n log n candidates, then test those
 * 
 * The caller should:
 * 1. Call this function to get candidate pairs (query phase)
 * 2. For each candidate pair, call intersect(line1, line2, timeStep)
 * 3. Process collision events as normal
 * 
 * This separation enables:
 * - Parallel testing of candidates (Phase 2: parallelization)
 * - Debugging: Can inspect candidate list before testing
 * - Flexibility: Can test with different collision algorithms
 * 
 * @param tree Quadtree to query
 * @param timeStep Time step for computing future line positions
 * @param candidateList Output parameter for candidate pairs (must be
 *                      initialized with QuadTreeCandidateList_init)
 * @return QUADTREE_SUCCESS on success, error code otherwise
 *
 * Note: The candidateList will be populated with pairs. The caller is
 * responsible for freeing the pairs array (but not the Line pointers).
 */
QuadTreeError QuadTree_findCandidatePairs(QuadTree* tree,
                                          double timeStep,
                                          QuadTreeCandidateList* candidateList);

/**
 * Initialize a candidate list (allocate initial capacity).
 *
 * @param list Candidate list to initialize
 * @param initialCapacity Initial capacity (0 uses default)
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
QuadTreeError QuadTreeCandidateList_init(QuadTreeCandidateList* list,
                                        unsigned int initialCapacity);

/**
 * Destroy a candidate list and free its memory.
 *
 * @param list Candidate list to destroy
 */
void QuadTreeCandidateList_destroy(QuadTreeCandidateList* list);

// ============================================================================
// Debug and Statistics Functions
// ============================================================================

/**
 * Get debug statistics from a quadtree.
 * Only available if enableDebugStats was true when creating the tree.
 *
 * @param tree Quadtree to query
 * @param stats Output parameter for statistics (can be NULL)
 * @return QUADTREE_SUCCESS if stats available, error code otherwise
 */
QuadTreeError QuadTree_getDebugStats(const QuadTree* tree,
                                     QuadTreeDebugStats* stats);

/**
 * Print debug statistics to stdout.
 * Convenience function for debugging and performance analysis.
 *
 * @param tree Quadtree to print stats for
 */
void QuadTree_printDebugStats(const QuadTree* tree);

/**
 * Reset debug statistics (set all counters to zero).
 * Useful for measuring performance of specific operations.
 *
 * @param tree Quadtree to reset stats for
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
QuadTreeError QuadTree_resetDebugStats(QuadTree* tree);

// ============================================================================
// Utility Functions
// ============================================================================

/**
 * Get a human-readable error message for an error code.
 *
 * @param error Error code
 * @return String describing the error
 */
const char* QuadTree_errorString(QuadTreeError error);

/**
 * Check if a quadtree is empty (contains no lines).
 *
 * @param tree Quadtree to check
 * @param isEmpty Output parameter (true if empty)
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
QuadTreeError QuadTree_isEmpty(const QuadTree* tree, bool* isEmpty);

/**
 * Get the number of lines in a quadtree.
 * Note: This counts all lines, including duplicates if a line
 * spans multiple cells.
 *
 * @param tree Quadtree to query
 * @param count Output parameter for line count
 * @return QUADTREE_SUCCESS on success, error code otherwise
 */
QuadTreeError QuadTree_getLineCount(const QuadTree* tree, unsigned int* count);

#endif  // QUADTREE_H_

