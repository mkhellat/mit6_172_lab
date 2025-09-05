## 🔹 Part 3: Performance Analysis

At its core, bits-array rotation involves shifting a sequence of bits
and wrapping bits that exit one end of a buffer back to the other,
thus preserving the overall content but reordering it. Despite its
apparent simplicity, bitarray rotation in C is a subtle performance
minefield, involving nuanced trade-offs among **work (number of bit
moves), time complexity, space (auxiliary storage), and cache
performance**. The diversity of real-world bit layouts (unbounded
size, arbitrary alignment, and variable packing) further complicates
the practical implementation of these algorithms.




---
### 3.A. Bitarray Representation: Anatomy and Constraints
---


#### Memory Layout

Bitarrays are typically implemented in C as compact buffers of `char`
or `uint8_t` bytes, each holding 8 bits. Bits are accessed by
addressing the correct byte and applying bitwise shift and mask
operations:

```c
struct arrayBits {
    size_t numBits;
    char *buf; // Packed bit storage
};
```

Every bit access involves:
- Finding the relevant byte `ab->buf[index/8]`
- Extracting or changing the target bit by masking and shifting

**Block diagram: Bitarray buffer for 20 bits**  
```
Logical view: [b19 | b18 | ... | b0]
Buf[0]: b0-b7 Buf[1]: b8-b15 Buf[2]: b16-b19
```

This low-level detail sets the stage for all rotation strategies,
forcibly shaping memory access patterns and dictating which methods
can exploit word-level or byte-level parallelism.




---
### 3.B. Buffer-Copy (Out-of-Place) Rotation
---


#### Algorithm Insight

The **buffer-copy method** is the most direct, conceptually simple
approach. It constructs a new buffer (as big as the rotated segment)
and copies each relevant bit from its original to its new (rotated)
location, then (optionally) copies back:

```c
for (int i = 0; i < bit_length; i++) {
    int dest_idx = (i + shift) % bit_length;
    setBit(temp_buf, dest_idx, getBit(src_buf, i));
}
```
To rotate only a subrange, adjust `i` and `dest_idx` offsets.


#### Performance Characteristics

- **Work:** $O(n)$ bit moves (where $n$ is the `bit_length`). Each bit
    is read and written once.
- **Time Complexity:** $O(n)$
- **Space Complexity:** $O(n)$ (requires temp buffer same size as
    rotated region)
- **Cache Performance:** Typically good. Sequential reads/writes
    maximize spatial cache locality, but the extra buffer incurs
    copying overhead and twice the memory traffic.
- **In-place:** No (unless a clever double buffer or in-situ chunk
    copy is used, which reverts to block-swap or triple-reverse
    ultimately).

**Major limitation:** Extra space can exceed cache for large arrays
  and, in constrained systems, is prohibitive.




---
### 3.C. Triple-Reverse (Reversal) Rotation
---


#### Algorithm Insight

The **triple-reverse method** (also called the **reversal algorithm**)
achieves in-place rotation by leveraging the mathematical identity:
`Rotate(A+B, k) = Reverse(Reverse(A) + Reverse(B)) =
Reverse(Reverse(A+B))`

**Steps:**
1. Reverse the first `k` bits in-place
2. Reverse the remaining `n-k` bits in-place
3. Reverse the entire segment in-place


#### Implementation Snippet

```c
static void arrayReversal(bitarray_t *ba, size_t begin, size_t end) {
    while(begin < end) {
        swap_bits(ba, begin, end);
        begin++; end--;
    }
}
void bitarray_rotate(bitarray_t *ba, size_t bit_off, size_t bit_len, ssize_t bit_right_amount) {
    size_t k = modulo(-bit_right_amount, bit_len);
    if(k == 0) return;
    arrayReversal(ba, bit_off, bit_off + k - 1);
    arrayReversal(ba, bit_off + k, bit_off + bit_len - 1);
    arrayReversal(ba, bit_off, bit_off + bit_len - 1);
}
```


#### Performance Characteristics

- **Work:** At most $3n$ bit swaps (each bit is swapped at most once
    per reversal, up to 3 times)
- **Time Complexity:** $O(n)$
- **Space Complexity:** $O(1)$ - strictly in-place, needs only a
    temporary variable for swapping
- **Cache Locality:** POOR for large regions. The mirror accesses in
    reversals mean traversing from both ends toward the middle,
    leading to cache thrashing for blocks larger than L1 cache.

**Intuition About Locality:**  
Triple-reverse yields non-sequential memory access when bitarrays
cross byte or word boundaries. Each reversal requires “hopping”
between distant memory locations, making repeated cache line fetches
likely.


#### Comparative Note

Despite $O(n)$ theoretical work, in practice the reversal method’s
poor cache utilization means it can be (on very large or poorly
aligned bitarrays) outperformed by block-swap or cyclic (juggling)
methods for the same problem size.



