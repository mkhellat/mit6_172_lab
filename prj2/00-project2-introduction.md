# Spatial Collision Detection: Quadtree Optimization

Collision detection is a fundamental problem in computational physics,
computer graphics, and simulation systems. Given $n$ moving line segments
in a 2D space, determining which pairs will intersect in the next time step
is essential for accurate physical simulation. The naive approach tests all
$\binom{n}{2} = \frac{n(n-1)}{2}$ pairs, resulting in $O(n^2)$ time
complexity. For large numbers of lines, this becomes prohibitively slow.

This project implements and evaluates a **quadtree-based spatial indexing**
algorithm to optimize collision detection, reducing the complexity to
$O(n \log n)$ average case while maintaining **100% correctness** (perfect
match with brute-force algorithm on all test cases).

---

## Problem Statement

### The Collision Detection Challenge

In a 2D simulation with moving line segments, we must determine which line
pairs will intersect before the next time step. Each line segment has:
- Two endpoints $(p_1, p_2)$ defining its current position
- A velocity vector $\vec{v}$ describing its movement
- A time step $\Delta t$ over which to predict collisions

The collision detection problem:
- **Input:** $n$ line segments with positions and velocities
- **Output:** All pairs $(L_i, L_j)$ that will intersect in time $\Delta t$
- **Constraint:** Must be correct (no false negatives, minimal false
  positives resolved by testing)

### Computational Complexity

The brute-force approach tests every pair:

\[
T_{\text{brute-force}}(n) = \sum_{i=0}^{n-1} \sum_{j=i+1}^{n-1} T_{\text{intersect}} = \frac{n(n-1)}{2} \cdot T_{\text{intersect}}
\]

where $T_{\text{intersect}}$ is the time to test one pair. This yields
$O(n^2)$ time complexity, which becomes prohibitive for large $n$.

### Motivation for Spatial Indexing

Spatial data structures exploit the observation that **most line pairs are
too far apart to collide**. By organizing lines spatially, we can:
1. Quickly identify lines that are spatially close (candidates)
2. Test only those candidate pairs
3. Achieve $O(n \log n)$ average case complexity

---

## Project Overview

This project implements a **dynamic quadtree** for collision detection,
comparing it against the brute-force baseline. The implementation includes:

1. **Quadtree Data Structure:** Hierarchical spatial subdivision
2. **Three-Phase Algorithm:** Build → Query → Test
3. **Comprehensive Evaluation:** Performance analysis and correctness
   verification
4. **Design Analysis:** Key decisions and trade-offs

### Report Structure

**Part 1: Algorithm Comparison and Design Decisions**
- Brute-force vs quadtree algorithms
- Handling line segments that span partitions
- Design decisions and implementation choices

**Part 2: Performance Analysis**
- Speedup measurements and analysis
- Impact of parameter $r$ (maxLinesPerNode)
- Performance characteristics across input sizes

**Part 3: Correctness and Verification**
- Correctness guarantees
- Comparison with brute-force results
- Edge case handling

**Part 4: Future Work and Optimizations**
- Parallelization opportunities
- Additional optimizations
- Extensions and improvements

---

<div style="text-align: right">

[NEXT: Algorithm Comparison and Design Decisions](01-project2-algorithm-comparison.md)

</div>

