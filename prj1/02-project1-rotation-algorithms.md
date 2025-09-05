## 🔹 Part 2: Rotation Algorithms

Although C lacks a built-in rotate operator, the following methods
allow for both correctness and efficiency in software
implementations. Each approach leverages the foundations just
described.



---
### 1. Buffer-Copy Rotation Method
---

**Concept:** Copy the contents of the bit array into a temporary
  buffer, then write back each bit (or block) to its rotated
  destination index.

**Pseudocode (for a bit array of length `N`, rotate by `k`):**
```c
// input[] is a copy of the original array
for (int i = 0; i < N; ++i)
    output[(i + k) % N] = input[i];
```

**ASCII Block Diagram:**
```
Before:      [ 0 1 2 3 4 5 ]
After (k=2): [ 4 5 0 1 2 3 ]
```

**Analysis:**  
- **Mathematical Principle:** Direct modular indexing; each element
    mapped with `new_index = (old_index + k) % N`.
- **Properties:** Clear, intuitive; requires $O(N)$ extra memory; not
    in-place.
- **Application:** Useful for subarray rotations and scenarios where
    extra space is available or in small-data scenarios.

**Limitations:**  
- Not memory efficient for large arrays or in embedded systems.
- Not optimal for in-place requirements, as is often specified in
  systems, crypto, or high-performance code.



---
### 2. Triple-Reverse (Reversal) Algorithm
---

**Principle:** Bit rotation can be performed via three in-place
  reversals:

1. Reverse the first \( k \) bits (call this segment $A$).
2. Reverse the remaining \( N-k \) bits (segment $B$).
3. Reverse the whole array ($A + B$ now in new order).

This sequence achieves the same arrangement as a rotation without the
need for extra space. The core idea is that cyclic rotation of the
composite array $AB$ could be formulated in terms of reversal
(involution) as follows:

\[
AB \rightarrow BA = \text{rev}(\text{rev}(BA)) =
\text{rev} (\text{rev}(A)\cdot\text{rev}(B))
\]

**C Pseudocode (concept applied to arrays):**
```c
// Reverse(A)
reverse(arr, 0, k-1);
// Reverse(B)
reverse(arr, k, N-1);
// Reverse(Reverse(A)Reverse(B))
reverse(arr, 0, N-1);
```
On a bitarray, bitwise operations (accessing/setting bit i) generalize
this logic.


**ASCII Block Diagram (8-bit) left-rotate by 3:**  
- partition the 8-bit array to $A$ of size 3 and $B$ of size 5.
- Reverse $A$ and $B$
- Revrse the full array
```
Original: [A1 A2 A3 B1 B2 B3 B4 B5]
Step 1:   [A3 A2 A1 B1 B2 B3 B4 B5]
Step 2:   [A3 A2 A1 B5 B4 B3 B2 B1]
Step 3:   [B1 B2 B3 B4 A1 A2 A3 A4]  <- Rotated
```
***This efficiently implements a rotation using only $O(1)$ auxiliary
   space.***

**Mathematics:**  
- Each reversal is an involution (its own inverse); the composition of
  three reversals is equivalent to a rotation.
- This construction exploits the symmetry of reversals over a cycle.

**Performance:**  
- $O(N)$ time, $O(1)$ space.
- Excellent cache and memory locality.

**Edge Cases:**  
- If the rotation count is zero or a multiple of the array length,
  only the identity mapping is produced; all reversals become no-ops.
- Safe to use for any range of bits: carefully handles misaligned byte
  boundaries in true bitarrays.



---
### 3. Block-Swap Rotation Method
---

**Principle:**  
Block-swap rotates an array by recursively swapping sections (blocks)
of the array, reducing the problem to progressively smaller
subproblems until only equal-sized blocks need swapping.

**Algorithm Outline:**
1. Define two blocks: $A$ (first $d$ elements), $B$ (remaining $n-d$).
2. If $A$ and $B$ are of equal size, swap them.  
3. If sizes differ, recursively swap the smaller block with the
"matching" block in the larger segment, shrinking the problem size.


**Swap Cases:**  

0. If $|A| = |B|$:
   - swap and we are done ;).

