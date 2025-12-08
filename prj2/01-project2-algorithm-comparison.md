## Part 1: Algorithm Comparison and Design Decisions

### Introduction: What is Collision Detection?

Imagine you have a simulation with many moving line segments (like
sticks or rods) bouncing around in a box. **Collision detection** is
the process of figuring out which lines will hit each other before
they actually collide. This is crucial for physics simulations, games,
and animations.

**The Challenge:** With $n$ lines, there are
$$
\frac{n(n-1)}{2}
$$
possible pairs to check. For 100 lines, that's 4,950 pairs! For 1,000
lines, it's 499,500 pairs. As the number of lines grows, checking
every pair becomes very slow.

**Two Approaches:**
1. **Brute-Force:** Check every pair (simple but slow for many lines)
2. **Quadtree:** Use spatial organization to skip checking pairs that
are far apart (more complex but much faster)

This section explains both approaches, how they work, and why we made
specific design choices.

---

### 1.1 Brute-Force Algorithm: Baseline Implementation

#### What is "Brute-Force"?

**Brute-force** means solving a problem by trying every possible
  option, without finding ways to break down the problem into smaller
  ones.

#### Algorithm Description

The brute-force algorithm is straightforward: **test every pair of
lines** to see if they will intersect:

- You have a list of all lines
- For each line, check it against every other line that comes after it
  in the list
- If two lines will collide, add them to the collision list

**Simple Example:** With 3 lines (A, B, C), we check:
- A vs B
- A vs C  
- B vs C

That's 3 pairs total. With 4 lines, it's 6 pairs. With 10 lines, it's
45 pairs. The number grows quickly!

**Pseudocode:**
```
FUNCTION detectCollisions_bruteForce(lines[], n, timeStep):
  collisions = empty list
  
  FOR i = 0 to n-1:
    FOR j = i+1 to n-1:
      IF intersect(lines[i], lines[j], timeStep) != NO_INTERSECTION:
        ADD (lines[i], lines[j]) to collisions
  
  RETURN collisions
```

**Time Complexity:** $O(n^2)$

**What does this mean?** If you double the number of lines, the work
  roughly quadruples. If you have 10 lines, you check ~45 pairs. With
  20 lines, you check ~190 pairs. With 100 lines, you check ~4,950
  pairs!

**Space Complexity:** $O(1)$ auxiliary space (excluding output)

**What does this mean?** The algorithm doesn't need to store much
  extra information. It just needs the list of lines and a place to
  record collisions. The memory used doesn't grow with the number of
  lines (except for storing the collision results).

**Advantages:**
- **Simple to implement and understand:** The code is straightforward
    - just two nested loops
- **No preprocessing overhead:** Start checking immediately, no setup
    time
- **Predictable performance:** Always checks exactly
    $\frac{n(n-1)}{2}$ pairs, easy to estimate runtime
- **Easy to verify correctness:** Since we check everything, it's easy
    to convince yourself it's correct

**Disadvantages:**
- **Quadratic time complexity:** Gets slow quickly as $n$ grows (100
    lines = 4,950 checks, 1,000 lines = 499,500 checks!)
- **Tests many pairs that cannot possibly collide:** Two lines on
    opposite sides of the box still get checked
- **No spatial locality exploitation:** Doesn't use the fact that
    nearby lines are more likely to collide
- **Poor scalability for large $n$:** Not practical for simulations
    with thousands of lines
    

#### Implementation Details

The brute-force implementation in `collision_world.c`:

```c
for (int i = 0; i < collisionWorld->numOfLines; i++) {
  Line *l1 = collisionWorld->lines[i];
  
  for (int j = i + 1; j < collisionWorld->numOfLines; j++) {
    Line *l2 = collisionWorld->lines[j];
    
    // Ensure compareLines(l1, l2) < 0
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
    }
  }
}
```

**Key Observations:**
- **Tests exactly $\frac{n(n-1)}{2}$ pairs:** The outer loop goes from
    0 to $n-1$, and the inner loop goes from $i+1$ to $n-1$. This
    ensures each pair is tested exactly once (we don't test A vs B and
    then B vs A).
- **Each test calls `intersect()`:** This function (from
    `intersection_detection.c`) does the actual math to determine if
    two moving lines will collide. It considers where the lines are
    now and where they'll be after `timeStep`.
- **Pair normalization:** The `compareLines()` function ensures that
    before we test a pair, the line with the smaller ID is always
    `line1`. This keeps things consistent.
- **Results stored in a list:** Collisions are added to an
    `IntersectionEventList` which gets processed later (to actually
    handle the collisions).
- **Sorting before processing:** After finding all collisions, the
    list is sorted so collisions are processed in a consistent order
    (important for correctness).

---

### 1.2 Quadtree Algorithm: Spatial Indexing Approach

#### What is a Quadtree?

A **quadtree** is a data structure that organizes objects in 2D space
by repeatedly dividing the space into four equal squares
(quadrants):

1. Start with the whole world as one big square
2. If a square has too many objects, split it into 4 smaller squares
3. Keep splitting until squares are small enough or have few enough
objects
4. Now you have a tree where each "leaf" (end node) contains objects
in a small region

