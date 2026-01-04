# In-Place Matrix Transpose Using Divide-and-Conquer

> "Divide-and-conquer algorithms naturally lend themselves to
> parallelization." - CLRS Chapter 27

---

## Problem Statement

Give pseudocode for an efficient multithreaded algorithm that
transposes an $n \times n$ matrix in place by using divide-and-conquer
and no parallel for loops to divide the matrix recursively into four
$n/2 \times n/2$ submatrices.  Analyze your algorithm.

---

## Algorithm Design

### Divide-and-Conquer Strategy

For an $n \times n$ matrix $A$, we divide it into four quadrants:

- **Top-left (TL)**: rows $[0, n/2-1]$, columns $[0, n/2-1]$
- **Top-right (TR)**: rows $[0, n/2-1]$, columns $[n/2, n-1]$
- **Bottom-left (BL)**: rows $[n/2, n-1]$, columns $[0, n/2-1]$
- **Bottom-right (BR)**: rows $[n/2, n-1]$, columns $[n/2, n-1]$

When transposing a matrix:
- The top-left quadrant transposes in place (becomes the top-left of
  the transposed matrix)
- The bottom-right quadrant transposes in place (becomes the
  bottom-right of the transposed matrix)
- The top-right and bottom-left quadrants swap positions, and each
  must be transposed

### Recursive Structure

1. **Base case**: If $n \leq 1$, return (matrix is already transposed)
2. **Recursive case**:
   - Recursively transpose the top-left quadrant
   - Recursively transpose the bottom-right quadrant
   - Swap the top-right and bottom-left quadrants
   - Recursively transpose the swapped quadrants (each in its new
     position)

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
    spawn TRANSPOSE(A, row, col, half)               // Top-left
    spawn TRANSPOSE(A, row + half, col + half, half) // Bottom-right
    sync
    
    // Swap top-right and bottom-left quadrants
    spawn SWAP_QUADRANTS(A, row, col + half, row + half, col, half)
    sync
    
    // Recursively transpose the swapped quadrants
    spawn TRANSPOSE(A, row, col + half, half)        // Was bottom-left
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

**Wait, what if swap is done recursively?**: The swap operation needs
to swap $n^2/4$ pairs of elements. In the serial case, this is
$\Theta(n^2)$. But we can parallelize it recursively.

Let $S_1(n)$ be the work for swapping two $n \times n$ quadrants:
- Base case: $S_1(1) = \Theta(1)$ (swap one element)
- Recursive: $S_1(n) = 4S_1(n/2) + \Theta(1)$
- By master theorem: $S_1(n) = \Theta(n^2)$

So the work recurrence is:
$$
T_1(n) = 4T_1(n/2) + \Theta(n^2)
$$

This gives $T_1(n) = \Theta(n^2 \log n)$.

**Could we optimize more?** we swap and transpose in one step. But the
problem asks for divide-and-conquer with four submatrices.


### Span Analysis

Let $T_{\infty}(n)$ be the span (critical path length) for transposing
an $n \times n$ matrix.

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

### Master Theorem Analysis

This recurrence is of the form $T(n) = aT(n/b) + f(n)$ where:
- $a = 2$
- $b = 2$
- $f(n) = \Theta(\log n)$

For the Master Theorem, we compute:
- $n^{\log_b a} = n^{\log_2 2} = n^1 = n$

Now we compare $f(n) = \Theta(\log n)$ with $n^{\log_b a} = n$:
- Since $\log n = o(n)$, we have $\log n = O(n^{1-\epsilon})$ for any $\epsilon > 0$ (e.g., $\epsilon = 0.5$ gives $O(n^{0.5})$)

**Master Theorem Case 1** states: If $f(n) = O(n^{\log_b a - \epsilon})$ for some $\epsilon > 0$, then $T(n) = \Theta(n^{\log_b a}) = \Theta(n)$.

Since $\log n = O(n^{1-\epsilon})$ for any $\epsilon > 0$ (e.g., $\epsilon = 0.5$), **Case 1 applies directly**, giving us:
$$
T_{\infty}(n) = \Theta(n)
$$

The Master Theorem correctly predicts the solution! The explicit calculation below confirms this result and provides insight into why the $\log n$ terms don't accumulate to $\Theta(n \log n)$ as might be initially expected.

### Explicit Solution with Rigorous Proof

We solve the recurrence:
$$
T_{\infty}(n) = 2T_{\infty}(n/2) + c \log n
$$

for some constant $c > 0$. Expanding recursively:
$$
T_{\infty}(n) = 2^k T_{\infty}(n/2^k) + c \sum_{i=0}^{k-1} 2^i \log(n/2^i)
$$

Setting $k = \log_2 n$ (assuming $n$ is a power of 2 for simplicity):
$$
T_{\infty}(n) = n \cdot T_{\infty}(1) + c \sum_{i=0}^{\log_2 n - 1} 2^i (\log_2 n - i)
$$

Since $T_{\infty}(1) = \Theta(1)$, we have $n \cdot T_{\infty}(1) = \Theta(n)$. Now we need to rigorously evaluate the sum:

$$
S(n) = \sum_{i=0}^{\log_2 n - 1} 2^i (\log_2 n - i)
$$

**Step 1: Split the sum**

$$
S(n) = \sum_{i=0}^{k-1} 2^i (\log_2 n - i) = \log_2 n \sum_{i=0}^{k-1} 2^i - \sum_{i=0}^{k-1} i \cdot 2^i
$$

where $k = \log_2 n$.

**Step 2: Evaluate the first sum**

