## `memcpy` Pattern Recognition

**Write-up 4**: Inspect the assembly and determine why the assembly does not include
instructions with vector registers. Do you think it would be faster if it did vectorize?
Explain.

**This is NOT vectorized, but it's OPTIMIZED differently!**

### **What the Compiler Did**

**Source Code:**
```c
for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1];
}
```

**Generated Assembly:**
```assembly
incq    %rsi                   # b = b + 1 (point to b[1])
movl    $65536, %edx           # count = 65536 bytes
jmp     memcpy@PLT             # memcpy(a, b+1, 65536)
```

### **Why This is NOT Vectorized**

**1. No SIMD Instructions**
- No `movdqa`, `movdqu`, `xmm`, or `ymm` registers
- No parallel processing of multiple elements

**2. No Loop Structure**
- The entire loop was **eliminated**!
- No loop counter, no conditional jumps


Now let's test what happens if we force the compiler to NOT use
`memcpy` optimization by making the pattern less obvious:

```c
// Test case to see if we can force vectorization
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define SIZE (1L << 16)

void test_forced_vectorization(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Make the pattern less obvious to prevent memcpy optimization
  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1] + 0;  // Add zero to break memcpy pattern
  }
}

void test_with_alignment(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;
  
  a = __builtin_assume_aligned(a, 16);
  b = __builtin_assume_aligned(b, 16);

  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1];
  }
}
```

Interesting! Even with `+ 0` it still uses `memcpy`.

```asm
# %bb.0:
	#DEBUG_VALUE: test_forced_vectorization:a <- $rdi
	#DEBUG_VALUE: test_forced_vectorization:b <- $rsi
	#DEBUG_VALUE: test_forced_vectorization:i <- 0
	.loc	0 12 3 prologue_end             # test_vectorization.c:12:3
	incq	%rsi
.Ltmp0:
	#DEBUG_VALUE: test_forced_vectorization:b <- [DW_OP_LLVM_entry_value 1] $rsi
	.loc	0 13 10                         # test_vectorization.c:13:10
	movl	$65536, %edx                    # imm = 0x10000
.Ltmp1:
	jmp	memcpy@PLT                      # TAILCALL
.Ltmp2:
.Lfunc_end0:
	.size	test_forced_vectorization, .Lfunc_end0-test_forced_vectorization
	.cfi_endproc
                                        # -- End function
	.globl	test_with_alignment             # -- Begin function test_with_alignment
	.p2align	4
	.type	test_with_alignment,@function
test_with_alignment:                    # @test_with_alignment
.Lfunc_begin1:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: test_with_alignment:a <- $rdi
	#DEBUG_VALUE: test_with_alignment:b <- $rsi
	#DEBUG_VALUE: test_with_alignment:i <- 0
	.loc	0 23 3 prologue_end             # test_vectorization.c:23:3
	incq	%rsi
.Ltmp3:
	#DEBUG_VALUE: test_with_alignment:b <- [DW_OP_LLVM_entry_value 1] $rsi
	.loc	0 24 10                         # test_vectorization.c:24:10
	movl	$65536, %edx                    # imm = 0x10000
.Ltmp4:
	jmp	memcpy@PLT                      # TAILCALL
```

Let's try a more complex case:

```c
// Test case to force vectorization by breaking memcpy pattern
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define SIZE (1L << 16)

void test_complex_pattern(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Break the memcpy pattern with a more complex operation
  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1] ^ 0;  // XOR with 0 (identity) but breaks memcpy pattern
  }
}

void test_with_condition(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Add a condition that prevents memcpy optimization
  for (i = 0; i < SIZE; i++) {
    if (i < SIZE) {  // Always true, but breaks memcpy pattern
      a[i] = b[i + 1];
    }
  }
}
```

The compiler is very smart!

