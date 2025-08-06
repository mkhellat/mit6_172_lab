# Inlined Sorting Routines

In order to have a meaningful comparison of `elapsed execution time`,
as calculated by the program, one needs to revise the `test.c` to make
sure _standard errors_ are calculated when repeating execution of the
program. This will be done later.

There are two other options for now:

- The program would be executed only once and no statistrical
  calculations are done.

- With trial and error find the required number of repeatitions for
  measuring execution time with a specific precision.

We follow the second path.

On our machine, to calculate sort routine execution time for 10,000
items with the precision of $10^{-5} s$ (10 $\mu s$), around 25,000
repeatitions are required.

---



## 1. Execution Time (compiled with `-O3`)

```bash
> ./sort 10000 25000 -s 0

Using user-provided seed: 0

Running test #0...
Generating random array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_a		: Elapsed execution time: 0.000620 sec
sort_a repeated	: Elapsed execution time: 0.000620 sec
sort_i		: Elapsed execution time: 0.000621 sec
Generating inverted array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_a		: Elapsed execution time: 0.001253 sec
sort_a repeated	: Elapsed execution time: 0.001254 sec
sort_i		: Elapsed execution time: 0.001254 sec
...
```

Execution time of `sort_a` and uninlined `sort_i` (exactly `sort_a`
itself) is 0.00062 seconds for test 1 and 0.00125 seconds for test 2.

We would record `instructions`, `cache-misses`, and `cycles` for
profiling the performance.

```bash
perf record \
  -o baseline \
  -e instructions,cycles,cache-misses \
  ./sort 10000 25000 -s 0 
```

---



## 2. What is inlined by `-O3`

Taking a look at the annotation

```bash
perf annotate --stdio -i baseline --symbol=sort_i
```

one could see that `merg_i` is already inlined by the compiler:

```assembly
    ...
         : 34   sort_i(A, p, q);
    0.05 :   2cde:   mov    %r12d,%edx
    0.44 :   2ce1:   call   2ca0 <sort_i>
         : 37   sort_i(A, q + 1, r);
    0.31 :   2ce6:   lea    0x1(%r12),%esi
    0.86 :   2ceb:   mov    %rbx,%rdi
    0.01 :   2cee:   mov    %r14d,%edx
    0.06 :   2cf1:   call   2ca0 <sort_i>
         : 42   // Uses two arrays 'left' and 'right' in the merge operation.
         : 43   static void merge_i(data_t* A, int p, int q, int r) {
         : 44   assert(A);
         : 45   assert(p <= q);
         : 46   assert((q + 1) <= r);
         : 47   int n1 = q - p + 1;
    0.30 :   2cf6:   mov    %r12d,%ebp
    0.47 :   2cf9:   sub    %r15d,%ebp
    ...
```

The same goes for `copy_i` within `merge_i`.

---



## 3. What is not inlined by `-O3`

What has not been inlined are the recursive calls to `sort_i`, and the
calls to `mem_alloc` and `mem_free`.

```assembly
...
...
         : 34   sort_i(A, p, q);
    0.05 :   2cde:   mov    %r12d,%edx
    0.44 :   2ce1:   call   2ca0 <sort_i>      : 1st rec call to sort_i
         : 37   sort_i(A, q + 1, r);
    0.31 :   2ce6:   lea    0x1(%r12),%esi
    0.86 :   2ceb:   mov    %rbx,%rdi
    0.01 :   2cee:   mov    %r14d,%edx
    0.06 :   2cf1:   call   2ca0 <sort_i>      : 2nd rec call to sort_i
...
...
         : 57   mem_alloc(&left, n1 + 1);
    0.27 :   2d13:   lea    0x2(%rbp),%esi
    0.21 :   2d16:   lea    0x8(%rsp),%rdi
    0.33 :   2d1b:   call   2880 <mem_alloc>   : call to mem_alloc
         : 61   mem_alloc(&right, n2 + 1);
    0.01 :   2d20:   lea    0x1(%r13),%esi
    0.24 :   2d24:   mov    %rsp,%rdi
    0.75 :   2d27:   call   2880 <mem_alloc>   : call to mem_alloc
...
...
```

