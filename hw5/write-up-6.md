# Multithreaded Floyd-Warshall Algorithm

> "Dynamic programming algorithms can often be parallelized by identifying
> independent subproblems." - CLRS Chapter 27

---

## Problem Statement

Give pseudocode for an efficient multithreaded implementation of the
Floyd-Warshall algorithm (see Section 25.2), which computes shortest paths
between all pairs of vertices in an edge-weighted graph. Analyze your
algorithm.

---

## Algorithm Overview

The Floyd-Warshall algorithm computes shortest paths between all pairs of
vertices in a graph. It uses dynamic programming with the recurrence:

$$
d_{ij}^{(k)} = \min\left(d_{ij}^{(k-1)}, d_{ik}^{(k-1)} + d_{kj}^{(k-1)}\right)
$$

where $d_{ij}^{(k)}$ is the shortest path from vertex $i$ to vertex $j$ using
only vertices $\{1, 2, \ldots, k\}$ as intermediate vertices.

The standard serial algorithm has three nested loops:
- Outer loop: $k = 1$ to $n$ (intermediate vertices)
- Middle loop: $i = 1$ to $n$ (source vertices)
- Inner loop: $j = 1$ to $n$ (destination vertices)

---

## Parallelization Strategy

### Key Observation

For a fixed value of $k$, the updates to $d[i][j]$ for different pairs $(i,j)$
are **independent** of each other. Each update requires:
- $d[i][j]$ from the previous iteration (or initial value)
- $d[i][k]$ from the previous iteration
- $d[k][j]$ from the previous iteration

Since we're reading from the previous iteration (or initial values) and
writing to different locations, all updates for a fixed $k$ can be performed
in parallel.

### Parallel Structure

1. **Sequential outer loop**: Iterate over $k = 1$ to $n$ (must be sequential
   due to dependencies between iterations)
2. **Parallel middle loop**: For each $k$, process all $i$ values in parallel
3. **Parallel inner loop**: For each $(k,i)$, process all $j$ values in
   parallel

Alternatively, we can parallelize over all $(i,j)$ pairs for a fixed $k$,
which is more natural for spawn/sync.

---

## Pseudocode

We present a divide-and-conquer approach that parallelizes the updates for each
intermediate vertex $k$:

```
FLOYD-WARSHALL(W, n)
    // W is the n×n adjacency matrix (W[i][j] = weight of edge (i,j))
    // Initialize distance matrix D
    D = new matrix[n][n]
    
    // Initialize D^(0) = W
    for i = 1 to n
        for j = 1 to n
            D[i][j] = W[i][j]
    
    // Main algorithm: compute D^(k) for k = 1 to n
    for k = 1 to n
        // Parallelize updates over all (i,j) pairs using divide-and-conquer
        spawn UPDATE_MATRIX(D, n, k, 1, 1, n, n)
        sync

UPDATE_MATRIX(D, n, k, i_start, j_start, i_end, j_end)
    // Update all elements D[i][j] in the submatrix [i_start..i_end] × [j_start..j_end]
    // using the recurrence: D[i][j] = min(D[i][j], D[i][k] + D[k][j])
    
    size_i = i_end - i_start + 1
    size_j = j_end - j_start + 1
    
    // Base case: small submatrix, update serially
    if size_i <= 1 and size_j <= 1
        D[i_start][j_start] = min(D[i_start][j_start], 
                                  D[i_start][k] + D[k][j_start])
        return
    
    // Recursive case: divide into submatrices
    if size_i >= size_j
        // Split by rows
        mid_i = (i_start + i_end) / 2
        spawn UPDATE_MATRIX(D, n, k, i_start, j_start, mid_i, j_end)
        spawn UPDATE_MATRIX(D, n, k, mid_i + 1, j_start, i_end, j_end)
        sync
    else
        // Split by columns
        mid_j = (j_start + j_end) / 2
        spawn UPDATE_MATRIX(D, n, k, i_start, j_start, i_end, mid_j)
        spawn UPDATE_MATRIX(D, n, k, i_start, mid_j + 1, i_end, j_end)
        sync
```

