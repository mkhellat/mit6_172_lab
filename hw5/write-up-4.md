# Professor Karan's Inconsistent Measurements

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

---

## Problem Statement

Professor Karan measures her deterministic multithreaded algorithm on 4,
10, and 64 processors of an ideal parallel computer using a greedy
scheduler. She claims that the three runs yielded:
- $T_4 = 80$ seconds
- $T_{10} = 42$ seconds
- $T_{64} = 10$ seconds

Argue that the professor is either lying or incompetent.

---

## Constraints from Each Measurement

For any valid set of measurements, the following laws must be satisfied:
- **Work Law**: $T_P \geq T_1/P$
- **Span Law**: $T_P \geq T_{\infty}$
- **Greedy Scheduler Bound** (from Exercise 27.1-3): $T_P \leq (T_1 - T_{\infty})/P + T_{\infty}$

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
T_4 \leq \frac{T_1 - T_{\infty}}{4} + T_{\infty}
$$

$$
80 \leq \frac{T_1 - T_{\infty}}{4} + T_{\infty}
$$

$$
320 \leq T_1 - T_{\infty} + 4T_{\infty}
$$

$$
320 \leq T_1 + 3T_{\infty}
$$

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
T_{10} \leq \frac{T_1 - T_{\infty}}{10} + T_{\infty}
$$

$$
42 \leq \frac{T_1 - T_{\infty}}{10} + T_{\infty}
$$

$$
420 \leq T_1 - T_{\infty} + 10T_{\infty}
$$

$$
420 \leq T_1 + 9T_{\infty}
$$

$$
T_1 \geq 420 - 9T_{\infty}
$$

### Constraints from $T_{64} = 10$

**Work Law**:
$$
T_1 \leq 640
$$

**Span Law**:
$$
T_{\infty} \leq 10
$$

**Greedy Scheduler Bound**:
$$
T_{64} \leq \frac{T_1 - T_{\infty}}{64} + T_{\infty}
$$

$$
10 \leq \frac{T_1 - T_{\infty}}{64} + T_{\infty}
$$

$$
640 \leq T_1 - T_{\infty} + 64T_{\infty}
$$

$$
640 \leq T_1 + 63T_{\infty}
$$

$$
T_1 \geq 640 - 63T_{\infty}
$$

---

## Finding a Contradiction

### Step 1: Combine Span Law Constraints

From the Span Law:
- $T_{\infty} \leq 80$ (from $T_4$)
- $T_{\infty} \leq 42$ (from $T_{10}$)
- $T_{\infty} \leq 10$ (from $T_{64}$)

The tightest constraint is:

$$
T_{\infty} \leq 10
$$

### Step 2: Check Consistency of $T_4$ and $T_{10}$

If $T_{\infty} \leq 10$, then from the greedy scheduler bounds:
- From $T_4$: $T_1 \geq 320 - 3T_{\infty} \geq 320 - 3(10) = 290$
- From $T_{10}$: $T_1 \geq 420 - 9T_{\infty} \geq 420 - 9(10) = 330$

Also from Work Law:
- From $T_4$: $T_1 \leq 320$
- From $T_{10}$: $T_1 \leq 420$

So we need:

$$
330 \leq T_1 \leq 320
$$

This is **impossible**! There is no value of $T_1$ that satisfies both
$330 \leq T_1$ and $T_1 \leq 320$.

### Step 3: The Contradiction

From $T_{64} = 10$ (Span Law), we have $T_{\infty} \leq 10$.

Combining this with the greedy scheduler bounds:
- From $T_4$: $T_1 \geq 320 - 3T_{\infty} \geq 320 - 3(10) = 290$
- From $T_{10}$: $T_1 \geq 420 - 9T_{\infty} \geq 420 - 9(10) = 330$

From the Work Law:
- From $T_4$: $T_1 \leq 320$

Therefore, we need:

$$
330 \leq T_1 \leq 320
$$

which is **impossible**.

This contradiction proves that **at least one measurement is
incorrect**, meaning the professor is either lying or incompetent.

---

## Conclusion

The measurements are **inconsistent**. The contradiction arises as
follows:

1. From $T_{64} = 10$ (Span Law): $T_{\infty} \leq 10$
2. From $T_{10} = 42$ (greedy scheduler bound): $T_1 \geq 420 -
9T_{\infty} \geq 420 - 90 = 330$
3. From $T_4 = 80$ (Work Law): $T_1 \leq 320$

These constraints require:

$$
330 \leq T_1 \leq 320
$$

which is **impossible**.

**Therefore, Professor Karan is either lying or incompetent. At least one
of the measurements ($T_4$, $T_{10}$, or $T_{64}$) must be incorrect.**

The contradiction shows that it is impossible for all three
measurements to be correct simultaneously, given the fundamental laws
of parallel computation (Work Law, Span Law, and Greedy Scheduler
Bound from Exercise 27.1-3).

