# Parallelization Plan for Screensaver Project

## Overview

This document outlines the plan for parallelizing the screensaver collision detection code using OpenCilk. The goal is to achieve good parallel speedup on multi-core systems while maintaining correctness and determinacy.

**Environment:**
- Cilk installation: `/opt/opencilk-2`
- Target: 8-core system (simulate with `CILK_NWORKERS=8`)
- Tools: `perf`, `cilkscale`, `cilksan`

---

## Phase 1: Profiling the Serial Program

### 1.1 Setup Profiling Environment

**Build with profiling support:**
```bash
cd prj2/MIT6_172F18-project2
# Use regular gcc for profiling (not Cilk yet)
make clean
make
```

**Profile with perf:**
```bash
# Profile the serial program
perf record -g ./screensaver -q 4000 input/koch.in
perf report
```

**Expected hotspots:**
- Build phase: ~70% (quadtree construction)
- Query phase: ~5-6% (candidate pair finding)
- Sort phase: ~15.75% (sorting candidates)
- Test phase: ~1-2% (intersection testing)

**Goal:** Identify regions with:
- Large percentage of execution time
- Coarse-grained parallelism potential
- Independent operations that can run in parallel

### 1.2 Profile Different Inputs

**Test cases:**
```bash
# Small input
perf record -g ./screensaver -q 100 input/beaver.in

# Medium input
perf record -g ./screensaver -q 100 input/mit.in

# Large input (most important)
perf record -g ./screensaver -q 100 input/koch.in
```

**Document findings:**
- Which phase takes most time?
- Are there nested loops that can be parallelized?
- Are there independent operations?

---

## Phase 2: Converting Global Variables to Reducers

### 2.1 Identify Shared State

**Critical shared state that needs reducers:**

1. **`IntersectionEventList`** (MOST IMPORTANT)
   - Location: `collision_world.c:175`
   - Usage: `IntersectionEventList_appendNode()` called from multiple places
   - Problem: Multiple threads appending to same list = race condition
   - Solution: Use `CILK_C_REDUCER` with list reducer

2. **`collisionWorld->numLineLineCollisions`**
   - Location: `collision_world.c:215, 361, 425`
   - Usage: Incremented when collision detected
   - Problem: Multiple threads incrementing same counter = race condition
   - Solution: Use `CILK_C_REDUCER_OPADD` reducer

3. **Debug counters (optional)**
   - Location: `collision_world.c:179-182` (static variables)
   - Usage: Debug statistics
   - Solution: Use reducers or disable in parallel builds

### 2.2 Implement Reducers

**Step 1: Create reducer for IntersectionEventList**

**File:** `intersection_event_list.h`
```c
#include <cilk/cilk.h>
#include <cilk/reducer.h>

// Reducer type for IntersectionEventList
CILK_C_REDUCER_TYPE(IntersectionEventList_reducer,
                     IntersectionEventList,
                     IntersectionEventList_make(),
                     IntersectionEventList_merge,
                     IntersectionEventList_identity);

// Helper function to merge two lists (for reducer)
void IntersectionEventList_merge(void* reducer, void* other);
void IntersectionEventList_identity(void* reducer);
```

**File:** `intersection_event_list.c`
```c
// Merge two lists (for reducer)
void IntersectionEventList_merge(void* reducer, void* other) {
  IntersectionEventList* list1 = (IntersectionEventList*)reducer;
  IntersectionEventList* list2 = (IntersectionEventList*)other;
  
  if (list1->head == NULL) {
    *list1 = *list2;
  } else if (list2->head != NULL) {
    list1->tail->next = list2->head;
    list1->tail = list2->tail;
  }
  
  // Clear list2 (it's been merged)
  list2->head = NULL;
  list2->tail = NULL;
}

void IntersectionEventList_identity(void* reducer) {
  IntersectionEventList* list = (IntersectionEventList*)reducer;
  list->head = NULL;
  list->tail = NULL;
}
```

**Step 2: Create reducer for collision counter**

**File:** `collision_world.c`
```c
#include <cilk/cilk.h>
#include <cilk/reducer.h>

// Reducer for collision counter
CILK_C_REDUCER_OPADD(collisionCounter_reducer, unsigned int, 0);
```

**Step 3: Update CollisionWorld_detectIntersection**

