## 🔹 Part 4: Bonus Methods

Bitarray rotation, which cyclically shifts the positions of bits
within a memory buffer or register, is a fundamental low-level
operation. We did review the conventional rotation methods: naïve,
triple-reverse, block-swap, bit-by-bit, cyclic (juggling), and
word-shift/OR techniques; however, a rich set of more advanced or
less-traditional approaches exists. These methods are vital for
performance-sensitive software, cryptographic routines, digital signal
processing, and compressed data structures, especially when
implemented in standard C without resorting to compiler intrinsics or
assembly.

Following is a touch on some of the less-traditional rotation methods.



---
### 1. De Bruijn Sequence Bit Rotation
---

#### Conceptual Overview

The **de Bruijn sequence** is a cyclic sequence where every possible
substring of length \(n\) over an alphabet of size \(k\) appears
exactly once. In binary contexts (\(k=2\)), it enables efficient
computation of bit positions, such as finding the index of the lowest
set bit (LSB) in constant time. This is particularly useful in
bitarray rotation algorithms, where knowing bit positions helps
optimize operations like cycle detection and alignment.

The classic method uses a de Bruijn sequence to map a unique substring
(generated via multiplication and shifting) to a precomputed lookup
table, yielding the LSB index without loops.

```c
#include <stdio.h>
#include <stdint.h>

// Finds the index (0-31) of the lowest set bit in a 32-bit word
uint8_t lowestBitIndex(uint32_t v) {
    // Precomputed de Bruijn sequence lookup table for n=5 (32 values)
    static const uint8_t BitPositionLookup[32] = {
        0, 1, 28, 2, 29, 14, 24, 3,
        30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7,
        26, 12, 18, 6, 11, 5, 10, 9
    };
    
    // Isolate the lowest set bit: v & -v uses two's complement
    uint32_t isolatedBit = v & -v;
    
    // Multiply by de Bruijn constant 0x077CB531 (generator for B(2,5))
    uint32_t hash = isolatedBit * 0x077CB531U;
    
    // Shift right by 27 to get the unique 5-bit substring (hash value)
    uint8_t index = hash >> 27;
    
    // Map the hash value to the actual bit position
    return BitPositionLookup[index];
}

// Example: Rotate a 32-bit word right by the position of its LSB
uint32_t rotateWordByLSB(uint32_t word) {
    uint8_t pos = lowestBitIndex(word);
    return (word >> pos) | (word << (32 - pos)); // Standard bit rotation
}

int main() {
    // Example: Rotate 0x90 (binary 10010000) by its LSB position (4)
    uint32_t testWord = 0x90; // Binary: 10010000
    uint32_t rotated = rotateWordByLSB(testWord);
    
    printf("Original word: 0x%x\n", testWord);
    printf("LSB position: %u\n", lowestBitIndex(testWord));
    printf("Rotated word: 0x%x\n", rotated); // Output: 0x90000009
    return 0;
}
```


#### Step-by-Step Example

1. **Isolate the Lowest Set Bit**:
   - For `v = 0x90` (binary `10010000`), `v & -v` isolates the LSB:
     ```
     v:      10010000
     -v:     01110000 (two's complement)
     v & -v: 00010000 (0x10 in hex)
     ```

2. **Multiply by De Bruijn Constant**:
   - `0x10 * 0x077CB531 = 0x77CB5310` (produces a unique 32-bit product).

3. **Extract Hash Index**:
   - Shift `0x77CB5310` right by 27 bits: `0x77CB5310 >> 27 = 0xF` (decimal 15).

4. **Look Up Bit Position**:
   - `BitPositionLookup[15] = 4` (the LSB position).

5. **Apply Rotation**:
   - Rotate `0x90` right by 4 positions:
     ```
     Original: 10010000 00000000 00000000 00000000 (0x90000000 in big-endian)
     After rotation: 10010000 00000000 00000000 00001001 (0x90000009)
     ```


#### Performance and Use Cases

- **Time Complexity**: Constant time (\(O(1)\)) for finding bit positions.
- **Applications**: 
  - Accelerating bit-level rotations in cryptography or graphics.
  - Cycle detection in juggling-based array rotation algorithms.
  - Optimizing boundary handling in bitarray manipulations.



---
### 2. Lookup Table Chunk Rotation
---

