## `-ffast-math` re-ordering vs IEE 754

**Write-up 5**: Check the assembly and verify that it does in fact
vectorize properly. Also what do you notice when you run the command

```bash
$ clang -O3 example4.c -o example4; ./example4
```

with and without the `-ffast-math` ﬂag? Speciﬁcally, why do you a see
a difference in the output.

---

### **Vectorization Evidence**

**1. SIMD Instructions Used**
```assembly
addpd  (%rdi,%rax,8), %xmm0    # Add 2 doubles from memory to xmm0
addpd  16(%rdi,%rax,8), %xmm1  # Add 2 doubles from memory to xmm1
addpd  32(%rdi,%rax,8), %xmm0  # Add 2 doubles from memory to xmm0
addpd  48(%rdi,%rax,8), %xmm1  # Add 2 doubles from memory to xmm1
# ... continues for 8 iterations
```

**2. Loop Unrolling**
- **8-way unrolling**: Processes 8 memory locations per iteration
- **16 doubles per iteration**: 8 × 2 doubles per `addpd` = 16 doubles
- **Loop increment**: `addq $16, %rax` (16 doubles = 128 bytes)

**3. Multiple Accumulators**
- Uses **2 XMM registers** (`%xmm0` and `%xmm1`) as accumulators
- **Reduces dependency chains** and improves instruction-level parallelism

---

### **Why This Vectorization Works**

**1. Perfect Data Type Match**
- **`double`** (8 bytes) fits perfectly in **SSE2** (16-byte registers)
- **2 doubles per `addpd`** instruction
- **Aligned access** with `__builtin_assume_aligned(a, 16)`

**2. Simple Reduction Pattern**
```c
y += x[i];  // Simple accumulation
```
- **No dependencies** between iterations
- **Commutative operation** (addition order doesn't matter)
- **Perfect for SIMD reduction**

**3. Aggressive Loop Unrolling**
- **8-way unrolling** reduces loop overhead
- **Multiple accumulators** prevent register pressure
- **Optimal memory access pattern**

---

### **Vectorization Quality**

**1. Instruction Selection: EXCELLENT**
- `addpd`: **Parallel double-precision addition** (SSE2)
- `movapd`: **Aligned move** for double-precision data
- `unpckhpd`: **Unpack high packed double** for final reduction

**2. Memory Access Pattern: OPTIMAL**
- **Sequential access**: Perfect for prefetching
- **Aligned access**: No alignment penalties
- **128 bytes per iteration**: Good cache line utilization

**3. Reduction Strategy: SOPHISTICATED**
```assembly
addpd  %xmm0, %xmm1           # Combine two accumulators
movapd %xmm1, %xmm0           # Move to xmm0
unpckhpd %xmm1, %xmm0         # Extract high double
addsd  %xmm1, %xmm0           # Final scalar addition
```

---

### **Different Results**

**With `-ffast-math`:**
```
Raw result: 0x1.755cccec10aa3p+3
```

**Without `-ffast-math`:**
```
Raw result: 0x1.755cccec10aa5p+3
```

The difference is in the **last few bits** of the mantissa: `aa3` vs
`aa5`.


### **Why This Happens**

#### **1. Floating-Point Associativity**

**Mathematical associativity**: `(a + b) + c = a + (b + c)`  
**Floating-point associativity**: `(a + b) + c ≠ a + (b + c)` (due to
  rounding errors)

#### **2. `-ffast-math` Changes the Order of Operations**

**Without `-ffast-math` (strict IEEE 754):**
- Compiler must preserve **exact order** of additions
- **Sequential summation**: `((((x[0] + x[1]) + x[2]) + x[3]) + ...)`
- **Conservative optimization**

**With `-ffast-math` (relaxed):**
- Compiler can **reorder operations** for performance
- **Parallel summation**: Multiple accumulators, different grouping
- **Aggressive optimization**

#### **3. Vectorization Impact**

Looking at the assembly we analyzed:

```assembly
# Multiple accumulators (xmm0 and xmm1)
addpd  (%rdi,%rax,8), %xmm0    # Accumulator 1
addpd  16(%rdi,%rax,8), %xmm1  # Accumulator 2
addpd  32(%rdi,%rax,8), %xmm0  # Accumulator 1
addpd  48(%rdi,%rax,8), %xmm1  # Accumulator 2
# ... continues

# Final reduction
addpd  %xmm0, %xmm1           # Combine accumulators
```

**This creates a different summation order:**
- **Without `-ffast-math`**: Sequential order preserved
- **With `-ffast-math`**: Parallel accumulators → different grouping →
    different result

---

### **The Mathematical Explanation**

#### **Sequential Summation (without `-ffast-math`):**
```
sum = ((((x[0] + x[1]) + x[2]) + x[3]) + ...)
```

#### **Parallel Summation (with `-ffast-math`):**
```
acc1 = x[0] + x[2] + x[4] + x[6] + ...
acc2 = x[1] + x[3] + x[5] + x[7] + ...
sum = acc1 + acc2
```

**These produce different results** due to floating-point rounding!

---

### **Why `-ffast-math` Enables This**

**`-ffast-math` includes several flags:**
- `-ffinite-math-only`: No NaN/Inf handling
- `-fno-signed-zeros`: Ignore sign of zero
- `-fno-trapping-math`: No floating-point exceptions
- **`-fassociative-math`**: Allow reordering of operations

**The key flag**: `-fassociative-math` allows the compiler to:
- **Reorder additions** for better performance
- **Use multiple accumulators** (vectorization)
- **Group operations differently**

---

### **Performance vs. Accuracy Trade-off**

#### **Without `-ffast-math`:**
- ✅ **IEEE 754 compliant** (deterministic results)
- ✅ **Reproducible** across different compilers/platforms
- ❌ **Slower** (sequential operations)
- ❌ **Less vectorization**

#### **With `-ffast-math`:**
- ✅ **Much faster** (parallel operations, vectorization)
- ✅ **Better SIMD utilization**
- ❌ **Non-deterministic** results
- ❌ **May violate IEEE 754** standards

---

### **Real-World Impact**

**For scientific computing:**
- **Without `-ffast-math`**: Reproducible, but slower
- **With `-ffast-math`**: Faster, but results may vary

**For graphics/games:**
- **`-ffast-math`** is often acceptable (speed > precision)

**For financial calculations:**
- **Avoid `-ffast-math`** (precision critical)


### **The Bottom Line**

The difference you observed (`aa3` vs `aa5`) is **expected behavior**
when using `-ffast-math`. It's not a bug - it's the compiler trading
**exact reproducibility** for **performance optimization**.

**Both results are "correct"** in their respective contexts:
- **Without `-ffast-math`**: IEEE 754 compliant, deterministic
- **With `-ffast-math`**: Optimized, potentially faster, non-deterministic