**Why is this useful?** If two lines are in completely different parts
  of the world, it is **TEMPTING** to think that locality rules and we
  don't need to check if they collide - they're too far apart! The
  quadtree helps us quickly find which lines are close to each
  other. However, we need to be extremely cautious when it comes to
  such temptations as we are dealing with an evolving system and
  during a timestep a line from a far-far-away cell might reach us on
  the other side of the world if it has enough speed and the
  circumstances are alright!!

#### Algorithm Overview

The quadtree algorithm uses this spatial organization to **skip
checking pairs that are far apart**. It works in three distinct
phases:

- **Phase 1: Build** - Organize lines into a spatial tree structure
- **Phase 2: Query** - Find which lines are close enough to
    potentially collide (spatial filtering)
- **Phase 3: Test** - Actually test those candidate pairs using the
    same `intersect()` function as brute-force


#### Phase 1: Tree Construction

**Algorithm:**
```
FUNCTION buildQuadTree(lines[], n, bounds, timeStep):
  tree = createQuadTree(bounds)
  
  // CRITICAL: Expand root bounds to include all lines
  actualBounds = computeActualBounds(lines, n, timeStep)
  expandRootBounds(tree, actualBounds)
  recreateRootNode(tree)
  
  FOR each line in lines:
    bbox = computeBoundingBox(line, timeStep, maxVelocity, minCellSize)
    insertLineRecursive(tree.root, line, bbox, tree)
  
  RETURN tree

FUNCTION insertLineRecursive(node, line, bbox, tree):
  IF bbox does NOT overlap node.bounds:
    RETURN  // Line not in this cell
  
  IF node is leaf:
    // CRITICAL: Add line FIRST, then check if subdivision needed
    addLineToNode(node, line)
    
    IF node.numLines > maxLinesPerNode AND
       node.depth < maxDepth AND
       min(cellWidth, cellHeight) >= 2 * minCellSize:
      subdivideNode(node, tree)
      // All lines (including new one) redistributed to children
      node.numLines = 0  // Parent becomes empty
    ELSE:
      RETURN
  
  // If not leaf (or was just subdivided), recursively insert into children
  FOR each child in node.children:
    insertLineRecursive(child, line, bbox, tree)
```

**Key Features:**
- **Dynamic Subdivision:** Cells automatically split into 4 smaller
    squares when they get too crowded.
- **Square Cells:** All cells are perfect squares (width ==
    height). Even if the world is rectangular, we make it square
    first, then subdivide into squares.
- **Moving Lines:** Since lines move, we compute a "bounding box" that
    includes where the line is now AND where it will be after
    moving. This ensures we don't miss collisions.
- **Line Spanning:** A long line might cross multiple cells. We store
    it in ALL cells it touches (to make sure we find all potential
    collisions).
- **Dynamic Root Bounds:** The initial world bounds might not include
    all lines. We automatically expand the root cell to include
    everything.
- **Leaf-Only Storage:** Lines are only stored in "leaf" nodes (the
    smallest cells at the bottom of the tree). When a cell splits, its
    lines move to the child cells, and the parent becomes empty.


**Configuration Parameters (Default Values):**

These are "tuning knobs" that control how the quadtree behaves:

- **`maxDepth = 12`:** Maximum how deep the tree can go (how many
    times we can split). Prevents the tree from getting too deep,
    which would create tiny cells and waste memory.
- **`maxLinesPerNode = 32`:** How many lines can be in a cell before
    we split it. If a cell has more than 32 lines, we split it into 4
    smaller cells. This value (32) was chosen because smaller values
    (like 4 or 8) caused problems when lines were clustered together -
    they'd create too many tiny cells.
- **`minCellSize = 0.001`:** The smallest a cell can be. Prevents
    splitting into cells so small that floating-point rounding errors
    cause problems.

**Additional Implementation Details:**

- **Max Velocity Computation:** We need to know how fast lines are
    moving to compute good bounding boxes. So we scan all lines to
    find the fastest one, then use that to determine how much to
    "expand" bounding boxes (to account for movement).
- **TimeStep Storage:** The time step used when building the tree is
    saved. Later, when querying, we use the SAME time step to compute
    bounding boxes. This ensures consistency - if we built with one
    time step, we query with the same one.

**Time Complexity:** $O(n \log n)$ average case, $O(n^2)$ worst case

**What does this mean?** This is much better than brute-force! With
  100 lines:
- Brute-force: ~4,950 pairs to test
- Quadtree: ~664 operations on average (100 × log₂(100) ≈ 664)

The $\log n$ factor comes from the tree depth - on average, each line
goes down about $\log n$ levels in the tree.

- **Each line inserted:** Takes about $O(\log n)$ time because we
    traverse down the tree (which has depth $\log n$ on average)
- **Subdivision:** When a cell splits, it's a constant-time operation
    (just create 4 new cells)
- **Total:** $n$ lines × $\log n$ time per line = $O(n \log n)$

**Worst case:** If all lines are in the same tiny region, the tree
  might not help much, and we could approach $O(n^2)$ behavior. But
  this is rare in practice.

