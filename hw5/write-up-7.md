# Multithreading Reductions and Prefix Computations

> "Reduction and prefix computations are fundamental parallel primitives that
> appear in many algorithms." - CLRS Chapter 27

---

## Problem Statement

Problem 27-4 from CLRS Chapter 27 covers multithreaded implementations of
$\otimes$-reduction and $\otimes$-prefix computations (also called $\otimes$-scan),
where $\otimes$ is an associative operator.

A $\otimes$-reduction of an array $x[1..n]$ is the value:
$$
y = x[1] \otimes x[2] \otimes \cdots \otimes x[n]
$$

A $\otimes$-prefix computation (scan) produces the array $y[1..n]$ where:
$$
\begin{align}
y[1] &= x[1] \\
y[2] &= x[1] \otimes x[2] \\
y[3] &= x[1] \otimes x[2] \otimes x[3] \\
&\vdots \\
y[n] &= x[1] \otimes x[2] \otimes x[3] \otimes \cdots \otimes x[n]
\end{align}
$$

---

## Part (a): P-REDUCE Implementation

### Algorithm Design

We use a divide-and-conquer approach to parallelize the reduction:

```
P-REDUCE(x, i, j)
    if i == j
        return x[i]
    else
        k = floor((i + j) / 2)
        left = spawn P-REDUCE(x, i, k)
        right = P-REDUCE(x, k + 1, j)
        sync
        return left ⊗ right
```

### Analysis

**Work**: $T_1(n) = 2T_1(n/2) + \Theta(1)$

Using the master theorem with $a = 2$, $b = 2$, $f(n) = \Theta(1)$:
- $n^{\log_b a} = n^{\log_2 2} = n$
- Since $f(n) = \Theta(1) = O(n^{1-\epsilon})$ for $\epsilon > 0$, we are in case 1
- $T_1(n) = \Theta(n)$

**Span**: $T_{\infty}(n) = T_{\infty}(n/2) + \Theta(1)$

This recurrence gives:
$$
T_{\infty}(n) = T_{\infty}(n/2) + c = \Theta(\log n)
$$

**Parallelism**: $\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n)}{\Theta(\log n)} = \Theta(n/\log n)$

---

## Part (b): Analysis of P-SCAN-1

### Algorithm

```
P-SCAN-1(x)
    n = x.length
    let y[1..n] be a new array
    P-SCAN-1-AUX(x, y, 1, n)
    return y

P-SCAN-1-AUX(x, y, i, j)
    parallel for l = i to j
        y[l] = P-REDUCE(x, 1, l)
```

### Analysis

**Work**: For each $l$ from $1$ to $n$, we compute $P\text{-}REDUCE(x, 1, l)$ which
takes $\Theta(l)$ work. Therefore:

$$
T_1(n) = \sum_{l=1}^{n} \Theta(l) = \Theta\left(\sum_{l=1}^{n} l\right) = \Theta(n^2)
$$

**Span**: All $n$ iterations of the parallel for loop execute concurrently. Each
iteration computes $P\text{-}REDUCE(x, 1, l)$ with span $\Theta(\log l)$.
Therefore:

$$
T_{\infty}(n) = \max_{1 \leq l \leq n} \Theta(\log l) = \Theta(\log n)
$$

**Parallelism**: $\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n^2)}{\Theta(\log n)} = \Theta(n^2/\log n)$

While the parallelism is high, the work is quadratic, making this algorithm
inefficient.

---

## Part (c): Analysis of P-SCAN-2

### Algorithm

```
P-SCAN-2(x)
    n = x.length
    let y[1..n] be a new array
    P-SCAN-2-AUX(x, y, 1, n)
    return y

P-SCAN-2-AUX(x, y, i, j)
    if i == j
        y[i] = x[i]
    else
        k = floor((i + j) / 2)
        spawn P-SCAN-2-AUX(x, y, i, k)
        P-SCAN-2-AUX(x, y, k + 1, j)
        sync
        parallel for l = k + 1 to j
            y[l] = y[k] ⊗ y[l]
```

### Correctness

We prove correctness by induction on the subarray size.

**Base case**: For $i = j$, we have $y[i] = x[i]$, which is correct.

**Inductive step**: Assume the algorithm correctly computes prefix sums for
subarrays of size less than $j - i + 1$.