```c
void CollisionWorld_detectIntersection(CollisionWorld* collisionWorld) {
  // Use reducer for event list
  CILK_C_REDUCER_TYPE(IntersectionEventList_reducer) eventListReducer;
  REDUCER_VIEW(eventListReducer) = IntersectionEventList_make();
  
  // Use reducer for collision counter
  CILK_C_REDUCER_OPADD(collisionCounter_reducer, unsigned int, 0);
  unsigned int* collisionCount = &REDUCER_VIEW(collisionCounter_reducer);
  
  // ... parallel code will append to eventListReducer ...
  
  // After parallel section, merge results
  IntersectionEventList intersectionEventList = REDUCER_VIEW(eventListReducer);
  collisionWorld->numLineLineCollisions += REDUCER_VIEW(collisionCounter_reducer);
  
  // ... continue with sorting and solving ...
}
```

### 2.3 Test Reducers

**Verify correctness:**
```bash
# Should produce same collision counts as serial
./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"
```

---

## Phase 3: Parallelizing the Application

### 3.1 Parallelize Brute-Force Algorithm

**Current code (serial):**
```c
for (int i = 0; i < collisionWorld->numOfLines; i++) {
  Line *l1 = collisionWorld->lines[i];
  for (int j = i + 1; j < collisionWorld->numOfLines; j++) {
    Line *l2 = collisionWorld->lines[j];
    // ... test intersection ...
    if (intersectionType != NO_INTERSECTION) {
      IntersectionEventList_appendNode(&intersectionEventList, l1, l2, intersectionType);
    }
  }
}
```

**Parallelized version:**
```c
cilk_for (int i = 0; i < collisionWorld->numOfLines; i++) {
  Line *l1 = collisionWorld->lines[i];
  for (int j = i + 1; j < collisionWorld->numOfLines; j++) {
    Line *l2 = collisionWorld->lines[j];
    // ... test intersection ...
    if (intersectionType != NO_INTERSECTION) {
      // Use reducer view
      IntersectionEventList_appendNode(&REDUCER_VIEW(eventListReducer), l1, l2, intersectionType);
      REDUCER_VIEW(collisionCounter_reducer)++;
    }
  }
}
```

**Note:** `cilk_for` parallelizes the outer loop. Each iteration is independent.

### 3.2 Parallelize Quadtree Query Phase

**Current code (serial):**
```c
for (unsigned int i = 0; i < tree->numLines; i++) {
  Line* line1 = tree->lines[i];
  // ... find candidates for line1 ...
  for (each candidate pair) {
    if (intersect(...)) {
      IntersectionEventList_appendNode(...);
    }
  }
}
```

**Parallelized version:**
```c
cilk_for (unsigned int i = 0; i < tree->numLines; i++) {
  Line* line1 = tree->lines[i];
  // ... find candidates for line1 ...
  for (each candidate pair) {
    if (intersect(...)) {
      IntersectionEventList_appendNode(&REDUCER_VIEW(eventListReducer), ...);
      REDUCER_VIEW(collisionCounter_reducer)++;
    }
  }
}
```

**Considerations:**
- Each line's candidate search is independent
- Need to ensure thread-safe candidate list access
- May need per-thread candidate lists, merge at end

### 3.3 Parallelize Candidate Testing

**Alternative approach:** Parallelize testing of candidate pairs

**Current code:**
```c
// After collecting all candidates
for (int i = 0; i < candidateList->count; i++) {
  CandidatePair* pair = &candidateList->pairs[i];
  if (intersect(pair->line1, pair->line2, timeStep)) {
    IntersectionEventList_appendNode(...);
  }
}
```

**Parallelized version:**
```c
cilk_for (int i = 0; i < candidateList->count; i++) {
  CandidatePair* pair = &candidateList->pairs[i];
  if (intersect(pair->line1, pair->line2, timeStep)) {
    IntersectionEventList_appendNode(&REDUCER_VIEW(eventListReducer), ...);
    REDUCER_VIEW(collisionCounter_reducer)++;
  }
}
```

**Benefits:**
- Embarrassingly parallel (no dependencies)
- Easy to implement
- Good load balancing

### 3.4 Parallelize Quadtree Build Phase (Advanced)

