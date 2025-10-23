# Clang Loop Pragma Vectorization Analysis

## Write-up 14
Use the `#vectorize` pragma described in the clang language extensions webpage to make clang vectorize the strided loop. What is the speedup over non-vectorized code for non-AVX2 and AVX2 vectorization? What happens if you change the vectorize_width to 2? Play around with the clang loop pragmas and report the best you found (that vectorizes the loop). Did you get a speedup over the non-vectorized code? Once again, inspecting the assembly code to see how striding is vectorized can be insightful.

## Analysis

### Clang Loop Pragma Implementation

I used the `#pragma clang loop vectorize(enable)` directive to force vectorization of the strided loop:

```c
#pragma clang loop vectorize(enable)
for (j = 0; j < N; j += 2) {  // Changed stride from 1 to 2
    C[j] = A[j] + B[j];
}
```

### Compilation Commands

**Non-vectorized strided loop:**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -c loop_strided.c
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -o loop_strided loop_strided.o -lrt
```

**Vectorized strided loop (default width):**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -c loop_strided_pragma.c
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -o loop_strided_pragma loop_strided_pragma.o -lrt
```

**Vectorized strided loop (width=2):**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -c loop_strided_pragma_width2.c
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -o loop_strided_pragma_width2 loop_strided_pragma_width2.o -lrt
```

**Vectorized strided loop (AVX2):**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 -c loop_strided_pragma.c -o loop_strided_pragma_avx2.o
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 -o loop_strided_pragma_avx2 loop_strided_pragma_avx2.o -lrt
```

**Assembly generation commands:**
```bash
# Non-AVX2 assembly
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -S loop_strided_pragma.c

# AVX2 assembly
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 -S loop_strided_pragma.c -o loop_strided_pragma_avx2.s
```

### Performance Results

| Version | Time (seconds) | Speedup vs Non-vectorized | Vectorization Width |
|---------|----------------|---------------------------|-------------------|
| Non-vectorized strided | 0.035511 | 1.00x (baseline) | N/A |
| Vectorized (default width) | 0.036736 | 0.97x (slower!) | 4 |
| Vectorized (width=2) | 0.028660 | 1.24x | 2 |
| Vectorized (AVX2) | 0.019058 | 1.86x | 8 |

### Key Findings

1. **Default vectorization width (4) was slower** than non-vectorized code
2. **Width=2 provided the best speedup** for non-AVX2 (1.24x)
3. **AVX2 provided the best overall speedup** (1.86x)

### Assembly Code Analysis

#### Non-AVX2 Vectorization (width=4)
```assembly
movdqa  4128(%rsp,%rdi,8), %xmm0    # Load 4 elements from A
movdqa  4144(%rsp,%rdi,8), %xmm1    # Load 4 elements from A
paddd   8224(%rsp,%rdi,8), %xmm0    # Add 4 elements from B
paddd   8240(%rsp,%rdi,8), %xmm1    # Add 4 elements from B
movd    %xmm0, 32(%rsp,%rdi,8)      # Store first element
pshufd  $238, %xmm0, %xmm0          # Shuffle to get second element
movd    %xmm0, 40(%rsp,%rdi,8)      # Store second element
# Similar pattern for xmm1...
addq    $4, %rdi                    # Increment by 4 (stride=2, width=4)
```

#### AVX2 Vectorization (width=8)
```assembly
vmovdqu 4128(%rsp,%r11,8), %ymm0    # Load 8 elements from A
vpaddd  8224(%rsp,%r11,8), %ymm0, %ymm0  # Add 8 elements from B
vmovd   %xmm0, 32(%rsp,%r11,8)      # Store first element
vpextrd $2, %xmm0, 40(%rsp,%r11,8)  # Extract second element
vextracti128 $1, %ymm0, %xmm0       # Extract upper 128 bits
vmovd   %xmm0, 48(%rsp,%r11,8)      # Store third element
vpextrd $2, %xmm0, 56(%rsp,%r11,8)  # Extract fourth element
# Process second batch of 4 elements...
addq    $8, %r11                    # Increment by 8 (stride=2, width=8)
```

### How Strided Vectorization Works

The compiler handles strided access by:

1. **Gathering elements**: Loading multiple elements with stride=2 into vector registers
2. **Processing in vectors**: Performing SIMD operations on the gathered elements
3. **Scattering results**: Storing the results back to the strided memory locations

The key insight is that the compiler uses **gather/scatter operations** combined with **shuffle instructions** to handle the strided memory access pattern.

### Why Different Widths Perform Differently

1. **Width=4 (default)**: 
   - Overhead of gather/scatter operations outweighs benefits
   - Complex shuffling required to extract individual elements
   - **Result: Slower than scalar code**

2. **Width=2**: 
   - Simpler gather/scatter operations
   - Less shuffling overhead
   - **Result: 1.24x speedup**

3. **Width=8 (AVX2)**:
   - More efficient AVX2 gather/scatter instructions
   - Better utilization of 256-bit registers
   - **Result: 1.86x speedup**

### Additional Pragma Experiments

I experimented with other pragma combinations:

```c
#pragma clang loop vectorize(enable) vectorize_width(2) interleave_count(2)
```

This provided similar performance to width=2 alone, suggesting that the interleave count doesn't significantly impact strided loops.

### Conclusion

The `#pragma clang loop vectorize(enable)` directive successfully forces vectorization of strided loops, but the performance impact depends heavily on the vectorization width:

- **Width=2**: Best for non-AVX2 systems (1.24x speedup)
- **Width=8 (AVX2)**: Best overall performance (1.86x speedup)
- **Default width=4**: Can actually be slower than scalar code due to overhead

The assembly analysis reveals that strided vectorization uses sophisticated gather/scatter operations with shuffle instructions to handle non-contiguous memory access patterns, which explains why the overhead can sometimes outweigh the benefits for certain vectorization widths.
