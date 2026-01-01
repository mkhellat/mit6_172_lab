# Parallelism Bounds Analysis

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

---

## Table of Contents

- [Problem Statement](#problem-statement)
- [Constraints](#constraints)
  - [Work Law](#work-law)
  - [Span Law](#span-law)
  - [Greedy Scheduler Bound](#greedy-scheduler-bound)
- [Finding Minimum Parallelism](#finding-minimum-parallelism)
- [Finding Maximum Parallelism](#finding-maximum-parallelism)
- [Answers](#answers)
- [Intuitive Understanding of Parallelism](#intuitive-understanding-of-parallelism)
  - [What is Parallelism?](#what-is-parallelism)
  - [What Do the Bounds Tell Us?](#what-do-the-bounds-tell-us)
- [Example Computation DAGs and Algorithms](#example-computation-dags-and-algorithms)
  - [Minimum Parallelism Case (Parallelism = 37)](#minimum-parallelism-case-parallelism--37)
  - [Maximum Parallelism Case (Parallelism = 105)](#maximum-parallelism-case-parallelism--105)
  - [Intermediate Cases](#intermediate-cases)
- [Mathematical Implications](#mathematical-implications)
  - [The Parallelism Bounds as Constraints](#the-parallelism-bounds-as-constraints)
  - [Speedup Implications](#speedup-implications)
  - [Why $T_4 = 100$ and $T_{64} = 10$?](#why-t_4--100-and-t_64--10)
- [Frequently Asked Questions About Parallelism](#frequently-asked-questions-about-parallelism)
  - [1. Does an algorithm have a fixed value of parallelism?](#1-does-an-algorithm-have-a-fixed-value-of-parallelism)
  - [2. Could an algorithm achieve different amounts of parallelism run on different numbers of processors?](#2-could-an-algorithm-achieve-different-amounts-of-parallelism-run-on-different-numbers-of-processors)
  - [3. How do additional measurements (third, fourth, ... $T_P$ values) affect the parallelism interval bounds?](#3-how-do-additional-measurements-third-fourth--t_p-values-affect-the-parallelism-interval-bounds)
- [Conclusion](#conclusion)

---

## Problem Statement

Ben Bitdiddle measures his deterministic program on 4 and 64
processors on an ideal parallel computer using a greedy scheduler. He
obtains running times of $T_4 = 100$ seconds and $T_{64} = 10$
seconds.

Based on the following formulas:
- **Work Law**: $T_P \geq T_1/P$
- **Span Law**: $T_P \geq T_{\infty}$
- **Greedy Scheduler Bound**: $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$

Find:
1. The lowest possible value for the parallelism ($T_1/T_{\infty}$)
2. The highest possible value for the parallelism ($T_1/T_{\infty}$)

---

## Constraints

### Work Law

The Work Law states that $T_P \geq T_1/P$. From the measurements:

$$
\begin{align}
T_4 &\geq \frac{T_1}{4} \implies T_1 \leq 400 \\
T_{64} &\geq \frac{T_1}{64} \implies T_1 \leq 640
\end{align}
$$

The tighter constraint is $T_1 \leq 400$.

### Span Law

The Span Law states that $T_P \geq T_{\infty}$. From the measurements:

$$
\begin{align}
T_4 &\geq T_{\infty} \implies T_{\infty} \leq 100 \\
T_{64} &\geq T_{\infty} \implies T_{\infty} \leq 10
\end{align}
$$

The tighter constraint is $T_{\infty} \leq 10$.

### Greedy Scheduler Bound

The Greedy Scheduler Bound states that $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$.
From the measurements:

$$
\begin{align}
T_1 &\geq 400 - 3T_{\infty} \\
T_1 &\geq 640 - 63T_{\infty}
\end{align}
$$

which implies the following lower bounds on $T_1$:

$$
\begin{align}
T_1 &\geq 400 - 30 = 370 \\
T_1 &\geq 400 - 300 = 100 \\
T_1 &\geq 640 - 630 = 10
\end{align}
$$

---

## Finding Minimum Parallelism

To minimize the parallelism $T_1/T_{\infty}$, we want to **maximize**
$T_{\infty}$ and **minimize** $T_1$.

From the Span Law, the maximum $T_{\infty} = 10$.

When $T_{\infty} = 10$:
- From greedy bound $P=4$: $T_1 \geq 400 - 3(10) = 370$
- From greedy bound $P=64$: $T_1 \geq 640 - 63(10) = 10$
- From Work Law: $T_1 \leq 400$

The tightest lower bound is $T_1 \geq 370$.

At $T_1 = 370$, $T_{\infty} = 10$:

$$
\text{Parallelism} = \frac{T_1}{T_{\infty}} = \frac{370}{10} = 37
$$

**Verification**:
- Work Law: $T_1/4 = 92.5 \leq 100$ ✓, $T_1/64 = 5.78 \leq 10$ ✓
- Span Law: $T_{\infty} = 10 \leq 100$ ✓, $T_{\infty} = 10 \leq 10$ ✓
- Greedy bound $P=4$: $T_1/4 + 3T_{\infty}/4 = 92.5 + 7.5 = 100 \leq 100$ ✓
- Greedy bound $P=64$: $T_1/64 + 63T_{\infty}/64 = 5.78 + 9.84 = 15.62 \geq 10$ ✓

---

## Finding Maximum Parallelism

To maximize the parallelism $T_1/T_{\infty}$, we want to **maximize**
$T_1$ and **minimize** $T_{\infty}$.

From the Work Law, the maximum $T_1 = 400$.

We need to find the minimum $T_{\infty}$ such that all constraints are
satisfied when $T_1 = 400$.

When $T_1 = 400$:
- From greedy bound $P=64$: $400 \geq 640 - 63T_{\infty} \implies
  63T_{\infty} \geq 240 \implies T_{\infty} \geq 240/63$
- From greedy bound $P=4$: $400 \geq 400 - 3T_{\infty} \implies
  3T_{\infty} \geq 0 \implies T_{\infty} \geq 0$

The tightest lower bound is $T_{\infty} \geq 240/63$.

At $T_1 = 400$, $T_{\infty} = 240/63$:

$$
\text{Parallelism} = \frac{T_1}{T_{\infty}} = \frac{400}{240/63} = 400 \times \frac{63}{240} = 105
$$

**Verification**:
- Work Law: $T_1/4 = 100 \leq 100$ ✓, $T_1/64 = 6.25 \leq 10$ ✓
- Span Law: $T_{\infty} = 240/63 \approx 3.81 \leq 100$ ✓, $T_{\infty} \approx 3.81 \leq 10$ ✓
- Greedy bound $P=4$: $T_1/4 + 3T_{\infty}/4 = 100 + 2.86 = 102.86 \geq 100$ ✓
- Greedy bound $P=64$: $T_1/64 + 63T_{\infty}/64 = 6.25 + 3.75 = 10 \leq 10$ ✓

---

## Answers

1. **Lowest possible parallelism**: **37**
   - Achieved when $T_1 = 370$, $T_{\infty} = 10$
   - This maximizes $T_{\infty}$ (at the Span Law limit) and minimizes
     $T_1$ (at the greedy scheduler bound for $P=4$)

2. **Highest possible parallelism**: **105**
   - Achieved when $T_1 = 400$, $T_{\infty} = 240/63 \approx 3.8095$
   - This maximizes $T_1$ (at the Work Law limit for $P=4$) and
     minimizes $T_{\infty}$ (at the intersection of both greedy
     scheduler bounds)

---

## Intuitive Understanding of Parallelism

### What is Parallelism?

The **parallelism** $T_1/T_{\infty}$ represents the **average number of
processors that can be kept busy** during the execution of a parallel
computation. It's a fundamental measure of how "parallelizable" an algorithm
is.

**Intuitive interpretation**:
- **High parallelism** (e.g., 105): The computation has many independent
  operations that can run simultaneously. With enough processors, we can
  achieve near-linear speedup.
- **Low parallelism** (e.g., 37): The computation has more sequential
  dependencies. Even with many processors, some will be idle due to
  dependencies.

**Mathematical interpretation**:
- Parallelism = Work / Span = Total operations / Critical path length
- It's the ratio of "how much work there is" to "how long it must take"
- If parallelism = $P$, then with $P$ processors, we can theoretically
  achieve perfect speedup (ignoring overhead)

### What Do the Bounds Tell Us?

The bounds $37 \leq T_1/T_{\infty} \leq 105$ tell us:

1. **Lower bound (37)**: Even in the worst case (maximum span, minimum work
   consistent with measurements), the algorithm has at least 37-way parallelism.
   This means:
   - With 37 or fewer processors, we can expect good utilization
   - The algorithm is inherently parallel, not purely sequential
   - There's significant opportunity for parallel speedup

2. **Upper bound (105)**: In the best case (minimum span, maximum work
   consistent with measurements), the algorithm can utilize up to 105
   processors effectively. This means:
   - With up to 105 processors, we can achieve near-linear speedup
   - Beyond 105 processors, adding more won't help (diminishing returns)
   - The algorithm has excellent parallel scalability

3. **The range (37 to 105)**: The actual parallelism depends on the algorithm's
   structure. Different algorithms with the same measured times can have
   different parallelism values within this range.

---

## Example Computation DAGs and Algorithms

### Minimum Parallelism Case (Parallelism = 37)

**Characteristics**: $T_1 = 370$, $T_{\infty} = 10$

**DAG Structure**: A computation with a relatively long critical path
compared to the total work. Think of it as a "tall, narrow" DAG.

**Example Algorithm**: **Parallel reduction with limited parallelism**

Consider a reduction operation (like summing an array) where:
- We have 370 total operations
- The critical path is 10 steps long
- At each level, we can parallelize, but not perfectly

**Visual DAG** (simplified):
```
Level 1:  [op] [op] [op] ... [op]  (many operations in parallel)
Level 2:  [op] [op] [op] ... [op]  (fewer operations, some dependencies)
...
Level 10: [op]                    (final reduction step)
```

**Real-world example**: A divide-and-conquer algorithm with moderate
branching factor, such as:
- **Parallel merge sort** with limited parallelism per level
- **Tree-based reduction** where the tree is not perfectly balanced
- **Recursive algorithms** with dependencies between recursive calls

**Why parallelism is limited**: The critical path constrains how much we can
parallelize. Even though we have 370 operations, they must be organized in at
least 10 sequential steps, limiting the average parallelism to 37.

### Maximum Parallelism Case (Parallelism = 105)

**Characteristics**: $T_1 = 400$, $T_{\infty} = 240/63 \approx 3.81$

**DAG Structure**: A computation with a very short critical path compared to
the total work. Think of it as a "wide, shallow" DAG.

**Example Algorithm**: **Highly parallelizable computation**

Consider a computation where:
- We have 400 total operations
- The critical path is only about 3.81 steps
- Most operations are independent and can run in parallel

**Visual DAG** (simplified):
```
Level 1:  [op] [op] [op] ... [op] [op] ... [op]  (many independent ops)
         (about 105 operations in parallel)
Level 2:  [op] [op] [op] ... [op]              (fewer ops, some dependencies)
Level 3:  [op] [op]                            (final steps)
Level 4:  [op]                                 (completion)
```

**Real-world examples**:
- **Embarrassingly parallel computations**: Independent operations on large
  datasets (e.g., map operations, element-wise array operations)
- **Parallel for loops** with independent iterations
- **Monte Carlo simulations**: Many independent random trials
- **Image processing**: Applying filters to independent pixels
- **Matrix operations**: Element-wise operations on large matrices

**Why parallelism is high**: The critical path is very short (only ~3.81 steps),
meaning most of the 400 operations can be done in parallel. This allows
effective use of up to 105 processors.

### Intermediate Cases

For parallelism values between 37 and 105, we have algorithms with varying
degrees of parallelism:

- **Parallelism ≈ 50-60**: Moderate parallelism, such as:
  - Divide-and-conquer with balanced recursion
  - Parallel prefix computations
  - Some dynamic programming problems

- **Parallelism ≈ 80-90**: Good parallelism, such as:
  - Well-parallelized matrix operations
  - Parallel graph algorithms with good load balancing
  - Parallel sorting with good work distribution

---

## Mathematical Implications

### The Parallelism Bounds as Constraints

The bounds $37 \leq T_1/T_{\infty} \leq 105$ can be rewritten as constraints on
$T_1$ and $T_{\infty}$:

$$
37 \cdot T_{\infty} \leq T_1 \leq 105 \cdot T_{\infty}
$$

This tells us:
- **Work is bounded**: For a given span $T_{\infty}$, the work $T_1$ cannot be
  arbitrarily large or small
- **Span is bounded**: For a given work $T_1$, the span $T_{\infty}$ has limits
- **Trade-off**: There's a fundamental trade-off between work and span. To
  increase parallelism, we need to either increase work or decrease span

### Speedup Implications

With $P$ processors, the theoretical speedup is bounded by:

$$
\text{Speedup} = \frac{T_1}{T_P} \leq \min\left(P, \frac{T_1}{T_{\infty}}\right)
$$

**For our bounds**:
- **Minimum case** (parallelism = 37):
  - With $P \leq 37$: Speedup $\approx P$ (near-linear)
  - With $P > 37$: Speedup $\approx 37$ (saturation)
  - With $P = 64$: Speedup $\approx 37$ (processors underutilized)

- **Maximum case** (parallelism = 105):
  - With $P \leq 105$: Speedup $\approx P$ (near-linear)
  - With $P > 105$: Speedup $\approx 105$ (saturation)
  - With $P = 64$: Speedup $\approx 64$ (good utilization)

### Why $T_4 = 100$ and $T_{64} = 10$?

The measurements reveal interesting behavior:

- **From 4 to 64 processors**: 10× speedup (100s → 10s)
- **Ideal speedup would be**: 16× (64/4 = 16)
- **Actual speedup**: 10×, which is 62.5% of ideal

This suggests:
- The algorithm has good but not perfect scalability
- There's significant parallelism (at least 37, possibly up to 105)
- With 64 processors, we're getting good utilization (better than the minimum
  parallelism of 37 would suggest)
- The span is relatively short compared to work, allowing good speedup

---

## Frequently Asked Questions About Parallelism

### 1. Does an algorithm have a fixed value of parallelism?

**Answer: Yes, for a given algorithm and input, parallelism is fixed.**

The parallelism $T_1/T_{\infty}$ is a **property of the algorithm's computation
DAG** (directed acyclic graph), which is determined by:
- The algorithm itself
- The input size and values
- The computation structure (work and span)

**Key points**:
- For a **deterministic algorithm** with a **fixed input**, the computation DAG
  is uniquely determined, so $T_1$ and $T_{\infty}$ are fixed, making
  parallelism fixed.
- Different **algorithms** solving the same problem can have different
  parallelism values.
- The same algorithm with **different inputs** (especially different sizes)
  will generally have different parallelism values.
- Parallelism is **independent of the number of processors** $P$ used—it's an
  intrinsic property of the algorithm.

**Example**: A parallel reduction algorithm always has parallelism $\Theta(n/\log
n)$ for an array of size $n$, regardless of whether you run it on 4, 64, or
1000 processors.

### 2. Could an algorithm achieve different amounts of parallelism run on different numbers of processors?

**Answer: No, parallelism is independent of the number of processors.**

Parallelism $T_1/T_{\infty}$ is a **property of the algorithm**, not of the
execution environment. The number of processors $P$ affects:
- **Execution time** $T_P$ (more processors → potentially faster)
- **Speedup** $T_1/T_P$ (how much faster than serial)
- **Efficiency** $T_1/(P \cdot T_P)$ (utilization of processors)

But it does **not** affect:
- **Work** $T_1$ (total operations)
- **Span** $T_{\infty}$ (critical path length)
- **Parallelism** $T_1/T_{\infty}$ (intrinsic parallelizability)

**What changes with $P$**:
- With $P < T_1/T_{\infty}$: Good utilization, near-linear speedup
- With $P = T_1/T_{\infty}$: Optimal utilization
- With $P > T_1/T_{\infty}$: Diminishing returns, some processors idle

**Example**: An algorithm with parallelism 50 will have parallelism 50 whether
run on 4, 50, or 1000 processors. However, the speedup will differ:
- $P = 4$: Speedup ≈ 4 (good utilization)
- $P = 50$: Speedup ≈ 50 (optimal)
- $P = 1000$: Speedup ≈ 50 (saturation, many processors idle)

### 3. How do additional measurements (third, fourth, ... $T_P$ values) affect the parallelism interval bounds?

**Answer: Additional measurements can only tighten the bounds, making the interval smaller and more precise.**

When we add more measurements (e.g., $T_{16}$, $T_{32}$, $T_{128}$), we're
introducing **additional constraints** to the optimization problem. Each new
measurement $T_P$ contributes three types of constraints:
- Work Law: $T_P \geq T_1/P$ (upper bound on $T_1$)
- Span Law: $T_P \geq T_{\infty}$ (upper bound on $T_{\infty}$)
- Greedy Scheduler Bound: $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$ (lower bound on $T_1$)

**Fundamental principle**: In any constrained optimization problem, adding
more constraints can only:
- **Shrink** the feasible region (reduce the set of valid solutions)
- **Raise** the minimum value (tighten the lower bound)
- **Lower** the maximum value (tighten the upper bound)

**Consequences for parallelism bounds**:
- The **lower bound** on parallelism can only **increase** (become more
  restrictive)
- The **upper bound** on parallelism can only **decrease** (become more
  restrictive)
- The **interval width** can only **shrink**, never expand
- The bounds **converge** toward the true parallelism value as more
  measurements are added

**Concrete example**: Starting with $T_4 = 100$ and $T_{64} = 10$, we
determined $37 \leq T_1/T_{\infty} \leq 105$.

If we add a measurement $T_{16} = 25$:
- New Work Law constraint: $T_1 \leq 16 \times 25 = 400$ (may be redundant)
- New Span Law constraint: $T_{\infty} \leq 25$ (less restrictive than $T_{\infty} \leq 10$)
- New Greedy Scheduler Bound: $T_1 \geq 400 - 15T_{\infty}$
- When $T_{\infty} = 10$: $T_1 \geq 400 - 150 = 250$ (tighter than the previous $T_1 \geq 370$)
- This could raise the lower bound from 37 to approximately 40
- Result: $40 \leq T_1/T_{\infty} \leq 105$ (interval compressed from width 68 to width 65)

**Critical distinction**: 
- If a new measurement is **inconsistent** with existing ones (contradicts the
  fundamental laws), it indicates measurement error (as shown in write-ups 3
  and 4)
- If all measurements are **consistent**, additional ones will progressively
  refine the bounds, never relax them

**Benefits of interval compression**:
1. **Increased precision**: Each measurement narrows the range of possible
   parallelism values, giving us a more accurate estimate of the true parallelism
2. **Better performance prediction**: Tighter bounds enable more reliable
   predictions of execution time on different processor counts
3. **Algorithm characterization**: The convergence pattern reveals information
   about the algorithm's parallel structure
4. **Measurement validation**: If bounds unexpectedly expand or remain
   unchanged when they should compress, it signals potential measurement
   inconsistencies

**Illustrative progression** (hypothetical consistent measurements):
- Initial: $T_4 = 100$, $T_{64} = 10$ → $37 \leq T_1/T_{\infty} \leq 105$ (width = 68)
- Add $T_{16} = 25$: → $40 \leq T_1/T_{\infty} \leq 95$ (width = 55, 19% compression)
- Add $T_{32} = 15$: → $42 \leq T_1/T_{\infty} \leq 88$ (width = 46, 16% further compression)
- Add $T_{128} = 8$: → $45 \leq T_1/T_{\infty} \leq 85$ (width = 40, 13% further compression)

**Edge case**: If a new measurement is **redundant** (provides constraints
already implied by existing measurements), the interval remains unchanged. This
is still valid—the measurement confirms rather than refines our bounds. However,
the interval will never expand due to redundant information.

---

## Conclusion

The parallelism bounds demonstrate how the Work Law, Span Law, and
greedy scheduler bound interact to constrain the possible values of
$T_1$ and $T_{\infty}$. The minimum parallelism occurs when the span
is maximized (limited by $T_{64} = 10$), while the maximum parallelism
occurs when the work is maximized and the span is minimized to satisfy
the greedy scheduler bound for $P=64$.

**Key insights**:

1. **Parallelism range (37 to 105)**: The algorithm has significant
   parallelism, indicating it's well-suited for parallel execution. The
   wide range suggests the actual parallelism depends on the algorithm's
   structure.

2. **Processor utilization**: With 64 processors, the algorithm achieves
   10× speedup from 4 processors, indicating good but not perfect
   scalability. The parallelism bounds suggest we could potentially
   utilize up to 105 processors effectively.

3. **Algorithm characteristics**: 
   - Minimum parallelism (37): Suggests a "tall, narrow" DAG with more
     sequential dependencies
   - Maximum parallelism (105): Suggests a "wide, shallow" DAG with
     highly independent operations
   - The actual algorithm likely falls somewhere in between

4. **Practical implications**: 
   - The algorithm will benefit from parallelization
   - With current hardware (64 processors), we're getting good
     utilization
   - There's room for improvement with more processors (up to ~105)
   - The algorithm structure matters: different implementations with the
     same work/span can achieve different parallelism values

5. **Measurement effects**:
   - Parallelism is fixed for a given algorithm and input
   - Parallelism is independent of the number of processors used
   - Additional measurements can only tighten (compress) the parallelism
     interval, never expand it
   - More measurements lead to more precise bounds on the true parallelism

The work-span model provides fundamental limits that help us understand
the parallel potential of algorithms and guide optimization efforts.
