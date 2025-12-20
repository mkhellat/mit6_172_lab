# Cilksan Race Detection in N-Queens

> "Race conditions are subtle bugs that can hide in parallel code - tools
> like Cilksan are essential for finding them." - Performance Engineering
> Principle

---

## Overview

This write-up documents the use of Cilksan to detect a determinacy race
condition in the parallelized N-Queens implementation. Cilksan successfully
identified concurrent access to the global `count` variable.

---

## Cilksan Setup

### Compilation

Added Cilksan support to the Makefile:

```makefile
ifeq ($(CILKSAN), 1)
	CC := /opt/opencilk-2/bin/clang
	CFLAGS += -fopencilk -fsanitize=cilk
	LDFLAGS += -fopencilk -fsanitize=cilk
endif
```

### Building with Cilksan

```bash
make clean
make CILKSAN=1
```

This compiles the program with Cilksan instrumentation enabled, allowing
runtime detection of determinacy races.

---

## Race Condition Detection

### Cilksan Output

When running the parallelized queens program with Cilksan, it detected **2
distinct races** (and suppressed 271 duplicate race reports):

```
Running Cilksan race detector.
Race detected on location 557ba439b060
*    Write 557b94389dc5 try /home/.../queens.c:15:22
|        `-to variable count (declared at /home/.../queens.c:10)
+     Call 557b9438a2bb try /home/.../queens.c:20:24
+    Spawn 557b94389e28 try /home/.../queens.c:20:24
...
|*    Read 557b94389da2 try /home/.../queens.c:15:22
||       `-to variable count (declared at /home/.../queens.c:10)
...
Cilksan detected 2 distinct races.
Cilksan suppressed 271 duplicate race reports.
```

### Race Location

**Line Numbers**:
- **Write**: Line 15, column 22 (`++count`)
- **Read**: Line 15, column 22 (reading `count` before increment)
- **Variable**: `count` declared at line 10

### Code Context

```c
static int count = 0;  // Line 10: Global variable declaration

void try(int row, int left, int right) {
    int poss, place;
    if (row == 0xFF) ++count;  // Line 15: Race condition here!
    else {
        poss = ~(row | left | right) & 0xFF;
        while (poss != 0) {
            place = poss & -poss;
            cilk_spawn try(row | place, (left | place) << 1, (right | place) >> 1);
            poss &= ~place;
        }
        cilk_sync;
    }
}
```

---

## Race Condition Description

### What is the Race?

The race condition occurs when **multiple parallel threads** simultaneously
execute the statement `++count` at line 15. This is a **determinacy race**
because:

1. **Multiple writes**: Different parallel tasks can all reach the base case
   (`row == 0xFF`) and try to increment `count` concurrently
2. **Read-modify-write**: The `++count` operation is not atomic - it
   involves:
   - Reading the current value of `count`
   - Incrementing it
   - Writing the new value back
3. **Lost updates**: When multiple threads perform this read-modify-write
   concurrently, some increments can be lost, leading to incorrect final
   counts

### Why Does It Happen?

The parallelization spawns multiple recursive calls in the `while` loop:

```c
while (poss != 0) {
    place = poss & -poss;
    cilk_spawn try(row | place, ...);  // Spawn parallel tasks
    poss &= ~place;
}
cilk_sync;  // Wait for all spawned tasks
```

Each spawned task explores a different branch of the search tree. When
multiple branches find valid solutions (reach `row == 0xFF`), they all try
to increment the shared global `count` variable simultaneously, causing the
race.

### Execution Flow

```
Thread 1: Finds solution → reads count (e.g., 50) → increments → writes 51
Thread 2: Finds solution → reads count (e.g., 50) → increments → writes 51
Thread 3: Finds solution → reads count (e.g., 50) → increments → writes 51
```

**Expected**: count should be 53 (50 + 3)  
**Actual**: count might be 51 (only one increment is preserved)

---

## Impact

### Correctness

- **Incorrect results**: The final count may be less than 92 (the correct
  number of solutions for N=8)
- **Non-deterministic**: Different runs may produce different counts
  depending on timing
- **Silent failure**: The program may appear to work but produce wrong
  answers

### Why It Might Not Be Obvious

For N=8, the problem is small enough that:
- The race might not always manifest (timing-dependent)
- The count might still be close to correct (not all increments are lost)
- The program completes so quickly that the issue isn't noticed

For larger N, the race would be more likely to cause significant errors.

---

## Solution

The race can be fixed using **Cilk reducers** (hyperobjects), which
provide thread-safe reduction operations. A reducer would ensure that all
increments to `count` are properly combined, eliminating the race condition.

This will be addressed in a future write-up when implementing reducers.

---

## Conclusion

**Was Cilksan able to detect the problem?**

**Yes!** Cilksan successfully detected the determinacy race condition on
the global `count` variable.

**Race Condition Description**:

Multiple parallel threads concurrently read and write to the shared global
`count` variable at line 15 when they find valid solutions. The
read-modify-write operation (`++count`) is not atomic, causing lost
updates when multiple threads increment the counter simultaneously.

**Relevant Line Numbers**:
- **Write**: Line 15, column 22 (`++count`)
- **Read**: Line 15, column 22 (reading `count` as part of increment)
- **Variable declaration**: Line 10 (`static int count = 0;`)

This demonstrates the importance of using race detection tools like Cilksan
when parallelizing code, as race conditions can be subtle and
timing-dependent, making them difficult to detect through testing alone.

