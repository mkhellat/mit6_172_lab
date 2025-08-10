# Pointers vs. Arrays in C

## **1. Performance Advantages of Pointers & Pointer Arithmetic**

**Direct Memory Access & Reduced Instructions**:
- **Pointer Arithmetic**: 
  - Operations like `ptr++` directly advance to the next memory
    address (e.g., `ptr + sizeof(int)`).
  - **Compiled to 2-3 machine instructions**: `Load`, `Modify`,
      `Store`.
  - Example:
    ```c
    int *ptr = array;
    for (int i = 0; i < n; i++) {
        sum += *ptr++;  // Single increment operation
    }
    ```
    **Assembly**: `add sum, [ptr]` + `add ptr, 4`.

- **Array Indexing**:
  - Each access calculates `base_address + index * sizeof(type)`.
  - **Extra instructions**: Multiply index by element size, then add
      to base.
  - Example:
    ```c
    for (int i = 0; i < n; i++) {
        sum += array[i];  // Address calculation per access
    }
    ```
    **Assembly**: `mov temp, i*4` + `add temp, array` + `add sum,
      [temp]`.

**Compiler Optimization**:
- Modern compilers (GCC, Clang) often convert array indexing to
  pointer arithmetic under `-O2`/`-O3`.
- **Edge Cases**: Optimizations fail with:
  - Non-linear indexing (e.g., `array[i*2]`).
  - Aliased pointers (e.g., two pointers referencing overlapping
    memory).

**Register Efficiency**:
- Pointers often remain in registers, reducing memory accesses.
- Indexing may require reloading the base address or recalculating
  offsets.

---



## **2. Overheads of Array Indexing**
**Address Calculation**:
- Per-element cost: `base + index * sizeof(type)`.
- **Worse with 2D+ Arrays**: Nested loops amplify overhead (e.g.,
    `matrix[i][j]` → `base + (i*cols + j)*sizeof(int)`).

**Redundant Operations**:
- Loop counters (`i`) and indices both require updates and
  comparisons.

---



## **3. Caveats of Using Pointers**
**Undefined Behavior (UB)**:
- **Out-of-Bounds Access**:
  ```c
  int arr[5];
  int *ptr = arr;
  ptr += 10;  // UB: Access beyond array.
  ```
- **Dangling Pointers**:
  ```c
  int *ptr;
  {
      int x = 10;
      ptr = &x;  // ptr becomes dangling when x goes out of scope
  }
  *ptr = 20;     // UB: Writes to invalid memory.
  ```

**Readability & Maintainability**:
- **Example (String Length)**:
  ```c
  // Array Indexing (Clear)
  int len_array(char *s) {
      int i = 0;
      while (s[i] != '\0') i++;
      return i;
  }

  // Pointer Arithmetic (Less Readable)
  int len_ptr(char *s) {
      char *p = s;
      while (*p != '\0') p++;
      return p - s;
  }
  ```

**Aliasing Issues**:
- Compiler optimizations fail if pointers alias:
  ```c
  void add(int *a, int *b, int *c) {
      *a += *c;  // Compiler can't assume c isn't pointing to a/b.
      *b += *c;
  }
  ```

**Type Safety**:
- `void*` pointers require careful casting:
  ```c
  float data[10];
  void *p = data;
  int *iptr = (int*)p;  // Misaligned/mistyped access.
  ```

---



## **4. When Arrays Are Preferable**

**Fixed-Size Data**:
- Arrays enforce size at compile-time:
  ```c
  char buffer[1024];  // Stack allocation, no heap overhead.
  ```

**Code Clarity**:
- Indexing is intuitive for multi-dimensional data:
  ```c
  int grid[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
  int val = grid[i][j];  // Clearer than *(*(grid + i) + j)
  ```

**Compiler Hints**:
- `restrict` Keyword: Tells the compiler pointers don't alias:
  ```c
  void add(int *restrict a, int *restrict b, int *restrict c) {
      *a += *c;  // Safe to optimize
      *b += *c;
  }
  ```

---



## **Key Takeaways**
| **Aspect**          | **Pointers**                                  | **Arrays**                                  |
|----------------------|-----------------------------------------------|---------------------------------------------|
| **Speed**            | ✓ Lower overhead (direct arithmetic)          | ✗ Address calculation per access           |
| **Memory Access**    | ✓ Efficient (stays in registers)              | ✗ May reload base address                  |
| **Readability**      | ✗ Risk of complex code                        | ✓ Intuitive indexing                       |
| **Safety**           | ✗ Prone to UB (bounds, aliasing, dangling)    | ✓ Bounds-checkable (static analysis)       |
| **Use Cases**        | Dynamic data, performance-critical sections   | Fixed-size data, multi-dimensional arrays  |

### **Recommendations**
1. **Use Pointers For**:
   - Iterating large buffers (e.g., image processing).
   - Dynamic structures (linked lists, trees).
   - When profiling confirms performance gains.

2. **Use Arrays For**:
   - Fixed-size lookup tables.
   - Multi-dimensional data (matrices).
   - Code where readability > micro-optimization.

3. **Always**:
   - Enable compiler optimizations (`-O3`).
   - Use `restrict` to avoid aliasing.
   - Validate bounds with pointers (e.g., `ptr < arr_end`).

---



## Observations for `sort_p`

Following are performance stats for `sort_i` and `sort_p` using `perf
stat`:

### `sort_i`

```bash
Using user-provided seed: 0

Running test #0...
Generating random array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_i		: Elapsed execution time: 0.000680 sec
Generating inverted array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_i		: Elapsed execution time: 0.001324 sec

Running test #1...
 --> test_zero_element at line 245: PASS

Running test #2...
 --> test_one_element at line 266: PASS
Done testing.

 Performance counter stats for './sort_i 10000 25000 -s 0':

         34,857.11 msec task-clock:u                     #    1.000 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               101      page-faults:u                    #    2.898 /sec                      
   243,006,740,228      instructions:u                   #    2.08  insn per cycle            
   117,082,548,016      cycles:u                         #    3.359 GHz                       
    38,531,077,027      branches:u                       #    1.105 G/sec                     
       197,168,525      branch-misses:u                  #    0.51% of all branches           

      34.860345868 seconds time elapsed

      34.769973000 seconds user
       0.001992000 seconds sys
```

### `sort_p`

```bash

Using user-provided seed: 0

Running test #0...
Generating random array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_p		: Elapsed execution time: 0.000576 sec
Generating inverted array of 10000 elements
Arrays are sorted: yes
 --> test_correctness at line 217: PASS
sort_p		: Elapsed execution time: 0.001190 sec

Running test #1...
 --> test_zero_element at line 245: PASS

Running test #2...
 --> test_one_element at line 266: PASS
Done testing.

 Performance counter stats for './sort_p 10000 25000 -s 0':

         31,203.26 msec task-clock:u                     #    1.000 CPUs utilized             
                 0      context-switches:u               #    0.000 /sec                      
                 0      cpu-migrations:u                 #    0.000 /sec                      
               101      page-faults:u                    #    3.237 /sec                      
   243,006,736,470      instructions:u                   #    2.15  insn per cycle            
   112,926,343,156      cycles:u                         #    3.619 GHz                       
    38,531,073,187      branches:u                       #    1.235 G/sec                     
        38,592,534      branch-misses:u                  #    0.10% of all branches           

      31.205493192 seconds time elapsed

      31.147973000 seconds user
       0.001001000 seconds sys
```

This indicates that there is not a considerable difference between the
number of instructions but branch mispredictions have considerably
reduced!.
