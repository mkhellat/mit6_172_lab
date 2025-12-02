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
  IntersectionEventList intersectionEventList = IntersectionEventList_make();

  // Debug: Track pairs tested (only in debug builds)
  #ifdef DEBUG_COLLISIONS
  static unsigned long long bruteForcePairsTested = 0;
  static unsigned long long quadtreePairsTested = 0;
  static unsigned long long bruteForceCollisionsFound = 0;
  static unsigned long long quadtreeCollisionsFound = 0;
  #endif

  // Debug: Initialize frame tracking for brute-force (DEBUG_BOX_IN)
  #ifdef DEBUG_BOX_IN
  static FILE* bfEventFileDebug = NULL;
  static int bfEventFrameCount = 0;
  static bool bfFrameStarted = false;
  bfEventFrameCount++;
  if (bfEventFileDebug == NULL && !collisionWorld->useQuadtree) {
    bfEventFileDebug = fopen("brute_force_events.txt", "w");
  }
  if (bfEventFileDebug != NULL && !collisionWorld->useQuadtree && 
      bfEventFrameCount >= 88 && bfEventFrameCount <= 100 && !bfFrameStarted) {
    fprintf(bfEventFileDebug, "=== Frame %d: Events BEFORE sorting ===\n", bfEventFrameCount);
    bfFrameStarted = true;
  }
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
	  IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
					   intersectionType);
	  collisionWorld->numLineLineCollisions++;
	  #ifdef DEBUG_COLLISIONS
	  bruteForceCollisionsFound++;
	  #endif
	  
	  // Debug: Track brute-force events (DEBUG_BOX_IN)
	  #ifdef DEBUG_BOX_IN
	  if (bfEventFileDebug != NULL && bfEventFrameCount >= 88 && bfEventFrameCount <= 100) {
	    unsigned int id1 = l1->id;
	    unsigned int id2 = l2->id;
	    if (id1 > id2) {
	      unsigned int temp = id1;
	      id1 = id2;
	      id2 = temp;
	    }
	    fprintf(bfEventFileDebug, "  (%u,%u) type=%d\n", id1, id2, intersectionType);
	  }
	  #endif
	}
      }
    }
    
    // Debug: Finalize frame for brute-force (DEBUG_BOX_IN)
    #ifdef DEBUG_BOX_IN
    if (bfEventFileDebug != NULL && bfEventFrameCount >= 88 && bfEventFrameCount <= 100 && bfFrameStarted) {
      unsigned int eventCount = 0;
      IntersectionEventNode* temp = intersectionEventList.head;
      while (temp != NULL) {
        eventCount++;
        temp = temp->next;
      }
      fprintf(bfEventFileDebug, "Total: %u events\n\n", eventCount);
      fflush(bfEventFileDebug);
      bfFrameStarted = false;
    }
    #endif
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
    // Enable debug stats for performance analysis (can be disabled in production)
    config.enableDebugStats = false;  // Set to true for debugging
    
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
                                           &candidateList);
        if (error != QUADTREE_SUCCESS) {
          fprintf(stderr, "Warning: Candidate pair query failed (%s), "
                  "falling back to brute-force algorithm.\n",
                  QuadTree_errorString(error));
          QuadTreeCandidateList_destroy(&candidateList);
          QuadTree_destroy(tree);
          tree = NULL;  // Signal to use brute-force
        } else {
          // Debug: Print candidate pair count and check for duplicates
          static int callCount = 0;
          callCount++;
          unsigned long long expectedPairs = ((unsigned long long)collisionWorld->numOfLines * 
                                             (collisionWorld->numOfLines - 1)) / 2;
          
          // Check for duplicates in candidate list (by pointer and by ID)
          unsigned int duplicateCount = 0;
          unsigned int duplicateByIdCount = 0;
          for (unsigned int i = 0; i < candidateList.count; i++) {
            for (unsigned int j = i + 1; j < candidateList.count; j++) {
              // Check by pointer
              if ((candidateList.pairs[i].line1 == candidateList.pairs[j].line1 &&
                   candidateList.pairs[i].line2 == candidateList.pairs[j].line2) ||
                  (candidateList.pairs[i].line1 == candidateList.pairs[j].line2 &&
                   candidateList.pairs[i].line2 == candidateList.pairs[j].line1)) {
                duplicateCount++;
              }
              // Check by ID (same IDs but different pointers = problem!)
              if ((candidateList.pairs[i].line1->id == candidateList.pairs[j].line1->id &&
                   candidateList.pairs[i].line2->id == candidateList.pairs[j].line2->id) ||
                  (candidateList.pairs[i].line1->id == candidateList.pairs[j].line2->id &&
                   candidateList.pairs[i].line2->id == candidateList.pairs[j].line1->id)) {
                if (candidateList.pairs[i].line1 != candidateList.pairs[j].line1 ||
                    candidateList.pairs[i].line2 != candidateList.pairs[j].line2) {
                  duplicateByIdCount++;
                  if (duplicateByIdCount <= 5) {
                    fprintf(stderr, "DEBUG: Duplicate by ID: (%u,%u) pointers differ\n",
                            candidateList.pairs[i].line1->id, candidateList.pairs[i].line2->id);
                  }
                }
              }
            }
          }
          
          // Debug output disabled for production (can enable with DEBUG_QUADTREE)
          #ifdef DEBUG_QUADTREE
          fprintf(stderr, "DEBUG [call %d]: Quadtree found %u candidate pairs (brute-force tests %llu pairs, ratio: %.2f%%, duplicates: %u)\n",
                  callCount, candidateList.count, expectedPairs, 
                  (candidateList.count * 100.0) / (expectedPairs > 0 ? expectedPairs : 1),
                  duplicateCount);
          #endif
          
          // Test candidate pairs using existing intersect() function
          // This is the TEST PHASE: spatial filtering already done in query phase
          #ifdef DEBUG_COLLISIONS
          quadtreePairsTested += candidateList.count;
          #endif
          
          // Debug: Track candidate pairs for comparison (DEBUG_BOX_IN)
          #ifdef DEBUG_BOX_IN
          static FILE* candidateFile = NULL;
          static int candidateFrameCount = 0;
          candidateFrameCount++;
          if (candidateFrameCount == 1) {
            candidateFile = fopen("quadtree_candidates.txt", "w");
          }
          if (candidateFile != NULL && candidateFrameCount >= 88 && candidateFrameCount <= 100) {
            fprintf(candidateFile, "=== Frame %d: %u candidate pairs ===\n", 
                    candidateFrameCount, candidateList.count);
            for (unsigned int i = 0; i < candidateList.count && i < 1000; i++) {
              unsigned int id1 = candidateList.pairs[i].line1->id;
              unsigned int id2 = candidateList.pairs[i].line2->id;
              if (id1 > id2) {
                unsigned int temp = id1;
                id1 = id2;
                id2 = temp;
              }
              fprintf(candidateFile, "  (%u,%u)\n", id1, id2);
            }
            if (candidateList.count > 1000) {
              fprintf(candidateFile, "  ... (%u more)\n", candidateList.count - 1000);
            }
            fflush(candidateFile);
          }
          #endif
          
          // Track which pairs we've tested to detect duplicates in candidate list
          bool* pairTested = calloc(candidateList.count, sizeof(bool));
          unsigned int duplicateTests = 0;
          
          for (unsigned int i = 0; i < candidateList.count; i++) {
            // Check if we've already tested this exact pair (by pointer)
            for (unsigned int j = 0; j < i; j++) {
              if ((candidateList.pairs[i].line1 == candidateList.pairs[j].line1 &&
                   candidateList.pairs[i].line2 == candidateList.pairs[j].line2) ||
                  (candidateList.pairs[i].line1 == candidateList.pairs[j].line2 &&
                   candidateList.pairs[i].line2 == candidateList.pairs[j].line1)) {
                duplicateTests++;
                if (duplicateTests <= 5) {
                  fprintf(stderr, "WARNING: Testing duplicate candidate pair #%u: (%u, %u) - same as pair #%u\n",
                          i, candidateList.pairs[i].line1->id, candidateList.pairs[i].line2->id, j);
                }
                pairTested[i] = true;  // Mark as duplicate, skip testing
                break;
              }
            }
            
            if (pairTested[i]) {
              continue;  // Skip duplicate
            }
            
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
              IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
                                               intersectionType);
              collisionWorld->numLineLineCollisions++;
              #ifdef DEBUG_COLLISIONS
              quadtreeCollisionsFound++;
              #endif
            }
          }
          
          // Cleanup - quadtree succeeded
          free(pairTested);
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
            IntersectionEventList_appendNode(&intersectionEventList, l1, l2,
                                             intersectionType);
            collisionWorld->numLineLineCollisions++;
            
            // Debug: Track brute-force pairs tested (DEBUG_BOX_IN)
            #ifdef DEBUG_BOX_IN
            static FILE* bfCandidateFile = NULL;
            static int bfCandidateFrameCount = 0;
            bfCandidateFrameCount++;
            if (bfCandidateFrameCount == 1) {
              bfCandidateFile = fopen("brute_force_candidates.txt", "w");
            }
            if (bfCandidateFile != NULL && bfCandidateFrameCount >= 88 && bfCandidateFrameCount <= 100) {
              unsigned int id1 = l1->id;
              unsigned int id2 = l2->id;
              if (id1 > id2) {
                unsigned int temp = id1;
                id1 = id2;
                id2 = temp;
              }
              fprintf(bfCandidateFile, "  (%u,%u) -> collision type=%d\n", id1, id2, intersectionType);
              if (bfCandidateFrameCount == 100) {
                fflush(bfCandidateFile);
              }
            }
            #endif
          }
        }
      }
    }
  }
  
  // Debug: Compare event lists between algorithms (only for box.in investigation)
  #ifdef DEBUG_BOX_IN
  static int frameCount = 0;
  frameCount++;
  
  // Track events for comparison
  static unsigned int* bfEventCounts = NULL;
  static unsigned int* qtEventCounts = NULL;
  static bool initialized = false;
  static FILE* bfEventFile = NULL;
  static FILE* qtEventFile = NULL;
  
  if (!initialized) {
    bfEventCounts = calloc(101, sizeof(unsigned int));
    qtEventCounts = calloc(101, sizeof(unsigned int));
    bfEventFile = fopen("brute_force_events.txt", "w");
    qtEventFile = fopen("quadtree_events.txt", "w");
    initialized = true;
  }
  
  // Count events before sorting and dump to file
  unsigned int eventCount = 0;
  IntersectionEventNode* temp = intersectionEventList.head;
  FILE* eventFile = collisionWorld->useQuadtree ? qtEventFile : bfEventFile;
  
  if (eventFile != NULL && frameCount >= 88 && frameCount <= 100) {
    fprintf(eventFile, "=== Frame %d: Events BEFORE sorting ===\n", frameCount);
  }
  
  while (temp != NULL) {
    eventCount++;
    if (eventFile != NULL && frameCount >= 88 && frameCount <= 100) {
      unsigned int id1 = temp->l1->id;
      unsigned int id2 = temp->l2->id;
      if (id1 > id2) {
        unsigned int temp_id = id1;
        id1 = id2;
        id2 = temp_id;
      }
      fprintf(eventFile, "  (%u,%u) type=%d\n", id1, id2, temp->intersectionType);
    }
    temp = temp->next;
  }
  
  if (eventFile != NULL && frameCount >= 88 && frameCount <= 100) {
    fprintf(eventFile, "Total: %u events\n\n", eventCount);
    fflush(eventFile);
  }
  
  if (collisionWorld->useQuadtree) {
    qtEventCounts[frameCount] = eventCount;
  } else {
    bfEventCounts[frameCount] = eventCount;
  }
  
  // Print comparison around frames where discrepancy appears
  if (frameCount >= 88 && frameCount <= 100) {
    if (collisionWorld->useQuadtree) {
      fprintf(stderr, "DEBUG [frame %d QT]: %u events before sorting\n", 
              frameCount, eventCount);
    } else {
      fprintf(stderr, "DEBUG [frame %d BF]: %u events before sorting", 
              frameCount, eventCount);
      if (qtEventCounts[frameCount] > 0) {
        int diff = (int)eventCount - (int)qtEventCounts[frameCount];
        fprintf(stderr, " (QT: %u, diff: %d)", qtEventCounts[frameCount], diff);
      }
      fprintf(stderr, "\n");
    }
  }
  #endif
  
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
  double m1 = Vec_length(Vec_subtract(l1->p1, l1->p2));
  double m2 = Vec_length(Vec_subtract(l2->p1, l2->p2));

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
