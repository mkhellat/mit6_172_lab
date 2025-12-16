# Project 2: Quadtree Collision Detection

Academic writeup and analysis of quadtree-based spatial collision detection
optimization for moving line segments.

## Report Structure

1. **[Introduction](00-project2-introduction.md)**
   - Problem statement and motivation
   - Project overview
   - Report structure

2. **[Algorithm Comparison and Design Decisions](01-project2-algorithm-comparison.md)**
   - Brute-force vs quadtree algorithms
   - Handling line segments spanning partitions
   - Key design decisions and rationale
   - Using quadtree to extract collisions

3. **[Performance Analysis](02-project2-performance-analysis.md)**
   - Speedup measurements and analysis
   - Impact of parameter $r$ (maxLinesPerNode)
   - Performance characteristics
   - Experimental methodology

4. **[Correctness and Verification](03-project2-correctness.md)**
   - Correctness guarantees
   - Verification strategy
   - Edge case handling
   - Experimental verification

5. **[Future Work and Conclusions](04-project2-future-work.md)**
   - Performance optimizations
   - Phase 2 parallelization
   - Additional optimizations
   - Conclusions and lessons learned

6. **[Parallelization with OpenCilk](MIT6_172F18-project2/README.md)** ⭐ NEW
   - Complete parallelization of collision detection
   - All 10 phases implemented and verified
   - Thread-safe reducers and atomic operations
   - Race condition fixes and correctness verification
   - Cilkscale setup for parallelism analysis

## Quick Navigation

- [What speedup did we achieve?](02-project2-performance-analysis.md#22-speedup-analysis)
- [Impact of parameter $r$](02-project2-performance-analysis.md#23-impact-of-parameter-r-maxlinespernode)
- [Handling lines spanning partitions](01-project2-algorithm-comparison.md#13-the-line-segment-partitioning-problem)
- [Key design decisions](01-project2-algorithm-comparison.md#14-key-design-decisions)
- [Using quadtree to extract collisions](01-project2-algorithm-comparison.md#15-using-the-quadtree-to-extract-collisions)

## Experimental Results Template

See [Performance Analysis](02-project2-performance-analysis.md) for detailed
experimental framework and results templates.

## Implementation Files

- `MIT6_172F18-project2/quadtree.h` / `quadtree.c`: Quadtree data structure implementation
- `MIT6_172F18-project2/collision_world.c`: Integration with collision detection (parallelized)
- `MIT6_172F18-project2/screensaver.c`: Command-line interface with `-q` flag
- `MIT6_172F18-project2/intersection_event_list.c`: Thread-safe event collection (reducers)

## Building and Testing

```bash
cd MIT6_172F18-project2
make clean && make

# Test brute-force
./screensaver 1000 input/mit.in

# Test quadtree (serial)
./screensaver -q 1000 input/mit.in

# Test quadtree (parallel)
CILK_NWORKERS=4 ./screensaver -q 1000 input/mit.in
```

### Parallelization

The collision detection system has been fully parallelized using OpenCilk:
- **Candidate testing:** Parallelized with `cilk_for`
- **Query phase:** Parallelized with `cilk_for` (race condition fixed)
- **Thread-safety:** Reducers and atomic operations
- **Correctness:** Verified with Cilksan (0 races) and extensive testing

See [MIT6_172F18-project2/README.md](MIT6_172F18-project2/README.md) for complete parallelization documentation.

---

[Start Reading: Introduction](00-project2-introduction.md)