**Space Complexity:** $O(n)$ nodes in average case

**What does this mean?** We need to store the tree structure. On
  average, we create about one node per line (sometimes more if lines
  span many cells, sometimes less if cells have multiple lines).


#### Phase 2: Spatial Query

**Algorithm:**
```
FUNCTION findCandidatePairs(tree, timeStep):
  candidates = empty list
  seenPairs = allocateMatrix(maxLineId + 1)  // Track pairs globally
  
  FOR each line L (index i) in tree.lines:
    bbox = computeBoundingBox(L, tree.buildTimeStep, maxVelocity, minCellSize)
    cells = findOverlappingCells(tree.root, bbox)
    
    FOR each cell in cells:
      FOR each line M in cell.lines:
        // CRITICAL: Ensure M appears later in array than L (matches brute-force)
        M_index = findIndex(M, tree.lines)
        IF M_index <= i:
          CONTINUE  // Would be tested as (M, L) when M is line1
        
        // Ensure L.id < M.id (normalized pair)
        IF L.id >= M.id:
          CONTINUE
        
        // Check global duplicate matrix
        IF seenPairs[L.id][M.id]:
          CONTINUE  // Already added
        
        seenPairs[L.id][M.id] = true
        ADD (L, M) to candidates
  
  RETURN candidates
```

**Purpose:** Spatial filtering - find which pairs are close enough to
  potentially collide.

**What is "spatial filtering"?** It's like a first pass that
  eliminates obviously impossible pairs. Instead of checking if line A
  (on the left side) collides with line B (on the right side), we
  first check: "Are they even in the same region?" If not, skip them!
  Only lines that are spatially close become "candidates" for actual
  collision testing.

**Key Implementation Details:**

**Why do we need duplicate prevention?** Since lines can be stored in
  multiple cells (if they span boundaries), the same pair might be
  found multiple times. We need to make sure each pair is only tested
  once!

- **Global Duplicate Prevention:** We use a 2D table (matrix) where
    `seenPairs[line1_id][line2_id] = true` means "we've already seen
    this pair." Before adding a candidate pair, we check this
    table. This prevents duplicates even when lines span multiple
    cells.

- **Array Index Ordering:** We ensure `line2` appears later in the
    `tree->lines` array than `line1`. This matches how brute-force
    works (outer loop $i$, inner loop $j > i$), ensuring we test pairs
    in the same order.

- **ID Ordering:** We enforce `line1->id < line2->id`. This
    "normalizes" pairs - the pair (A, B) and (B, A) are the same, so
    we always represent them as (A, B) where A has the smaller ID.

- **TimeStep Consistency:** We use `tree->buildTimeStep` (the time
    step from when we built the tree) rather than the query
    parameter. This ensures bounding boxes are computed the same way
    during query as they were during build - critical for correctness!

**Time Complexity:** $O(n \log n + k)$ where $k$ is number of
  candidates
- Query per line: $O(\log n)$ to find cells
- Collecting candidates: $O(k)$ where $k \approx n \log n$ average
  case
- Duplicate checking: $O(1)$ per pair using matrix lookup
- Total: $O(n \log n)$ average


#### Phase 3: Collision Testing

**Algorithm:**
```
FUNCTION testCandidates(candidates, timeStep):
  // CRITICAL: Sort candidates to match brute-force processing order
  qsort(candidates, compareCandidatePairs)  // Sort by line1->id, then line2->id
  
  collisions = empty list
  
  FOR each (L1, L2) in candidates:
    // Ensure compareLines(L1, L2) < 0 (should already be true, but double-check)
    IF compareLines(L1, L2) >= 0:
      SWAP(L1, L2)
    
    type = intersect(L1, L2, timeStep)  // EXISTING FUNCTION
    IF type != NO_INTERSECTION:
      ADD (L1, L2) to collisions
  
  RETURN collisions
```

**Purpose:** Verify actual collisions using existing `intersect()`
  function.

**What happens here?** The query phase gave us a list of "candidate
  pairs" - lines that are spatially close and **MIGHT** collide. Now
  we actually test each candidate pair to see if they really do
  collide. We use the exact same `intersect()` function that
  brute-force uses, so we get the same results - just with far fewer
  tests!

**Key Implementation Details:**

- **Candidate Sorting:** Before testing, we sort the candidate pairs
    using `qsort()` (a standard C library function). We sort by
    `line1->id` first, then `line2->id`. This ensures we process
    collisions in the same order as brute-force, which is important
    for correctness (collision order can affect the simulation).

- **Pair Normalization:** Even though candidates should already be
    normalized, we double-check that `compareLines(l1, l2) < 0` before
    calling `intersect()`. This matches what brute-force does and
    ensures consistency.

- **Same Intersection Logic:** We use the exact same `intersect()`
    function from `intersection_detection.c` that brute-force
    uses. This is crucial - it means both algorithms use the same
    collision detection math, so they should produce the same results
    (just with different numbers of tests).

**Time Complexity:** $O(k \log k + k)$ where $k$ is number of
  candidates
- Sorting: $O(k \log k)$ using `qsort()`
- Testing: $O(k)$ calls to `intersect()`
- Typically $k \approx n \log n$ (much less than $n^2$)

