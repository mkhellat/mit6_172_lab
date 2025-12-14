/** 
 * collision_world.c -- detect and handle line segment intersections
 * Copyright (c) 2012 the Massachusetts Institute of Technology
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

#include "./collision_world.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#include "./intersection_detection.h"
#include "./intersection_event_list.h"
#include "./line.h"
#include "./quadtree.h"
#include "./fasttime.h"  // For timing instrumentation

// Cilk reducer support for parallelization
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

// Identity and reduce functions for collision counter reducer
static void collisionCounter_identity(void* v) {
  *(unsigned int*)v = 0;
}

static void collisionCounter_reduce(void* left, void* right) {
  *(unsigned int*)left += *(unsigned int*)right;
}

// Static keys for reducer lookup (unique addresses for each reducer)
static void* eventListReducerKey = &eventListReducerKey;
static void* collisionCounterReducerKey = &collisionCounterReducerKey;


// Comparison function for qsort to sort candidate pairs
// Matches brute-force processing order: by line1->id, then line2->id
static int compareCandidatePairs(const void* a, const void* b) {
  const QuadTreeCandidatePair* pairA = (const QuadTreeCandidatePair*)a;
  const QuadTreeCandidatePair* pairB = (const QuadTreeCandidatePair*)b;
  
  Line* l1_a = pairA->line1;
  Line* l2_a = pairA->line2;
  Line* l1_b = pairB->line1;
  Line* l2_b = pairB->line2;
  
  // Ensure pairs are normalized (line1->id < line2->id)
  if (compareLines(l1_a, l2_a) >= 0) {
    Line* temp = l1_a;
    l1_a = l2_a;
    l2_a = temp;
  }
  if (compareLines(l1_b, l2_b) >= 0) {
    Line* temp = l1_b;
    l1_b = l2_b;
    l2_b = temp;
  }
  
  // Compare: first by line1->id, then by line2->id
  int cmp1 = compareLines(l1_a, l1_b);
  if (cmp1 != 0) {
    return cmp1;
  }
  return compareLines(l2_a, l2_b);
}

CollisionWorld* CollisionWorld_new(const unsigned int capacity) {
  assert(capacity > 0);

  CollisionWorld* collisionWorld = malloc(sizeof(CollisionWorld));
  if (collisionWorld == NULL) {
    return NULL;
  }

  collisionWorld->numLineWallCollisions = 0;
  collisionWorld->numLineLineCollisions = 0;
  collisionWorld->timeStep = 0.5;
  collisionWorld->lines = malloc(capacity * sizeof(Line*));
  collisionWorld->numOfLines = 0;
  
  // Initialize collision detection algorithm to brute-force by default.
  // Can be changed later via CollisionWorld_setUseQuadtree().
  collisionWorld->useQuadtree = false;
  
  return collisionWorld;
}

void CollisionWorld_delete(CollisionWorld* collisionWorld) {
  for (int i = 0; i < collisionWorld->numOfLines; i++) {
    free(collisionWorld->lines[i]);
  }
  free(collisionWorld->lines);
  free(collisionWorld);
}

unsigned int CollisionWorld_getNumOfLines(CollisionWorld* collisionWorld) {
  return collisionWorld->numOfLines;
}

void CollisionWorld_addLine(CollisionWorld* collisionWorld, Line *line) {
  collisionWorld->lines[collisionWorld->numOfLines] = line;
  collisionWorld->numOfLines++;
}

Line* CollisionWorld_getLine(CollisionWorld* collisionWorld,
                             const unsigned int index) {
  if (index >= collisionWorld->numOfLines) {
    return NULL;
  }
  return collisionWorld->lines[index];
}

void CollisionWorld_updateLines(CollisionWorld* collisionWorld) {
  CollisionWorld_detectIntersection(collisionWorld);
  CollisionWorld_updatePosition(collisionWorld);
  CollisionWorld_lineWallCollision(collisionWorld);
}

void CollisionWorld_updatePosition(CollisionWorld* collisionWorld) {
  double t = collisionWorld->timeStep;
  for (int i = 0; i < collisionWorld->numOfLines; i++) {
    Line *line = collisionWorld->lines[i];
    line->p1 = Vec_add(line->p1, Vec_multiply(line->velocity, t));
    line->p2 = Vec_add(line->p2, Vec_multiply(line->velocity, t));
    // Precompute line length once per frame (used in collision solver).
    // Since lines are rigid bodies, length is constant, but we recompute
    // after position updates to ensure correctness.
    line->cachedLength = Vec_length(Vec_subtract(line->p1, line->p2));
    // Precompute velocity magnitude once per frame (used in bounding box calculations).
    // Since velocity doesn't change during a frame (only position changes),
    // we compute it once here to avoid expensive sqrt operations.
    line->cachedVelocityMagnitude = Vec_length(line->velocity);
  }
}

void CollisionWorld_lineWallCollision(CollisionWorld* collisionWorld) {
  for (int i = 0; i < collisionWorld->numOfLines; i++) {
    Line *line = collisionWorld->lines[i];
    bool collide = false;

    // Right side
    if ((line->p1.x > BOX_XMAX || line->p2.x > BOX_XMAX)
        && (line->velocity.x > 0)) {
      line->velocity.x = -line->velocity.x;
      collide = true;
    }
    // Left side
    if ((line->p1.x < BOX_XMIN || line->p2.x < BOX_XMIN)
        && (line->velocity.x < 0)) {
      line->velocity.x = -line->velocity.x;
      collide = true;
    }
    // Top side
    if ((line->p1.y > BOX_YMAX || line->p2.y > BOX_YMAX)
        && (line->velocity.y > 0)) {
      line->velocity.y = -line->velocity.y;
      collide = true;
    }
    // Bottom side
    if ((line->p1.y < BOX_YMIN || line->p2.y < BOX_YMIN)
        && (line->velocity.y < 0)) {
      line->velocity.y = -line->velocity.y;
      collide = true;
    }
    // Update total number of collisions.
    if (collide == true) {
      collisionWorld->numLineWallCollisions++;
    }
  }
}

void CollisionWorld_detectIntersection(CollisionWorld* collisionWorld) {
  // PHASE 2: Set up reducer infrastructure for thread-safe parallel operations
  // Reducers will be used when we add parallelization in Phase 3
  // For now, code remains serial but reducer functions are implemented and ready
  // 
  // NOTE: OpenCilk 2.0's __cilkrts_reducer_lookup API may not work correctly
  // in serial code. For Phase 2, we use regular variables but structure code
  // to easily switch to reducers in Phase 3 when we add cilk_for.
  //
  // Reducer functions (IntersectionEventList_merge, IntersectionEventList_identity,
  // collisionCounter_identity, collisionCounter_reduce) are implemented and tested.
  // They will be used via __cilkrts_reducer_lookup when parallelization is added.
  
  // For Phase 2 (serial): Use regular variables
  // For Phase 3 (parallel): Will switch to reducer views via __cilkrts_reducer_lookup
  IntersectionEventList intersectionEventList = IntersectionEventList_make();
  unsigned int localCollisionCount = 0;

  // Debug: Track pairs tested (only in debug builds)
  #ifdef DEBUG_COLLISIONS
  static unsigned long long bruteForcePairsTested = 0;
  static unsigned long long quadtreePairsTested = 0;
  static unsigned long long bruteForceCollisionsFound = 0;
  static unsigned long long quadtreeCollisionsFound = 0;
  #endif

  
  // Choose collision detection algorithm based on configuration.
  // The useQuadtree flag is set via CollisionWorld_setUseQuadtree(),
  // typically based on command-line arguments (e.g., -q flag).
  if (!collisionWorld->useQuadtree) {
    // BRUTE-FORCE ALGORITHM: O(n^2) pairwise collision detection.
    // Test all line-line pairs to see if they will intersect before the
    // next time step. Simple but can be slow for large numbers of lines.
    for (int i = 0; i < collisionWorld->numOfLines; i++) {
      Line *l1 = collisionWorld->lines[i];
  
      for (int j = i + 1; j < collisionWorld->numOfLines; j++) {
	Line *l2 = collisionWorld->lines[j];
  
	// intersect expects compareLines(l1, l2) < 0 to be true.
	// Swap l1 and l2, if necessary.
	if (compareLines(l1, l2) >= 0) {
	  Line *temp = l1;
	  l1 = l2;
	  l2 = temp;
	}
  
	IntersectionType intersectionType =
          intersect(l1, l2, collisionWorld->timeStep);
	#ifdef DEBUG_COLLISIONS
	bruteForcePairsTested++;
	#endif
	if (intersectionType != NO_INTERSECTION) {
	  // Phase 2: Use regular variable (serial)
	  // Phase 3: Will switch to reducer view for thread-safe append
	  IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
					   intersectionType);
	  // Phase 2: Use regular variable (serial)
	  // Phase 3: Will switch to reducer view for thread-safe increment
	  localCollisionCount++;
	  #ifdef DEBUG_COLLISIONS
	  bruteForceCollisionsFound++;
	  #endif
	}
      }
    }
    
  }
  else {
    // QUADTREE ALGORITHM: Spatial partitioning for optimized collision detection.
    // Uses hierarchical space subdivision to reduce the number of pairwise
    // comparisons needed. More efficient for large numbers of lines.
    // 
    // Algorithm:
    // 1. Build quadtree from current line positions
    // 2. Query tree to find candidate pairs (spatially close lines)
    // 3. Test candidates using existing intersect() function
    // 4. Result: Same collisions as brute-force, but O(n log n) instead of O(n²)
    
    // Flag to track if quadtree succeeded (so we don't run fallback)
    bool quadtreeSucceeded = false;
    
    // Create quadtree with world bounds
    QuadTreeError error;
    QuadTreeConfig config = QuadTreeConfig_default();
    
    // Allow maxDepth to be overridden via environment variable for tuning
    // This enables benchmarking different maxDepth values without code changes
    const char* maxDepthEnv = getenv("QUADTREE_MAXDEPTH");
    if (maxDepthEnv != NULL) {
      unsigned int customMaxDepth = (unsigned int)atoi(maxDepthEnv);
      if (customMaxDepth > 0) {
        config.maxDepth = customMaxDepth;
      }
    }
    
    // Enable debug stats for performance analysis (can be disabled in production)
    config.enableDebugStats = true;  // Set to true for debugging
    
    QuadTree* tree = QuadTree_create(BOX_XMIN, BOX_XMAX,
                                      BOX_YMIN, BOX_YMAX,
                                      &config, &error);
    if (tree == NULL) {
      // If quadtree creation fails, fall back to brute-force
      fprintf(stderr, "Warning: Quadtree creation failed (%s), "
              "falling back to brute-force algorithm.\n",
              QuadTree_errorString(error));
      // Fall through to brute-force algorithm below
    } else {
      // Build quadtree from lines
      error = QuadTree_build(tree, collisionWorld->lines,
                            collisionWorld->numOfLines,
                            collisionWorld->timeStep);
      if (error != QUADTREE_SUCCESS) {
        fprintf(stderr, "Warning: Quadtree build failed (%s), "
                "falling back to brute-force algorithm.\n",
                QuadTree_errorString(error));
        QuadTree_destroy(tree);
        tree = NULL;  // Signal to use brute-force
      }
    }
    
    if (tree != NULL) {
      // Find candidate pairs using spatial queries
      QuadTreeCandidateList candidateList;
      error = QuadTreeCandidateList_init(&candidateList, 0);
      if (error != QUADTREE_SUCCESS) {
        fprintf(stderr, "Warning: Candidate list initialization failed (%s), "
                "falling back to brute-force algorithm.\n",
                QuadTree_errorString(error));
        QuadTree_destroy(tree);
        tree = NULL;  // Signal to use brute-force
      } else {
        error = QuadTree_findCandidatePairs(tree, collisionWorld->timeStep,
                                           &candidateList, 0);
        if (error != QUADTREE_SUCCESS) {
          fprintf(stderr, "Warning: Candidate pair query failed (%s), "
                  "falling back to brute-force algorithm.\n",
                  QuadTree_errorString(error));
          QuadTreeCandidateList_destroy(&candidateList);
          QuadTree_destroy(tree);
          tree = NULL;  // Signal to use brute-force
        } else {
          // Debug: Print candidate pair count (for performance analysis)
          // NOTE: Duplicate checking removed - quadtree.c already prevents duplicates
          // using seenPairs matrix. The previous O(n^2) duplicate check was a major
          // performance bottleneck that negated the quadtree's benefits.
          
          // Debug output disabled for production (can enable with DEBUG_QUADTREE)
          #ifdef DEBUG_QUADTREE
          static int callCount = 0;
          callCount++;
          unsigned long long expectedPairs = ((unsigned long long)collisionWorld->numOfLines * 
                                             (collisionWorld->numOfLines - 1)) / 2;
          fprintf(stderr, "DEBUG [call %d]: Quadtree found %u candidate pairs (brute-force tests %llu pairs, ratio: %.2f%%)\n",
                  callCount, candidateList.count, expectedPairs, 
                  (candidateList.count * 100.0) / (expectedPairs > 0 ? expectedPairs : 1));
          #endif
          
          // Sort candidate pairs to match brute-force processing order
          // This ensures collisions are processed in the same order as brute-force,
          // which should reduce discrepancies due to processing order differences
          // Sort by line1->id, then line2->id (matching brute-force nested loop order)
          // CRITICAL: Use qsort (O(n log n)) instead of bubble sort (O(n^2))
          // to avoid negating the quadtree's performance benefits
          #ifdef DEBUG_QUADTREE_TIMING
          fasttime_t start_sort = gettime();
          #endif
          if (candidateList.count > 0) {
            qsort(candidateList.pairs, candidateList.count, 
                  sizeof(QuadTreeCandidatePair), compareCandidatePairs);
          }
          #ifdef DEBUG_QUADTREE_TIMING
          fasttime_t end_sort = gettime();
          double sort_time = tdiff(start_sort, end_sort);
          #endif
          
          // Test candidate pairs using existing intersect() function
          // This is the TEST PHASE: spatial filtering already done in query phase
          #ifdef DEBUG_COLLISIONS
          quadtreePairsTested += candidateList.count;
          #endif
          
          #ifdef DEBUG_QUADTREE_TIMING
          fasttime_t start_test = gettime();
          #endif
          
          // Test candidate pairs - quadtree.c already prevents duplicates using seenPairs matrix
          // No need for additional duplicate checking here
          for (unsigned int i = 0; i < candidateList.count; i++) {
            Line* l1 = candidateList.pairs[i].line1;
            Line* l2 = candidateList.pairs[i].line2;
            
            // Ensure compareLines(l1, l2) < 0 (quadtree should guarantee this,
            // but double-check for safety)
            if (compareLines(l1, l2) >= 0) {
              Line* temp = l1;
              l1 = l2;
              l2 = temp;
            }
            
            // Use existing collision detection function
            IntersectionType intersectionType =
                intersect(l1, l2, collisionWorld->timeStep);
            if (intersectionType != NO_INTERSECTION) {
              // Phase 2: Use regular variable (serial)
              // Phase 3: Will switch to reducer view for thread-safe append
              IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
                                               intersectionType);
              // Phase 2: Use regular variable (serial)
              // Phase 3: Will switch to reducer view for thread-safe increment
              localCollisionCount++;
              #ifdef DEBUG_COLLISIONS
              quadtreeCollisionsFound++;
              #endif
            }
          }
          
          #ifdef DEBUG_QUADTREE_TIMING
          fasttime_t end_test = gettime();
          double test_time = tdiff(start_test, end_test);
          
          // STEP 2: Report complete time breakdown
          fprintf(stderr, "===== QUADTREE TIME BREAKDOWN (Step 2) =====\n");
          fprintf(stderr, "Sort phase: %.6fs\n", sort_time);
          fprintf(stderr, "Test phase: %.6fs\n", test_time);
          fprintf(stderr, "==========================================\n");
          fprintf(stderr, "NOTE: Build and Query phase timings reported in quadtree.c\n");
          #endif
          
          // Debug: Print quadtree statistics for performance analysis
          #ifdef DEBUG_QUADTREE_STATS
          static int statsFrameCount = 0;
          statsFrameCount++;
          if (statsFrameCount == 1) {
            QuadTree_printDebugStats(tree);
          }
          #endif
          
          // Cleanup - quadtree succeeded
          QuadTreeCandidateList_destroy(&candidateList);
          QuadTree_destroy(tree);
          quadtreeSucceeded = true;  // Mark success - do NOT run fallback
        }
        // If query failed, tree is already NULL, fall through to fallback
      }
      // If candidate list init failed, tree is already NULL, fall through to fallback
    }
    // If build failed or tree creation failed, tree is NULL, fall through to fallback
    
    // Fallback to brute-force ONLY if quadtree was attempted but failed
    // (do NOT run if quadtree succeeded)
    if (!quadtreeSucceeded) {
      // BRUTE-FORCE ALGORITHM: Fallback if quadtree fails
      for (int i = 0; i < collisionWorld->numOfLines; i++) {
        Line *l1 = collisionWorld->lines[i];
        
        for (int j = i + 1; j < collisionWorld->numOfLines; j++) {
          Line *l2 = collisionWorld->lines[j];
          
          // intersect expects compareLines(l1, l2) < 0 to be true.
          // Swap l1 and l2, if necessary.
          if (compareLines(l1, l2) >= 0) {
            Line *temp = l1;
            l1 = l2;
            l2 = temp;
          }
          
          IntersectionType intersectionType =
              intersect(l1, l2, collisionWorld->timeStep);
          if (intersectionType != NO_INTERSECTION) {
            // Phase 2: Use regular variable (serial)
            // Phase 3: Will switch to reducer view for thread-safe append
            IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
                                             intersectionType);
            // Phase 2: Use regular variable (serial)
            // Phase 3: Will switch to reducer view for thread-safe increment
            localCollisionCount++;
            
          }
        }
      }
    }
  }
  
  // PHASE 2: Use local variables (serial)
  // Phase 3: Will extract from reducer views after parallel sections
  // Reducers automatically merge at sync points (cilk_sync or end of function)
  // In parallel code, will use: intersectionEventList = *eventListReducerView;
  
  // Update collision world counter
  collisionWorld->numLineLineCollisions += localCollisionCount;
  
  // Sort the intersection event list.
  IntersectionEventNode* startNode = intersectionEventList.head;
  while (startNode != NULL) {
    IntersectionEventNode* minNode = startNode;
    IntersectionEventNode* curNode = startNode->next;
    while (curNode != NULL) {
      if (IntersectionEventNode_compareData(curNode, minNode) < 0) {
        minNode = curNode;
      }
      curNode = curNode->next;
    }
    if (minNode != startNode) {
      IntersectionEventNode_swapData(minNode, startNode);
    }
    startNode = startNode->next;
  }
  
  // Call the collision solver for each intersection event.
  IntersectionEventNode* curNode = intersectionEventList.head;

  while (curNode != NULL) {
    CollisionWorld_collisionSolver(collisionWorld, curNode->l1, curNode->l2,
                                   curNode->intersectionType);
    curNode = curNode->next;
  }

  IntersectionEventList_deleteNodes(&intersectionEventList);
}

unsigned int CollisionWorld_getNumLineWallCollisions(
    CollisionWorld* collisionWorld) {
  return collisionWorld->numLineWallCollisions;
}

unsigned int CollisionWorld_getNumLineLineCollisions(
    CollisionWorld* collisionWorld) {
  return collisionWorld->numLineLineCollisions;
}

void CollisionWorld_collisionSolver(CollisionWorld* collisionWorld,
                                    Line *l1, Line *l2,
                                    IntersectionType intersectionType) {
  assert(compareLines(l1, l2) < 0);
  assert(intersectionType == L1_WITH_L2
         || intersectionType == L2_WITH_L1
         || intersectionType == ALREADY_INTERSECTED);

  // Despite our efforts to determine whether lines will intersect ahead
  // of time (and to modify their velocities appropriately), our
  // simplified model can sometimes cause lines to intersect.  In such a
  // case, we compute velocities so that the two lines can get unstuck in
  // the fastest possible way, while still conserving momentum and kinetic
  // energy.
  if (intersectionType == ALREADY_INTERSECTED) {
    Vec p = getIntersectionPoint(l1->p1, l1->p2, l2->p1, l2->p2);

    if (Vec_length(Vec_subtract(l1->p1, p))
        < Vec_length(Vec_subtract(l1->p2, p))) {
      l1->velocity = Vec_multiply(Vec_normalize(Vec_subtract(l1->p2, p)),
                                  Vec_length(l1->velocity));
    } else {
      l1->velocity = Vec_multiply(Vec_normalize(Vec_subtract(l1->p1, p)),
                                  Vec_length(l1->velocity));
    }
    if (Vec_length(Vec_subtract(l2->p1, p))
        < Vec_length(Vec_subtract(l2->p2, p))) {
      l2->velocity = Vec_multiply(Vec_normalize(Vec_subtract(l2->p2, p)),
                                  Vec_length(l2->velocity));
    } else {
      l2->velocity = Vec_multiply(Vec_normalize(Vec_subtract(l2->p1, p)),
                                  Vec_length(l2->velocity));
    }
    return;
  }

  // Compute the collision face/normal vectors.
  Vec face;
  Vec normal;
  if (intersectionType == L1_WITH_L2) {
    Vec v = Vec_makeFromLine(*l2);
    face = Vec_normalize(v);
  } else {
    Vec v = Vec_makeFromLine(*l1);
    face = Vec_normalize(v);
  }
  normal = Vec_orthogonal(face);

  // Obtain each line's velocity components with respect to the collision
  // face/normal vectors.
  double v1Face = Vec_dotProduct(l1->velocity, face);
  double v2Face = Vec_dotProduct(l2->velocity, face);
  double v1Normal = Vec_dotProduct(l1->velocity, normal);
  double v2Normal = Vec_dotProduct(l2->velocity, normal);

  // Compute the mass of each line (we simply use its length).
  // Use cached length to avoid expensive sqrt operations.
  double m1 = l1->cachedLength;
  double m2 = l2->cachedLength;

  // Perform the collision calculation (computes the new velocities along
  // the direction normal to the collision face such that momentum and
  // kinetic energy are conserved).
  double newV1Normal = ((m1 - m2) / (m1 + m2)) * v1Normal
      + (2 * m2 / (m1 + m2)) * v2Normal;
  double newV2Normal = (2 * m1 / (m1 + m2)) * v1Normal
      + ((m2 - m1) / (m2 + m1)) * v2Normal;

  // Combine the resulting velocities.
  l1->velocity = Vec_add(Vec_multiply(normal, newV1Normal),
                         Vec_multiply(face, v1Face));
  l2->velocity = Vec_add(Vec_multiply(normal, newV2Normal),
                         Vec_multiply(face, v2Face));

  return;
}

void CollisionWorld_setUseQuadtree(CollisionWorld* collisionWorld,
                                   bool useQuadtree) {
  // Configure the collision detection algorithm.
  // This should be called after CollisionWorld_new() and before running
  // the simulation, typically based on command-line flags.
  assert(collisionWorld != NULL);
  collisionWorld->useQuadtree = useQuadtree;
}
