# In-Place Matrix Transpose Using Divide-and-Conquer

> "Divide-and-conquer algorithms naturally lend themselves to
> parallelization." - CLRS Chapter 27

---

## Problem Statement

Give pseudocode for an efficient multithreaded algorithm that transposes an
$n \times n$ matrix in place by using divide-and-conquer and no parallel for
loops to divide the matrix recursively into four $n/2 \times n/2$ submatrices.
Analyze your algorithm.

---

## Algorithm Design

### Divide-and-Conquer Strategy

For an $n \times n$ matrix $A$, we divide it into four quadrants:

- **Top-left (TL)**: rows $[0, n/2-1]$, columns $[0, n/2-1]$
- **Top-right (TR)**: rows $[0, n/2-1]$, columns $[n/2, n-1]$
- **Bottom-left (BL)**: rows $[n/2, n-1]$, columns $[0, n/2-1]$
- **Bottom-right (BR)**: rows $[n/2, n-1]$, columns $[n/2, n-1]$

When transposing a matrix:
- The top-left quadrant transposes in place (becomes the top-left of the
  transposed matrix)
- The bottom-right quadrant transposes in place (becomes the bottom-right of
  the transposed matrix)
- The top-right and bottom-left quadrants swap positions, and each must be
  transposed

### Recursive Structure

1. **Base case**: If $n \leq 1$, return (matrix is already transposed)
2. **Recursive case**:
   - Recursively transpose the top-left quadrant
   - Recursively transpose the bottom-right quadrant
   - Swap the top-right and bottom-left quadrants
   - Recursively transpose the swapped quadrants (each in its new position)

### Parallelization