**Alternative simpler version (parallelizing over rows):**

For a simpler implementation, we can parallelize over rows:

```
FLOYD-WARSHALL(W, n)
    // Initialize distance matrix
    D = new matrix[n][n]
    for i = 1 to n
        for j = 1 to n
            D[i][j] = W[i][j]
    
    // Main algorithm
    for k = 1 to n
        // Parallelize over all rows
        for i = 1 to n
            spawn UPDATE_ROW(D, n, k, i)
        sync

UPDATE_ROW(D, n, k, i)
    // Update all elements in row i
    for j = 1 to n
        D[i][j] = min(D[i][j], D[i][k] + D[k][j])
```

The divide-and-conquer version provides more parallelism (up to $n^2$ parallel
tasks per iteration), while the row-based version is simpler and still provides
good parallelism (up to $n$ parallel tasks per iteration).

---

## Algorithm Analysis

### Work Analysis

Let $T_1(n)$ be the work (serial time) for the Floyd-Warshall algorithm on an
$n$-vertex graph.

**Initialization**: $\Theta(n^2)$ to initialize the distance matrix.

**Main algorithm**: For each $k$ from $1$ to $n$:
- We update $n^2$ elements in parallel
- Each update is $\Theta(1)$ work
- Total work per iteration: $\Theta(n^2)$

Therefore:
$$
T_1(n) = \Theta(n^2) + n \cdot \Theta(n^2) = \Theta(n^3)
$$

This matches the serial complexity of Floyd-Warshall.

### Span Analysis

Let $T_{\infty}(n)$ be the span (critical path length) for the algorithm.

**Initialization**: Can be done in parallel with span $\Theta(\log n)$ using
divide-and-conquer, or $\Theta(n)$ if done row-by-row in parallel. For
simplicity, we'll assume $\Theta(n)$ span for initialization.

**Main algorithm**: For each $k$ from $1$ to $n$ (sequential):
- **Row-based version**: All $n$ rows updated in parallel
  - Each row processes $n$ elements serially: $\Theta(n)$ span per row
  - Since rows are parallel, span per iteration is $\Theta(n)$
- **Divide-and-conquer version**: Recursively divides the $n \times n$ matrix
  - Depth of recursion: $\Theta(\log n)$ (splitting $n \times n$ matrix)
  - Work per level: $\Theta(1)$ (just comparisons and splits)
  - Span per iteration: $\Theta(\log n)$

For the row-based version:
$$
T_{\infty}(n) = \Theta(n) + n \cdot \Theta(n) = \Theta(n^2)
$$

For the divide-and-conquer version:
$$
T_{\infty}(n) = \Theta(n) + n \cdot \Theta(\log n) = \Theta(n \log n)
$$

The divide-and-conquer version has better span, but for simplicity and
practical purposes, we'll analyze the row-based version which gives:
$$
T_{\infty}(n) = \Theta(n^2)
$$

### Parallelism

The parallelism is:
$$
\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n^3)}{\Theta(n^2)} = \Theta(n)
$$

This shows good parallelism that scales linearly with the number of vertices.

---

## Conclusion

The multithreaded Floyd-Warshall algorithm:

- **Work**: $T_1(n) = \Theta(n^3)$ (same as serial algorithm)
- **Span**: $T_{\infty}(n) = \Theta(n^2)$
- **Parallelism**: $\Theta(n)$

The algorithm efficiently parallelizes the Floyd-Warshall algorithm by
processing all $(i,j)$ pairs in parallel for each intermediate vertex $k$. The
key insight is that for a fixed $k$, all updates are independent and can be
performed concurrently. The outer loop over $k$ must remain sequential due to
dependencies between iterations.

The parallelism of $\Theta(n)$ means the algorithm can effectively utilize up to
$n$ processors, making it efficient for parallel execution on multicore
systems. For graphs with many vertices, this provides significant speedup
potential.