```asm
# %bb.0:
	#DEBUG_VALUE: test_complex_pattern:a <- $rdi
	#DEBUG_VALUE: test_complex_pattern:b <- $rsi
	#DEBUG_VALUE: test_complex_pattern:i <- 0
	.loc	0 12 3 prologue_end             # test_vectorization2.c:12:3
	incq	%rsi
.Ltmp0:
	#DEBUG_VALUE: test_complex_pattern:b <- [DW_OP_LLVM_entry_value 1] $rsi
	.loc	0 13 10                         # test_vectorization2.c:13:10
	movl	$65536, %edx                    # imm = 0x10000
.Ltmp1:
	jmp	memcpy@PLT                      # TAILCALL
.Ltmp2:
.Lfunc_end0:
	.size	test_complex_pattern, .Lfunc_end0-test_complex_pattern
	.cfi_endproc
                                        # -- End function
	.globl	test_with_condition             # -- Begin function test_with_condition
	.p2align	4
	.type	test_with_condition,@function
test_with_condition:                    # @test_with_condition
.Lfunc_begin1:
	.cfi_startproc
# %bb.0:
	#DEBUG_VALUE: test_with_condition:a <- $rdi
	#DEBUG_VALUE: test_with_condition:b <- $rsi
	#DEBUG_VALUE: test_with_condition:i <- 0
	.loc	0 21 8 prologue_end             # test_vectorization2.c:21:8
	incq	%rsi
.Ltmp3:
	#DEBUG_VALUE: test_with_condition:b <- [DW_OP_LLVM_entry_value 1] $rsi
	.loc	0 23 12                         # test_vectorization2.c:23:12
	movl	$65536, %edx                    # imm = 0x10000
.Ltmp4:
	jmp	memcpy@PLT                      # TAILCALL
```

Let's try a different approach - let's disable the memcpy
optimization:

```bash
clang -Wall -g -std=gnu99 -O3 \
      -Rpass=loop-vectorize \
      -Rpass-missed=loop-vectorize \
      -S -mavx2 -fno-builtin-memcpy example3.c
```

Now let's see what the vectorized version looks like:


```asm
# %bb.0:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	xorl	%eax, %eax
.Ltmp0:
	#DEBUG_VALUE: test:i <- 0
	.loc	0 0 0 is_stmt 0                 # :0:0
.Ltmp1:
	.p2align	4
.LBB0_1:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 13 12 prologue_end is_stmt 1  # example3.c:13:12
	vmovups	1(%rsi,%rax), %ymm0
	vmovups	33(%rsi,%rax), %ymm1
	vmovups	65(%rsi,%rax), %ymm2
	vmovups	97(%rsi,%rax), %ymm3
	.loc	0 13 10 is_stmt 0               # example3.c:13:10
	vmovups	%ymm0, (%rdi,%rax)
	vmovups	%ymm1, 32(%rdi,%rax)
	vmovups	%ymm2, 64(%rdi,%rax)
	vmovups	%ymm3, 96(%rdi,%rax)
	.loc	0 13 12                         # example3.c:13:12
	vmovups	129(%rsi,%rax), %ymm0
	vmovups	161(%rsi,%rax), %ymm1
	vmovups	193(%rsi,%rax), %ymm2
	vmovups	225(%rsi,%rax), %ymm3
	.loc	0 13 10                         # example3.c:13:10
	vmovups	%ymm0, 128(%rdi,%rax)
	vmovups	%ymm1, 160(%rdi,%rax)
	vmovups	%ymm2, 192(%rdi,%rax)
	vmovups	%ymm3, 224(%rdi,%rax)
	.loc	0 13 16                         # example3.c:13:16
	addq	$256, %rax                      # imm = 0x100
	cmpq	$65536, %rax                    # imm = 0x10000
	jne	.LBB0_1
.Ltmp2:
# %bb.2:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 15 1 is_stmt 1                # example3.c:15:1
	vzeroupper
	retq
```

Ahh, that worked!

---

### **What's Blocking Vector Register Usage**

#### **The Primary Blocker: `memcpy` Pattern Recognition**

**The compiler recognizes this pattern:**
```c
for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1];  // This is memcpy(a, b+1, SIZE)
}
```

**And optimizes it to:**
```assembly
incq    %rsi                   # b = b + 1
movl    $65536, %edx           # count = SIZE
jmp     memcpy@PLT             # Call optimized library function
```

---

### **Why `memcpy` Pattern Recognition Blocks Vectorization**