The four recursive calls can be executed in parallel:
- Transpose TL and BR in parallel (they don't overlap)
- After swapping TR and BL, transpose the swapped quadrants in parallel
- The swap operation itself can be parallelized by swapping elements in
  parallel

---

## Pseudocode

```
TRANSPOSE(A, row, col, n)
    if n <= 1
        return
    
    half = n / 2
    
    // Recursively transpose quadrants that stay in place
    spawn TRANSPOSE(A, row, col, half)              // Top-left
    spawn TRANSPOSE(A, row + half, col + half, half) // Bottom-right
    sync
    
    // Swap top-right and bottom-left quadrants
    spawn SWAP_QUADRANTS(A, row, col + half, row + half, col, half)
    sync
    
    // Recursively transpose the swapped quadrants
    spawn TRANSPOSE(A, row, col + half, half)       // Was bottom-left
    spawn TRANSPOSE(A, row + half, col, half)        // Was top-right
    sync

SWAP_QUADRANTS(A, row1, col1, row2, col2, n)
    if n <= 1
        swap A[row1][col1] with A[row2][col2]
        return
    
    half = n / 2
    
    // Recursively swap four sub-quadrants in parallel
    spawn SWAP_QUADRANTS(A, row1, col1, row2, col2, half)
    spawn SWAP_QUADRANTS(A, row1, col1 + half, row2, col2 + half, half)
    spawn SWAP_QUADRANTS(A, row1 + half, col1, row2 + half, col2, half)
    spawn SWAP_QUADRANTS(A, row1 + half, col1 + half, row2 + half, col2 + half, half)
    sync
```

**Initial call**: `TRANSPOSE(A, 0, 0, n)`

---

## Algorithm Analysis

### Work Analysis

Let $T_1(n)$ be the work (serial time) for transposing an $n \times n$ matrix.

**Base case**: $T_1(1) = \Theta(1)$

**Recursive case**: We have:
- Two recursive transpose calls on $n/2 \times n/2$ matrices: $2T_1(n/2)$
- One swap operation on two $n/2 \times n/2$ quadrants: $S_1(n)$
- Two more recursive transpose calls: $2T_1(n/2)$

The swap operation $S_1(n)$ swaps $n^2/4$ elements, so $S_1(n) = \Theta(n^2)$.

Therefore:
$$
T_1(n) = 4T_1(n/2) + \Theta(n^2)
$$

Using the master theorem with $a = 4$, $b = 2$, $f(n) = \Theta(n^2)$:
- $n^{\log_b a} = n^{\log_2 4} = n^2$
- Since $f(n) = \Theta(n^2) = \Theta(n^{\log_b a})$, we are in case 2
- $T_1(n) = \Theta(n^2 \log n)$

**Wait, let's reconsider**: The swap operation swaps $n^2/4$ elements, which is
$\Theta(n^2)$. But we can optimize the swap to be done in parallel during the
recursive calls. However, for work analysis, we still need to account for all
the element swaps.

Actually, let's think more carefully. The swap operation needs to swap $n^2/4$
pairs of elements. In the serial case, this is $\Theta(n^2)$. But we can
parallelize it recursively.

Let $S_1(n)$ be the work for swapping two $n \times n$ quadrants:
- Base case: $S_1(1) = \Theta(1)$ (swap one element)
- Recursive: $S_1(n) = 4S_1(n/2) + \Theta(1)$
- By master theorem: $S_1(n) = \Theta(n^2)$

So the work recurrence is:
$$
T_1(n) = 4T_1(n/2) + \Theta(n^2)
$$

This gives $T_1(n) = \Theta(n^2 \log n)$.

However, we can optimize: instead of doing a separate swap, we can combine the
swap and transpose operations. But for the standard approach, the work is
$\Theta(n^2 \log n)$.

Actually, let me reconsider the algorithm. A simpler approach: we swap and
transpose in one step. But the problem asks for divide-and-conquer with four
submatrices.

Let me refine: The swap of two $n/2 \times n/2$ quadrants requires swapping
$n^2/4$ elements. This is $\Theta(n^2)$ work. So:

$$
T_1(n) = 4T_1(n/2) + \Theta(n^2)
$$

By master theorem: $T_1(n) = \Theta(n^2 \log n)$.

### Span Analysis

Let $T_{\infty}(n)$ be the span (critical path length) for transposing an $n
\times n$ matrix.

**Base case**: $T_{\infty}(1) = \Theta(1)$

**Recursive case**: The critical path is:
1. One recursive transpose: $T_{\infty}(n/2)$ (the two parallel transposes both
   take this time)
2. The swap operation: $S_{\infty}(n)$
3. One more recursive transpose: $T_{\infty}(n/2)$ (the two parallel transposes
   both take this time)

The swap operation $S_{\infty}(n)$ swaps two $n/2 \times n/2$ quadrants. Since
we recursively swap four sub-quadrants in parallel:
- Base case: $S_{\infty}(1) = \Theta(1)$ (swap one element)
- Recursive: $S_{\infty}(n) = S_{\infty}(n/2) + \Theta(1)$ (all four recursive
  swaps happen in parallel, so we take the max)
- This gives: $S_{\infty}(n) = \Theta(\log n)$

Therefore:
$$
T_{\infty}(n) = T_{\infty}(n/2) + S_{\infty}(n) + T_{\infty}(n/2) = 2T_{\infty}(n/2) + \Theta(\log n)
$$

Solving this recurrence:
$$
T_{\infty}(n) = 2T_{\infty}(n/2) + c \log n
$$

for some constant $c$. Expanding:
$$
T_{\infty}(n) = 2^k T_{\infty}(n/2^k) + c \sum_{i=0}^{k-1} 2^i \log(n/2^i)
$$

Setting $k = \log_2 n$:
$$
T_{\infty}(n) = n \cdot T_{\infty}(1) + c \sum_{i=0}^{\log_2 n - 1} 2^i (\log_2 n - i)
$$

The sum $\sum_{i=0}^{\log_2 n - 1} 2^i (\log_2 n - i)$:
- The term $2^i \log_2 n$ contributes approximately $(n-1) \log_2 n$
- The term $-i \cdot 2^i$ contributes approximately $O(n)$
- Therefore, the sum is $\Theta(n \log n)$

So:
$$
T_{\infty}(n) = \Theta(n) + \Theta(n \log n) = \Theta(n \log n)
$$

### Parallelism

The parallelism is:
$$
\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n^2 \log n)}{\Theta(n \log n)} = \Theta(n)
$$

This shows good parallelism that scales linearly with the matrix size.

---

## Conclusion

The divide-and-conquer matrix transpose algorithm:

- **Work**: $T_1(n) = \Theta(n^2 \log n)$
- **Span**: $T_{\infty}(n) = \Theta(n \log n)$
- **Parallelism**: $\Theta(n)$

The algorithm efficiently parallelizes the matrix transpose operation by
recursively dividing the matrix into four quadrants and processing them in
parallel. The work is slightly higher than the optimal $\Theta(n^2)$ due to the
recursive overhead, but the algorithm achieves good parallelism of $\Theta(n)$,
making it efficient for parallel execution on multiple processors.