Lookup tables (LUTs) could be used for bit manipulation, particularly
in the context of Morton encoding, which is a form of bit
interleaving—not direct bitarray rotation. However, LUTs can indeed be
applied to bitarray rotation in certain scenarios, especially when
dealing with small chunks (e.g., bytes) or when rotation involves
complex bit-level permutations.

In bitarray rotation, the goal is to shift bits left or right with
wrap-around. LUTs can accelerate this process by precomputing rotated
versions of small bit chunks (e.g., 8-bit bytes). For example:
- If you need to rotate a large bitarray by a fixed number of bits,
  you can precompute an LUT that maps each possible byte value to its
  rotated version for that fixed amount.
- Then, you process the bitarray byte-by-byte: apply the LUT to each
  byte, and handle the carry-over between bytes manually (since bits
  that shift out of one byte must move to the adjacent byte).

This approach is useful when:
- The rotation amount is fixed and known at compile time.
- You are working on systems without hardware support for bit rotation
  (e.g., some embedded systems).
- You need to rotate many bytes repeatedly, and the LUT reduces
  instruction count.

However, for arbitrary rotation amounts, LUTs become less practical
due to the need for multiple LUTs (one per rotation amount), which can
consume memory. For standard rotation of whole words (e.g., 32-bit or
64-bit), direct shift-and-OR operations are usually more efficient and
avoid LUT overhead.


#### Step-by-Step Example

Suppose you want to rotate a single byte left by 3 bits
frequently. You can precompute a LUT for all 256 possible bytes:

```c
#include <stdint.h>

// Precompute a LUT for rotating an 8-bit byte left by 3 bits
void precompute_rotate_left_3(uint8_t lut[256]) {
    for (uint32_t i = 0; i < 256; i++) {
        lut[i] = (i << 3) | (i >> 5); // Standard rotate left by 3 bits
    }
}

int main() {
    uint8_t lut[256];
    precompute_rotate_left_3(lut);

    uint8_t byte = 0b10010001; // Example byte: 0x91
    uint8_t rotated_byte = lut[byte]; // Should be 0b10001100 (0x8C)

    // Verify: 
    // Original: 10010001
    // Rotated left by 3: 10001100 (the leftmost 3 bits "100" wrap to the right)
    return 0;
}
```

For a larger bitarray (e.g., a string of bytes), you would apply this
LUT to each byte and then handle the carry-over between bytes. For
example, when rotating a multi-byte array left by 3 bits:
1. Apply the LUT to each byte, which rotates each byte individually.
2. The top 3 bits of each byte (before rotation) are lost in the
within-byte rotation, so you need to carry these bits to the next
byte. Specifically, the top 3 bits of byte `i` should become the
bottom 3 bits of byte `i-1` (for left rotation), requiring additional
bitwise operations.

This carry-handling makes the process more complex, and often, direct
shift operations on the whole array are preferred for clarity and
performance.


#### Performance Analysis

- **Time Complexity**: $O(1)$ per chunk (e.g., byte) when using a LUT,
    as opposed to $O(n)$ for a loop-based bit-by-bit
    rotation. However, for whole words, direct rotation instructions
    are $O(1)$ and faster than LUTs on modern CPUs.
- **Memory Cost**: A LUT for byte rotation with a fixed amount
    requires 256 bytes. For multiple rotation amounts, memory usage
    grows linearly (e.g., 8 rotation amounts would need 2 KB).
- **Practical Use**: LUTs are best suited for rotating small chunks in
    isolation or in environments without hardware rotation
    support. For general bitarray rotation, they are often not the
    optimal choice due to carry-over complexities.


#### Conclusion

While LUTs can be used for bitarray rotation in specific cases, they
are more commonly applied to other bit manipulations like bit reversal
or interleaving. For standard rotation, prefer direct bitwise
operations unless you have a proven performance need for LUTs in your
context.



---
### 3. Arithmetic Barrel Shifter Simulation
---

#### Conceptual Overview

The arithmetic barrel shifter simulation uses bitwise operations to
rotate a word left by a specified number of bits. The operation is
performed in constant time (\(O(1)\)) for a single word by combining
left and right shifts with a bitwise OR. This method efficiently
handles wrap-around of bits without loops, making it suitable for
hardware simulation and high-performance software.