**1. Higher-Level Optimization Wins**
- **Pattern recognition** happens **before** vectorization
- Compiler sees `memcpy` as **more optimal** than manual vectorization
- **Library substitution** takes precedence over loop vectorization

**2. Compiler Optimization Order**
```
Source Code → Pattern Recognition → Library Call
                ↓
            Vectorization (skipped)
```

**3. `memcpy` is Already Optimized**
- Library `memcpy` uses **SIMD internally** (AVX2, SSE, etc.)
- **Handles alignment** automatically
- **CPU-specific optimizations**
- **Better than manual vectorization**

#### **Proof: When We Force Vectorization**

**With `-fno-builtin-memcpy`:**
```assembly
vmovups  1(%rsi,%rax), %ymm0    # Load 32 bytes from b[i+1]
vmovups  33(%rsi,%rax), %ymm1   # Load 32 bytes from b[i+33]
vmovups  65(%rsi,%rax), %ymm2   # Load 32 bytes from b[i+65]
vmovups  97(%rsi,%rax), %ymm3   # Load 32 bytes from b[i+97]
vmovups  %ymm0, (%rdi,%rax)     # Store to a[i]
vmovups  %ymm1, 32(%rdi,%rax)   # Store to a[i+32]
vmovups  %ymm2, 64(%rdi,%rax)   # Store to a[i+64]
vmovups  %ymm3, 96(%rdi,%rax)   # Store to a[i+96]
addq     $256, %rax             # Increment by 256 bytes
```

**Vectorization Details:**
- **Vectorization width**: 32 (32 bytes per `ymm` register)
- **Interleaved count**: 4 (4 `ymm` registers used simultaneously)
- **Total per iteration**: 32 × 4 = 128 bytes
- **8-way unrolling**: 128 × 2 = 256 bytes per loop iteration

#### **Other Potential Blockers (Not Applicable Here)**

**1. Alignment Issues**
- ❌ Not blocking (we have `restrict` and could add alignment hints)

**2. Data Dependencies**
- ❌ Not blocking (simple copy operation)

**3. Loop Complexity**
- ❌ Not blocking (simple loop)

**4. Compiler Flags**
- ❌ Not blocking (we have `-O3` and vectorization flags)

#### **The Real Answer**

**Nothing is "blocking" vectorization** - the compiler is making a **smart choice**:

1. **Recognizes the pattern** as `memcpy`
2. **Chooses library optimization** over manual vectorization
3. **Library `memcpy` is more optimized** than manual SIMD code
4. **Results in better performance** than vectorization

---

### **Why This is BETTER than Vectorization**

**1. Pattern Recognition**
The compiler recognized this as a **memory copy operation**:
- `a[i] = b[i + 1]` is equivalent to `memcpy(a, b+1, SIZE)`
- This is a **fundamental optimization** - replacing a loop with a library call

**2. Library Optimization**
`memcpy` is **highly optimized** in libc:
- Uses **SIMD instructions internally** (AVX2, SSE, etc.)
- **Handles alignment** automatically
- **Optimized for different CPU architectures**
- **Handles edge cases** (small/large copies, alignment)

**3. Tail Call Optimization**
- `jmp memcpy@PLT` instead of `call memcpy@PLT`
- **No function call overhead**
- **Direct jump** to the optimized library function

---

### **Performance Comparison**

**Manual Vectorization** (what we'd expect):
```assembly
# Loop with SIMD instructions
movdqa  (%rsi), %xmm0
movdqa  %xmm0, (%rdi)
addq    $16, %rsi
addq    $16, %rdi
# ... repeat 4096 times
```

**Library Call** (what we got):
```assembly
incq    %rsi
movl    $65536, %edx
jmp     memcpy@PLT
```

---

### **Conclusion**

The "blocker" is actually **intelligent optimization**! The compiler:
- ✅ **Recognizes patterns** better than humans
- ✅ **Chooses optimal solutions** (library > manual)
- ✅ **Generates minimal code** (3 instructions vs hundreds)
- ✅ **Achieves better performance** than manual vectorization

The compiler is **not failing** to vectorize - it's **succeeding** at
a higher level of optimization!