---
### 3.D. Block-Swap Rotation
---

#### Algorithm Insight

The **block-swap method** recursively (or iteratively) swaps
fixed-size blocks of elements, moving whole regions at a time until
the rotated array is in the desired order:

1. Divide the array into two blocks: A (k elements), B (n-k elements)
2. Swap blocks recursively until each is in place


#### Implementation Snippet

```c
while (block_size_A != block_size_B):
    if (block_size_A < block_size_B):
        swap(A, trailing block in B, size=A)
        shrink B to trailing subblock
    else:
        swap(leading subblock in A, B, size=B)
        shrink A
final: swap(A, B, size=A/B)
```


#### Performance Characteristics

- **Work:** $O(n)$ total moves, but each block swap moves several
    bits/bytes at once
- **Time Complexity:** $O(n)$
- **Space Complexity:** $O(1)$ (in-place, uses only temp for block
    swapping)
- **Cache Locality:** BETTER than reversal: swaps are typically
    performed in contiguous chunks, aligning better with cache
    lines. Especially efficient when implemented at byte or word
    granularity.

**Note:** When implemented per-bit, efficiency is poor; with
  word-sized blocks, block-swap (sometimes called rotation by blocks)
  is among the best in terms of minimizing total memory movement and
  maximizing locality.



---
### 3.E. Cyclic (Juggling) Rotation
---

#### Algorithm Insight

The **cyclic (juggling) algorithm** (famed in *Programming Pearls*)
organizes the rotation in terms of *cycles*, leveraging the greatest
common divisor (GCD) of bitarray length (`n`) and rotation step (`k`):

- The bit at position `i` is moved to `(i + k) % n`, and this process,
  repeated, loops back to `i` after `lcm(n, k)/k` steps, forming a
  cycle.
- There are $\text{GCD}(n, k)$ distinct cycles: each bit belongs to
  one cycle, and each cycle can be “juggled” in-place using a single
  scratch variable.


#### ASCII Visualization

```
n = 6, k = 2; cycles shown as arrows
[0] → [2] → [4] → [0]
[1] → [3] → [5] → [1]
```

- Each number indicates a bit index; arrows visualize the permutation
  cycles


#### Performance Characteristics

- **Work:** $n$ moves (each bit written once)
- **Time Complexity:** $O(n)$
- **Space Complexity:** $O(1)$ (single temp variable for swapping
    along cycles)
- **Cache Locality:** MODERATE to POOR. Since cycles may induce
    widely-scattered memory accesses, random access latency and
    cache-line reloads sap performance, particularly for large
    unaligned bitarrays.

**Pro:** Minimizes writes; each bit moved exactly once  
**Con:** Memory access is non-sequential. Good for small or
  cache-fitting arrays; suffers badly when bitarray is much larger
  than L1/L2 cache.



---
### 3.F. Bit-by-Bit (Naive) Method
---

#### Algorithm Insight

This approach rotates by shifting bits one position per assignment,
repeatedly, until the full rotation is done. For each of `k` rotation
steps, all bits in the array are shifted, and the displaced bit is
wrapped around.

**For left-rotate by k:**  
- Repeat `k` times:
  - Save leftmost bit.
  - Shift all bits left by one (in order).
  - Place the saved bit at rightmost position.


#### Implementation Snippet

```c
for(step = 0; step < k; step++) {
    save = getBit(array, 0);
    for(i = 0; i < n-1; i++) setBit(array, i, getBit(array, i+1));
    setBit(array, n-1, save);
}
```


#### Performance Characteristics

- **Work:** $O(n × k)$; *very slow* for large $n$ or $k$
- **Space:** $O(1)$
- **Cache Locality:** BAD. Since every bit is visited $O(k)$ times,
    poor spatial/temporal locality. Particularly punishing for large
    arrays.


#### Summary Judgment

**Use only as baseline or when arrays are tiny.** All other methods
  outperform the naive approach by orders of magnitude for real-world
  sizes.



---
### 3.G. Word-Shift and OR Method
---

#### Algorithm Insight

Ideal when the bitarray can be indexed as an array of fixed-width
machine words (e.g., `uint32_t[]` or `uint64_t[]`). **Within a word**,
rotate using:
```
rot(x, k) = (x << k) | (x >> (W - k))
```
Where `W = word size` (often 32 or 64).

**For arrays:**
- Each output word is a combination of bits from two input words:
  - Upper part comes from one word shifted right.
  - Lower part comes from next word shifted left.


#### Implementation Snippet

```c
for(int i = 0; i < ARR_SIZE; i++) {
    int upperPos = (i + ARR_SIZE - upperOff) % ARR_SIZE;
    int lowerPos = (i + ARR_SIZE - lowerOff) % ARR_SIZE;
    out[i] = (x[upperPos] >> (W-k)) | (x[lowerPos] << k);
}
```