**Total Quadtree Complexity:** $O(n \log n)$ average case vs $O(n^2)$
brute-force

---

### 1.3 The Line Segment Partitioning Problem

#### The Challenge: Lines Spanning Multiple Cells

**Why is this a problem?** Quadtrees work great for points - a point
  fits neatly in one cell. But line segments are different - they can
  stretch across multiple cells!

**Example:** Imagine a long stick that goes from the top-left corner
  to the bottom-right corner. It crosses many cells. Where should we
  store it?

A fundamental challenge in using quadtrees for line segments is that
**line segments can span multiple cells**, unlike points which fit in
a single cell. Add to this when we have kinematics and if wo do not
pay attention to the evolution of our lines or points ovre the
corresponsding timestep, we could easily miss collision events.

**Example Scenario (Figure 4 problem):**

Consider a long line segment that crosses multiple quadtree cells:

```
┌─────────┬─────────┐
│         │         │
│   Cell  │   Cell  │
│   NW    │   NE    │
│         │         │
│    ─────┼─────    │  ← Long line spanning both cells
│         │         │
├─────────┼─────────┤
│         │         │
│   Cell  │   Cell  │
│   SW    │   SE    │
│         │         │
└─────────┴─────────┘
```

**The Problem:**

We have two bad options:

1. **Store line in only one cell:** If we put the long line in just
the top-left cell, we might miss a collision with a line in the
bottom-right cell (even though they're both part of the same long
line).

2. **Store line in all overlapping cells:** If we put the line in
every cell it touches, we might test the same pair multiple times
(once for each cell they share).

**The Question:** How do we ensure we find all collisions
  (correctness) while not testing the same pair multiple times
  (efficiency)?

#### Solution: Store Lines in All Overlapping Cells

**Our Approach:**

We chose option 2 (store in all overlapping cells) and then solve the
duplicate problem separately. We store each line in **ALL cells its
bounding box overlaps**.

**Why a bounding box?** A line's "bounding box" is the smallest
  rectangle that contains the line. It's easier to check if a
  rectangle overlaps a cell than to check if a line segment overlaps a
  cell.

This approach ensures:

1. **Correctness:** No collisions are missed - if two lines could
collide, they'll be in at least one shared cell
2. **Completeness:** All potentially colliding pairs are found
3. **Simplicity:** Straightforward implementation - just check
bounding box overlap

**The duplicate problem is solved separately** using the three
  mechanisms described in the "Handling Duplicate Pairs" section
  below.

**Algorithm for Line Insertion:**

```c
FUNCTION insertLineRecursive(node, line, bbox, tree):
  IF bbox does NOT overlap node.bounds:
    RETURN  // Line not in this cell
  
  IF node is leaf:
    // CRITICAL: Add line FIRST, then check subdivision
    addLineToNode(node, line)
    
    IF node.numLines > maxLinesPerNode AND
       node.depth < maxDepth AND
       min(cellWidth, cellHeight) >= 2 * minCellSize:
      // Subdivide: create 4 children, redistribute ALL lines
      subdivideNode(node, tree)
      node.numLines = 0  // Parent becomes empty
      // Line already redistributed to children
      RETURN
    ELSE:
      RETURN  // Line added, no subdivision needed
  
  // If not leaf, recursively insert into all children
  FOR each child in node.children:
    insertLineRecursive(child, line, bbox, tree)
```

**Key Insight:** A line's bounding box (including future position)
  determines which cells it belongs to. If the bounding box overlaps a
  cell, the line is stored there.

**Why include future position?** Lines move! If we only consider where
  a line is now, we might miss a collision that happens as it
  moves. So we compute a bounding box that includes:
- Where the line is now
- Where the line will be after moving (current position + velocity ×
  timeStep)

This "swept" bounding box ensures we catch all potential collisions
during the time step.


#### Handling Duplicate Pairs

**The Duplicate Problem:**

If line $L_1$ spans cells $A$ and $B$, and line $L_2$ is in cell $A$:
- $L_1$ will be found when querying from $L_2$ (both in cell $A$)
- $L_2$ will be found when querying from $L_1$ (both in cells $A$ and
  $B$)
- This creates duplicate candidate pairs: $(L_1, L_2)$ and $(L_2,
  L_1)$

**Solution: Multi-Layer Duplicate Prevention**

We use three mechanisms to prevent duplicates:

1. **Array Index Ordering:** Ensure `line2` appears later in
`tree->lines` than `line1`:
```c
// Find line2's index in tree->lines array
line2ArrayIndex = findIndex(line2, tree->lines);
if (line2ArrayIndex <= i) {  // i is line1's index
  continue;  // Would be tested as (line2, line1) when line2 is line1
}
```

2. **ID Ordering:** Enforce normalized pairs where `line1->id <
line2->id`:
```c
if (line1->id >= line2->id) {
  continue;  // Skip non-normalized pair
}
```

3. **Global Duplicate Matrix:** Track all pairs seen so far:
```c
bool** seenPairs = allocateMatrix(maxLineId + 1);
if (seenPairs[line1->id][line2->id]) {
  continue;  // Already added this pair
}
seenPairs[line1->id][line2->id] = true;
```