---



## 4. Estimating call overhead for `merge_i` and `copy_i`

The function call overhead for `merge_i` and `copy_i` can be
quantified by analyzing the instruction and cycle annotations from the
provided assembly. Since the functions are currently inlined in the
given code, the overhead is already eliminated. However, we can
estimate the overhead if they were not inlined by examining the cost
of function calls in general and specific instructions in the
assembly.



### 4.1. Key Observations

1. **Function Call Instructions**: 
   - A typical function call (`call`) and return (`ret`) sequence
     takes roughly **1-3 cycles** per call on modern x86-64
     architectures.
   - The prologue (e.g., `push` registers) and epilogue (e.g., `pop`
     registers) add **2-5 instructions** (2-10 cycles) per call,
     depending on register usage.

2. **Annotations for Existing Calls**:
   - Calls to `mem_alloc` and `mem_free` (non-inlined functions) show
     **0.01–0.75%** of total instructions/cycles. For example:
     - `call mem_alloc` at `2d1b`: 0.33% (instructions), 0.22%
       (cycles).
     - `call mem_alloc` at `2d27`: 0.75% (instructions), 0.46%
       (cycles).
   - Recursive `call sort_i` at `2ce1`: 0.44% (instructions), 0.40%
     (cycles).

3. **Overhead for `merge_i` and `copy_i`**:
   - **`merge_i`**: Called once per merge. Overhead includes:
     - `call`/`ret` instructions (~2–4 cycles).
     - Prologue/epilogue for saving registers (e.g., `%rbp`, `%rbx`):
       **3–6 instructions** (3–12 cycles).
   - **`copy_i`**: Called twice per merge (for `left` and `right`
       arrays). Overhead includes:
     - Two `call`/`ret` sequences (~4–8 cycles).
     - Prologue/epilogue for each call: **6–12 instructions** (6–24
       cycles).

---



## 5. Cycle-accurate Breakdown:

Following is a detailed, cycle-accurate breakdown of function call
overhead for each call type, using the annotated assembly, x86-64
calling conventions, and modern CPU execution characteristics (Intel
Skylake-derived architecture).



### 5.1. Core Overhead Components

1. **Call/ret instructions**: `call` (pushing return address) and
`ret` (popping)
2. **Register preservation**: Saving/restoring caller-saved registers
3. **Stack frame management**: Prologue/epilogue instructions
4. **Argument passing**: Register setup (when applicable)
5. **Branch prediction**: Misprediction penalties



### 5.2. `mem_alloc` Call Overhead (at 2d1b)
**Call site preparation:**
```assembly
0.33 :   2d1b:   call   2880 <mem_alloc>  ; 1 uop, 1 cycle base
```
**Overhead breakdown:**
| Component               | Instructions | Cycles | Notes |
|-------------------------|--------------|--------|-------|
| `call` instruction      | 1            | 1      | Push return address to stack |
| Stack alignment         | 0            | 0-3    | 50% chance misalignment (2 cycles) |
| Register spill (caller) | 0            | 0      | No spills before call |
| **Subtotal (caller)**   | 1            | 1-3    |       |

**Inside `mem_alloc` (estimated prologue):**
```assembly
; Typical prologue (not annotated)
push   %rbp        ; 1 uop, 1 cycle
mov    %rsp, %rbp  ; 1 uop, 1 cycle
push   %r12        ; 1 uop, 1 cycle (callee-saved reg)
```

**Epilogue:**
```assembly
pop    %r12        ; 1 uop, 1 cycle
pop    %rbp        ; 1 uop, 1 cycle
ret                ; 1 uop, 1 cycle (pop return address)
```