**Recursive depth-first build:**
```c
void buildQuadTreeRecursive(QuadNode* node, Line** lines, unsigned int numLines, ...) {
  if (shouldSubdivide(node, lines, numLines)) {
    // Subdivide into 4 children
    cilk_spawn buildQuadTreeRecursive(node->children[0], ...);
    cilk_spawn buildQuadTreeRecursive(node->children[1], ...);
    cilk_spawn buildQuadTreeRecursive(node->children[2], ...);
    buildQuadTreeRecursive(node->children[3], ...);
    cilk_sync;
  } else {
    // Insert lines into leaf node
    insertLinesIntoNode(node, lines, numLines);
  }
}
```

**Complexity:** Higher - need to ensure thread-safe node operations

**Recommendation:** Start with query/test phase parallelization, add build phase later if needed.

### 3.5 Update Makefile for Cilk

**File:** `Makefile`
```makefile
# Cilk compiler path
CILK_PATH = /opt/opencilk-2
CILK_CC = $(CILK_PATH)/bin/clang
CILK_CXX = $(CILK_PATH)/bin/clang++

# Cilk flags
CILK_FLAGS = -fopencilk -O3 -g

# Update CC variable
CC = $(CILK_CC)
CFLAGS += $(CILK_FLAGS)

# Link with Cilk runtime
LDFLAGS += -L$(CILK_PATH)/lib -lopencilk
```

**Build:**
```bash
make clean
make
```

---

## Phase 4: Profiling the Parallel Code

### 4.1 Setup Cilkscale

**Build with Cilkscale:**
```makefile
# Add Cilkscale flags
CILKSCALE_FLAGS = -fcilktool=cilkscale
CFLAGS += $(CILKSCALE_FLAGS)
```

**Run with Cilkscale:**
```bash
export CILK_NWORKERS=8
./screensaver -q 100 input/koch.in
```

**Analyze output:**
- Work: Total operations
- Span: Critical path length
- Parallelism: Work / Span
- Speedup: Expected vs actual

### 4.2 Measure Parallelism

**Vary parameters:**
```bash
# Test different maxDepth values
QUADTREE_MAXDEPTH=8 CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in
QUADTREE_MAXDEPTH=12 CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in
QUADTREE_MAXDEPTH=16 CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in

# Test different spawn cutoffs (if using recursive parallelization)
```

**Document:**
- Maximum parallelism achieved
- Optimal parameters
- Bottlenecks (serial sections)

---

## Phase 5: Race Detection

### 5.1 Setup Cilksan

**Build with Cilksan:**
```makefile
# Add Cilksan flags
CILKSAN_FLAGS = -fsanitize=cilk
CFLAGS += $(CILKSAN_FLAGS)
LDFLAGS += -L$(CILK_PATH)/lib -lcilksan
```

**Run with Cilksan:**
```bash
export CILK_NWORKERS=8
./screensaver -q 100 input/koch.in
```

**Check for races:**
- Cilksan will report determinacy races
- Fix any races found
- Re-test until race-free

### 5.2 Verify Correctness

**Compare collision counts:**
```bash
# Serial (no Cilk)
./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"

# Parallel
CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in | grep "Line-Line Collisions"
```

**Should match exactly!**

---

## Phase 6: Performance Tuning

### 6.1 Tune for 8 Cores

**Set workers:**
```bash
export CILK_NWORKERS=8
./screensaver -q 100 input/koch.in
```

**Measure speedup:**
```bash
# Serial time
time ./screensaver -q 100 input/koch.in

# Parallel time
time CILK_NWORKERS=8 ./screensaver -q 100 input/koch.in
```

**Calculate speedup:**
```
Speedup = Serial Time / Parallel Time
```

### 6.2 Optimize Spawn Cutoffs

**If using recursive parallelization:**
```c
void parallelFunction(...) {
  if (size < CUTOFF) {
    // Serial version
    serialFunction(...);
    return;
  }
  // Parallel version
  cilk_spawn parallelFunction(...);
  parallelFunction(...);
  cilk_sync;
}
```

**Tune CUTOFF:**
- Too small: Overhead dominates
- Too large: Not enough parallelism
- Optimal: Balance between overhead and parallelism

### 6.3 Load Balancing

**Check load balance:**
- Use Cilkscale to identify imbalanced sections
- Adjust work distribution
- Consider dynamic scheduling

