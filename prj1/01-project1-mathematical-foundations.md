## 🔹 Part 1: Mathematical Foundations of Bitarray Rotation

Bitarray rotations—shifting the bits within a fixed-size array so that
those falling off one end wrap around to the other—are central
operations in low-level programming, cryptography, signal processing,
and simulation software.

We focus on the essential ideas of **modular arithmetic**,
**permutations**, and **symmetry** as they relate to common rotation
algorithms—**buffer-copy**, **triple-reverse**, **block-swap**,
**bit-by-bit**, **cyclic (juggling)**, and **word-shift and
OR**. Detailed ASCII/Unicode diagrams are incorporated to make
permutation mapping and memory movement concrete, with careful
attention to standard C idioms, best practices, edge cases, and
frequently misunderstood aspects.


---
### (Part 1.1) Modular Arithmetic: Wrapping Around in Bit Operations
---

At its core, a bitarray rotation is a **circular shift**: each bit
moves a certain number of positions, and those that would "fall off"
the end wrap around to the start. Unlike a logical shift, which simply
discards overflowed bits, a circular shift ensures every bit is
preserved and repositioned. This behavior is governed by the rules of
**modular arithmetic**.

When rotating an \( N \)-bit array by \( k \) positions, we only care
about the rotation distance modulo \( N \), since rotating by a full
cycle returns the array to its original state:

\[
k_{\text{effective}} = k \bmod N
\]

Thus, rotating by \( k = N \) is the identity operation. Similarly,
rotating by \( k \) positions to the right is equivalent to rotating
by \( N - k \) to the left, thanks to the symmetry in the modular
cycle.


**Equation for New Position:**

\[
\text{new position} = (\text{current position} + k) \bmod N
\]

This simple use of modular arithmetic is foundational to all bit
rotation algorithms in C and underpins their correctness.


#### ASCII Block Diagram: Modular Rotation (6-bit Example)

Before (indices 0–5):

```
┌──┬──┬──┬──┬──┬──┐
│0 │1 │2 │3 │4 │5 │
└──┴──┴──┴──┴──┴──┘
```

After rotating left by 2 (mod 6):

```
┌──┬──┬──┬──┬──┬──┐
│2 │3 │4 │5 │0 │1 │
└──┴──┴──┴──┴──┴──┘
```

Where each index moves to:
- 0 → 2 = $(0 + 2) \mod 6$
- 1 → 3 = $(1 + 2) \mod 6$
- 2 → 4 = $(2 + 2) \mod 6$
- 3 → 5 = $(3 + 2) \mod 6$
- 4 → 0 = $(4 + 2) \mod 6$
- 5 → 1 = $(5 + 2) \mod 6$

In other words, in the context of index mapping, rotating left by $d$
means:

\[
i \mapsto (i + d) \mod N
\]

And rotating right by $d$ means:

\[
i \mapsto (i - d + N) \mod N
\]

This is a permutation of the bit positions. It rearranges the bits
without changing their values.


#### Modular Arithmetic in C Rotations

Bit rotations are not built into the C language, but standard idioms
use bitwise operators to simulate them. For example, to rotate a
32-bit unsigned integer `n` left by `k` bits:

```c
#define BITS 32
uint32_t rotl(uint32_t n, unsigned int k) {
    k %= BITS; // modular arithmetic!
    return (n << k) | (n >> (BITS - k));
}
```

This approach leverages modular wraparound by using bitwise shifts and
the OR operator to recombine overflowed bits, as illustrated below:

- `n << k` shifts the bits left, placing zeros on the right.
- `n >> (BITS - k)` shifts the bits right, catching the bits that
  “wrapped around”.
- The bitwise OR completes the cycle, producing the rotated value.

**Critical Point:** Modular arithmetic ensures that the rotation count
  is always within the valid range (`k % BITS`), preventing undefined
  behavior from excessive shift counts.



---
### (Part 1.2) Bit Rotation as a Permutation
---

At a deeper level, a **bit rotation** is a specific example of a
permutation: it rearranges the bits in a deterministic cycle. Each bit
moves to a new location determined by modular addition. In permutation
theory, such an operation is called a **cyclic permutation**.


**Permutation Mapping:**

\[
\text{Destination index} = (\text{Source index} + k) \bmod N
\]

Each bit “travels” around the circle, returning to its original
position after \( N \) rotations. This deterministic cycle ensures
every bit occupies every possible position over repeated operations, a
property crucial for cryptography and encoding schemes.


#### Cyclic Permutation Diagram (8-bit Case, Left Rotation by 3):

Bit positions labeled B0 (LSB) to B7 (MSB):

```
Before: B7 B6 B5 B4 B3 B2 B1 B0
         |  |  |  |  |  |  |  |
After:  B4 B3 B2 B1 B0 B7 B6 B5

Legend: Arrow from before to after:
B7 → B4, B6 → B3, ..., B2 → B7 (wraps around), ,,,
```

Each bit moves three places to the left, with those “falling off” the
left end wrapping to the right end, thanks to modular arithmetic.



#### Cycle Decomposition

Permutations can be expressed as **cycles**. For bit rotations, there
is always a single cycle (if \( N \) and \( k \) are coprime) or
multiple shorter cycles otherwise, as determined by the greatest
common divisor (GCD) of \( N \) and the rotation distance \( k \):

- Number of cycles \( = \gcd(N, k) \)
- Cycle length \( = N / \gcd(N, k) \)

The famous **juggling algorithm** for array/bit rotations directly
exploits this fact, walking each cycle once.

**Example (n = 6, d = 2):**
- $\text{GCD}(6, 2) = 2$
- Cycle 1: indices 0 → 2 → 4 → 0
- Cycle 2: indices 1 → 3 → 5 → 1



---
### (Part 1.3)  Symmetry and Inverses
---

Bit rotations possess strong symmetries:

1. **Rotating by \( k \) and then by \( N-k \) undoes the operation
(restores the original sequence)**, just as rotating by \( k \) and
then by \(-k\) (the inverse operation modulo \( N \)) returns to the
starting configuration.

2. **Bidirectionality:** Rotating left by \( k \) is the same as
rotating right by \( N-k \).

3. **Identity Element:** Rotating by a multiple of the array length
yields the identity (original array), a reflection of the system's
cyclic structure.

These properties arise from the underlying **cyclic symmetry**: every
bit position is equivalent under rotations, and operations commute in
the sense that sequences of rotations (modulo the array length) can be
combined or reordered without changing the end result.

---

<div style="text-align: right">

[PREV: The In-Place Mandate](00-project1-the-inplace-mandate.md) | 
[NEXT: (Part 2) Rotation Algotithms](02-project1-rotation-algorithms.md)

</div>