#### Performance Characteristrics 

- **Work:** $O(n)$ word moves for n bits, typically implemented by
    block-wise memory accesses
- **Time Complexity:** $O(n)$
- **Space Complexity:** $O(n)$ if output buffer is separate; can
    sometimes be made in-place for specific settings with careful
    dance (but gets complicated for non-aligned/partial words)
- **Cache Locality:** EXCELLENT when buffers are contiguous and blocks
    are large, as the code walks memory linearly and uses native
    machine word size.

**Variants:**  
- **Rotate on x86 and ARM:** Both provide rotate instructions (`ROL`,
    `ROR`) which compilers can map from idiomatic C shift-or code. On
    x86 (`rol`, `ror`), often a single cycle, if the C code is
    recognized as a rotation. On ARM, newer chips have barrel shifters
    and may execute rotates as efficiently.

**Compiler Impact:** Compilers nowadays detect rotate idioms in C and
  optimize them to use machine instructions. For non-word-aligned
  bitarrays, performance drops back to manual bit-masking and
  shifting, limiting optimized use.



---
### 3.H. Lookup Table (LUT) Accelerated Rotation
---

#### Specialization: Byte/Word Reversal

If the rotation (or a step in a multistage word rotation) involves
reversing or shifting bytes, you can exploit a **precomputed lookup
table** for byte-level reversal or rotation.

For an 8-bit value, a reverse-LUT table maps `0bxxxxxxxx` to
`0bxxxxxxxx` reversed. This is particularly fast for sub-operations
like
- Reversing a byte before/after rotation
- Adjusting edge bits in partial bytes


#### Implementation Snippet

```c
static const unsigned char BitReverseTable256[256] = { ... };
reversed_byte = BitReverseTable256[byte_to_reverse];
```


### Performance Characteristics

- **Work:** Constant time per byte (table lookup)
- **Time Complexity:** $O(n)$ per bitarray, but fast per operation
- **Space Complexity:** $O(256) = 256$ bytes for the lookup table
- **Cache Locality:** EXCELLENT. Table fits in L1/2 caches, so lookup
    is very fast. However, real gains are achieved only if the
    rotation maps to per-byte operations; not generalizable to
    arbitrary bit-level rotations.


#### Advanced Use

Combined with block-wise copying (copy a region then reverse each byte
in-place via lookup), this method is extremely effective for certain
transforms—such as “rotating” data for hardware with different
endianness or bit transposition requirements.



---
### Comparative Summary Table
---

| Method           | Time    | Space   | Work (Bit Ops) | In-Place | Cache Performance | Implementation Notes                   |
|------------------|---------|---------|----------------|----------|-------------------|----------------------------------------|
| Buffer-Copy      | $O(n) $ | $O(n)$  | n              | No       | Good              | Simple, extra buffer required          |
| Triple-Reverse   | $O(n) $ | $O(1)$  | ≤3n swaps      | Yes      | Poor              | Best all-around in-place, non-seq      |
| Block-Swap       | $O(n) $ | $O(1)$  | ~n             | Yes      | Good              | Excellent for arrays/word-based ops    |
| Cyclic/Juggling  | $O(n) $ | $O(1)$  | n              | Yes      | Moderate/Poor     | Each bit written once, scattered access|
| Bit-by-Bit       | $O(nk)$ | $O(1)$  | nk             | Yes      | Very Poor         | Only for tiny buf, for correctness     |
| Word-Shift/OR    | $O(n) $ | $O(n)$  | n word ops     | No*      | Excellent         | Ideal when word-aligned                |
| Lookup-Table     | $O(n) $ | $O(1)$  | fast per byte  | Yes*     | Excellent         | Great for byte reversal in pipeline    |

*Word-shift/OR can be in-place with complexity, as can lookup-table,
 but “out-of-place” more general for generic buffers.*



---
### Cache Locality and Memory Access: Why It Matters
---

**Cache locality** is a major factor distinguishing “theoretically
  optimal” and “actually fast” methods.

- **Spatial Locality:** Sequential data access (as in buffer-copy,
    block-swap, word-shift) allows cache lines to be efficiently
    preloaded; subsequent accesses hit in L1/L2 cache.
- **Temporal Locality:** If data is accessed repeatedly in a short
    time window, as in repeated shifts in the naive method, cache
    benefits arise only for small arrays.

- **Triple-reverse/juggling:** Large “mirror” or cyclic leaps span
    memory, leading to cache lines being loaded in and evicted before
    being reused.
- **Block-swap/buffer:** Walks memory in contiguous chunks, maximizing
    cache utilization.
- **Word-shift:** Aligned block access is best-case for caches.