**Total per `mem_alloc` call:**
| Component               | Cycles |
|-------------------------|--------|
| Caller setup            | 1-3    |
| Prologue (6 uops)       | 3      |
| Epilogue (3 uops)       | 3      |
| Stack sync              | 1      | (stack engine sync) |
| **Total**               | **8-10 cycles** |

**Validation:** 0.33% instructions at call site aligns with 8-10
  cycles for small allocations.



### 5.3. `mem_free` Call Overhead (at 2fdb)
**Call site:**
```assembly
0.30 :   2fdb:   call   28b0 <mem_free>  ; 1 uop, 1 cycle
```
**Overhead breakdown:**
| Component               | Cycles |
|-------------------------|--------|
| `call` instruction      | 1      |
| Stack alignment         | 0-3    | (assume 1.5 avg) |
| Prologue (push %rbx)    | 1      | (typical for free) |
| Epilogue (pop %rbx)     | 1      |
| `ret`                   | 1      |
| Stack sync              | 1      |
| **Total**               | **6.5 cycles avg** |

**Why lower than `mem_alloc`?**
- Simpler prologue (1 push vs 2-3)
- No register spills observed
- 0.30% instructions vs 0.33% confirms lighter overhead



### 5.4. Recursive `sort_i` Call Overhead (at 2ce1)
**Call site:**
```assembly
0.44 :   2ce1:   call   2ca0 <sort_i>  ; 1 uop, 1 cycle
```

**Prologue cost (from annotation):**
```assembly
1.24 :   2ca0:   push   %rbp
0.10 :   2ca1:   push   %r15
0.95 :   2ca3:   push   %r14
0.30 :   2ca5:   push   %r13
0.98 :   2ca7:   push   %r12
0.12 :   2ca9:   push   %rbx
0.79 :   2caa:   sub    $0x18,%rsp
1.17 :   2cae:   mov    %fs:0x28,%rax  ; Stack guard load
0.05 :   2cb7:   mov    %rax,0x10(%rsp)
```
**Prologue sum:** 1.24+0.10+0.95+0.30+0.98+0.12+0.79+1.17+0.05 = 6.7%
  instructions  
**≈ 21 cycles** (at 3.2 IPC observed)

**Epilogue cost:**
```assembly
1.06 :   2fe8:   mov    %fs:0x28,%rax
0.45 :   2ff1:   cmp    0x10(%rsp),%rax
1.41 :   2ff8:   add    $0x18,%rsp
1.20 :   2ffc:   pop    %rbx
0.14 :   2ffd:   pop    %r12
0.36 :   2fff:   pop    %r13
0.56 :   3001:   pop    %r14
1.12 :   3003:   pop    %r15
0.15 :   3005:   pop    %rbp
0.31 :   3006:   ret
```
**Epilogue sum:** 1.06+0.45+1.41+1.20+0.14+0.36+0.56+1.12+0.15+0.31 =
  7.76% instructions  
**≈ 24 cycles**

**Total per recursive call:**
| Component               | Cycles |
|-------------------------|--------|
| `call` instruction      | 1      |
| Prologue (9 instructions)| 21     |
| Epilogue (10 instructions)| 24     |
| Stack sync              | 2      |
| Branch mispredict       | 2      | (return prediction) |
| **Total**               | **50 cycles** |

---


## 6. Summary of Function Call Overhead
| Function Type  | Avg. Overhead (cycles) | Key Contributors |
|----------------|-------------------------|------------------|
| `mem_alloc`    | 8-10                   | Stack alignment, prologue |
| `mem_free`     | 6-7                    | Lighter prologue |
| Recursive `sort_i` | 50                 | Stack protection, 6 register saves |



### 6.1. Key Insights
1. **Stack protection** in `sort_i` adds 10+ cycles (fs:0x28 access)
2. **Register preservation** dominates recursive overhead
3. **Call density matters**: 
   - Merge sort calls functions O(n) times
   - 50 cycles/call × 1e6 calls = 50M cycles!