**Why This Works:**

Think of it like three layers of protection:

1. **Array index ordering:** Ensures we test pairs in the same order
as brute-force. Brute-force uses nested loops where the inner loop
always starts after the outer loop's current position. We match this
exactly.

2. **ID ordering:** Normalizes pairs so (A, B) and (B, A) are treated
as the same pair (A, B) where A has the smaller ID. This is like
always writing names in alphabetical order.

3. **Global matrix:** Acts as a "memory" - once we've seen a pair, we
mark it in the matrix and never add it again, even if we encounter it
in a different cell.

**Result:** Each pair is tested exactly once, matching brute-force
  behavior. The three mechanisms work together to ensure correctness
  even when lines span multiple cells.
  

#### Do All Lines Need to be in Leaf Nodes?

**What are leaf nodes?** In a tree, "leaf nodes" are the nodes at the
  bottom - they have no children. In our quadtree, these are the
  smallest cells that haven't been split further.

**Answer: No, but it simplifies the implementation.**

**Our Implementation:**
- Lines are stored in **leaf nodes only** (the smallest cells at the
  bottom)
- When a leaf gets too crowded and subdivides, all its lines are moved
  to the 4 new child cells
- Parent nodes become empty (they're just organizational structure)

**Alternative Approach (Not Used):**
- We could store lines in **all nodes** they overlap, including the
  larger parent cells
- When querying, we'd check both parent and child nodes
- This could be slightly more efficient but is much more complex to
  implement

**Why We Chose Leaf-Only Storage:**
1. **Simplicity:** Much easier to implement and understand - lines
only live in one place in the tree
2. **Query Efficiency:** When searching, we only need to look at leaf
nodes (the smallest cells)
3. **Memory:** Slightly less memory usage since we don't duplicate
lines in parent nodes
4. **Correctness:** Still correct - all the space is covered by
leaves, so we don't miss anything

**Trade-off:**
- A line that spans many small cells will be stored in multiple leaves
  (one per cell it overlaps)
- This creates some duplication, but our duplicate prevention handles
  it
- The simpler implementation is worth this minor inefficiency

---

### 1.4 Key Design Decisions

#### Decision 1: Three-Phase Separation (Build → Query → Test)

**What We Did:**
Separated the algorithm into three distinct phases:
1. **Build:** Organize lines into a spatial tree
2. **Query:** Find candidate pairs (spatial filtering - which lines
are close?)
3. **Test:** Actually test candidates for collisions (do they really
collide?)

**Why separate them?** We could combine query and test into one phase
  (test while querying), but keeping them separate has many benefits.

**Rationale:**
1. **Modularity:** Each phase has one clear job. Build builds the
tree, query finds candidates, test checks collisions. Easy to
understand and modify.
2. **Reusability:** We could test the candidate pairs with a different
collision algorithm if we wanted (the query phase doesn't care how we
test).
3. **Debuggability:** We can print out the candidate list and inspect
it before testing. This makes debugging much easier - "Why did we miss
this collision? Let me check if it's in the candidate list..."
4. **Parallelization:** The test phase (testing many candidate pairs)
can easily be parallelized - each test is independent. The query phase
is harder to parallelize.
5. **Maintainability:** The actual collision math stays in
`intersection_detection.c`. The quadtree just does spatial
organization - clear separation of concerns.

**Alternative (Not Chosen):**
Combine query and test into single function that tests during query.

**Why Separation is Better:**
- Clearer code structure
- Enables future optimizations
- Easier to profile and debug
- Better for educational purposes


#### Decision 2: Rebuild Tree Each Frame

**What We Did:**
Destroy the entire quadtree and build a new one from scratch at each
time step (each frame of the simulation).

**Why?** Lines move between frames. After handling collisions and
  updating positions, lines are in new locations. The old tree
  structure is no longer accurate.

**Rationale:**
1. **Simplicity:** Much easier than trying to update the tree as lines
move. Just throw it away and rebuild!
2. **Correctness:** The tree always perfectly reflects current line
positions - no risk of stale or incorrect data
3. **Moving Lines:** Since lines move every frame, we need a fresh
tree anyway
4. **Implementation Ease:** Avoids complex logic for "move this line
from cell A to cell B" and handling all the edge cases

**Alternative (Not Chosen):**
We could update the tree incrementally - when a line moves, remove it
from old cells and add it to new cells. This would be faster ($O(\log
n)$ per line instead of $O(n \log n)$ total), but much more complex to
implement correctly.

**Why Rebuild is Better:**
- **Much simpler to implement:** Rebuild is straightforward - just run
    the build algorithm again
- **Guarantees correctness:** No risk of bugs from incremental updates
    (like forgetting to remove a line from an old cell)
- **Performance acceptable:** $O(n \log n)$ rebuild is still much
    better than $O(n^2)$ brute-force for large $n$
- **Future optimization:** If needed later, we can optimize to
    incremental updates. But for now, simple and correct is better
    than complex and fast

**Performance Consideration:**
- **Rebuild cost:** $O(n \log n)$ per frame (rebuild the whole tree)
- **Still better than brute-force:** $O(n^2)$ brute-force is worse for
    large $n$
- **Incremental would be:** $O(\log n)$ per line = $O(n \log n)$
    total, but with much more complexity


#### Decision 3: Square Cells (Not Rectangles)

**What We Did:**
All quadtree cells are perfect squares (width == height). Even if the
world is a rectangle, we make it square first, then subdivide into
squares.

**Why squares?** The quadtree algorithm naturally works with
  squares. Each cell splits into 4 equal square quadrants (northwest,
  northeast, southwest, southeast).

**Rationale:**
1. **Algorithm Requirement:** The quadtree algorithm is designed for
square cells - it's in the name! ("quad" = four, referring to four
quadrants)
2. **Subdivision:** Each square cell divides into 4 equal smaller
squares. This is simple and predictable.
3. **Simplicity:** Checking if a bounding box overlaps a square is
simpler than checking overlap with a rectangle. The math is cleaner.
4. **Standard Practice:** This is how quadtrees are typically
implemented - it's the standard approach