---

## Implementation Checklist

### Phase 1: Profiling
- [ ] Profile serial program with `perf`
- [ ] Identify hotspots
- [ ] Document findings

### Phase 2: Reducers
- [ ] Implement `IntersectionEventList` reducer
- [ ] Implement collision counter reducer
- [ ] Test reducers for correctness
- [ ] Verify no races with Cilksan

### Phase 3: Parallelization
- [ ] Parallelize brute-force algorithm
- [ ] Parallelize quadtree query phase
- [ ] Parallelize candidate testing
- [ ] (Optional) Parallelize quadtree build phase
- [ ] Update Makefile for Cilk

### Phase 4: Profiling
- [ ] Setup Cilkscale
- [ ] Measure work, span, parallelism
- [ ] Vary parameters (maxDepth, cutoffs)
- [ ] Document maximum parallelism

### Phase 5: Race Detection
- [ ] Setup Cilksan
- [ ] Run race detector
- [ ] Fix any races
- [ ] Verify race-free

### Phase 6: Tuning
- [ ] Tune for 8 cores
- [ ] Measure speedup
- [ ] Optimize spawn cutoffs
- [ ] Load balancing
- [ ] Final performance benchmarks

---

## Expected Results

### Performance Targets

**Serial performance (baseline):**
- koch.in (3901 lines): ~X seconds
- mit.in (810 lines): ~Y seconds

**Parallel performance (8 cores):**
- Target: 4-8x speedup
- Ideal: Near-linear speedup (8x)
- Realistic: 5-7x (due to serial sections, overhead)

### Parallelism Analysis

**Expected parallelism:**
- Query phase: High (independent lines)
- Test phase: Very high (embarrassingly parallel)
- Build phase: Moderate (tree dependencies)
- Overall: Should achieve good parallelism

---

## Files to Modify

1. **`intersection_event_list.h`**
   - Add reducer type declaration
   - Add merge/identity functions

2. **`intersection_event_list.c`**
   - Implement merge/identity functions

3. **`collision_world.c`**
   - Add reducer declarations
   - Parallelize loops
   - Use reducer views

4. **`quadtree.c`** (optional)
   - Parallelize build phase
   - Parallelize query phase

5. **`Makefile`**
   - Add Cilk compiler
   - Add Cilk flags
   - Add Cilkscale/Cilksan support

---

## Testing Strategy

### Correctness Tests
```bash
# Test all inputs
for input in input/*.in; do
  echo "Testing: $input"
  SERIAL_COLL=$(./screensaver -q 100 "$input" 2>&1 | grep -oP '\d+ Line-Line Collisions')
  PARALLEL_COLL=$(CILK_NWORKERS=8 ./screensaver -q 100 "$input" 2>&1 | grep -oP '\d+ Line-Line Collisions')
  if [ "$SERIAL_COLL" = "$PARALLEL_COLL" ]; then
    echo "  ✅ CORRECT"
  else
    echo "  ❌ MISMATCH: Serial=$SERIAL_COLL, Parallel=$PARALLEL_COLL"
  fi
done
```

### Performance Tests
```bash
# Benchmark with different worker counts
for workers in 1 2 4 8; do
  echo "Workers: $workers"
  time CILK_NWORKERS=$workers ./screensaver -q 100 input/koch.in
done
```

### Race Detection
```bash
# Run Cilksan on all test cases
for input in input/*.in; do
  CILK_NWORKERS=8 ./screensaver -q 100 "$input"
done
```

---

## Documentation

### Write-up Requirements

1. **Strategy Description:**
   - Which phases parallelized
   - Why these phases
   - How reducers used

2. **Experimentation:**
   - Parameter tuning results
   - Performance measurements
   - Parallelism analysis

3. **Justification:**
   - Why chosen approach
   - Trade-offs considered
   - Alternatives evaluated

4. **Results:**
   - Speedup achieved
   - Maximum parallelism
   - Correctness verification

---

## Next Steps

1. **Start with Phase 1:** Profile serial program
2. **Implement Phase 2:** Convert to reducers
3. **Implement Phase 3:** Add parallelization
4. **Verify Phase 5:** Race detection
5. **Optimize Phase 6:** Performance tuning

**Priority:** Get basic parallelization working first, then optimize.