The code for rotating an 8-bit word is:
```c
#include <stdint.h>

uint8_t barrel_rotate_left(uint8_t val, uint8_t shift) {
    shift = shift % 8; // Ensure shift is within 0-7
    return (val << shift) | (val >> (8 - shift));
}
```


#### Arithmetic Barrel Shifter Hardware

A barrel shifter is a digital circuit that can rotate or shift a data
word by any number of bits in a single clock cycle. It is constructed
using a series of multiplexers arranged in stages, each handling a
power-of-two shift amount. For an \(n\)-bit word, there are
\(\log_2(n)\) stages. For an 8-bit barrel shifter, there are 3 stages,
handling shifts by 1, 2, and 4 bits respectively. The control signals
are the bits of the shift amount \(s\): \(s_0\) (LSB) for shift-by-1,
\(s_1\) for shift-by-2, and \(s_2\) (MSB) for shift-by-4.

**Hardware Structure**  

- Each stage consists of 8 multiplexers (for 8-bit word), each
  selecting between the current bit and a bit from a shifted position
  based on the control signal.
- For left rotation, each stage rotates the bits circularly:
  - **Shift-by-1 stage**: If \(s_0 = 1\), output bit \(i\) is input
      bit \((i-1) \mod 8\).
  - **Shift-by-2 stage**: If \(s_1 = 1\), output bit \(i\) is input
      bit \((i-2) \mod 8\).
  - **Shift-by-4 stage**: If \(s_2 = 1\), output bit \(i\) is input
      bit \((i-4) \mod 8\).
- The stages are cascaded, and the output of one stage is the input to
  the next. The final output is the rotated word.


**Example: Left Rotation by 3 of an 8-bit Word**  

- **Input word**: \(0xD2\) (binary `11010010`), where:
  - Bit 7 (MSB) = 1
  - Bit 6 = 1
  - Bit 5 = 0
  - Bit 4 = 1
  - Bit 3 = 0
  - Bit 2 = 0
  - Bit 1 = 1
  - Bit 0 (LSB) = 0
- **Shift amount**: \(s = 3\) (binary `011`), so \(s_0 = 1\), \(s_1 =
    1\), \(s_2 = 0\).

**Step-by-Step Hardware Simulation**  

1. **Initial input**: Bits labeled as \(A_7\) to \(A_0\):
   ```
   A7 A6 A5 A4 A3 A2 A1 A0
   1  1  0  1  0  0  1  0
   ```

2. **Shift-by-1 stage** (\(s_0 = 1\)):
   - Output bit \(i\) becomes input bit \((i-1) \mod 8\).
   - So:
     - Output O1_7 = A6 = 1
     - O1_6 = A5 = 0
     - O1_5 = A4 = 1
     - O1_4 = A3 = 0
     - O1_3 = A2 = 0
     - O1_2 = A1 = 1
     - O1_1 = A0 = 0
     - O1_0 = A7 = 1
   - After this stage:
     ```
     O1_7 O1_6 O1_5 O1_4 O1_3 O1_2 O1_1 O1_0
     1    0    1    0    0    1    0    1
     ```

3. **Shift-by-2 stage** (\(s_1 = 1\)):
   - Output bit \(i\) becomes O1 bit \((i-2) \mod 8\).
   - So:
     - Output O2_7 = O1_5 = 1
     - O2_6 = O1_4 = 0
     - O2_5 = O1_3 = 0
     - O2_4 = O1_2 = 1
     - O2_3 = O1_1 = 0
     - O2_2 = O1_0 = 1
     - O2_1 = O1_7 = 1
     - O2_0 = O1_6 = 0
   - After this stage:
     ```
     O2_7 O2_6 O2_5 O2_4 O2_3 O2_2 O2_1 O2_0
     1    0    0    1    0    1    1    0
     ```

4. **Shift-by-4 stage** (\(s_2 = 0\)):
   - Since \(s_2 = 0\), this stage is disabled; output is unchanged.
   - Final output:
     ```
     O3_7 O3_6 O3_5 O3_4 O3_3 O3_2 O3_1 O3_0
     1    0    0    1    0    1    1    0
     ```
   - This is \(0x96\) (binary `10010110`), which is the correct result
     after left rotation by 3.


