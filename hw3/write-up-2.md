## AVX2 Aligned Moves

**Write-up 2**: This code is still not aligned when using AVX2
registers. Fix the code to make sure it uses aligned moves for the
best performance.

We should do 32-byte alignment as AVX2 registers are 256-bit long.

```c
// Copyright (c) 2015 MIT License by 6.172 Staff

#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define SIZE (1L << 16)

void test(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;
  
  a = __builtin_assume_aligned(a, 32);
  b = __builtin_assume_aligned(b, 32);
  
  for (i = 0; i < SIZE; i++) {
    a[i] += b[i];
  }
}
```

which lead to assembly code:

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
	.loc	0 16 10 prologue_end is_stmt 1  # example1.c:16:10
	vmovdqa	(%rdi,%rax), %ymm0
	vmovdqa	32(%rdi,%rax), %ymm1
	vmovdqa	64(%rdi,%rax), %ymm2
	vmovdqa	96(%rdi,%rax), %ymm3
	vpaddb	(%rsi,%rax), %ymm0, %ymm0
	vpaddb	32(%rsi,%rax), %ymm1, %ymm1
	vpaddb	64(%rsi,%rax), %ymm2, %ymm2
	vpaddb	96(%rsi,%rax), %ymm3, %ymm3
	vmovdqa	%ymm0, (%rdi,%rax)
	vmovdqa	%ymm1, 32(%rdi,%rax)
	vmovdqa	%ymm2, 64(%rdi,%rax)
	vmovdqa	%ymm3, 96(%rdi,%rax)
.Ltmp2:
	.loc	0 15 26                         # example1.c:15:26
	subq	$-128, %rax
	cmpq	$65536, %rax                    # imm = 0x10000
	jne	.LBB0_1
.Ltmp3:
# %bb.2:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 18 1                          # example1.c:18:1
	vzeroupper
	retq
```

---

### 1. **AVX2 Register Sizes and Alignment Requirements**

**AVX2 (Advanced Vector Extensions 2)** uses 256-bit (32-byte)
  registers:
- `ymm0`, `ymm1`, `ymm2`, etc. are 32-byte registers
- Each `ymm` register can hold 32 `uint8_t` values simultaneously
- **Critical**: AVX2 aligned instructions (`vmovdqa`) require 32-byte
    alignment

**SSE/SSE2** uses 128-bit (16-byte) registers:
- `xmm0`, `xmm1`, etc. are 16-byte registers  
- Aligned instructions (`movdqa`) require 16-byte alignment


### 2. **Why 16-byte Alignment Failed**

When you used `__builtin_assume_aligned(a, 16)`:

```c
// This tells compiler: "a points to 16-byte aligned data"
a = __builtin_assume_aligned(a, 16);
```

But the compiler was generating **AVX2 code** (`vmovdqu`/`vmovdqa`
with `ymm` registers), which needs **32-byte alignment**. The
compiler's logic was:

> "Even if the base pointer is 16-byte aligned, when I access `a[i]`
  for arbitrary `i`, I can't guarantee 32-byte alignment for AVX2
  operations. Better use unaligned moves to be safe."


### 3. **Why 32-byte Alignment Worked**

With `__builtin_assume_aligned(a, 32)`:

```c
// This tells compiler: "a points to 32-byte aligned data"  
a = __builtin_assume_aligned(a, 32);
```

Now the compiler can reason:
> "The base pointer is 32-byte aligned, and I'm processing 32-byte
  chunks (128 bytes per iteration with 4-way unrolling), so all my
  AVX2 accesses will be aligned. I can use `vmovdqa`!"


### 4. **The Generated Code Analysis**

Let's examine what the compiler generated:

```assembly
vmovdqa  (%rdi,%rax), %ymm0      # Load 32 bytes from a[i] to ymm0
vmovdqa  32(%rdi,%rax), %ymm1    # Load 32 bytes from a[i+32] to ymm1  
vmovdqa  64(%rdi,%rax), %ymm2    # Load 32 bytes from a[i+64] to ymm2
vmovdqa  96(%rdi,%rax), %ymm3    # Load 32 bytes from a[i+96] to ymm3
```

**Key insight**: The compiler is processing **128 bytes per
  iteration** (4 × 32-byte chunks), and incrementing `%rax` by 128
  each time. Since the base pointer is 32-byte aligned, all these
  offsets maintain 32-byte alignment.


### 5. **Loop Unrolling and Vectorization**

The compiler applied **4-way unrolling**:
- **Vectorization width**: 32 (32 bytes per `ymm` register)
- **Interleaved count**: 4 (4 `ymm` registers used simultaneously)
- **Total per iteration**: 32 × 4 = 128 bytes

This means:
- **1 iteration** processes 128 array elements
- **512 iterations** total (65536 ÷ 128 = 512)
- Each iteration does 4 parallel AVX2 operations


### 6. **Memory Access Pattern**

With 32-byte alignment guarantee, the memory accesses are:
```
Iteration 0: a[0-31], a[32-63], a[64-95], a[96-127]     (all 32-byte aligned)
Iteration 1: a[128-159], a[160-191], a[192-223], a[224-255]  (all 32-byte aligned)
...
```

### 7. **Why `restrict` Helps**

The `restrict` keyword tells the compiler:
> "Pointers `a` and `b` don't alias - they point to different memory
  regions."

This enables:
- **Better vectorization** (no need to check for overlap)
- **More aggressive optimizations**
- **Elimination of redundant loads**


### 8. **Performance Implications**

**Aligned moves (`vmovdqa`)**:
- Slightly faster on older CPUs
- Can enable additional CPU optimizations
- No penalty for misalignment handling

**Modern CPUs** (Sandy Bridge and later):
- Handle unaligned loads very efficiently
- Performance difference is often negligible
- But aligned moves are still preferred when possible


### 9. **The Compiler's Conservative Nature**

Even with alignment hints, compilers are conservative because:
- **Safety first**: Wrong alignment assumptions can cause crashes
- **Portability**: Code must work across different architectures
- **Complex analysis**: Proving alignment for arbitrary array indexing is hard

The 32-byte alignment worked because it matched the AVX2 register size
and the compiler's vectorization strategy perfectly.
