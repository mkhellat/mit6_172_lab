# Parallelism Bounds Analysis

> "The work-span model provides fundamental limits on parallel
> performance, independent of the scheduling algorithm." - CLRS Chapter 27

---

## Problem Statement

Ben Bitdiddle measures his deterministic program on 4 and 64 processors on an ideal parallel computer using a greedy scheduler. He obtains running times of **T4 = 100 seconds** and **T64 = 10 seconds**.

Based on the following formulas:
- **Work Law**: TP ≥ T1/P
- **Span Law**: TP ≥ T∞
- **Greedy Scheduler Bound**: TP ≤ (T1 - T∞)/P + T∞

Find:
1. The lowest possible value for the parallelism (T1/T∞)
2. The highest possible value for the parallelism (T1/T∞)

---

## Constraints

### Work Law (TP ≥ T1/P)

From the Work Law:
- T4 ≥ T1/4 → T1 ≤ 400
- T64 ≥ T1/64 → T1 ≤ 640

The tighter constraint is **T1 ≤ 400**.

### Span Law (TP ≥ T∞)

From the Span Law:
- T4 ≥ T∞ → T∞ ≤ 100
- T64 ≥ T∞ → T∞ ≤ 10

The tighter constraint is **T∞ ≤ 10**.

### Greedy Scheduler Bound (TP ≤ (T1 - T∞)/P + T∞)

From the greedy scheduler bound:
- T4 ≤ (T1 - T∞)/4 + T∞ → 100 ≤ T1/4 + 3T∞/4 → **T1 ≥ 400 - 3T∞**
- T64 ≤ (T1 - T∞)/64 + T∞ → 10 ≤ T1/64 + 63T∞/64 → **T1 ≥ 640 - 63T∞**

---

## Finding Minimum Parallelism

To minimize T1/T∞, we want to **maximize T∞** and **minimize T1**.

From the Span Law, the maximum T∞ = 10.

When T∞ = 10:
- From greedy bound P=4: T1 ≥ 400 - 3(10) = 370
- From greedy bound P=64: T1 ≥ 640 - 63(10) = 10
- From Work Law: T1 ≤ 400

The tightest lower bound is **T1 ≥ 370**.

At T1 = 370, T∞ = 10:
- Parallelism = 370/10 = **37**

**Verification**:
- Work Law: T1/4 = 92.5 ≤ 100 ✓, T1/64 = 5.78 ≤ 10 ✓
- Span Law: T∞ = 10 ≤ 100 ✓, T∞ = 10 ≤ 10 ✓
- Greedy bound P=4: T1/4 + 3T∞/4 = 92.5 + 7.5 = 100 ≤ 100 ✓
- Greedy bound P=64: T1/64 + 63T∞/64 = 5.78 + 9.84 = 15.62 ≥ 10 ✓

---

## Finding Maximum Parallelism

To maximize T1/T∞, we want to **maximize T1** and **minimize T∞**.

From the Work Law, the maximum T1 = 400.

We need to find the minimum T∞ such that all constraints are satisfied when T1 = 400.

When T1 = 400:
- From greedy bound P=64: 400 ≥ 640 - 63T∞ → 63T∞ ≥ 240 → **T∞ ≥ 240/63**
- From greedy bound P=4: 400 ≥ 400 - 3T∞ → 3T∞ ≥ 0 → T∞ ≥ 0

The tightest lower bound is **T∞ ≥ 240/63**.

At T1 = 400, T∞ = 240/63:
- Parallelism = 400 / (240/63) = 400 × 63/240 = **105**

**Verification**:
- Work Law: T1/4 = 100 ≤ 100 ✓, T1/64 = 6.25 ≤ 10 ✓
- Span Law: T∞ = 240/63 ≈ 3.81 ≤ 100 ✓, T∞ ≈ 3.81 ≤ 10 ✓
- Greedy bound P=4: T1/4 + 3T∞/4 = 100 + 2.86 = 102.86 ≥ 100 ✓
- Greedy bound P=64: T1/64 + 63T∞/64 = 6.25 + 3.75 = 10 ≤ 10 ✓

---

## Answers

1. **Lowest possible parallelism**: **37**
   - Achieved when T1 = 370, T∞ = 10
   - This maximizes T∞ (at the Span Law limit) and minimizes T1 (at the greedy scheduler bound for P=4)

2. **Highest possible parallelism**: **105**
   - Achieved when T1 = 400, T∞ = 240/63 ≈ 3.8095
   - This maximizes T1 (at the Work Law limit for P=4) and minimizes T∞ (at the intersection of both greedy scheduler bounds)

---

## Conclusion

The parallelism bounds demonstrate how the Work Law, Span Law, and greedy scheduler bound interact to constrain the possible values of T1 and T∞. The minimum parallelism occurs when the span is maximized (limited by T64 = 10), while the maximum parallelism occurs when the work is maximized and the span is minimized to satisfy the greedy scheduler bound for P=64.