- The barrel shifter is a combinatorial circuit, meaning the output is
  determined solely by the current input and control signals without
  any clocked registers between stages.
- The propagation delay is the time for signals to pass through all
  multiplexer stages. For an 8-bit shifter, this delay is small and
  within one clock cycle for typical clock speeds.
- Thus, after the clock edge, the output stabilizes to the rotated
  value within the same cycle, effectively performing the rotation in
  one clock cycle.

This design ensures high-speed operation and is widely used in
processors for efficient bit manipulation. For larger words, the
number of stages increases logarithmically, but the operation remains
within one cycle due to parallel processing.


#### Step-by-Step Example

Consider an 8-bit word (array of 8 bits) with value `0xD2` (binary
`11010010`). We want to rotate it left by 3 bits.

**Step 1: Initialize input**
- `val = 0xD2` (binary `11010010`)
- `shift = 3`

**Step 2: Calculate effective shift**
- `shift = 3 % 8 = 3`

**Step 3: Compute left shift**
- `val << shift = 0xD2 << 3`
  - Binary: `11010010` << 3 = `10010000` (bits shift left, new right
    bits are 0)
  - Hexadecimal: `0x90`

**Step 4: Compute right shift**
- `val >> (8 - shift) = 0xD2 >> 5`
  - Binary: `11010010` >> 5 = `00000110` (bits shift right, new left
    bits are 0)
  - Hexadecimal: `0x06`

**Step 5: Combine with OR**
- `(val << shift) | (val >> (8 - shift)) = 0x90 | 0x06`
  - Binary: `10010000 | 00000110 = 10010110`
  - Hexadecimal: `0x96`

**Result:** The rotated value is `0x96` (binary `10010110`).


#### Performance Analysis

- **Time complexity:** \(O(1)\) for a single word, as it uses
    fixed-time bitwise operations.
- **Space complexity:** \(O(1)\), no additional memory required.
- **For large arrays:** If rotating a large bitarray composed of
    multiple words, apply this method to each word individually and
    handle carry-over between words (e.g., by shifting bits between
    adjacent words). The overall complexity would be \(O(n)\) for
    \(n\) words, but efficient due to word-level operations.

This method is optimal for software-based bit rotation and is widely
used in systems without hardware barrel shifters.



---
### 4. SIMD-Style Bit Parallelism in Standard C
---

#### Conceptual Overview

For rotating a full bitarray as a single entity (across multiple
words), we need to ensure that bits shift across word boundaries
correctly. The SIMD-style approach here refers to using efficient
word-level bitwise operations to handle the rotation in a way that
minimizes instructions and leverages parallel processing within words,
even though standard C doesn't have explicit SIMD intrinsics. The key
is to process each word in the array while managing the carry-over
bits between adjacent words. This method is efficient for large
bitarrays and can be implemented with a loop that uses shifts and OR
operations.

The code below rotates a bitarray left by `k` bits, where the bitarray
is represented as an array of words (e.g., `uint8_t`, `uint16_t`,
etc.). For simplicity, we use `uint8_t` words (8-bit bytes) in the
example, but the method generalizes to larger words.


#### Code Example for Multi-Word Bitarray Rotation

```c
#include <stdint.h>

// Rotate a bitarray left by k bits
// array: pointer to the array of words
// num_words: number of words in the array
// k: number of bits to rotate left (0 < k < word_size)
void rotate_bitarray_left(uint8_t *array, int num_words, int k) {
    const int word_size = 8; // bits per word
    uint8_t temp = array[0] >> (word_size - k); // Save the top k bits of the first word
    for (int i = 0; i < num_words - 1; i++) {
        // Current word: shift left by k, then OR with the top k bits from the next word
        array[i] = (array[i] << k) | (array[i+1] >> (word_size - k));
    }
    // Last word: shift left by k, then OR with the saved bits from the first word
    array[num_words-1] = (array[num_words-1] << k) | temp;
}

// Example usage with two words
int main() {
    uint8_t array[2] = {0xD2, 0xAA}; // Binary: 11010010, 10101010
    rotate_bitarray_left(array, 2, 3);
    // After rotation: array[0] = 0x95, array[1] = 0x56
    return 0;
}
```


#### Step-by-Step Example

**Input bitarray:** Two words: `word0 = 0xD2` (binary `11010010`),
  `word1 = 0xAA` (binary `10101010`).  