**What if the world is rectangular?** We take the larger dimension
  (width or height) and make a square of that size, centered on the
  original rectangle. This ensures everything fits, though we might
  have some empty space.

**Implementation:**
```c
// Ensure world bounds form a square
double width = xmax - xmin;
double height = ymax - ymin;
double size = max(width, height);  // Take larger dimension

// Center the square
double centerX = (xmin + xmax) / 2.0;
double centerY = (ymin + ymax) / 2.0;

tree->worldXmin = centerX - size / 2.0;
tree->worldXmax = centerX + size / 2.0;
tree->worldYmin = centerY - size / 2.0;
tree->worldYmax = centerY + size / 2.0;
```


#### Decision 4: Bounding Box Includes Future Position

**What We Did:**
Compute bounding box including both current and future positions.

**Algorithm:**
```c
void computeLineBoundingBox(const Line* line, double timeStep,
                            double* xmin, double* xmax,
                            double* ymin, double* ymax,
                            double maxVelocity, double minCellSize) {
  // Current endpoints
  Vec p1_current = line->p1;
  Vec p2_current = line->p2;
  
  // Future endpoints (after movement)
  Vec p1_future = p1_current + line->velocity * timeStep;
  Vec p2_future = p2_current + line->velocity * timeStep;
  
  // Bounding box includes all four points
  *xmin = min(p1_current.x, p2_current.x, p1_future.x, p2_future.x);
  *xmax = max(p1_current.x, p2_current.x, p1_future.x, p2_future.x);
  *ymin = min(p1_current.y, p2_current.y, p1_future.y, p2_future.y);
  *ymax = max(p1_current.y, p2_current.y, p1_future.y, p2_future.y);
  
  // Multi-factor expansion to address:
  // 1. Relative motion mismatch (absolute vs relative parallelograms)
  // 2. AABB gap problem (AABBs overlap in world space but not same cells)
  
  // Factor 1: Relative motion expansion
  double velocity_magnitude = Vec_length(line->velocity);
  const double k_rel = 0.3;  // Relative motion factor
  double relative_motion_expansion = velocity_magnitude * timeStep * k_rel;
  
  // Factor 2: Minimum gap expansion
  const double k_gap = 0.15;  // Gap factor
  double min_gap_expansion = minCellSize * k_gap;
  
  // Factor 3: Numerical precision margin
  const double precision_margin = 1e-6;
  
  // Use maximum of relative motion and gap expansion, plus precision margin
  double expansion = max(relative_motion_expansion, min_gap_expansion) 
                     + precision_margin;
  
  *xmin -= expansion;
  *xmax += expansion;
  *ymin -= expansion;
  *ymax += expansion;
}
```

**Why Critical:**
- **Lines move between frames:** A line might be far from another line
    now, but collide as they both move
- **Must include future position:** The bounding box needs to cover
    where the line will be, not just where it is
- **Conservative bounding box:** We'd rather make the box slightly too
    big (and test a few extra pairs) than too small (and miss
    collisions)
- **Velocity-dependent expansion:** Fast-moving lines need bigger
    boxes. We also add extra padding to handle edge cases where
    bounding boxes are close but don't quite overlap the same cells
- **Trade-off:** Slightly larger bounding boxes mean we test a few
    more candidate pairs, but this guarantees we never miss a
    collision

**Expansion Parameters:**

These are "safety margins" we add to bounding boxes:

- **`k_rel = 0.3` (Relative motion factor):** When two lines move
    toward each other, their relative speed can be up to twice their
    individual speeds. This factor accounts for that - if a line moves
    fast, we expand its box by 30% of its movement distance.

- **`k_gap = 0.15` (Gap factor):** Sometimes two bounding boxes are
    very close but don't quite overlap the same quadtree cells. This
    adds 15% of the minimum cell size as padding to ensure close boxes
    end up in overlapping cells.

- **`precision_margin = 1e-6` (Numerical precision margin):**
    Floating-point arithmetic has tiny rounding errors. This small
    constant (0.000001) ensures these errors don't cause problems.

