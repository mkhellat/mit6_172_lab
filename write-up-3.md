## Ternary Magic

**Write-up 3:** Provide a theory for why the compiler is generating
  dramatically different assembly.

### **The Real Change**

**OLD Version**: Used `if` statement
```c
if (y[i] > x[i]) x[i] = y[i];
```

**NEW Version**: Used ternary operator  
```c
a[i] = (b[i] > a[i]) ? b[i] : a[i];
```

### **Why This Made Such a Difference**

**1. Compiler Recognition**
- **Ternary operator**: Compiler immediately recognizes this as
    `max(a[i], b[i])`
- **If statement**: Compiler has to analyze the conditional logic more
    carefully

**2. Direct SIMD Mapping**
- **Ternary**: Maps directly to `pmaxub` instruction
- **If statement**: Requires complex conditional logic with `pminub` +
    `pcmpeqb` + branching

**3. Eliminated Intermediate Variables**
- **Old**: Used `x` and `y` pointers (extra indirection)
- **New**: Direct `a` and `b` usage (cleaner)


### **Assembly Comparison**

**OLD (`example2.s.old`):**
```assembly
movdqa  (%rsi,%rax), %xmm0      # Load y[i] 
movdqa  (%rdi,%rax), %xmm1      # Load x[i]
pminub  %xmm0, %xmm1            # min(y[i], x[i])
pcmpeqb %xmm0, %xmm1            # Compare y[i] == min(...)
# Complex branching logic to conditionally update
addq    $16, %rax               # 16 bytes per iteration
```

**NEW (`example2.s`):**
```assembly
movdqa  (%rsi,%rax), %xmm0      # Load b[i]
pmaxub  (%rdi,%rax), %xmm0      # max(a[i], b[i]) directly!
movdqa  %xmm0, (%rdi,%rax)      # Store result
# 4-way unrolled, no branches
addq    $64, %rax               # 64 bytes per iteration
```

### **The Key Insight**

The **ternary operator** (`?:`) is semantically equivalent to `max()`,
so the compiler can:
1. **Recognize the pattern** immediately
2. **Use the optimal instruction** (`pmaxub`)
3. **Apply aggressive unrolling** (4x more)
4. **Eliminate all branching** in the hot path

The **if statement** requires the compiler to:
1. **Analyze the conditional logic**
2. **Use indirect approach** (`pminub` + comparison)
3. **Generate complex branching** code
4. **Be more conservative** with optimizations