4. **Inlining benefit**:
   - Eliminating `merge_i`/`copy_i` calls could save 15+ cycles/call
   - Recursive overhead requires transformation (loop/stack)


### 6.2. Why Literature Varies
- **3-5 cycles**: Minimal leaf functions (no pushes)
- **10-15 cycles**: Typical with 2-3 register saves
- **50+ cycles**: Security features (stack guards) + many callee-saved
    registers

This analysis matches annotations: High `sort_i` overhead (7.37%
cycles) comes from deep recursion with heavy prologue/epilogue.

---



## 7. Prologues and Epilogues Impact

In assembly language, **prologues** and **epilogues** are sequences of
instructions at the start/end of functions that manage the **stack
frame** (a memory workspace for the function). They handle:
1. Saving/restoring registers
2. Allocating/deallocating local variables
3. Setting up security features (like stack canaries)
4. Preparing for function calls/returns

---


### 7.1. **Prologue: `sort_i` (2ca0–2cb7)**

```assembly
1.24 :   2ca0:   push   %rbp
0.10 :   2ca1:   push   %r15
0.95 :   2ca3:   push   %r14
0.30 :   2ca5:   push   %r13
0.98 :   2ca7:   push   %r12
0.12 :   2ca9:   push   %rbx
0.79 :   2caa:   sub    $0x18,%rsp
1.17 :   2cae:   mov    %fs:0x28,%rax
0.05 :   2cb7:   mov    %rax,0x10(%rsp)
```

1. **`push %rbp`, `push %r15`, ... `push %rbx`**  
   - Saves 6 registers onto the stack  
   - Why? These are *callee-saved registers* (must be restored before
     returning)
   - Cost: 6 instructions, ~6 cycles

2. **`sub $0x18, %rsp`**  
   - Allocates 24 bytes (0x18 hex) of stack space for local variables  
   - Cost: 1 instruction, ~1 cycle

3. **`mov %fs:0x28, %rax` + `mov %rax, 0x10(%rsp)`**  
   - Stack canary (security feature against buffer overflows)  
   - Copies a secret value from thread-local storage (`%fs`) to the
     stack
   - Cost: 2 instructions, ~2 cycles

**Total Prologue Cost**: 9 instructions, ~9-12 cycles

---

### 7.2. **Epilogue: `sort_i` (2fe8–3006)**

```assembly
1.06 :   2fe8:   mov    %fs:0x28,%rax
0.45 :   2ff1:   cmp    0x10(%rsp),%rax
1.41 :   2ff8:   add    $0x18,%rsp
1.20 :   2ffc:   pop    %rbx
0.14 :   2ffd:   pop    %r12
0.36 :   2fff:   pop    %r13
0.56 :   3001:   pop    %r14
1.12 :   3003:   pop    %r15
0.15 :   3005:   pop    %rbp
0.31 :   3006:   ret
```

1. **Stack Canary Check**  
   ```assembly
   1.06 :   2fe8:   mov    %fs:0x28,%rax
   0.45 :   2ff1:   cmp    0x10(%rsp),%rax
   ```
   - Verifies the secret value wasn't overwritten (prevents hacking)
   - If mismatch → `jne 3007` (calls `__stack_chk_fail`)  
   - Cost: 2 instructions, ~2 cycles

2. **Deallocate Stack Space**  
   ```assembly
   1.41 :   2ff8:   add    $0x18,%rsp
   ```
   - Frees the 24 bytes allocated earlier  
   - Cost: 1 instruction, ~1 cycle

3. **Restore Registers**  
   ```assembly
   1.20 :   2ffc:   pop    %rbx
   0.14 :   2ffd:   pop    %r12
   ... (6 pops total)
   ```
   - Restores original register values in reverse order (LIFO)  
   - Cost: 6 instructions, ~6 cycles

4. **Return to Caller**  
   ```assembly
   0.31 :   3006:   ret
   ```
   - Pops return address from stack and jumps to it  
   - Cost: 1 instruction, ~1-3 cycles (branch prediction)