1. If $|A| < |B|$:
   - decompose the array as $A B_l B_r$ with
     - $|B_r| = |A|$
     - $|A| = d_{current}$
     - $N_{current} = |A| + |B|$
   - rearrange the array as $B_r B_l A$; $A$ is now in its final
     position;
   - the problem is reduced to the smaller rotation problem $B_r B_l
     \rightarrow B_l B_r$ with
     - $N_{new} = N_{current} - d_{current}$
     - $d_{new} = |B_r| = d_{current}$.
   - divide and conquer until $N_{new} \approx \ln N$.

2. If $|A| > |B|$:
   - decompose the array as $A_l A_r B$ with
     - $|A_l| = |B|$
     - $|A| = d_{current}$
     - $N_{current} = |A| + |B|$
   - rearrange the array as $B A_r A_l$; $B$ is now in its final
     position;
   - the problem is reduced to the smaller rotation problem $A_r A_l
     \rightarrow A_l A_r$ with
     - $N_{new} = d_{current}$
     - $d_{new} = |A_r| = 2.d_{current} - N_{current}$.
   - divide and conquer until $N_{new} \approx \ln N$.


**ASCII Block Diagram (n = 8, d = 3):**
```
Original: [1 2 3 4 5 6 7 8]

STEP 1 -- n1 = 8 | d1 = 3
           A + B : [1 2 3] [4 5 6 7 8]
  ABlBr -> BlBrA : [1 2 3] [4 5] [6 7 8] -> [6 7 8] [4 5] [1 2 3]
    Intermediate : [6 7 8 4 5] **[1 2 3]**
    (A is final)

STEP 2 -- n2 = n1 - d1 = 5 | d2 = d1 = 3
           A + B : [6 7 8] [4 5]
  AlArB -> BArAl : [6 7] [8] [4 5] -> [4 5] [8] [6 7]
    Intermediate : **[4 5]** [8 6 7] **[1 2 3]**
    (B is final)

STEP 3 -- n3 = d2 = 3 | d3 = 2d2 - n2 = 1
           A + B : [8] [6 7]
  ABlBr -> BlBrA : [8] [6] [7] -> [7] [6] [8]
    Intermediate : **[4 5]** [7 6] **[8 1 2 3]**
    (A is final)

STEP 4 -- n4 = n3 - d3 = 2 | d4 = d3 = 1
           A + B : [7] [6] -> [6] [7]
           FINAL : **[4 5 6 7 8 1 2 3]**
```

**Mathematics:**  
- Carefully tracks the cyclic permutation via a sequence of block-wise
  swaps, ultimately matching what modular arithmetic would dictate.

**Performance:**  
- $O(N)$ time, $O(1)$ extra space; slightly more complex but optimal
  for systems where per-block (e.g., word, doubleword) movement is
  faster than many bit twiddles.
- Performs fewer element moves than triple-reverse in some cases.

**Edge Cases:**  
- Particularly amenable to word-aligned (multi-byte) and
  hardware-accelerated moves, which can outperform the reversal
  approach in non-bitaligned bitarrays.

**Alternative:**  
- In bitarrays, may require careful bitwise address calculation (e.g.,
  moving partial bytes or words).



---
### 4. Bit-by-Bit Rotation Approach
---

**Principle:**  
Perform the rotation one bit at a time, shifting each bit and
explicitly handling the wrapping bit.

**Example (Rotate Right by 1):**
```c
unsigned int rightrotate(unsigned int x, unsigned int bits) {
    return (x >> 1) | ((x & 1) << (bits - 1));
}
```
**ASCII (8-bit):**
```
Before: [B7 B6 B5 B4 B3 B2 B1 B0]
After:  [B0 B7 B6 B5 B4 B3 B2 B1]
```

**Mathematics:**  
- Effectively applies the modular mapping per bit: each move is a
  permutation cycle of length one.

**Performance:**  
- $O(N*k)$, inefficient for large numbers of rotations or long arrays,
  but generalizes to arbitrary numbers of positions with inner loops
  or combined shift-and-mask.
- Useful for teaching, illustration, or small, nonperformance-critical
  usage.

**Edge Cases:**  
- Be careful with signed types: use unsigned ints to avoid issues with
  arithmetic shifts vs. logical shifts.



---
### 5. Cyclic (Juggling) Algorithm
---

