# Bit-array Cyclic Rotation

Rotating a bitarray of length $n$ by $d$ positions is a cyclic
permutation. A left rotation by $d$ on positions $0…n–1$ is the map  

\[
R_d(i) \;=\;
\begin{cases}
i - d + n, & i < d,\\
i - d, & i \ge d.
\end{cases}
\]  

Bitarray rotation is a fundamental primitive in computer science,
relevant for cryptography, scientific computing, signal processing,
and low-level systems programming. Possible use cases include:
- **Cryptography**: block ciphers (mixing bits across S-boxes), stream
    ciphers, hash functions.
- **Data Compression**: Burrows–Wheeler transform uses rotations of
    symbol sequences (bit or byte arrays).
- **Networking**: CRC generation, bit-level framing, protocol header
    alignment.
- **Signal Processing**: circular convolution, sliding window FFT
    bit-reversal permutations.
- **Graphics & Imaging**: bitplane rotations, texture swizzling for
    GPUs.
- **Bitboards in Game Engines**: rotating board representations for
    symmetry detection in chess or Go.
- **Genetic Algorithms**: circular gene shuffling in bitstring
    genomes.
- **Error-Correcting Codes**: cyclic redundancy check, LDPC code
    construction

Specifically, efficient bit-array rotation with the constraints of
being performed _in place_ and at _minimal auxiliary memory cost_ is
essential for the practical performance of high-throughput systems and
embedded devices.

---

Following is a rather comprehesnive discussion of bitarray rotation
methods—including mathematical foundations, technical details,
examples, and performance comparisons.

### Part 1: Mathematical Foundations

We’ll start by explaining the core mathematical ideas behind bitarray
rotations using intuitive concepts from modular arithmetic,
permutations, and symmetry—without requiring deep knowledge of set
theory or abstract algebra.


### Part 2: Rotation Algorithms

For each method (buffer-copy, triple-reverse, block-swap, bit-by-bit,
cyclic, word-shift & OR), we will provide:

- Conceptual overview
- Step-by-step example
- Pseudocode
- Standard C implementation


### Part 3: Performance Analysis

We’ll compare:

- Work (bit-moves)
- Time complexity
- Space complexity
- Cache locality and memory access patterns


### Part 4: Bonus Methods

we’ll include a few additional rotation strategies that are
interesting for performance or simplicity, such as:

- Lookup Table Chunk Rotation
- Arithmetic Barrel Shifter Simulation
- Adaptive Algorithm Selection Based on Size

---

<div style="text-align: right">

[NEXT: The In-Place Mandate](00-project1-the-inplace-mandate.md)

</div>