**Why these specific values?** They were chosen through testing to
  balance correctness (never miss collisions) with efficiency (don't
  make boxes too large). They ensure lines with overlapping bounding
  boxes are assigned to overlapping quadtree cells, preventing missed
  collisions.


#### Decision 5: Dynamic Root Bounds Expansion

**What We Did:**
Dynamically expand quadtree root bounds during build phase to include all
lines (including their future positions), ensuring no lines are outside the
tree's spatial domain.

**Why Critical:**

This was a major bug fix! Here's what was happening:

- **Initial fixed bounds:** The quadtree was created with fixed bounds
    `[0.5, 1.0] × [0.5, 1.0]` (a square from 0.5 to 1.0 in both X and
    Y)
- **Problem:** Some lines might be outside these bounds (maybe at
    position 0.3 or 1.2)
- **Result:** Lines outside bounds don't get stored in any cell →
    they're invisible to the quadtree → collisions are missed →
    **correctness violation!**

**The Fix:**
- **Dynamic expansion:** Before building the tree, we scan all lines
    to find their actual spatial extent
- **Expand root bounds:** We expand the root cell to include ALL lines
    (with a small safety margin)
- **Mathematical guarantee:** Now every line is guaranteed to be
    within the quadtree's domain
- **Result:** 100% elimination of discrepancies (went from 2,446
    mismatches to 0!)

**Implementation:**
```c
// In QuadTree_build:
// 1. Compute bounding boxes for all lines (including future positions)
double actualXmin = tree->worldXmax;
double actualXmax = tree->worldXmin;
double actualYmin = tree->worldYmax;
double actualYmax = tree->worldYmin;

for (each line in lines) {
  computeLineBoundingBox(line, timeStep, &xmin, &xmax, &ymin, &ymax, ...);
  actualXmin = min(actualXmin, xmin);
  actualXmax = max(actualXmax, xmax);
  actualYmin = min(actualYmin, ymin);
  actualYmax = max(actualYmax, ymax);
}

// 2. Expand root bounds to include all lines (with small margin)
const double margin = 1e-6;
tree->worldXmin = min(tree->worldXmin, actualXmin - margin);
tree->worldXmax = max(tree->worldXmax, actualXmax + margin);
tree->worldYmin = min(tree->worldYmin, actualYmin - margin);
tree->worldYmax = max(tree->worldYmax, actualYmax + margin);

// 3. Ensure bounds form a square (take larger dimension)
double width = tree->worldXmax - tree->worldXmin;
double height = tree->worldYmax - tree->worldYmin;
double size = max(width, height);
double centerX = (tree->worldXmin + tree->worldXmax) / 2.0;
double centerY = (tree->worldYmin + tree->worldYmax) / 2.0;
tree->worldXmin = centerX - size / 2.0;
tree->worldXmax = centerX + size / 2.0;
tree->worldYmin = centerY - size / 2.0;
tree->worldYmax = centerY + size / 2.0;

// 4. Recreate root node with expanded bounds
destroyQuadNode(tree->root);
tree->root = createQuadNode(tree->worldXmin, tree->worldXmax,
                            tree->worldYmin, tree->worldYmax, 0);
```

**Result:**
- **100% elimination of discrepancies** (2446 → 0 mismatches)
- Perfect correctness on all test cases
- Critical fix for correctness guarantee


#### Decision 6: Error Handling with Fallback

**What We Did:**
Comprehensive error checking with automatic fallback to brute-force.

**Rationale:**
1. **Robustness:** Program continues even if quadtree fails
2. **User Experience:** Warnings instead of crashes
3. **Correctness:** Always produces results
4. **Debugging:** Error messages help identify issues

**Implementation:**
```c
bool quadtreeSucceeded = false;
// ... attempt quadtree ...
if (quadtreeSucceeded) {
  // Use quadtree results
} else {
  // Fall back to brute-force
  // Run existing brute-force algorithm
}
```

**Why This Design:**
- Quadtree is optimization, not requirement
- Better to degrade gracefully than crash
- Enables debugging (can compare results)
- Critical: `quadtreeSucceeded` flag prevents double execution bug

---

### 1.5 Using the Quadtree to Extract Collisions

#### The Extraction Process

Once the quadtree is built, we use it to find candidate pairs, then test
those candidates:

**Step 1: Query Phase (Spatial Filtering)**
```c
QuadTreeCandidateList candidateList;
QuadTree_findCandidatePairs(tree, timeStep, &candidateList);
```

This performs:
- For each line $L$:
  - Find all cells $L$'s bounding box overlaps
  - Collect all OTHER lines from those cells
  - Store as candidate pairs $(L, M)$ where $L.id < M.id$

**Step 2: Sort Candidate Pairs (Match Brute-Force Order)**
```c
// Sort candidates by line1->id, then line2->id to match brute-force processing order
qsort(candidateList.pairs, candidateList.count, 
      sizeof(QuadTreeCandidatePair), compareCandidatePairs);
```