**Rotate left by 3 bits.**

1. **Initial state:**  
   - Entire bitarray: `11010010 10101010`  
   - We want to rotate so that the first 3 bits (`110`) move to the
     end.

2. **Compute `temp`:**  
   - `temp = word0 >> (8 - 3) = 0xD2 >> 5`  
     - `0xD2` in binary: `11010010`  
     - Right shift by 5: `00000110` (value `0x06`) — this captures the
       top 3 bits (`110`) of `word0`.

3. **Process `word0`:**  
   - `word0_new = (word0 << 3) | (word1 >> (8 - 3))`  
     - `word0 << 3`: `11010010 << 3` = `10010000` (value `0x90`)  
     - `word1 >> 5`: `10101010 >> 5` = `00000101` (value `0x05`)  
     - OR: `10010000 | 00000101` = `10010101` (value `0x95`)  
   - Now `word0` becomes `0x95`.

4. **Process `word1`:**  
   - `word1_new = (word1 << 3) | temp`  
     - `word1 << 3`: `10101010 << 3` = `01010000` (value `0x50`)  
     - `temp`: `00000110` (value `0x06`)  
     - OR: `01010000 | 00000110` = `01010110` (value `0x56`)  
   - Now `word1` becomes `0x56`.

5. **Final state:**  
   - `word0 = 0x95` (binary `10010101`)  
   - `word1 = 0x56` (binary `01010110`)  
   - Entire bitarray: `10010101 01010110` — which is the correct
     rotation of the original by 3 bits left.


#### ASCII Diagram

**Before rotation:**  
```
Word0: 11010010
Word1: 10101010
```
**After rotation by 3 bits left:**  
```
Word0: 10010101
Word1: 01010110
```
**Bit movement:**  
- The top 3 bits of Word0 (`110`) move to the low 3 bits of Word1.
- The rest of the bits shift left accordingly.


#### Performance Analysis

- **Time Complexity:** $O(n)$ for n words, as each word is processed
    once in the loop. The operations within the loop are constant-time
    (shifts and ORs).
- **Space Complexity:** $O(1)$, only a temporary variable is used.
- **Advantages:** This method efficiently handles cross-word bit
    rotation using simple bitwise operations. It is suitable for any
    word size (e.g., 32-bit or 64-bit words) by adjusting the
    `word_size` parameter.
- **Limitations:** The rotation amount `k` must be less than the word
    size. For larger rotations, use `k % total_bits` to reduce it
    within range.

This approach ensures that the bitarray is treated as a single entity,
with bits wrapping around across word boundaries. It can be extended
to larger arrays and different word types, making it practical for
applications like cryptography, data compression, and low-level
systems programming.



---
### 5. Adaptive Algorithm Selection Based on Size
---

#### Conceptual Overview

Adaptive algorithm selection chooses the most efficient bitarray
rotation method based on the size of the bitarray (`n` bits) and the
rotation amount (`k` bits). For small bitarrays (`n ≤ 64`), we use a
efficient method that loads the bits into a `uint64_t`, performs the
rotation, and stores them back. For larger bitarrays (`n > 64`), we
use a byte-wise method that handles rotation by combining byte-level
shifts and bit-level shifts. This approach ensures optimal performance
across different input sizes.

The code below demonstrates this adaptive selection. Note that for `n
> 64` and `n` not a multiple of 8, we treat the bitarray as having
`ceil(n/8)*8` bits during rotation and then mask the last byte to
preserve the original bit length.


#### Code Example with Adaptive Selection

