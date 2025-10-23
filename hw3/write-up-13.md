# Strided Loop Vectorization Analysis

## Write-up 13
Set `__TYPE__` to `uint32_t` and `__OP__` to `+`, and change your inner loop to be strided. Does clang vectorize the code? Why might it choose not to vectorize the code?




## Analysis



### Code Changes
I modified the inner loop from:
```c
for (j = 0; j < N; j++) {
    C[j] = A[j] + B[j];
}
```

To:
```c
for (j = 0; j < N; j += 2) {  // Changed stride from 1 to 2
    C[j] = A[j] + B[j];
}
```



### Vectorization Results

**Original Loop (stride = 1):**
- ✅ **Vectorized**: `remark: vectorized loop (vectorization width: 4, interleaved count: 2)`
- Uses SIMD instructions (`movdqa`, `paddd`) to process 4 elements at once
- Performance: 0.014747 seconds

**Strided Loop (stride = 2):**
- ❌ **Not vectorized**: `remark: the cost-model indicates that vectorization is not beneficial`
- Uses scalar instructions (`movl`, `addl`) to process elements individually
- Performance: 0.029453 seconds (exactly 2x slower)



### Assembly Code Analysis

**Vectorized version (stride = 1):**
```assembly
movdqa  4080(%rsp,%rax,4), %xmm0    # Load 4 elements from A
movdqa  4096(%rsp,%rax,4), %xmm1    # Load 4 elements from A
paddd   8176(%rsp,%rax,4), %xmm0    # Add 4 elements from B
paddd   8192(%rsp,%rax,4), %xmm1    # Add 4 elements from B
movdqa  %xmm0, -16(%rsp,%rax,4)     # Store 4 results to C
movdqa  %xmm1, (%rsp,%rax,4)        # Store 4 results to C
addq    $16, %rax                   # Increment by 16 (4 elements × 4 bytes)
```

**Non-vectorized version (stride = 2):**
```assembly
movl    4136(%rsp,%rax,4), %ecx     # Load 1 element from A
movl    4144(%rsp,%rax,4), %edx     # Load 1 element from A
addl    8232(%rsp,%rax,4), %ecx     # Add 1 element from B
addl    8240(%rsp,%rax,4), %edx     # Add 1 element from B
movl    %ecx, 40(%rsp,%rax,4)       # Store 1 result to C
movl    %edx, 48(%rsp,%rax,4)       # Store 1 result to C
addq    $8, %rax                    # Increment by 8 (2 elements × 4 bytes)
```



### Why Clang Chooses Not to Vectorize Strided Loops

1. **Memory Access Pattern Complexity**: Strided access patterns are more complex to vectorize because:
   - SIMD instructions expect contiguous memory access
   - Strided access requires gathering/scattering operations
   - The overhead of gathering/scattering may outweigh vectorization benefits

2. **Cost Model Analysis**: Clang's cost model determines that:
   - The overhead of vectorizing strided access is too high
   - Scalar code is more efficient for this particular stride pattern
   - The performance gain from vectorization doesn't justify the complexity

3. **Hardware Limitations**: While modern CPUs have gather/scatter instructions:
   - These instructions have higher latency than contiguous loads
   - The stride of 2 creates a specific pattern that may not map well to available vector instructions
   - The compiler may not have optimized patterns for this specific stride

4. **Memory Bandwidth**: With stride = 2:
   - Only half the array elements are accessed
   - Memory bandwidth utilization is reduced
   - Vectorization overhead becomes more significant relative to the work done



### Performance Impact
The strided version runs exactly 2x slower (0.029453s vs 0.014747s),
which makes sense because:
- Only half the elements are processed (stride = 2)
- No vectorization benefits
- Scalar operations are inherently slower than SIMD operations

This demonstrates the significant performance impact of vectorization
and why compilers carefully analyze whether vectorization will be
beneficial for different access patterns.
