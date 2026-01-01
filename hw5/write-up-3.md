# Inconsistent Parallel Execution Time Measurements

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

---

## Table of Contents

- [Problem Statement](#problem-statement)
- [Constraints from Each Measurement](#constraints-from-each-measurement)
  - [Constraints from $T_4 = 80$](#constraints-from-t_4--80)
  - [Constraints from $T_{10} = 42$](#constraints-from-t_10--42)
  - [Constraints from $T_{64} = 9$](#constraints-from-t_64--9)
- [Finding a Contradiction](#finding-a-contradiction)
  - [Step 1: Combine Span Law Constraints](#step-1-combine-span-law-constraints)
  - [Step 2: Check Consistency of $T_4$ and $T_{10}$](#step-2-check-consistency-of-t_4-and-t_10)
  - [Step 3: The Contradiction](#step-3-the-contradiction)
- [Intuitive Understanding of the Contradiction](#intuitive-understanding-of-the-contradiction)
- [What Could Have Gone Wrong?](#what-could-have-gone-wrong)
- [Which Measurement(s) Are Likely Wrong?](#which-measurements-are-likely-wrong)
- [Conclusion](#conclusion)

---

## Problem Statement

Ben Bitdiddle measures the running time of his deterministic parallel
program scheduled using a greedy scheduler on an ideal parallel
computer with 4, 10, and 64 processors. Ben obtains the following
running times:
- $T_4 = 80$ seconds
- $T_{10} = 42$ seconds
- $T_{64} = 9$ seconds

Argue that Ben messed up at least one of his measurements.

---

## Constraints from Each Measurement

For any valid set of measurements, the following laws must be satisfied:
- **Work Law**: $T_P \geq T_1/P$
- **Span Law**: $T_P \geq T_{\infty}$
- **Greedy Scheduler Bound**: $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$

Let us derive constraints from each measurement.

### Constraints from $T_4 = 80$

**Work Law**:
$$
T_1 \leq 320
$$

**Span Law**:
$$
T_{\infty} \leq 80
$$

**Greedy Scheduler Bound**:
$$
T_1 \geq 320 - 3T_{\infty}
$$

### Constraints from $T_{10} = 42$

**Work Law**:
$$
T_1 \leq 420
$$

**Span Law**:
$$
T_{\infty} \leq 42
$$

**Greedy Scheduler Bound**:
$$
T_1 \geq 420 - 9T_{\infty}
$$

### Constraints from $T_{64} = 9$

**Work Law**:
$$
T_1 \leq 576
$$

**Span Law**:
$$
T_{\infty} \leq 9
$$

**Greedy Scheduler Bound**:
$$
T_1 \geq 576 - 63T_{\infty}
$$

---

## Finding a Contradiction

### Step 1: Combine Span Law Constraints

From the Span Law:
- $T_{\infty} \leq 80$ (from $T_4$)
- $T_{\infty} \leq 42$ (from $T_{10}$)
- $T_{\infty} \leq 9$ (from $T_{64}$)

The tightest constraint is:

$$
T_{\infty} \leq 9
$$

### Step 2: Check Consistency of $T_4$ and $T_{10}$

If $T_{\infty} \leq 9$, then from the greedy scheduler bounds:
- From $T_4$: $T_1 \geq 320 - 3T_{\infty} \geq 320 - 3(9) = 293$
- From $T_{10}$: $T_1 \geq 420 - 9T_{\infty} \geq 420 - 9(9) = 339$

Also from Work Law:
- From $T_4$: $T_1 \leq 320$
- From $T_{10}$: $T_1 \leq 420$

So we need:

$$
339 \leq T_1 \leq 320
$$

This is **impossible**! There is no value of $T_1$ that satisfies both
$339 \leq T_1$ and $T_1 \leq 320$.

### Step 3: The Contradiction

From $T_{64} = 9$ (Span Law), we have $T_{\infty} \leq 9$.

Combining this with the greedy scheduler bounds:
- From $T_4$: $T_1 \geq 320 - 3T_{\infty} \geq 320 - 3(9) = 293$
- From $T_{10}$: $T_1 \geq 420 - 9T_{\infty} \geq 420 - 9(9) = 339$

From the Work Law:
- From $T_4$: $T_1 \leq 320$

Therefore, we need:

$$
339 \leq T_1 \leq 320
$$

which is **impossible**.

This contradiction proves that **at least one measurement is
incorrect**.

---

## Intuitive Understanding of the Contradiction

### Why the Measurements Cannot All Be Correct

The contradiction $339 \leq T_1 \leq 320$ is **mathematically impossible**—no
number can be both at least 339 and at most 320. This impossibility arises
from the fundamental laws of parallel computation, which must hold for any
valid parallel algorithm.

**The core issue**: The three measurements tell conflicting stories about
the algorithm's work and span:

1. **$T_{64} = 9$ says**: "The critical path is very short ($T_{\infty} \leq
   9$), so the algorithm has high parallelism potential."

2. **$T_{10} = 42$ says**: "With 10 processors, it takes 42 seconds. Given
   the short span, this implies significant work ($T_1 \geq 339$)."

3. **$T_4 = 80$ says**: "With 4 processors, it takes 80 seconds. This
   implies limited work ($T_1 \leq 320$)."

These cannot all be true simultaneously. If the span is short (from $T_{64}$)
and the work is large (from $T_{10}$), then with 4 processors we should see
better performance than 80 seconds. Conversely, if the work is limited (from
$T_4$), then with 10 processors we shouldn't need 42 seconds.

### Visual Interpretation

Think of the measurements as constraints in a 2D space (work vs. span):

- **$T_4 = 80$** creates a region: "Work is low, span can be anything up to
  80"
- **$T_{10} = 42$** creates a region: "Given short span, work must be high"
- **$T_{64} = 9$** creates a region: "Span is very short"

When we combine these regions, **they don't overlap**—there's no point
$(T_1, T_{\infty})$ that satisfies all three constraints simultaneously.

### The Greedy Scheduler Bound in Action

The greedy scheduler bound $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$ is
the key to detecting the inconsistency. This bound says:

> "With $P$ processors, execution time is at most the average work per
> processor plus the span."

When we apply this bound to different processor counts, we get different
constraints on $T_1$ and $T_{\infty}$. The measurements violate these
constraints, revealing the inconsistency.

**Example**: If $T_{\infty} = 9$ (from $T_{64}$), then:
- With 10 processors: $T_{10} \leq (T_1 - 9)/10 + 9$
- If $T_{10} = 42$, we need $T_1 \geq 339$
- But with 4 processors: $T_4 \leq (T_1 - 9)/4 + 9$
- If $T_1 \geq 339$, then $T_4 \leq (339 - 9)/4 + 9 = 91.5$
- But we measured $T_4 = 80$, which is consistent with this bound
- However, the Work Law says $T_1 \leq 320$ (from $T_4$), contradicting $T_1
  \geq 339$

The contradiction emerges from the **interaction** between the Work Law and
the greedy scheduler bound across different processor counts.

---

## What Could Have Gone Wrong?

### Possible Sources of Measurement Error

1. **Timing errors**:
   - Clock synchronization issues
   - Measurement overhead not accounted for
   - System load affecting measurements
   - Incorrect time units (seconds vs. milliseconds)

2. **Algorithm inconsistency**:
   - Non-deterministic behavior (race conditions, random number generation)
   - Different inputs used for different measurements
   - Algorithm modifications between measurements
   - Compiler optimizations applied inconsistently

3. **Hardware/environment issues**:
   - Processor frequency scaling (turbo boost, power management)
   - Cache warm-up effects (first run vs. subsequent runs)
   - Memory bandwidth contention
   - Different processor architectures or configurations

4. **Scheduler behavior**:
   - Non-greedy scheduling (if the scheduler isn't truly greedy)
   - Load balancing issues
   - Overhead from work-stealing not accounted for

5. **Measurement methodology**:
   - Averaging inconsistent runs
   - Not accounting for initialization/cleanup time
   - Measuring wall-clock time instead of CPU time
   - Including I/O or other non-computation time

### Most Likely Scenarios

Given the specific contradiction ($339 \leq T_1 \leq 320$), the most likely
issues are:

1. **$T_4$ or $T_{10}$ is wrong**: One of these measurements doesn't reflect
   the true execution time, possibly due to system load, measurement error,
   or inconsistent conditions.

2. **$T_{64}$ is wrong**: If the span is actually longer than 9 seconds, the
   contradiction might resolve. However, this seems less likely given that
   $T_{64} = 9$ is the fastest measurement.

3. **Non-deterministic algorithm**: If the algorithm has non-deterministic
   behavior, different runs might produce different execution times,
   invalidating the assumption of a deterministic program.

---

## Which Measurement(s) Are Likely Wrong?

### Analyzing Each Measurement

**$T_4 = 80$ seconds**:
- Implies: $T_1 \leq 320$ (Work Law)
- Implies: $T_1 \geq 293$ when $T_{\infty} \leq 9$ (greedy scheduler bound)
- **Consistency check**: These bounds are consistent ($293 \leq 320$)
- **Likelihood of error**: Medium—could be affected by system load or
  measurement overhead

**$T_{10} = 42$ seconds**:
- Implies: $T_1 \leq 420$ (Work Law)
- Implies: $T_1 \geq 339$ when $T_{\infty} \leq 9$ (greedy scheduler bound)
- **Consistency check**: These bounds are consistent ($339 \leq 420$)
- **Likelihood of error**: Medium—similar to $T_4$

**$T_{64} = 9$ seconds**:
- Implies: $T_{\infty} \leq 9$ (Span Law)
- Implies: $T_1 \leq 576$ (Work Law)
- Implies: $T_1 \geq 10$ when $T_{\infty} \leq 9$ (greedy scheduler bound)
- **Consistency check**: These bounds are very loose and consistent
- **Likelihood of error**: Lower—this is the fastest measurement, and errors
  typically make things slower, not faster

### The Contradiction Pattern

The contradiction $339 \leq T_1 \leq 320$ suggests:

- **$T_{10} = 42$** requires $T_1 \geq 339$ (from greedy scheduler bound)
- **$T_4 = 80$** requires $T_1 \leq 320$ (from Work Law)

These two measurements are **directly conflicting**. If we assume $T_{64} = 9$
is correct (which is reasonable, as it's the tightest constraint and fastest
measurement), then:

- **Most likely**: Either $T_4$ or $T_{10}$ is incorrect
- **Less likely**: Both $T_4$ and $T_{10}$ are incorrect
- **Unlikely**: $T_{64}$ is incorrect (would require it to be slower, which
  is less common for measurement errors)

### Practical Investigation Steps

To identify which measurement is wrong, Ben should:

1. **Re-measure all three values** under controlled conditions:
   - Same input, same hardware configuration
   - Multiple runs to check for consistency
   - Account for measurement overhead

2. **Check for non-determinism**:
   - Run the algorithm multiple times with the same input
   - Verify that execution times are consistent
   - Check for race conditions or random behavior

3. **Verify the scheduler**:
   - Confirm that a greedy scheduler is actually being used
   - Check for any scheduler overhead or non-ideal behavior

4. **Analyze the algorithm**:
   - Verify the algorithm is truly deterministic
   - Check that the same computation is performed in all cases
   - Ensure no external factors (I/O, network, etc.) affect timing

---

## Conclusion

The measurements are **inconsistent**. The contradiction arises as
follows:

1. From $T_{64} = 9$ (Span Law): $T_{\infty} \leq 9$
2. From $T_{10} = 42$ (greedy scheduler bound): $T_1 \geq 420 -
9T_{\infty} \geq 420 - 81 = 339$
3. From $T_4 = 80$ (Work Law): $T_1 \leq 320$

These constraints require:

$$
339 \leq T_1 \leq 320
$$

which is **impossible**.

**Therefore, at least one of the measurements ($T_4$, $T_{10}$, or $T_{64}$) must be incorrect.**

The contradiction shows that it is impossible for all three
measurements to be correct simultaneously, given the fundamental laws
of parallel computation (Work Law, Span Law, and Greedy Scheduler
Bound).

### Key Takeaways

1. **Fundamental laws are inviolable**: The Work Law, Span Law, and Greedy
   Scheduler Bound are mathematical truths that must hold for any valid
   parallel computation. If measurements violate these laws, the
   measurements must be wrong, not the laws.

2. **Cross-validation is essential**: Multiple measurements at different
   processor counts provide cross-validation. When they conflict, it reveals
   measurement errors or algorithm inconsistencies.

3. **The greedy scheduler bound is powerful**: This bound, which relates
   execution time to work, span, and processor count, is particularly
   effective at detecting inconsistencies because it creates tight
   constraints that must be satisfied simultaneously.

4. **Measurement methodology matters**: Accurate parallel performance
   measurement requires careful attention to timing, system conditions, and
   algorithm determinism. Small errors can lead to mathematically
   impossible results.

5. **The contradiction is definitive**: Unlike statistical uncertainty, this
   is a mathematical impossibility. There is no "maybe" or "probably"—at
   least one measurement is definitively incorrect.

This analysis demonstrates the power of the work-span model as a tool for
validating parallel performance measurements and detecting errors in
experimental data.