```c
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Rotate left for small bitarrays (n <= 64) using uint64_t
void small_rotate_left(uint8_t *bits, int n, int k) {
    if (n <= 0) return;
    k = k % n;
    if (k == 0) return;

    int num_bytes = (n + 7) / 8;
    uint64_t tmp = 0;
    // Load bits into tmp (first bit becomes the highest bit of tmp)
    for (int i = 0; i < num_bytes; i++) {
        tmp = (tmp << 8) | bits[i];
    }
    // Mask to n bits
    uint64_t mask = (1ULL << n) - 1;
    tmp &= mask;
    // Rotate left by k
    tmp = (tmp << k) | (tmp >> (n - k));
    tmp &= mask; // Ensure only n bits are set
    // Store back to bits (highest byte of tmp goes to bits[0])
    for (int i = 0; i < num_bytes; i++) {
        bits[i] = (tmp >> (8 * (num_bytes - 1 - i))) & 0xFF;
    }
}

// Rotate left for large bitarrays (n > 64) that are multiple of 8
void bytewise_rotate_left(uint8_t *bits, int n, int k) {
    int num_bytes = n / 8;
    k = k % n;
    if (k == 0) return;

    int byte_shift = k / 8;
    int bit_shift = k % 8;

    // Rotate the bytes by byte_shift
    if (byte_shift > 0) {
        uint8_t *temp = (uint8_t*)malloc(byte_shift);
        if (!temp) return;
        memcpy(temp, bits, byte_shift);
        memmove(bits, bits + byte_shift, num_bytes - byte_shift);
        memcpy(bits + num_bytes - byte_shift, temp, byte_shift);
        free(temp);
    }

    // Rotate the bits within bytes by bit_shift
    if (bit_shift > 0) {
        uint8_t carry = bits[0] >> (8 - bit_shift);
        for (int i = 0; i < num_bytes - 1; i++) {
            bits[i] = (bits[i] << bit_shift) | (bits[i+1] >> (8 - bit_shift));
        }
        bits[num_bytes-1] = (bits[num_bytes-1] << bit_shift) | carry;
    }
}

// Adaptive selector function
void adaptive_rotate_left(uint8_t *bits, int n, int k) {
    if (n <= 64) {
        small_rotate_left(bits, n, k);
    } else {
        int num_bytes = (n + 7) / 8;
        // For n not multiple of 8, use bytewise_rotate_left on padded length
        bytewise_rotate_left(bits, num_bytes * 8, k);
        // Mask the last byte if n not multiple of 8
        if (n % 8 != 0) {
            bits[num_bytes-1] &= (1 << (n % 8)) - 1;
        }
    }
}

// Example usage
int main() {
    // Example 1: Small bitarray (n=16 bits)
    uint8_t bits_small[2] = {0xD2, 0xAA}; // Binary: 11010010 10101010
    adaptive_rotate_left(bits_small, 16, 3);
    // After rotation: bits_small[0] = 0x95, bits_small[1] = 0x56

    // Example 2: Large bitarray (n=128 bits, multiple of 8)
    uint8_t bits_large[16] = {0}; // Initialize with zeros
    adaptive_rotate_left(bits_large, 128, 7);

    return 0;
}
```


#### Step-by-Step Example

Consider a small bitarray of 16 bits stored in two bytes: `bits[0] =
0xD2` (binary `11010010`), `bits[1] = 0xAA` (binary `10101010`). We
want to rotate left by 3 bits.

1. **Load into uint64_t:**  
   `tmp = (0 << 8) | 0xD2 = 0xD2`  
   `tmp = (0xD2 << 8) | 0xAA = 0xD2AA`  
   Mask with `0xFFFF`: `tmp = 0xD2AA`

2. **Rotate left by 3:**  
   `tmp << 3 = 0x9550`  
   `tmp >> 13 = 0x0006` (since `n=16`, `16-3=13`)  
   `tmp = 0x9550 | 0x0006 = 0x9556`

3. **Store back:**  
   `num_bytes = 2`  
   `bits[0] = (0x9556 >> 8) & 0xFF = 0x95`  
   `bits[1] = (0x9556 >> 0) & 0xFF = 0x56`  

Result: `bits[0] = 0x95`, `bits[1] = 0x56` (binary `10010101
01010110`).


#### Performance Analysis

- **Small n (≤64):** The `small_rotate_left` function uses a
    `uint64_t` for rotation, which is O(1) time complexity due to
    fixed-width operations.
- **Large n (>64):** The `bytewise_rotate_left` function uses
    byte-level shifts and rotations. The time complexity is O(n) for
    byte-wise operations, but it is efficient due to minimal overhead.
- The adaptive approach ensures that the best method is selected based
  on size, maximizing performance across different inputs.

This strategy is commonly used in high-performance libraries to handle
bitarray rotation efficiently. The code leverages bit-level operations
from Bit Twiddling Hacks for optimization.

---

<div style="text-align: right">

[PREV: (Part 3) Performance Analysis](03-project1-performance-analysis.md)

</div>