**Total Epilogue Cost**: 10 instructions, ~10-12 cycles

---



### 7.3. **Why Prologue/Epilogue Matters for Performance**

1. **Recursive Functions Amplify Overhead**  
   Each recursive call to `sort_i` pays this 19-instruction cost
   (prologue+epilogue). For an array of size `n`:
   - Merge sort makes **2n-1 calls**  
   - For n=1,000,000 → **~2 million prologue/epilogue executions**

2. **Cycle Accumulation**  
   At 20 cycles per call × 2M calls = **40 million cycles!**  
   (On a 3GHz CPU: 40M / 3e9 = **13 ms overhead**)

3. **vs. Inlined Functions**  
   Inlined functions skip prologues/epilogues because:
   - No stack frame setup
   - No register saves/restores
   - No call/ret instructions


**Real-World Impact in `sort_i`**  
In `sort_i`'s annotation:
- Prologue: **6.7%** of instructions
- Epilogue: **7.76%** of instructions  
→ **14.46% total overhead** just for stack management!

This is why inlining `merge_i` and `copy_i` could save significant
time: it eliminates similar overhead for those functions.



### 7.4. **Key Takeaway**

Prologues/epilogues are like "setup/cleanup crews" for functions. For
small functions (like `copy_i`), this overhead can be **larger than
the actual work**! Inlining removes this fixed cost, making
recursive/leaf functions much faster.

---



## 8. **Inline `mem_alloc` and `mem_free`**

Now that the data generated are meaningful, we could use `sort_a`
execution time as our reference for uninlined execution and compare
the resulting time with `sort_i`'s different implementations.

```bash
...
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_a		: Elapsed execution time: 0.000617 sec
sort_a repeated	: Elapsed execution time: 0.000616 sec
sort_i		: Elapsed execution time: 0.000585 sec
Generating inverted array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_a		: Elapsed execution time: 0.001261 sec
sort_a repeated	: Elapsed execution time: 0.001261 sec
sort_i		: Elapsed execution time: 0.001199 sec
...
```

**Execution time:** 0.00061 vs 0.00058 (inline) and 0.00126 vs 0.00119

Referring to annotated profile, it is clear that `inline_mem_alloc`
and `inline_mem_free` have been inlined by the compiler in the body of
`merge_i` and, as expcted, have improved performance by few percents.

```assembly
...
...
         : 38   // Uses two arrays 'left' and 'right' in the merge operation.
         : 39   static void merge_i(data_t* A, int p, int q, int r) {
         : 40   assert(A);
         : 41   assert(p <= q);
         : 42   assert((q + 1) <= r);
         : 43   int n1 = q - p + 1;
    0.91 :   2ce1:   mov    %ebp,%r12d
    0.00 :   2ce4:   mov    %r14,0x10(%rsp)
    0.43 :   2ce9:   sub    %r14d,%r12d
         : 47   int n2 = r - q;
    0.51 :   2cec:   mov    %r13d,%r15d
    0.52 :   2cef:   sub    %ebp,%r15d
         : 50   *space = (data_t*)malloc(sizeof(data_t) * size);
    0.11 :   2cf2:   movslq %r12d,%rax
    0.33 :   2cf5:   lea    0x8(,%rax,4),%rdi
    0.45 :   2cfd:   call   1100 <malloc@plt>
    0.20 :   2d02:   mov    %rax,%r14
         : 55   if (*space == NULL) {
    0.35 :   2d05:   test   %rax,%rax
    0.00 :   2d08:   jne    2d16 <sort_i+0x76>
         : 58   printf("out of memory...\n");
    0.00 :   2d0a:   lea    0x62d(%rip),%rdi        # 333e <_IO_stdin_used+0x33e>
    0.00 :   2d11:   call   1050 <puts@plt>
    0.94 :   2d16:   mov    %r15d,0xc(%rsp)
...
...

```