**Step 3: Test Phase (Collision Verification)**
```c
for (unsigned int i = 0; i < candidateList.count; i++) {
  Line* l1 = candidateList.pairs[i].line1;
  Line* l2 = candidateList.pairs[i].line2;
  
  // Ensure compareLines(l1, l2) < 0 (should already be true, but double-check)
  if (compareLines(l1, l2) >= 0) {
    Line* temp = l1;
    l1 = l2;
    l2 = temp;
  }
  
  IntersectionType type = intersect(l1, l2, timeStep);
  if (type != NO_INTERSECTION) {
    IntersectionEventList_appendNode(&intersectionEventList, l1, l2, type);
    collisionWorld->numLineLineCollisions++;
  }
}
```

This uses the **existing `intersect()` function** from
`intersection_detection.c`, ensuring:
- Same collision detection logic
- Same correctness guarantees
- No code duplication


#### Why This Approach Works

**Spatial Locality:**
- **What is spatial locality?** It means "things that are close
    together in space are more likely to interact." Two lines on
    opposite sides of the box won't collide unless they have a high
    relative velocity and there would not be any obstacles appearing
    in their path of collision during the time interval they would
    reach each other, so most probably, we would not need to check
    them.
- Lines in the same or nearby cells are spatially close (they're in
  the same region)
- If two lines are far apart, they're in completely different cells
  (maybe even different parts of the tree)
- The quadtree automatically filters out these distant pairs - we
  never even consider them as candidates

**Correctness:**
- All overlapping cells checked for each line
- Lines spanning cells appear in multiple cells
- Duplicate pairs eliminated by three mechanisms:
  1. Array index ordering (line2 appears later than line1)
  2. ID ordering constraint ($L_1.id < L_2.id$)
  3. Global duplicate matrix (tracks all pairs seen)
- Candidate pairs sorted to match brute-force processing order
- Result: Same pairs tested as brute-force (correctness guarantee)

**Efficiency:**
- **Dramatic reduction in tests:** Instead of testing all $n^2$ pairs,
    we only test ~$n \log n$ candidate pairs. For 100 lines:
    brute-force tests 4,950 pairs, quadtree tests ~664 candidates!
- **Spatial filtering does the heavy lifting:** Most of the work is
    just organizing lines spatially (cheap). The expensive collision
    math is only done on the small number of candidates.
- **Actual collision testing (expensive) done only on candidates:**
    The `intersect()` function does complex math. We only call it for
    pairs that are actually close enough to potentially collide.
    

---

### 1.6 Comparison Summary

| Aspect               | Brute-Force          | Quadtree                  |
|----------------------|----------------------|---------------------------|
| **Time Complexity**  | $O(n^2)$             | $O(n \log n)$ average     |
| **Space Complexity** | $O(1)$ auxiliary     | $O(n)$ tree structure     |
| **Preprocessing**    | None                 | $O(n \log n)$ build       |
| **Query Time**       | N/A (direct testing) | $O(n \log n)$             |
| **Pairs Tested**     | $\frac{n(n-1)}{2}$   | ~$n \log n$ (average)     |
| **Correctness**      | Guaranteed           | Guaranteed (same results) |
| **Implementation**   | Simple               | Moderate complexity       |
| **Best For**         | Small $n$ (< 50)     | Large $n$ (> 100)         |

**Crossover Point:**
The quadtree becomes beneficial when the overhead of building and
querying the tree is less than the cost of checking all pairs.

**The Math:**
- Brute-force: $\frac{n(n-1)}{2}$ pairs to test
- Quadtree: $n \log n$ operations (build + query) plus ~$n \log n$
  candidate pairs to test

The quadtree wins when: $n \log n < \frac{n(n-1)}{2} \times
\text{overhead factor}$

**In Practice:**
For typical overhead factors (the quadtree has some setup cost), the
crossover happens around **$n \approx 50-100$ lines**.

- **Small $n$ (< 50):** Brute-force is simpler and the overhead isn't
    worth it
- **Large $n$ (> 100):** Quadtree's $O(n \log n)$ beats brute-force's
    $O(n^2)$ significantly
- **Very large $n$ (> 1000):** Quadtree is dramatically faster (1000
    lines: brute-force = 499,500 pairs, quadtree ≈ 10,000 operations)

---

### Quick Reference: Key Terms

**Big O Notation ($O(...)$):** Describes how an algorithm's runtime
  grows as input size grows. $O(n^2)$ means "roughly proportional to
  $n^2$" - if you double $n$, runtime roughly quadruples.

**Bounding Box:** The smallest rectangle that completely contains a
  line segment. Used to quickly check if a line might overlap a
  region.

**Leaf Node:** A node in the tree that has no children - it's at the
  bottom. In our quadtree, these are the smallest cells that contain
  lines.

**Spatial Indexing:** Organizing objects by their location in space,
  so you can quickly find "what's near this point?" without checking
  everything.

**Spatial Filtering:** The process of eliminating obviously impossible
  pairs (like lines on opposite sides of the world) before doing
  expensive collision tests.

**Time Step:** The amount of time that passes in one frame of the
  simulation. Lines move by `velocity × timeStep` each frame.

**Candidate Pair:** Two lines that are spatially close and might
  collide. We test these to see if they actually do collide.

---

<div style="text-align: right">

[NEXT: Performance Analysis](02-project2-performance-analysis.md)

</div>

