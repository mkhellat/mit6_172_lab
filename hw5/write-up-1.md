# Tight Bound for Greedy Schedulers in Parallel Computation

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

An intution towards the tight bound is that the worst-case parallelism
is one in which no strands could be run concurrently; i.e. a single
branch whose length is

$$
T_1 = T_P \quad \forall P,
$$

and since there is a single branch in the dag, that branch is
basically the span:

$$
T_1 = T_P = T_{\infty}
$$

The loose bound on $T_P$, i.e. $T_1/P + T_{\infty}$ is looser compared
to our obtained constraint for zero-parallelism :

$$
T_P = T_{\infty}
$$

by the amount $T_1/p$. However, It is instantly evident that the new
bound satisfies the zero-parallelism constraint.

To formally prove the valididty of the tight bound, one basically
would need to classify each step of a computation, using $P$ threads,
either as complete or incomplete. For each complete step, $P$ units of
computations are performed and for each incomplete step, strictly less
than $P$ units of computation is done. The number of complete steps is
denoted by $n_p$ and the number of incomplete steps by $n_i$:

$$
T_P = n_p + n_i.
$$

Now if there is a single critical path in the dag:

$$
T_{\infty} > \frac{T_1}{P},
$$

the number of incomplete steps would be greater than zero ($n_i > 0$)
since there would at least be one strand that could not be concurrenly
run alongside $(P-1)$ other strands:

$$
T_{\infty} > \frac{T_1}{P} \iff n_i > 0.
$$

Since we are trying to find an upper bound for $T_P$, we should assume
$n_i > 0$. Furthermore, the length of the single critical path (span)
is $T_{\infty}$ steps and for the sake of generality (to find the
upper bound), we should assume that during each of these steps, only
and only one strand is executed:

$$
n_{i} \leq T_{\infty}.
$$

Consequenly:

$$
n_p P \leq T_1 - T_{\infty}
$$

And this concludes the proof.