**Principle:**  
The **juggling algorithm** (from Programming Pearls) decomposes the
permuted bit positions into cycles determined by the GCD of the array
length \( n \) and rotation step \( d \).

- Each cycle is followed, moving its bits directly to their final
  destination in a single pass, minimizing data moves and requiring
  only a temporary variable per cycle.

**Steps:**
1. For each of the $\text{GCD}(n, d)$ cycles:
   - Store the starting value in a temporary.
   - Move each value in the cycle forward by `d` steps (modulo n)
     until returning to starting point.
   - Place the stored temporary in the last slot.

**Block Diagram (n=6, d=2):**
```
Cycle 1: Indices 0→2→4→0
Cycle 2: Indices 1→3→5→1

[0]→[2]→[4]→[0]
[1]→[3]→[5]→[1]
```

**Mathematics:**  
- Exploits the fact that each position moves through a cycle: for any
  \( i \), its successor is \((i+d)\bmod n\), and there are
  $\text{GCD}(n,d)$ such cycles.

**Performance:**  
- O(N) time, O(1) space.
- Each element moved only once.

**Edge Cases:**  
- If \( d \) and \( n \) are coprime (GCD=1), single long cycle;
  otherwise, multiple shorter cycles.



---
### 6. Word-Shift and OR Rotation Technique
---

**Principle:**  
Rotation can be performed using standard shift and bitwise OR
operators, especially in single-machine word sizes. This is the most
idiomatic C approach for rotating values:

```c
#define BITS (sizeof(val) * CHAR_BIT)
uint32_t rotl(uint32_t val, unsigned int shift) {
    shift %= BITS; // modular safe
    return (val << shift) | (val >> (BITS - shift));
}
```

**Block Diagram (8-bit, Rotate Left by 3):**
```
Original:   b7 b6 b5 b4 b3 b2 b1 b0
LeftShift3: b4 b3 b2 b1 b0 0  0  0
RightShift(N-3): 0  0  0  b7 b6 b5
OR result:  b4 b3 b2 b1 b0 b7 b6 b5
```

**Mathematics:**  
- The mask `(BITS - shift)` ensures modular index mapping.
- This matches the permutation function fundamental to circular bit
  rotation.

**Performance:**  
- $O(1)$ time, $O(1)$ space.
- Compilers may recognize these patterns and emit single rotate
  assembly instructions on architectures with such support.

**Edge Cases:**  
- Always mask shift count to avoid undefined behavior for shifts equal
  to or exceeding type width.
- Use unsigned types for all bitwise manipulation to ensure logical,
  not arithmetic, shifting.



---
### ASCII/Unicode Block Diagrams for Permutations and Memory Movements
---

Here are illustrative diagrams for several methods:

#### Triple-Reverse Algorithm (Rotating Right by 4, 8-bit array)

```
Original: [A1 A2 A3 A4 B1 B2 B3 B4]

Step 1 - Reverse A (first k=4):  [A4 A3 A2 A1 B1 B2 B3 B4]
Step 2 - Reverse B (next n-k):   [A4 A3 A2 A1 B4 B3 B2 B1]
Step 3 - Reverse whole array:    [B1 B2 B3 B4 A1 A2 A3 A4]  // Rotated right by 4
```

#### Cyclic (Juggling) Algorithm (n=6, d=2):

```
[0] -> [2] -> [4] -> [0]
[1] -> [3] -> [5] -> [1]
```

#### Word-Shift and OR Technique (8-bit, Rotate Left by 3):

```
[ b7 b6 b5 b4 b3 b2 b1 b0 ] (original)
[ b4 b3 b2 b1 b0 b7 b6 b5 ] (after rotate left by 3)
```

#### Permutation Map (Generalized):

```
Original        Rotated by k
index: 0 1 2 3 4 ... N-1
maps:  k k+1 k+2 ... (k-1) (all mod N)
```



---
### Standard C Bit Rotation Intrinsics
---

C does not provide built-in rotate operators, but many compilers and
system libraries define **intrinsics** for efficient bit rotation
exploiting native CPU instructions:

- **MSVC:** `_rotl`, `_rotr`, `_rotl64`, `_rotr64` in `<stdlib.h>`
- **GCC/Clang/ICC:** Often recognize idioms like `(x << r) | (x >> (N
    - r))` as rotate and produce a single assembly instruction (where
    supported).
