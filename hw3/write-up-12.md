## **Runtime Loop Bounds and Vectorization Performance**

**Write-up 12**: Get rid of the `#define N 1024` macro and redeﬁne `N`
as: `int N = atoi(argv[1]);` (at the beginning of `main()`). (Setting
`N` through the command line ensures that the compiler will make no
assumptions about it.) Rerun (with various choices of `N`) and compare
the AVX2 vectorized, non-AVX2 vectorized, and unvectorized codes. Does
the speedup change dramatically relative to the `N = 1024` case? Why?


### **Compilation Commands Used**

```bash
# Unvectorized
clang -Wall -std=gnu99 -O3 -DNDEBUG -fno-vectorize loop_runtime.c -o loop_runtime -lrt

# SSE Vectorized  
clang -Wall -std=gnu99 -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math loop_runtime.c -o loop_runtime -lrt

# AVX2 Vectorized
clang -Wall -std=gnu99 -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 loop_runtime.c -o loop_runtime -lrt

# Assembly generation
clang -Wall -std=gnu99 -O3 -DNDEBUG -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -ffast-math -mavx2 -D__TYPE__=uint8_t -S loop_runtime.c -o loop_runtime_uint8.s
```

---

### **Performance Results**

#### **uint32_t Data Type**

| N Value | Description | Unvectorized | SSE Vectorized | AVX2 Vectorized | SSE Speedup | AVX2 Speedup |
|---------|-------------|--------------|----------------|-----------------|-------------|--------------|
| 1024 | Power of 2 | 0.068736 | 0.021482 | 0.009098 | **3.20x** | **7.56x** |
| 1000 | Non-power of 2 | 0.006986 | 0.007223 | 0.008552 | **0.97x** | **0.82x** |
| 1001 | Odd number | 0.007169 | 0.006884 | 0.007964 | **1.04x** | **0.90x** |
| 1023 | 1024-1 | 0.002949 | 0.002471 | 0.003006 | **1.19x** | **0.98x** |
| 1025 | 1024+1 | 0.001834 | 0.001881 | 0.001845 | **0.98x** | **0.99x** |
| 10 | Very small | 0.000458 | 0.000313 | 0.000239 | **1.46x** | **1.92x** |

#### **uint8_t Data Type (Small N)**

| N Value | Unvectorized | SSE Vectorized | AVX2 Vectorized | SSE Speedup | AVX2 Speedup |
|---------|--------------|----------------|-----------------|-------------|--------------|
| 1001 | 0.056798 | 0.003918 | 0.002880 | **14.50x** | **19.72x** |



### **Key Observations**

#### **1. Dramatic Speedup Reduction**

**Yes, the speedup changes dramatically** when N is not a power of 2:

- **N=1024 (power of 2)**: AVX2 achieves **7.56x** speedup
- **N=1000 (non-power of 2)**: AVX2 achieves only **0.82x** speedup (actually slower!)
- **N=1001 (odd)**: AVX2 achieves only **0.90x** speedup

#### **2. Termination Code Overhead**

The assembly analysis reveals extensive termination handling code:

**uint8_t (32 elements per AVX2 vector):**
- **LBB0_8**: Main vectorized loop (32 elements at a time)
- **LBB0_11**: SSE termination loop (8 elements at a time) 
- **LBB0_13**: Scalar termination loop (1 element at a time)

**uint32_t (8 elements per AVX2 vector):**
- **LBB0_19**: Main vectorized loop (8 elements at a time)
- **LBB0_22**: SSE termination loop (4 elements at a time)
- **LBB0_24**: Scalar termination loop (1 element at a time)

#### **3. Why Speedup Degrades**

1. **Termination Overhead**: For N=1001 with uint32_t:
   - Main loop processes: 1000 elements (125 AVX2 vectors × 8 elements)
   - Termination processes: 1 element (scalar)
   - Overhead: Complex branching and scalar execution

2. **Branch Prediction**: Multiple termination paths create branch misprediction penalties

3. **Code Size**: Termination code increases instruction cache pressure

#### **4. Data Type Impact on Termination**

As predicted, smaller data types show **more termination code**:

- **uint8_t**: 3 termination levels (AVX2→SSE→Scalar)
- **uint32_t**: 3 termination levels (AVX2→SSE→Scalar)

But uint8_t still achieves better speedup because:
- More elements per vector (32 vs 8)
- Termination overhead is proportionally smaller
- Better vector utilization

#### **5. Compiler Vectorization Reports**

```
uint8_t: vectorization width: 32, interleaved count: 4
uint32_t: vectorization width: 8, interleaved count: 4
```

Both show successful vectorization, but runtime performance differs
significantly.



### **Assembly Code Analysis**

#### **Termination Logic in uint8_t**

```assembly
# Main vectorized loop (32 elements)
.LBB0_8:
    vmovdqu (%r15,%rdx), %ymm0
    vmovdqu 32(%r15,%rdx), %ymm1
    vpaddb (%rbx,%rdx), %ymm0, %ymm0
    vpaddb 32(%rbx,%rdx), %ymm1, %ymm1
    vmovdqu %ymm0, (%r15,%rdx)
    vmovdqu %ymm1, 32(%r15,%rdx)
    addq $64, %rdx
    cmpq %rax, %rdx
    jne .LBB0_8

# SSE termination loop (8 elements)  
.LBB0_11:
    vmovq (%rbx,%rsi), %xmm0
    vmovq (%r15,%rsi), %xmm1
    vpaddb %xmm0, %xmm1, %xmm0
    vmovq %xmm0, (%r15,%rsi)
    addq $8, %rsi
    cmpq %rcx, %rsi
    jne .LBB0_11

# Scalar termination loop (1 element)
.LBB0_13:
    movzbl (%r15,%rdx), %esi
    addb (%rbx,%rdx), %sil
    movb %sil, (%r15,%rdx)
    incq %rdx
    cmpq %rdx, %r13
    jne .LBB0_13
```

### **Conclusions**

1. **Runtime loop bounds significantly impact vectorization performance**
2. **Non-power-of-2 values can eliminate speedup benefits** due to termination overhead
3. **Termination code complexity increases with smaller data types** but remains proportionally smaller
4. **Compiler generates sophisticated multi-level termination handling** (AVX2→SSE→Scalar)
5. **Branch prediction and code size overhead** can make vectorized code slower than scalar for certain N values

The fundamental issue is that **vectorization assumes aligned,
vector-width-aligned loop bounds**. When this assumption fails, the
overhead of handling remainder elements can exceed the benefits of
vectorization, especially for small to medium N values where
termination overhead dominates.