$$
\sum_{i=0}^{k-1} 2^i = 2^k - 1 = n - 1
$$

Therefore:
$$
\log_2 n \sum_{i=0}^{k-1} 2^i = \log_2 n (n - 1) = n \log_2 n - \log_2 n
$$

**Step 3: Evaluate the second sum**

We use the known formula for $\sum_{i=0}^{k-1} i \cdot 2^i$:

**Lemma**: For $k \geq 1$, $\sum_{i=0}^{k-1} i \cdot 2^i = (k-2)2^k + 2$.

**Proof of Lemma** (by induction):
- **Base case** ($k=1$): $\sum_{i=0}^{0} i \cdot 2^i = 0 = (1-2)2^1 + 2 = -2 + 2 = 0$ ✓
- **Inductive step**: Assume true for $k$, prove for $k+1$:
  $$
  \sum_{i=0}^{k} i \cdot 2^i = \sum_{i=0}^{k-1} i \cdot 2^i + k \cdot 2^k = (k-2)2^k + 2 + k \cdot 2^k = (2k-2)2^k + 2 = (k-1)2^{k+1} + 2
  $$
  This equals $((k+1)-2)2^{k+1} + 2$ ✓

Applying the lemma with $k = \log_2 n$:
$$
\sum_{i=0}^{k-1} i \cdot 2^i = (k-2)2^k + 2 = (\log_2 n - 2)n + 2 = n \log_2 n - 2n + 2
$$

**Step 4: Combine the results**

$$
\begin{align}
S(n) &= (n \log_2 n - \log_2 n) - (n \log_2 n - 2n + 2) \\
     &= n \log_2 n - \log_2 n - n \log_2 n + 2n - 2 \\
     &= 2n - \log_2 n - 2
\end{align}
$$

**Step 5: Establish tight $\Theta$ bounds**

We have $S(n) = 2n - \log_2 n - 2$. To prove $S(n) = \Theta(n)$, we need to show there exist positive constants $c_1, c_2$ and $n_0$ such that for all $n \geq n_0$:

$$c_1 n \leq S(n) \leq c_2 n$$

**Lower bound**: For $n \geq 4$:
$$
S(n) = 2n - \log_2 n - 2 \geq 2n - \frac{n}{2} - 2 = \frac{3n}{2} - 2
$$

For $n \geq 4$, we have $\frac{3n}{2} - 2 \geq n$ (since $\frac{3n}{2} - 2 \geq n$ implies $\frac{n}{2} \geq 2$, which holds for $n \geq 4$). Therefore:
$$
S(n) \geq n \quad \text{for } n \geq 4
$$

**Upper bound**: For all $n \geq 2$:
$$
S(n) = 2n - \log_2 n - 2 \leq 2n
$$

**Combining bounds**: For $n \geq 4$:
$$
n \leq S(n) \leq 2n
$$

This proves that $S(n) = \Theta(n)$ with constants $c_1 = 1$ and $c_2 = 2$.

**Step 6: Final result**

$$
\begin{align}
T_{\infty}(n) &= n \cdot T_{\infty}(1) + c \cdot S(n) \\
               &= \Theta(n) + c \cdot \Theta(n) \\
               &= \Theta(n)
\end{align}
$$

**Confirmation**: This rigorous explicit calculation confirms the Master Theorem result: $T_{\infty}(n) = \Theta(n)$.

The key insight revealed by the explicit calculation is *why* the Master Theorem works: while each level contributes $\Theta(\log n)$ work, the geometric series structure causes the $\log n$ factors to nearly cancel when summed across all $\log_2 n$ levels. Specifically:
- The term $\sum_{i=0}^{k-1} 2^i \log_2 n = n \log_2 n - \log_2 n$ contributes $\Theta(n \log n)$
- The term $\sum_{i=0}^{k-1} i \cdot 2^i = n \log_2 n - 2n + 2$ also contributes $\Theta(n \log n)$
- Their difference yields only $\Theta(n)$

This explicit calculation provides intuition for why the Master Theorem's Case 1 correctly predicts $\Theta(n)$: the logarithmic terms accumulate in a way that their total contribution is dominated by the recursive structure, not by the sum of the logarithms.
</think>
Re-evaluating the sum with exact bounds:
<｜tool▁calls▁begin｜><｜tool▁call▁begin｜>
read_file

### Parallelism

The parallelism is:
$$
\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n^2 \log n)}{\Theta(n)} = \Theta(n \log n)
$$

This shows excellent parallelism that scales super-linearly with the matrix size. The parallelism of $\Theta(n \log n)$ means the algorithm can effectively utilize a large number of processors, making it highly efficient for parallel execution.

---

## Conclusion

The divide-and-conquer matrix transpose algorithm:

- **Work**: $T_1(n) = \Theta(n^2 \log n)$
- **Span**: $T_{\infty}(n) = \Theta(n)$ (as rigorously proven above)
- **Parallelism**: $\Theta(n \log n)$

The algorithm efficiently parallelizes the matrix transpose operation by
recursively dividing the matrix into four quadrants and processing them in
parallel. The work is slightly higher than the optimal $\Theta(n^2)$ due to the
recursive overhead, but the algorithm achieves excellent parallelism of
$\Theta(n \log n)$, making it highly efficient for parallel execution on
multiple processors. The span analysis reveals an interesting mathematical
phenomenon where the $\Theta(\log n)$ terms at each level accumulate in a way
that yields only $\Theta(n)$ total span, not $\Theta(n \log n)$ as might be
initially expected.

