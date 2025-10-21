## **Write-up 7: Assembly Code Analysis**

### **Comparison of Assembly Code**

**Unvectorized Version (`loop_unvectorized.s`):**
```assembly
movl	4116(%rsp,%rax,4), %ecx
movl	4120(%rsp,%rax,4), %edx
addl	8212(%rsp,%rax,4), %ecx
addl	8216(%rsp,%rax,4), %edx
movl	%ecx, 20(%rsp,%rax,4)
movl	4124(%rsp,%rax,4), %ecx
addl	8220(%rsp,%rax,4), %ecx
movl	%edx, 24(%rsp,%rax,4)
movl	4128(%rsp,%rax,4), %edx
addl	8224(%rsp,%rax,4), %edx
movl	%ecx, 28(%rsp,%rax,4)
movl	%edx, 32(%rsp,%rax,4)
addq	$4, %rax
```
- **Scalar operations**: Uses `addl` (32-bit integer addition)
- **4-way unrolling**: Processes 4 elements per iteration
- **Loop increment**: `addq $4` (4 bytes = 1 uint32_t)

**SSE Vectorized Version (`loop_vectorized.s`):**
```assembly
movdqa	4080(%rsp,%rax,4), %xmm0
movdqa	4096(%rsp,%rax,4), %xmm1
movdqa	4112(%rsp,%rax,4), %xmm2
movdqa	4128(%rsp,%rax,4), %xmm3
paddd	8176(%rsp,%rax,4), %xmm0
paddd	8192(%rsp,%rax,4), %xmm1
movdqa	%xmm0, -16(%rsp,%rax,4)
movdqa	%xmm1, (%rsp,%rax,4)
paddd	8208(%rsp,%rax,4), %xmm2
paddd	8224(%rsp,%rax,4), %xmm3
movdqa	%xmm2, 16(%rsp,%rax,4)
movdqa	%xmm3, 32(%rsp,%rax,4)
addq	$16, %rax
```
- **SIMD operations**: Uses `paddd` (packed 32-bit integer addition)
- **4-way unrolling**: Processes 4 elements per iteration
- **Loop increment**: `addq $16` (16 bytes = 4 uint32_t)

**AVX2 Vectorized Version (`loop_avx2.s`):**
```assembly
vmovdqu	4032(%rsp,%rax,4), %ymm0
vmovdqu	4064(%rsp,%rax,4), %ymm1
vmovdqu	4096(%rsp,%rax,4), %ymm2
vmovdqu	4128(%rsp,%rax,4), %ymm3
vpaddd	8128(%rsp,%rax,4), %ymm0, %ymm0
vpaddd	8160(%rsp,%rax,4), %ymm1, %ymm1
vpaddd	8192(%rsp,%rax,4), %ymm2, %ymm2
vpaddd	8224(%rsp,%rax,4), %ymm3, %ymm3
vmovdqu	%ymm0, -64(%rsp,%rax,4)
vmovdqu	%ymm1, -32(%rsp,%rax,4)
vmovdqu	%ymm2, (%rsp,%rax,4)
vmovdqu	%ymm3, 32(%rsp,%rax,4)
addq	$32, %rax
```
- **AVX2 operations**: Uses `vpaddd` (packed 32-bit integer addition)
- **8-way unrolling**: Processes 8 elements per iteration
- **Loop increment**: `addq $32` (32 bytes = 8 uint32_t)

### **Vector Add Operations**

**1. SSE Vector Add Operation (VECTORIZE=1):**
```
paddd	8176(%rsp,%rax,4), %xmm0
```
- **Instruction**: `paddd` (Packed ADD Doubleword)
- **Operation**: Adds 4 packed 32-bit integers from memory to 4 packed 32-bit integers in `%xmm0`
- **Register**: `%xmm0` (128-bit SSE register)
- **Data type**: 4 × 32-bit integers = 128 bits total

**2. AVX2 Vector Add Operation (VECTORIZE=1 AVX2=1):**
```
vpaddd	8128(%rsp,%rax,4), %ymm0, %ymm0
```
- **Instruction**: `vpaddd` (Vector Packed ADD Doubleword)
- **Operation**: Adds 8 packed 32-bit integers from memory to 8 packed 32-bit integers in `%ymm0`
- **Register**: `%ymm0` (256-bit AVX2 register)
- **Data type**: 8 × 32-bit integers = 256 bits total

### **Instruction Analysis**

**`paddd` (SSE2 Instruction):**
- **Purpose**: Packed addition of doubleword (32-bit) integers
- **Operands**: `paddd source, destination`
- **Effect**: Adds 4 packed 32-bit integers from source to destination
- **Register width**: 128 bits (SSE2)

**`vpaddd` (AVX2 Instruction):**
- **Purpose**: Vector packed addition of doubleword (32-bit) integers
- **Operands**: `vpaddd source, destination, result`
- **Effect**: Adds 8 packed 32-bit integers from source to destination
- **Register width**: 256 bits (AVX2)

### **Key Differences**

1. **Register Size**: SSE uses 128-bit `%xmm` registers, AVX2 uses 256-bit `%ymm` registers
2. **Elements per instruction**: SSE processes 4 elements, AVX2 processes 8 elements
3. **Loop increment**: SSE increments by 16 bytes (4 elements), AVX2 by 32 bytes (8 elements)
4. **Performance**: AVX2 provides 2× theoretical speedup over SSE for this workload

The assembly analysis clearly shows the progression from scalar
operations (`addl`) to SSE vectorization (`paddd`) to AVX2
vectorization (`vpaddd`), with each step processing more elements per
instruction.