# Reduction Loop Vectorization Analysis

## Write-up 15
This code vectorizes, but how does it vectorize? Turn on ASSEMBLE=1, look at the assembly dump, and explain what the compiler is doing. As discussed in lecture, this reduction will only vectorize if the combination operation (+) is associative.

## Analysis

### Code Changes
I replaced the data parallel inner loop with a reduction operation:

```c
// Commented out outer loop to focus on inner loop vectorization
// for (i = 0; i < I; i++) {
    for (j = 0; j < N; j++) {
        total += A[j];
    }
// }
```

### Compilation Commands

**Non-AVX2 reduction loop:**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -S loop_reduction.c
```

**AVX2 reduction loop:**
```bash
clang -Wall -std=gnu99 -g -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 -S loop_reduction.c -o loop_reduction_avx2.s
```

### Vectorization Results

**Non-AVX2:** `remark: vectorized loop (vectorization width: 4, interleaved count: 2)`
**AVX2:** `remark: vectorized loop (vectorization width: 8, interleaved count: 4)`

### How Reduction Vectorization Works

The compiler implements reduction vectorization using a **tree reduction pattern**:

#### 1. Initialization
```assembly
pxor    %xmm0, %xmm0    # Initialize accumulator vector register 1 to zero
pxor    %xmm1, %xmm1    # Initialize accumulator vector register 2 to zero
```

#### 2. Vectorized Loop Body
```assembly
.LBB0_3:                                # Inner Loop Header
    paddd   -80(%rsp,%rax,4), %xmm0     # Add 4 elements to accumulator 1
    paddd   -64(%rsp,%rax,4), %xmm1     # Add 4 elements to accumulator 2
    paddd   -48(%rsp,%rax,4), %xmm0     # Add 4 elements to accumulator 1
    paddd   -32(%rsp,%rax,4), %xmm1     # Add 4 elements to accumulator 2
    paddd   -16(%rsp,%rax,4), %xmm0     # Add 4 elements to accumulator 1
    paddd   (%rsp,%rax,4), %xmm1        # Add 4 elements to accumulator 2
    paddd   16(%rsp,%rax,4), %xmm0     # Add 4 elements to accumulator 1
    paddd   32(%rsp,%rax,4), %xmm1     # Add 4 elements to accumulator 2
    addq    $32, %rax                  # Increment by 32 (8 elements × 4 bytes)
    cmpq    $1052, %rax               # Check loop condition
    jne     .LBB0_3                   # Continue loop
```

#### 3. Tree Reduction (Post-Loop)
```assembly
paddd   %xmm0, %xmm1                  # Combine the two accumulators
pshufd  $238, %xmm1, %xmm0            # Shuffle: xmm0 = xmm1[2,3,2,3]
paddd   %xmm1, %xmm0                  # Add upper and lower halves
pshufd  $85, %xmm0, %xmm1             # Shuffle: xmm1 = xmm0[1,1,1,1]
paddd   %xmm0, %xmm1                  # Add elements 0+1 and 2+3
movd    %xmm1, %ebp                   # Extract final scalar result
```

### Key Insights

#### 1. **Multiple Accumulators**
- Uses **two vector registers** (`%xmm0`, `%xmm1`) as accumulators
- Each accumulator processes **4 elements** per iteration
- **Interleaved count of 2** means 2 accumulators working in parallel

#### 2. **Tree Reduction Pattern**
The post-loop reduction follows a tree pattern:
- **Step 1**: Combine two 4-element accumulators → 1 vector with 4 partial sums
- **Step 2**: Shuffle and add → 1 vector with 2 partial sums  
- **Step 3**: Shuffle and add → 1 scalar result

#### 3. **Why This Works**
- **Associativity**: `(a + b) + (c + d) = a + b + c + d`
- **Commutativity**: Order doesn't matter for addition
- **Parallel Accumulation**: Multiple accumulators prevent dependencies

#### 4. **Performance Benefits**
- **Vectorization width: 4** (non-AVX2) or **8** (AVX2)
- **Interleaved count: 2** (non-AVX2) or **4** (AVX2)
- Processes **8 elements per iteration** (non-AVX2) or **32 elements per iteration** (AVX2)

### Verification
The program correctly computes the sum: `total: 1796` (1024 + random addition), confirming the vectorization preserves correctness.

### Why Only Associative Operations Vectorize

Reduction vectorization requires **associativity** because:
1. **Parallel Accumulation**: Multiple accumulators compute partial sums independently
2. **Tree Reduction**: Partial sums are combined in a tree pattern
3. **Order Independence**: The final result must be the same regardless of the order of operations

For non-associative operations (like subtraction), the order matters:
- `(a - b) - (c - d) ≠ a - b - c - d`
- This breaks the parallel accumulation strategy

### Conclusion

Clang implements reduction vectorization using a sophisticated **tree reduction pattern** with multiple accumulators. This approach:
- Maintains correctness through associativity
- Achieves significant speedup through parallel processing
- Uses efficient SIMD instructions for both accumulation and final reduction
- Scales well with wider vector units (AVX2 achieves width=8, interleaved count=4)