**Empirical data:** Modern CPUs load cache lines at 32-128
  bytes. Scattered accesses means every such chunk must be loaded
  separately, causing overhead. Linear sweeps are always faster for
  large data.

**Interpretation:**  
- **Triple-reverse with optimized byte reversal dominates for large
    buffers.**
- **Cyclic (juggling) is better than block-swap when the array is
    short enough to fit in cache, or when you can accept scattered
    access.**
- **Naive and per-bit approaches are only useful for correctness or
    very small test cases.**



---
### CPU-Level Rotation: Word-Shift/OR and Hardware Instructions
---

On most CPUs, rotating within a (byte/word/doubleword) uses a single
machine instruction:
- **x86:** use `ROL`, `ROR` (rotate left/right)
- **ARM:** use barrel shifter or `rbit` for reversal

**Compilers (GCC, Clang, etc.) now auto-detect C shift/or idioms**:
```c
result = (x << k) | (x >> (N-k));
```
and directly emit fast hardware rotate instructions.

**Benchmark Note:**  
Native register rotate is always fastest (one instruction, one cycle
on newer CPUs). For large arrays, performance then depends entirely on
how you stride and combine across words; methods that allow sequential
processing (e.g., block-swap, word-shift) map best to this capability.



---
### ARM vs x86: Rotation Performance Nuances
---

- Both ARM (AArch64) and x86_64 have single-cycle rotate instructions,
  but the best-case only applies to in-register, word-swappable
  buffers.
- For generic bitarrays (arbitrary bit offset/length and unaligned),
  performance is dictated by bit masking, shifting, and cross-word
  composition, making the in-memory rotate less efficient.
- In embedded environments, cache and memory bandwidth differences
  dominate. x86 server CPUs may outrun ARM on large buffer rotations;
  ARM embedded CPUs may close the gap for small, cache-resident
  buffers. For on-die memory (L1/2), both architectures are very
  similar for word-aligned code.



---
### In-Place vs Out-of-Place Rotation: Summary of Trade-Offs
---

- **In-Place ($O(1)$ space):** Triple-reverse, block-swap, cyclic,
    bit-by-bit, some word-shift.
  - Use for constrained memory or very large buffers.
  - May pay for non-sequential access (worse cache).
- **Out-of-Place ($O(n)$ space):** Buffer-copy, lookup-table when not
    fully in-situ, most word-shift.
  - Use for best cache and pipeline performance.
  - Only use if enough RAM is available.
- **Hybrid:** For byte/word-aligned and buffer-fittable cases (when
    buffer fits in L1/L2), often doesn't matter; go for simplest
    correct method first and profile.



---
### Best Practice Recommendations
---
- **For practical in-place rotation of arbitrary bitarrays in C:** Use
    triple-reverse if code clarity and constant space come first;
    optimize with block-wise swaps or bite-sized reversals where
    possible.
- **For arrays aligned to bytes or words:** Use word-shift/OR with
    compiler or CPU rotate support for maximal speed.
- **For very large arrays and when space is available:** Consider
    buffer-copy or block-swap for superior cache performance and
    parallelizability.
- **Always profile:** Microarchitectural effects (cache, memory,
    compiler optimization) interact in subtle ways that can change
    which algorithm wins in practice.
- **Exploit lookup tables where local reversal of small blocks is
    needed:** Ideal for bytewise transformation; not a generic
    rotation solution.
- **Hybrid approaches (reversal for large regions, LUT or word-shift
    for small sub-blocks) often yield best real-world results.**



---
### Concluding Remarks
---

**Bitarray rotation in C illustrates the nuanced interplay between
algorithmic theory, hardware realities, and implementation detail.**
While time and space complexity guide the theoretical limits,
**cache-friendliness and memory access patterns determine true
performance**, especially as arrays outgrow on-chip caches. The
**triple-reverse (reversal) algorithm**, especially when fused with
lookup-table byte reversal for subbyte regions, currently stands as
the **empirical gold standard** for in-place bitarray rotation.

However, for maximum efficiency on modern CPUs, **block-swap and
word-shift/OR** methods—where they can be applied—are superior due to
their use of contiguous, cache-optimized memory
transfers. Juggling/cyclic methods are optimal in terms of the number
of writes but often lose in practice to block-based swaps given cache
geometries.

**In sum:**  
- Choose algorithms not only by complexity but by memory and cache
  patterns.
- Combine in-place strategies with word/byte-level optimizations
  whenever possible.
- Let practical measurements—on your real data sizes and
  hardware—guide final implementation choices.


**“The difference between fast and slow is not O(n) vs O(n), but
  sequential vs scattered.”**

---

<div style="text-align: right">

[PREV: (Part 2) Rotation Algorithms](02-project1-rotation-algorithms.md) | 
[NEXT: (Part 4) Bonus Methods](04-project1-bonus-methods.md)

</div>
