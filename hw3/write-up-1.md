## Strength Reduction or Countdown Loop

Look at the assembly code above. The compiler has translated the code
to set the start index at −216 and adds to it for each memory
access. Why doesn’t it set the start index to 0 and use small positive
offsets?

### What is the register that stores the `for` loop indexiing variable `i`?

Looking at the provided assembly snippet, the 64-bit register `%rax`
holds the for loop indexing variable `-i` and is set to `-1L << 16` at
the start of the koop

```assembly
movq    $-65536, %rax       # imm = 0xFFFF0000
```

However, on the generated assembly on my machine it is not the case:

```assembly
# %bb.0:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 12 3 prologue_end             # example1.c:12:3
	leaq	65536(%rdi), %rax
	leaq	65536(%rsi), %rcx
	cmpq	%rcx, %rdi
	setb	%cl
	cmpq	%rax, %rsi
	setb	%al
	testb	%al, %cl
	je	.LBB0_1
.Ltmp0:
# %bb.3:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 0 3 is_stmt 0                 # example1.c:0:3
	xorl	%eax, %eax
.Ltmp1:
	.p2align	4
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- $rax
	.loc	0 13 13 is_stmt 1               # example1.c:13:13
	movzbl	(%rsi,%rax), %ecx
	.loc	0 13 10 is_stmt 0               # example1.c:13:10
	addb	%cl, (%rdi,%rax)
.Ltmp2:
	#DEBUG_VALUE: test:i <- [DW_OP_constu 1, DW_OP_or, DW_OP_stack_value] $rax
	.loc	0 13 13                         # example1.c:13:13
	movzbl	1(%rsi,%rax), %ecx
	.loc	0 13 10                         # example1.c:13:10
	addb	%cl, 1(%rdi,%rax)
.Ltmp3:
	#DEBUG_VALUE: test:i <- [DW_OP_constu 2, DW_OP_or, DW_OP_stack_value] $rax
	.loc	0 13 13                         # example1.c:13:13
	movzbl	2(%rsi,%rax), %ecx
	.loc	0 13 10                         # example1.c:13:10
	addb	%cl, 2(%rdi,%rax)
.Ltmp4:
	#DEBUG_VALUE: test:i <- [DW_OP_constu 3, DW_OP_or, DW_OP_stack_value] $rax
	.loc	0 13 13                         # example1.c:13:13
	movzbl	3(%rsi,%rax), %ecx
	.loc	0 13 10                         # example1.c:13:10
	addb	%cl, 3(%rdi,%rax)
.Ltmp5:
	.loc	0 12 26 is_stmt 1               # example1.c:12:26
	addq	$4, %rax
.Ltmp6:
	#DEBUG_VALUE: test:i <- $rax
	.loc	0 12 17 is_stmt 0               # example1.c:12:17
	cmpq	$65536, %rax                    # imm = 0x10000
.Ltmp7:
	.loc	0 12 3                          # example1.c:12:3
	jne	.LBB0_4
	jmp	.LBB0_5
.Ltmp8:
.LBB0_1:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 0 3                           # example1.c:0:3
	xorl	%eax, %eax
.Ltmp9:
	.p2align	4
.LBB0_2:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 13 13 is_stmt 1               # example1.c:13:13
	movdqu	(%rsi,%rax), %xmm0
	movdqu	16(%rsi,%rax), %xmm1
	.loc	0 13 10 is_stmt 0               # example1.c:13:10
	movdqu	(%rdi,%rax), %xmm2
	paddb	%xmm0, %xmm2
	movdqu	16(%rdi,%rax), %xmm0
	paddb	%xmm1, %xmm0
	movdqu	32(%rdi,%rax), %xmm1
	movdqu	48(%rdi,%rax), %xmm3
	movdqu	%xmm2, (%rdi,%rax)
	movdqu	%xmm0, 16(%rdi,%rax)
	.loc	0 13 13                         # example1.c:13:13
	movdqu	32(%rsi,%rax), %xmm0
	.loc	0 13 10                         # example1.c:13:10
	paddb	%xmm1, %xmm0
	.loc	0 13 13                         # example1.c:13:13
	movdqu	48(%rsi,%rax), %xmm1
	.loc	0 13 10                         # example1.c:13:10
	paddb	%xmm3, %xmm1
	movdqu	%xmm0, 32(%rdi,%rax)
	movdqu	%xmm1, 48(%rdi,%rax)
.Ltmp10:
	.loc	0 12 26 is_stmt 1               # example1.c:12:26
	addq	$64, %rax
	cmpq	$65536, %rax                    # imm = 0x10000
	jne	.LBB0_2
.Ltmp11:
.LBB0_5:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	.loc	0 15 1                          # example1.c:15:1
	retq
.Ltmp12:
.Lfunc_end0:
	.size	test, .Lfunc_end0-test
	.cfi_endproc
                                        # -- End function

```

From the debug info, it is clear that `%rax` is again holdong the
indexing variable:

```
.LBB0_4:                                # =>This Inner Loop Header: Depth=1
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- $rax
```

and from loop unrolling (incrementing `%rax` value by 4):

```assembly
.Ltmp5:
	.loc	0 12 26 is_stmt 1               # example1.c:12:26
	addq	$4, %rax
```

and from comparing `%rax` with `SIZE`:

```assembly
.Ltmp6:
	#DEBUG_VALUE: test:i <- $rax
	.loc	0 12 17 is_stmt 0               # example1.c:12:17
	cmpq	$65536, %rax                    # imm = 0x10000
```

Also it seems that at the beginning, the 32-bit rehister `%eax`
register is zeroed-out:


```assembly
# %bb.3:
	#DEBUG_VALUE: test:a <- $rdi
	#DEBUG_VALUE: test:b <- $rsi
	#DEBUG_VALUE: test:i <- 0
	.loc	0 0 3 is_stmt 0                 # example1.c:0:3
	xorl	%eax, %eax
```

#### why is not the register `%rax` zeroed out?

It seems that this a common compiler optimization where when you write
to a 32-bit register on an x86_64 arcgitecture, it zutomatically
__zero-extends__ to the 64-bit register `%rax`.

The thing is that `xorl %eax, %eax` (3 bytes) is shorter than `xorq
%rax, %rax` (4 bytes) but they both lead to the same result.

---

### Why is the compiler implementing a countdown loop instead of the original loop?

Well, I had to cheat and research about it. This is a clever compiler
optimization called **"strength reduction"** or a **"countdown
loop"**:

**Original logic (conceptually):**
- `i = 0`
- Loop while `i < 65536`
- Access `a[i]` and `b[i]`
- Increment `i`

**What this compiler does:**
- `i = -65536`
- Loop while `i != 0`
- Access `a[i + 65536]` and `b[i + 65536]`
- Increment `i`

Look at the memory access patterns in the loop (lines 47-65):
- `movzbl 65536(%rsi,%rax), %ecx` - This is `b[%rax + 65536]`
- `addb %cl, 65536(%rdi,%rax)` - This is `a[%rax + 65536]`

When `%rax = -65536`, you get `a[-65536 + 65536] = a[0]`  
When `%rax = -65535`, you get `a[-65535 + 65536] = a[1]`  
...  
When `%rax = -1`, you get `a[-1 + 65536] = a[65535]`  
When `%rax = 0`, the loop exits

**Benefits:**
- Comparing against zero (`jne` after incrementing) can be faster than
  comparing against a constant
- The zero flag is set automatically by arithmetic operations

The same pattern appears in the vectorized version (line 84), also
initializing to `-65536`.
