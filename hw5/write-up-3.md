# Inconsistent Parallel Execution Time Measurements

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

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