After the recursive calls:
- $y[i..k]$ contains the correct prefix sums for $x[i..k]$
- $y[k+1..j]$ contains the prefix sums for $x[k+1..j]$ relative to the start
  of that subarray

The prefix sum for $x[k+1..j]$ relative to the start of the entire array $x[1..n]$
needs to include the reduction of $x[i..k]$. Since $y[k]$ contains $x[i] \otimes
x[i+1] \otimes \cdots \otimes x[k]$, we update:
$$
y[l] = y[k] \otimes y[l] \quad \text{for } l \in [k+1, j]
$$

This correctly computes the prefix sums for $x[k+1..j]$ relative to the start
of $x[1..n]$.

### Analysis

**Work**: $T_1(n) = 2T_1(n/2) + \Theta(n)$

Using the master theorem with $a = 2$, $b = 2$, $f(n) = \Theta(n)$:
- $n^{\log_b a} = n^{\log_2 2} = n$
- Since $f(n) = \Theta(n) = \Theta(n^{\log_b a})$, we are in case 2
- $T_1(n) = \Theta(n \log n)$

**Span**: $T_{\infty}(n) = T_{\infty}(n/2) + \Theta(\log n)$

The parallel for loop has span $\Theta(\log n)$ (processing $n/2$ elements in
parallel). Solving the recurrence:

$$
T_{\infty}(n) = T_{\infty}(n/2) + c \log n
$$

Expanding:
$$
T_{\infty}(n) = c \sum_{i=0}^{\log_2 n - 1} \log(n/2^i) = c \sum_{i=0}^{\log_2 n - 1} (\log n - i)
$$

$$
= c \left(\log_2 n \cdot \log n - \frac{\log_2 n (\log_2 n - 1)}{2}\right) = \Theta((\log n)^2)
$$

**Parallelism**: $\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n \log n)}{\Theta((\log n)^2)} = \Theta(n/\log n)$

---

## Part (d): P-SCAN-3 Implementation

### Filling in the Missing Expressions

The algorithm uses a two-pass approach:
1. **Up pass**: Build a binary tree of reductions for contiguous subarrays
2. **Down pass**: Propagate prefix sums down the tree

```
P-SCAN-3(x)
    n = x.length
    let y[1..n] and t[1..n] be new arrays
    y[1] = x[1]
    if n > 1
        P-SCAN-UP(x, t, 2, n)
        P-SCAN-DOWN(x[1], x, t, y, 2, n)
    return y

P-SCAN-UP(x, t, i, j)
    if i == j
        return x[i]
    else
        k = floor((i + j) / 2)
        t[k] = spawn P-SCAN-UP(x, t, i, k)
        right = P-SCAN-UP(x, t, k + 1, j)
        sync
        return t[k] ⊗ right

P-SCAN-DOWN(ν, x, t, y, i, j)
    if i == j
        y[i] = ν ⊗ x[i]
    else
        k = floor((i + j) / 2)
        spawn P-SCAN-DOWN(ν, x, t, y, i, k)
        P-SCAN-DOWN(ν ⊗ t[k], x, t, y, k + 1, j)
        sync
```

**Missing expressions:**
- Line 8 of P-SCAN-UP: `return t[k] ⊗ right`
- Line 5 of P-SCAN-DOWN: `spawn P-SCAN-DOWN(ν, x, t, y, i, k)`
- Line 6 of P-SCAN-DOWN: `P-SCAN-DOWN(ν ⊗ t[k], x, t, y, k + 1, j)`

### Correctness Proof

We prove by induction that:
1. **P-SCAN-UP**: For $P\text{-}SCAN\text{-}UP(x, t, i, j)$, the return value is the
   reduction of $x[i..j]$, and $t[k]$ (where $k$ is the midpoint) contains the
   reduction of $x[i..k]$.
2. **P-SCAN-DOWN**: For $P\text{-}SCAN\text{-}DOWN(\nu, x, t, y, i, j)$, if $\nu =
   x[1] \otimes x[2] \otimes \cdots \otimes x[i-1]$, then $y[i..j]$ will contain
   the correct prefix sums.

**Base case for P-SCAN-UP**: For $i = j$, we return $x[i]$, which is the
reduction of the subarray $x[i..i]$. ✓

**Inductive step for P-SCAN-UP**: 
- $P\text{-}SCAN\text{-}UP(x, t, i, k)$ returns the reduction of $x[i..k]$ and
  stores it in $t[k]$ (by induction).
