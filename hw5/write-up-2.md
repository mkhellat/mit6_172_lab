# Parallelism Bounds Analysis

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

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

## Conclusion

The parallelism bounds demonstrate how the Work Law, Span Law, and
greedy scheduler bound interact to constrain the possible values of
$T_1$ and $T_{\infty}$. The minimum parallelism occurs when the span
is maximized (limited by $T_{64} = 10$), while the maximum parallelism
occurs when the work is maximized and the span is minimized to satisfy
the greedy scheduler bound for $P=64$.
