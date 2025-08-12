# Memory Scratch Space Reduction

To optimize memory usage in the merge operation, we modify the
`merge_c` function to use only one temporary scratch space instead of
two. The key change involves copying only the right half of the array
into a temporary buffer, leaving the left half in-place. The merge
then proceeds backwards from the end of the arrays to avoid
overwriting unprocessed data.

```c
static void merge_m(data_t* A, int p, int q, int r) {
  assert(A && p <= q && (q + 1) <= r);
  const int n1 = q - p + 1;
  const int n2 = r - q;

  data_t *temp = NULL;
  inline_mem_alloc(&temp, n2);  // Allocate temp for right part only

  // Copy right part (A[q+1..r]) to temp
  copy_m(A + q + 1, temp, n2);

  int i = q;      // Index in left part (A), starts at end
  int j = n2 - 1; // Index in temp (right part), starts at end
  int k = r;      // Current position to write in A, starts at end

  // Merge left (in A) and right (in temp) backwards
  while (i >= p && j >= 0) {
    if (A[i] > temp[j]) {
      A[k] = A[i];
      i--;
    } else {
      A[k] = temp[j];
      j--;
    }
    k--;
  }

  // Copy remaining right elements (if any)
  while (j >= 0) {
    A[k] = temp[j];
    j--;
    k--;
  }

  inline_mem_free(&temp);
}

static void copy_m(data_t* source, data_t* dest, int n) {
  assert(source && dest);
  for (int i = 0; i < n; i++) {
    *(dest + i) = *(source + i);
  }
}
```

**Key Changes and Explanation:**

1. **Single Temporary Buffer:**
   - Only the right half of the array (`A[q+1..r]`) is copied into
     `temp` (size `n2`), reducing memory usage by half compared to the
     original which allocated two buffers (total size `n1 + n2`).

2. **Backward Merge:**
   - The merge starts from the end of the left half (in `A`) and the
     end of the right half (in `temp`), writing the merged result
     backwards from the end of the output range (`A[r]` down to
     `A[p]`). This ensures:
     - The left half in `A` is read before being overwritten (since
       writes start beyond `q` until moving leftward).
     - The right half in `temp` is safe as it's a separate buffer.

3. **Remaining Elements:**
   - If the left half is exhausted first, remaining right elements in
     `temp` are copied to the start of the output range.
   - If the right half is exhausted first, the remaining left elements
     are already in place in `A` and require no action.

**Performance Implications:**

- **Memory Reduction:** Temporary memory usage is halved (from `O(n)`
    to `O(n/2)`), which can improve performance in memory-constrained
    systems or for very large arrays.
- **Cache Behavior:** 
  - **Positive:** The left half remains in `A`, potentially benefiting
      from cache locality if the left portion is accessed
      sequentially.
  - **Negative:** Backward traversal of the left half may cause cache
      misses due to reverse memory access patterns.
- **Overall:** The operation count remains `O(n)`, but real-world
    performance depends on the interaction with the memory
    hierarchy. Benchmarks may show variability based on array size and
    cache architecture.

**Compiler Optimization Potential:**
- **Unlikely:** Compilers cannot automatically transform the algorithm
    from two buffers to one. This requires:
  - Fundamental algorithmic changes (backward merge).
  - Insight into data dependencies to ensure correctness when
    overwriting the array.
  - Manual optimization, as compilers focus on low-level code
    transformations (e.g., loop unrolling) rather than high-level
    algorithmic restructuring.

---



## Demo

Below is a step-by-step demonstration of the optimized `merge_m`
function for merging two subarrays of width=2 into a width=4
array. We'll use the input array `A = [4, 3, 2, 1]` after initial
sorting of width=2 blocks.



### Initial State

```
Array A: [3, 4, 1, 2]  // After isort on width=2 blocks
Merge parameters: p=0, q=1, r=3
Left subarray (in-place): [3, 4]
Right subarray (to be copied): [1, 2]
```



### Step-by-Step Execution of `merge_m`:

| Step | Operation                  | i (left index) | j (temp index) | k (write index) | Temp Array | Array A State      | Description                                                                 |
|------|----------------------------|----------------|----------------|-----------------|------------|--------------------|-----------------------------------------------------------------------------|
| 0    | Copy right to temp         | -              | -              | -               | [1, 2]     | [3, 4, 1, 2]       | Initialize temp with right subarray                                         |
| 1    | Initialize indices         | 1 (A[1]=4)     | 1 (temp[1]=2)  | 3               | [1, 2]     | [3, 4, 1, 2]       | Start at ends of left/temp, write from end of merged array                  |
| 2    | Compare A[1]=4 > temp[1]=2 | 1              | 1              | 3               | [1, 2]     | [3, 4, 1, 4]       | Write 4 to A[3], decrement i to 0, k to 2                                   |
| 3    | Compare A[0]=3 > temp[1]=2 | 0              | 1              | 2               | [1, 2]     | [3, 4, 3, 4]       | Write 3 to A[2], decrement i to -1, k to 1                                  |
| 4    | Copy remaining temp        | -1 (done)      | 1              | 1               | [1, 2]     | [3, 2, 3, 4]       | Write temp[1]=2 to A[1], decrement j to 0, k to 0                           |
| 5    | Copy remaining temp        | -1 (done)      | 0              | 0               | [1, 2]     | [1, 2, 3, 4]       | Write temp[0]=1 to A[0], decrement j to -1, k to -1                         |
| 6    | Merge complete             | -              | -              | -               | (freed)    | [1, 2, 3, 4]       | Final sorted array                                                          |



### Key Observations:

1. **Memory Optimization**:  
   Only 1 temporary buffer (size=2) was used instead of 2 buffers
   (size=4 total), reducing memory by 50%.

2. **Backward Merge Process**:  
   - Compared elements from the **end** of left subarray (in-place)
     and temp buffer
   - Wrote results **backward** from the end of the merged region
     (index 3 → 0)
   - Avoided overwriting unprocessed left elements by working
     right-to-left

3. **Remaining Elements Handling**:  
   When left subarray is exhausted first (step 4-5), remaining temp
   elements are copied directly to the start of the merged region.

4. **Correctness**:  
   The final array `[1, 2, 3, 4]` verifies the merge works correctly
   despite in-place operations.

### Performance Implications:
- **👍 Memory Reduction**: Halved temporary memory usage benefits
    large sorts
- **👎 Cache Behavior**: Backward traversal may cause cache misses
    vs. forward merge
- **⚠️ Tradeoff**: Memory savings vs. potential cache penalties
    (profile to confirm)

### Why Compilers Can't Automate This:
1. **Algorithmic Change**: Requires fundamental rewrite of merge logic
2. **Data Dependency**: Compilers can't prove safety of in-place
overwrites
3. **High-Level Optimization**: Beyond scope of compiler
transformations (requires human insight)
4. **Correctness Proof**: Needs manual verification of backward merge
semantics

This optimization demonstrates how algorithmic changes can reduce
memory overhead, but requires careful implementation to maintain
correctness. The tradeoffs between memory usage and cache behavior
should be evaluated with performance profiling on target hardware.