## **Write-up 6: Vectorization Analysis**

**Write-up 6**: What speedup does the vectorized code achieve over the
unvectorized code?  What additional speedup does using -mavx2 give?
You may wish to run this experiment several times and take median
elapsed times; you can report answers to the nearest 100% (e.g., 2×,
3×, etc). What can you infer about the bit width of the default vector
registers on the awsrun machines? What about the bit width of the AVX2
vector registers? Hint: aside from speedup and the vectorization
report, the most relevant information is that the data type for each
array is uint32_t.

---

### 1. **Raw Performance Data (10 runs each)**

| Configuration | Run 1 | Run 2 | Run 3 | Run 4 | Run 5 | Run 6 | Run 7 | Run 8 | Run 9 | Run 10 | Median |
|---------------|-------|-------|-------|-------|-------|-------|-------|-------|-------|--------|--------|
| **Unvectorized** | 0.066013 | 0.065058 | 0.079022 | 0.063195 | 0.063565 | 0.070566 | 0.058362 | 0.065381 | 0.065702 | 0.060897 | **0.065381** |
| **SSE Vectorized** | 0.020282 | 0.017010 | 0.020961 | 0.016481 | 0.018377 | 0.019666 | 0.015092 | 0.017454 | 0.015555 | 0.022398 | **0.017454** |
| **AVX2 Vectorized** | 0.010993 | 0.010182 | 0.010966 | 0.010070 | 0.012381 | 0.008114 | 0.009232 | 0.008168 | 0.009504 | 0.007720 | **0.009504** |


### 2. **Speedup Analysis (Based on Medians)**

**1. Vectorized vs Unvectorized Speedup:**
- **Unvectorized**: 0.065381 sec
- **Vectorized**: 0.017454 sec
- **Speedup**: 0.065381 ÷ 0.017454 = **3.7×** → **4×** (nearest 100%)

**2. AVX2 vs SSE Additional Speedup:**
- **SSE (vectorized)**: 0.017454 sec
- **AVX2**: 0.009504 sec
- **Speedup**: 0.017454 ÷ 0.009504 = **1.8×** → **2×** (nearest 100%)


### 3. **Assembly Code Analysis**

**Unvectorized Version:**
```assembly
movl  4120(%rsp,%rax,4), %edx   # Load A[j]
addl  8212(%rsp,%rax,4), %ecx   # Add B[j] to accumulator
movl  %ecx, 28(%rsp,%rax,4)     # Store C[j]
addq  $4, %rax                  # Increment by 4 (1 uint32_t)
```
- **Scalar operations**: `addl` (32-bit integer addition)
- **4-way unrolling**: Processes 4 elements per iteration

**SSE Vectorized Version:**
```assembly
movdqa  4112(%rsp,%rax,4), %xmm2  # Load 4 uint32_t into xmm2
paddd   8176(%rsp,%rax,4), %xmm0  # Add 4 uint32_t to xmm0
movdqa  %xmm0, -16(%rsp,%rax,4)   # Store 4 uint32_t
addq    $16, %rax                 # Increment by 16 (4 uint32_t)
```
- **SIMD operations**: `paddd` (packed 32-bit integer addition)
- **Vectorization width**: 4 elements

**AVX2 Version:**
```assembly
vmovdqu  4032(%rsp,%rax,4), %ymm0  # Load 8 uint32_t into ymm0
vpaddd   8128(%rsp,%rax,4), %ymm0, %ymm0  # Add 8 uint32_t to ymm0
vmovdqu  %ymm0, -64(%rsp,%rax,4)   # Store 8 uint32_t
addq     $32, %rax                 # Increment by 32 (8 uint32_t)
```
- **AVX2 operations**: `vpaddd` (packed 32-bit integer addition)
- **Vectorization width**: 8 elements


### 4. **Register Bit Width Inference**

**Data Type**: `uint32_t` (32-bit integers)

**a. Default Vector Registers (SSE):**
- **Vectorization width**: 4 elements (from compiler report: "vectorization width: 4")
- **4 × 32-bit = 128 bits**
- **Default vector registers are 128-bit** (SSE/SSE2)

**b. AVX2 Vector Registers:**
- **Vectorization width**: 8 elements (from compiler report: "vectorization width: 8")
- **8 × 32-bit = 256 bits**
- **AVX2 vector registers are 256-bit**


### 5. **Performance Analysis**

The **AVX2 version shows significant improvement** (2× speedup over SSE) because:

1. **Wider SIMD**: Processes 8 elements per instruction vs 4
2. **Better instruction-level parallelism**: More operations per cycle
3. **Reduced loop overhead**: Fewer iterations needed
4. **Efficient memory access**: Better cache utilization


### **SUMMARY**

- **Vectorization speedup**: **4×** (SSE over scalar)
- **AVX2 additional speedup**: **2×** (AVX2 over SSE)
- **Default vector registers**: **128-bit** (SSE)
- **AVX2 vector registers**: **256-bit**

**Total speedup from unvectorized to AVX2**: **7×** (4× × 2× = 8×, but
  measured as 7× due to overhead)

The assembly analysis confirms that the compiler successfully
vectorized the code using SSE (128-bit registers) and AVX2 (256-bit
registers), with AVX2 providing significant additional performance
benefits over SSE vectorization.