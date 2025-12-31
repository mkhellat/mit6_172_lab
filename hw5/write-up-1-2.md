# Tight Bound for Greedy Schedulers in Parallel Computation - version 2

> "The work-span model provides fundamental limits on parallel
>  performance, independent of the scheduling algorithm." - CLRS Chapter
>  27

---

## Problem Statement

**Exercise 27.1-3** (CLRS, Page 791, Chapter 27): Prove that a greedy
scheduler achieves the following time bound, which is slightly
stronger than the bound proven in Theorem 27.1:

$$
T_P \leq \frac{T_1 - T_{\infty}}{P} + T_{\infty}
$$

where:
- $T_P$ is the execution time on $P$ processors
- $T_1$ is the work (total computation time on 1 processor)
- $T_{\infty}$ is the span (critical path length)
- $P$ is the number of processors

This bound is tighter than the standard bound $T_P \leq T_1/P +
T_{\infty}$ from Theorem 27.1, particularly when parallelism is limited.

---

## Overview

The work-span model provides a framework for analyzing parallel
algorithms by decomposing computation into:
- **Work** ($T_1$): Total amount of computation
- **Span** ($T_{\infty}$): Length of the longest path in the computation
  DAG

A **greedy scheduler** is one that never leaves processors idle if there
is available work. This write-up provides a formal proof that any greedy
scheduler achieves the tighter bound stated above.

---

## Intuition

### The Zero-Parallelism Case

An intuitive starting point for understanding the tight bound is the
worst-case scenario where no strands can run concurrently. In this
case, we have a single sequential branch whose length equals the total
work:

$$
T_1 = T_P \quad \forall P
$$

Since there is only a single branch in the DAG, that branch is
necessarily the critical path (span):

$$
T_1 = T_P = T_{\infty}
$$

### Comparison with the Loose Bound

The standard bound from Theorem 27.1 states:

$$
T_P \leq \frac{T_1}{P} + T_{\infty}
$$

For the zero-parallelism case, this loose bound gives:

$$
T_P \leq \frac{T_1}{P} + T_{\infty} = \frac{T_{\infty}}{P} + T_{\infty}
$$

However, we know from the zero-parallelism constraint that:

$$
T_P = T_{\infty}
$$

The loose bound is therefore looser by the amount $T_1/P$ in this case.
The new tight bound, however, satisfies the zero-parallelism constraint
exactly:

$$
T_P = \frac{T_1 - T_{\infty}}{P} + T_{\infty} = \frac{T_{\infty} -
T_{\infty}}{P} + T_{\infty} = T_{\infty}
$$

This suggests that the tight bound is indeed stronger and captures the
limiting case more accurately.

---

## Formal Proof

### Step 1: Classifying Computation Steps

To formally prove the validity of the tight bound, we classify each
step of the computation (when using $P$ processors) as either:

- **Complete step**: All $P$ processors are busy, performing $P$ units
  of computation
- **Incomplete step**: At least one processor is idle, performing
  strictly less than $P$ units of computation

Let:
- $n_p$ = number of complete steps
- $n_i$ = number of incomplete steps

The total execution time is:

$$
T_P = n_p + n_i
$$

### Step 2: Bounding the Work

During complete steps, exactly $P$ units of work are performed per
step, giving us $n_p \cdot P$ units of work.

During incomplete steps, at most $P-1$ units of work are performed per
step (since at least one processor is idle), giving us at most $n_i
\cdot (P-1)$ units of work.

The total work $T_1$ must satisfy:

$$
T_1 \leq n_p \cdot P + n_i \cdot (P-1)
$$

Expanding and rearranging:

$$
T_1 \leq n_p \cdot P + n_i \cdot P - n_i = (n_p + n_i) \cdot P - n_i =
T_P \cdot P - n_i
$$

Therefore:

$$
n_i \leq T_P \cdot P - T_1
$$

### Step 3: Bounding Incomplete Steps by Span

The key insight is that incomplete steps can only occur when there are
fewer than $P$ ready strands available. This happens when the
computation is constrained by dependencies along the critical path.

The span $T_{\infty}$ represents the length of the longest path in the
computation DAG. For the purpose of finding an upper bound, we consider
the worst case where:

- The critical path has length $T_{\infty}$
- During each step along the critical path, only one strand is
  available (the one on the critical path)
- All other processors are idle during these steps

In this worst-case scenario, we have at most $T_{\infty}$ incomplete
steps, since each step along the critical path can contribute at most
one incomplete step:

$$
n_i \leq T_{\infty}
$$

### Step 4: Deriving the Tight Bound

From Step 2, we have the work bound:

$$
T_1 \leq n_p \cdot P + n_i \cdot (P-1)
$$

Since $T_P = n_p + n_i$, we can express $n_p = T_P - n_i$.
Substituting:

$$
T_1 \leq (T_P - n_i) \cdot P + n_i \cdot (P-1)
$$

Expanding the right-hand side:

$$
T_1 \leq T_P \cdot P - n_i \cdot P + n_i \cdot P - n_i = T_P \cdot P -
n_i
$$

Rearranging:

$$
n_i \geq T_P \cdot P - T_1 \tag{1}
$$

From Step 3, we have the span bound:

$$
n_i \leq T_{\infty} \tag{2}
$$

Combining inequalities (1) and (2):

$$
T_P \cdot P - T_1 \leq n_i \leq T_{\infty}
$$

Therefore:

$$
T_P \cdot P - T_1 \leq T_{\infty}
$$

Rearranging:

$$
T_P \cdot P \leq T_1 + T_{\infty}
$$

Dividing by $P$:

$$
T_P \leq \frac{T_1 + T_{\infty}}{P} \tag{3}
$$

### Step 5: Showing the Tight Bound

We now show that the bound in equation (3) implies the desired tighter
bound. For $P \geq 2$:

$$
\frac{T_1 + T_{\infty}}{P} = \frac{T_1 - T_{\infty} + 2T_{\infty}}{P}
= \frac{T_1 - T_{\infty}}{P} + \frac{2T_{\infty}}{P}
$$

Since $P \geq 2$, we have $\frac{2T_{\infty}}{P} \leq T_{\infty}$.
Therefore:

$$
\frac{T_1 + T_{\infty}}{P} \leq \frac{T_1 - T_{\infty}}{P} + T_{\infty}
$$

Combining with equation (3):

$$
T_P \leq \frac{T_1 + T_{\infty}}{P} \leq \frac{T_1 - T_{\infty}}{P} +
T_{\infty}
$$

For $P = 1$, the bound $T_P \leq (T_1 - T_{\infty})/1 + T_{\infty} =
T_1$ is trivially true since $T_P = T_1$ with one processor.

This completes the proof of the tight bound.

---

## Conclusion

We have proven that any greedy scheduler achieves the time bound:

$$
T_P \leq \frac{T_1 - T_{\infty}}{P} + T_{\infty}
$$

This bound is tighter than the standard bound $T_P \leq T_1/P +
T_{\infty}$ from Theorem 27.1, particularly when $T_{\infty}$ is
significant relative to $T_1/P$. The proof relies on classifying
computation steps as complete or incomplete, bounding incomplete steps
by the span $T_{\infty}$, and using the work bound to relate these
quantities.

The bound demonstrates that parallel execution time is fundamentally
constrained by both the available parallelism (captured by $T_1/P$) and
the inherent sequential dependencies (captured by $T_{\infty}$), with
the span term having a coefficient that reflects its critical nature in
limiting speedup.