- $P\text{-}SCAN\text{-}UP(x, t, k+1, j)$ returns the reduction of $x[k+1..j]$
  (by induction).
- Therefore, `t[k] ⊗ right` correctly returns the reduction of $x[i..j]$. ✓

**Base case for P-SCAN-DOWN**: For $i = j$, we set $y[i] = \nu \otimes x[i]$.
By the inductive hypothesis, $\nu = x[1] \otimes \cdots \otimes x[i-1]$, so
$y[i] = x[1] \otimes \cdots \otimes x[i]$, which is correct. ✓

**Inductive step for P-SCAN-DOWN**: 
- For the left subarray $[i..k]$: We pass $\nu$ unchanged. Since $\nu = x[1]
  \otimes \cdots \otimes x[i-1]$, by induction $y[i..k]$ will contain correct
  prefix sums. ✓
- For the right subarray $[k+1..j]$: We pass $\nu \otimes t[k]$. 
  - From the up pass, $t[k]$ contains the reduction of $x[i..k]$.
  - Since $\nu = x[1] \otimes \cdots \otimes x[i-1]$, we have:
    $$
    \nu \otimes t[k] = (x[1] \otimes \cdots \otimes x[i-1]) \otimes (x[i] \otimes \cdots \otimes x[k]) = x[1] \otimes \cdots \otimes x[k]
    $$
  - By induction, $y[k+1..j]$ will contain correct prefix sums with $x[1]
    \otimes \cdots \otimes x[k]$ as the prefix. ✓

**Initial call**: $P\text{-}SCAN\text{-}DOWN(x[1], x, t, y, 2, n)$ is called with $\nu
= x[1]$, which satisfies the invariant. Therefore, the algorithm correctly
computes all prefix sums.

Therefore, the algorithm is correct.

---

## Part (e): Analysis of P-SCAN-3

### Work Analysis

**P-SCAN-UP**: $T_1^{UP}(n) = 2T_1^{UP}(n/2) + \Theta(1)$

By master theorem: $T_1^{UP}(n) = \Theta(n)$

**P-SCAN-DOWN**: $T_1^{DOWN}(n) = 2T_1^{DOWN}(n/2) + \Theta(1)$

By master theorem: $T_1^{DOWN}(n) = \Theta(n)$

**Total work**: $T_1(n) = \Theta(n) + \Theta(n) = \Theta(n)$

### Span Analysis

**P-SCAN-UP**: $T_{\infty}^{UP}(n) = T_{\infty}^{UP}(n/2) + \Theta(1)$

This gives: $T_{\infty}^{UP}(n) = \Theta(\log n)$

**P-SCAN-DOWN**: $T_{\infty}^{DOWN}(n) = T_{\infty}^{DOWN}(n/2) + \Theta(1)$

This gives: $T_{\infty}^{DOWN}(n) = \Theta(\log n)$

**Total span**: $T_{\infty}(n) = \Theta(\log n) + \Theta(\log n) = \Theta(\log n)$

### Parallelism

$$
\frac{T_1(n)}{T_{\infty}(n)} = \frac{\Theta(n)}{\Theta(\log n)} = \Theta(n/\log n)
$$

---

## Comparison of Algorithms

| Algorithm | Work | Span | Parallelism |
|-----------|------|------|-------------|
| P-SCAN-1  | $\Theta(n^2)$ | $\Theta(\log n)$ | $\Theta(n^2/\log n)$ |
| P-SCAN-2  | $\Theta(n \log n)$ | $\Theta((\log n)^2)$ | $\Theta(n/\log n)$ |
| P-SCAN-3  | $\Theta(n)$ | $\Theta(\log n)$ | $\Theta(n/\log n)$ |

**P-SCAN-3 is optimal** in terms of work (linear) and has the best span among
the efficient algorithms. It achieves the same parallelism as P-SCAN-2 but with
better work complexity.

---

## Conclusion

The two-pass approach in P-SCAN-3 achieves optimal work $\Theta(n)$ while
maintaining good parallelism $\Theta(n/\log n)$. The key insight is separating
the computation into:
1. **Up pass**: Build reduction tree (bottom-up)
2. **Down pass**: Propagate prefix sums (top-down)

This eliminates the quadratic work of P-SCAN-1 and the extra logarithmic
factor in the span of P-SCAN-2, making P-SCAN-3 the most efficient of the
three algorithms.

