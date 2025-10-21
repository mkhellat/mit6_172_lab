## **Variable vs Constant Shift**

**Write-up 8**: Use the `__OP__` macro to experiment with different
operators in the data parallel loop. For some operations, you will get
division by zero errors because we initialize array `B` to be full of
zeros—ﬁx this problem in any way you like. Do any versions of the loop
not vectorize with `VECTORIZE=1 AVX2=1`? Study the assembly code for
`<<` with just `VECTORIZE=1` and explain how it differs from the AVX2
version.



### **Key Insight: Variable Shift vs Constant Shift**

The surprising results are due to the difference between **variable
shift** and **constant shift** operations.



### **Performance Comparison**

| Operation | Type | SSE Time | AVX2 Time | Vectorization |
|-----------|------|----------|-----------|---------------|
| **A[j] * B[j]** | Variable multiply | - | 0.009233 | ✅ Full AVX2 |
| **A[j] << B[j]** | Variable shift | Complex workaround | 0.013455 | ✅ AVX2 only |
| **A[j] << 2** | Constant shift | 0.011079 | 0.012636 | ✅ Both SSE & AVX2 |



### **Assembly Analysis: Constant Shift vs Variable Shift**

**Constant Shift (A[j] << 2) - SSE:**
```assembly
movdqa  4080(%rsp,%rax,4), %xmm0
movdqa  4096(%rsp,%rax,4), %xmm1
movdqa  4112(%rsp,%rax,4), %xmm2
movdqa  4128(%rsp,%rax,4), %xmm3
pslld   $2, %xmm0              # Simple constant shift
pslld   $2, %xmm1              # Simple constant shift
pslld   $2, %xmm2              # Simple constant shift
pslld   $2, %xmm3              # Simple constant shift
movdqa  %xmm0, -16(%rsp,%rax,4)
movdqa  %xmm1, (%rsp,%rax,4)
movdqa  %xmm2, 16(%rsp,%rax,4)
movdqa  %xmm3, 32(%rsp,%rax,4)
addq    $16, %rax
```

**Constant Shift (A[j] << 2) - AVX2:**
```assembly
vmovdqu  4032(%rsp,%rax,4), %ymm0
vmovdqu  4064(%rsp,%rax,4), %ymm1
vmovdqu  4096(%rsp,%rax,4), %ymm2
vmovdqu  4128(%rsp,%rax,4), %ymm3
vpslld   $2, %ymm0, %ymm0      # Simple constant shift
vpslld   $2, %ymm1, %ymm1      # Simple constant shift
vpslld   $2, %ymm2, %ymm2      # Simple constant shift
vpslld   $2, %ymm3, %ymm3      # Simple constant shift
vmovdqu  %ymm0, -64(%rsp,%rax,4)
vmovdqu  %ymm1, -32(%rsp,%rax,4)
vmovdqu  %ymm2, (%rsp,%rax,4)
vmovdqu  %ymm3, 32(%rsp,%rax,4)
addq     $32, %rax
```

**Variable Shift (A[j] << B[j]) - SSE (from earlier):**
```assembly
movdqa  8224(%rsp,%rcx,4), %xmm1
movdqa  4128(%rsp,%rcx,4), %xmm2
pslld   $23, %xmm2             # Fixed shift amount
paddd   %xmm0, %xmm2
cvttps2dq %xmm2, %xmm2         # Convert to integer
pshufd  $245, %xmm1, %xmm3
pmuludq %xmm3, %xmm2           # Use multiplication for variable shift
pshufd  $232, %xmm2, %xmm2
punpckldq %xmm2, %xmm1
movdqa  %xmm1, 32(%rsp,%rcx,4)
addq    $4, %rcx
```

**Variable Shift (A[j] << B[j]) - AVX2 (from earlier):**
```assembly
vmovdqu  8128(%rsp,%rax,4), %ymm0
vmovdqu  8160(%rsp,%rax,4), %ymm1
vmovdqu  8192(%rsp,%rax,4), %ymm2
vmovdqu  8224(%rsp,%rax,4), %ymm3
vpsllvd  4032(%rsp,%rax,4), %ymm0, %ymm0  # Variable shift instruction
vpsllvd  4064(%rsp,%rax,4), %ymm1, %ymm1  # Variable shift instruction
vpsllvd  4096(%rsp,%rax,4), %ymm2, %ymm2  # Variable shift instruction
vpsllvd  4128(%rsp,%rax,4), %ymm3, %ymm3  # Variable shift instruction
vmovdqu  %ymm0, -64(%rsp,%rax,4)
vmovdqu  %ymm1, -32(%rsp,%rax,4)
vmovdqu  %ymm2, (%rsp,%rax,4)
vmovdqu  %ymm3, 32(%rsp,%rax,4)
addq     $32, %rax
```



### **Key Differences Explained**

**1. Constant Shift (`A[j] << 2`):**
- **SSE**: Uses `pslld $2` - simple, direct instruction
- **AVX2**: Uses `vpslld $2` - simple, direct instruction
- **Both vectorize efficiently** with straightforward instructions

**2. Variable Shift (`A[j] << B[j]`):**
- **SSE**: **No native variable shift support** - uses complex
    workaround with floating-point conversion and multiplication
- **AVX2**: Uses `vpsllvd` - **native variable shift instruction**
- **Only AVX2 vectorizes efficiently** for variable shifts



### **Why Multiplication is Faster Than Variable Shift**

**Multiplication (`A[j] * B[j]`):**
- **Native SIMD support** in both SSE and AVX2
- **Simple instruction**: `pmuludq` (SSE) or `vpmuludq` (AVX2)
- **Performance**: 0.009233 sec

**Variable Shift (`A[j] << B[j]`):**
- **SSE**: Complex workaround with multiple instructions
- **AVX2**: Native `vpsllvd` but still slower than multiplication
- **Performance**: 0.013455 sec



### **Hardware Support Analysis**

**SSE/SSE2 (128-bit registers):**
- ✅ **Constant shift**: `pslld` instruction
- ❌ **Variable shift**: No native support, requires workarounds
- ✅ **Multiplication**: `pmuludq` instruction

**AVX2 (256-bit registers):**
- ✅ **Constant shift**: `vpslld` instruction  
- ✅ **Variable shift**: `vpsllvd` instruction
- ✅ **Multiplication**: `vpmuludq` instruction



### **Conclusion**

The surprising performance results are explained by **hardware
instruction support**:

1. **Constant shifts** vectorize well in both SSE and AVX2
2. **Variable shifts** only vectorize efficiently in AVX2 (due to
`vpsllvd`)
3. **Multiplication** is faster than variable shift because it has
better hardware support
4. **SSE variable shift** requires complex workarounds, making it
inefficient

This demonstrates how **hardware vector instruction availability**
directly impacts compiler optimization decisions and performance
outcomes.