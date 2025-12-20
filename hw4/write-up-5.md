# Thread-Safe N-Queens Parallelization

> "Thread-safe data structures eliminate races, but overhead can still
> impact performance." - Performance Engineering Principle

---

## Overview

This write-up documents the re-parallelization of the N-Queens algorithm
using thread-safe temporary lists, eliminating the race condition while
maintaining correctness.

---

## Implementation Strategy

### Problem with Original Approach

The original parallelization had a race condition because multiple threads
concurrently accessed a shared global `count` variable. The solution is to
use **temporary lists** that are merged after all parallel tasks complete.

### Implementation Details

**Key Strategy**: Each recursive branch gets its own temporary `BoardList`,
and all lists are merged after `cilk_sync` completes.

```c
void try(int row, int left, int right, BoardList* board_list) {
    int poss, place;
    if (row == 0xFF) {
        // Found a solution: append to this branch's list
        append_board(board_list, (Board)row);
    } else {
        poss = ~(row | left | right) & 0xFF;
        
        // Count number of valid positions to allocate array
        int count = 0;
        int temp_poss = poss;
        while (temp_poss != 0) {
            temp_poss &= temp_poss - 1;  // Count bits
            count++;
        }
        
        // Allocate array of temporary lists (one per recursive branch)
        BoardList* temp_lists = (BoardList*)malloc(count * sizeof(BoardList));
        for (int i = 0; i < count; i++) {
            init_list(&temp_lists[i]);
        }
        
        // Spawn all recursive calls with their own temp lists
        int idx = 0;
        while (poss != 0) {
            place = poss & -poss;
            // Each spawned task works on its own list - no shared state!
            cilk_spawn try(row | place, (left | place) << 1, 
                          (right | place) >> 1, &temp_lists[idx]);
            poss &= ~place;
            idx++;
        }
        cilk_sync;  // Wait for all spawned tasks to complete
        
        // After sync: merge all temp lists into board_list (thread-safe)
        for (int i = 0; i < count; i++) {
            merge_lists(board_list, &temp_lists[i]);
        }
        
        free(temp_lists);
    }
}
```

### Why This is Thread-Safe

1. **No shared writes**: Each spawned task writes only to its own `temp_lists[i]`
2. **Synchronization point**: `cilk_sync` ensures all tasks complete before merging
3. **Sequential merging**: Merges happen sequentially after sync, so no concurrent
   access to `board_list`
4. **O(1) merge operations**: Each merge is constant time, so sequential merging
   is efficient

---

## Performance Results

### Execution Times

| Workers | Real Time | User Time | Sys Time | CPU Usage | Speedup | Solutions |
|---------|-----------|-----------|----------|-----------|---------|-----------|
| 1 (Serial) | 0.003s | 0.00s | 0.00s | 67% | 1.00x | 92 ✓ |
| 4 | 0.005s | 0.00s | 0.00s | 86% | 0.60x | 92 ✓ |
| 8 | 0.006s | 0.00s | 0.01s | 125% | 0.50x | 92 ✓ |

### Cilksan Verification

```bash
make clean && make CILKSAN=1
CILK_NWORKERS=4 ./queens
```

**Result**: `Cilksan detected 0 distinct races.` ✅

---

## Performance Analysis

### Why Parallel Code is Slower

The parallel version is **slower** than the serial version (0.60x-0.50x
speedup). This is expected and can be explained by several factors:

#### 1. **Spawn/Sync Overhead**

Each `cilk_spawn` and `cilk_sync` has overhead:
- Task creation and scheduling
- Work-stealing operations
- Synchronization barriers

For a problem that completes in **milliseconds**, this overhead is
significant relative to the actual work.

#### 2. **Memory Allocation Overhead**

The parallel version allocates:
- An array of `BoardList` structures for each recursive level
- Additional `BoardNode` allocations for solutions

This adds memory management overhead that the serial version doesn't have.

#### 3. **Merge Operations**

After each `cilk_sync`, we perform sequential merge operations:
- While each merge is O(1), there are many merges
- The sequential merging adds overhead

#### 4. **Problem Size**

For N=8:
- Only **92 solutions** total
- Completes in **milliseconds**
- Overhead dominates the actual computation

The parallel overhead exceeds any potential speedup from parallel execution.

#### 5. **Load Imbalance**

The backtracking tree is highly irregular:
- Some branches terminate early (invalid placements)
- Some branches find many solutions
- This creates load imbalance, reducing parallel efficiency

#### 6. **Cache Effects**

Parallel execution may cause:
- Cache misses from context switching
- Reduced cache locality
- Memory bandwidth contention

---

## Comparison with Serial Code

### Serial Code Performance

- **Time**: 0.003s
- **CPU Usage**: 67%
- **Memory**: Minimal (no temp list arrays)
- **Overhead**: Minimal (no spawn/sync)

### Parallel Code Performance

- **Time**: 0.005-0.006s (slower)
- **CPU Usage**: 86-125% (higher due to overhead)
- **Memory**: Higher (temp list arrays at each level)
- **Overhead**: Significant (spawn/sync, allocation, merging)

### Why Serial is Faster

1. **No spawn overhead**: Direct function calls are faster than task creation
2. **No synchronization**: No barriers or work-stealing overhead
3. **Better cache locality**: Sequential execution has better cache behavior
4. **Less memory allocation**: No temp list arrays needed

---

## When Would Parallelization Help?

Parallelization would be beneficial for:

1. **Larger problem sizes** (N > 12):
   - More work per task
   - Overhead becomes less significant relative to work
   - Better amortization of spawn/sync costs

2. **More balanced workloads**:
   - More uniform tree structure
   - Better load balance across threads

3. **Longer execution times**:
   - Overhead is fixed, so longer runs amortize it better
   - For N=8 (milliseconds), overhead dominates

---

## Correctness Verification

### Serial Code
- ✅ Finds 92 solutions (correct for N=8)
- ✅ No race conditions (single-threaded)

### Parallel Code
- ✅ Finds 92 solutions (correct for N=8)
- ✅ Cilksan: 0 races detected
- ✅ Deterministic results across multiple runs

---

## Conclusion

**Implementation Summary**:

The parallel implementation uses temporary `BoardList` objects, one per
recursive branch. Each spawned task works exclusively on its own list,
eliminating shared state. After `cilk_sync`, all temporary lists are merged
sequentially into the parent list using O(1) merge operations.

**Performance Comparison**:

The parallel code is **slower** than the serial code (0.60x-0.50x speedup)
due to:
- Spawn/sync overhead dominating for this small problem
- Memory allocation overhead for temp list arrays
- Sequential merge operations after each sync
- Cache effects and load imbalance

**Key Insight**:

For very fast problems (milliseconds), parallel overhead can exceed any
potential speedup. The thread-safe implementation is correct but not
beneficial for N=8. For larger N, the overhead would be amortized and
parallelization would likely show speedup.

The implementation successfully eliminates race conditions while maintaining
correctness, demonstrating that thread-safe parallelization is possible even
when it doesn't provide performance benefits for small problem sizes.