- **C++20:** Introduces `std::rotl(x, s)` and `std::rotr(x, s)` in
    `<bit>`, precisely and portably implementing modular rotation with
    built-in type and shift masking.

**Example:**
```c
#include <stdlib.h>
unsigned int rotated = _rotl(value, k); // MSVC
```
Always use unsigned types and mask the rotation count to prevent
undefined behavior.



---
### Memory Movement and Permutation Mapping
---

All efficient rotation methods fundamentally boil down to memory
rearrangement guided by modular indexing:

- Methods like **buffer-copy** simply use modular arithmetic to map
  each source to its destination in the temporary array.
- **In-place methods** (triple-reverse, block-swap, juggling)
    carefully orchestrate data movement to avoid overwriting
    still-needed values, using temporary variables/intermediate swaps.
- **Word-shift and OR** approaches perform the mapping in hardware,
    exploiting the CPU's ability to extract bit blocks from high and
    low positions and merge them.

Block diagrams above illustrate these mappings.



---
### Comparative Analysis of Rotation Methods
---

The table below summarizes key tradeoffs:

| Method              | Auxiliary Space | Time Complexity | Best Use Case             | Edge Cases/Notes                     |
|---------------------|-----------------|-----------------|---------------------------|--------------------------------------|
| Buffer-Copy         | $O(N)$          | $O(N)  $        | Simplicity, Small Arrays  | Not in-place, memory overhead        |
| Triple-Reverse      | $O(1)$          | $O(N)  $        | In-place, cache-friendly  | Edge-aligned bits require care       |
| Block-Swap          | $O(1)$          | $O(N)  $        | Large words, block-swaps  | Mildly more complex implementation   |
| Bit-by-Bit          | $O(1)$          | $O(N·k)$        | Didactic, small arrays    | Use unsigned, slow for large arrays  |
| Cyclic/Juggling     | $O(1)$          | $O(N)  $        | Minimum moves, in-place   | $\text{GCD}(N, k)$ determines efficiency |
| Word-Shift and OR   | $O(1)$          | $O(1)  $        | Fixed-width, CPU support  | Use unsigned, mask shift count       |

**Key Points:**

- For subarrays or misaligned bit blocks, triple-reverse and
  block-swap offer optimal in-place tradeoff.
- Cyclic/juggling is especially potent when $\text{GCD}(n, k)$ is
  small (few cycles).
- Buffer-copy is clearest but rarely used in performance-sensitive
  code.
- Use word-shift and OR when working on whole words/unsigned integers,
  letting the compiler or intrinsics optimize to machine instructions.



---
### Common Inaccuracies and Edge Cases
---

Practical use uncovers several common misunderstandings:

- **Signed vs. Unsigned Types:** Always use unsigned for rotations;
    right-shifting signed values may invoke arithmetic shifts, which
    change the sign bit instead of rotating it.
- **Zero or Large Rotation Counts:** Ensure rotation count is always
    taken modulo word/array length; `(x << N)` or `(x >> N)` is
    undefined in C if N equals or exceeds type width.
- **Alignment and Subarray Rotations:** Rotating subranges in a
    bitarray (not byte-aligned) often requires special care in bit
    addressing.
- **Masked Shift Amounts:** Always mask the rotation count to
    well-defined bounds using `k &= (BITS - 1);`.
- **Compiler Optimization:** Compilers may not always recognize
    shift/OR patterns unless written idiomatically and with unsigned
    types. Intrinsics or C++20's `<bit>` are safe choices where
    available.
- **Testing and Debugging:** For edge cases, such as complete or zero
    rotation (identity mapping), test cases should verify that data is
    unchanged.


**Illustrative example:**
```c
// Rotate a 32-bit unsigned integer left by k positions, safe and portable:
uint32_t rotate_left(uint32_t n, unsigned int k) {
    k %= 32; // modular arithmetic ensures wrap
    return (n << k) | (n >> (32 - k));
}
```
**This combines and makes explicit all essential mathematical insights
  described above.**

---

<div style="text-align: right">

[PREV: (Part 1) Mathematical Foundations](01-project1-mathematical-foundations.md) | 
[NEXT: (Part 3) Performance Analysis](03-project1-performance-analysis.md)

</div>

