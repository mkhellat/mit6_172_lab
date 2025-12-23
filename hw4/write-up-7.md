# Cilk Reducers: BoardListReducer

> "Reducers eliminate the need for temporary data structures by automatically
> managing per-strand views and merging them after synchronization." - Cilk
> Programming Principle

---

## Overview

This write-up explains how a `BoardListReducer` can eliminate the need for
temporary lists in Section 3.2's parallel implementation, and defines the
monoid structure required for the reducer.

---

## How BoardListReducer Eliminates Temporary Lists

### Current Implementation (Section 3.2)

In the current parallel implementation, temporary lists are created and managed
manually:

```c
void try(int row, int left, int right, BoardList* board_list) {
    // ...
    
    // Count number of valid positions to allocate array
    int count = 0;
    int temp_poss = poss;
    while (temp_poss != 0) {
        temp_poss &= temp_poss - 1;
        count++;
    }
    
    // Allocate array of temporary lists
    BoardList* temp_lists = (BoardList*)malloc(count * sizeof(BoardList));
    for (int i = 0; i < count; i++) {
        init_list(&temp_lists[i]);
    }
    
    // Spawn all recursive calls with their own temp lists
    int idx = 0;
    while (poss != 0) {
        place = poss & -poss;
        cilk_spawn try(row | place, (left | place) << 1, 
                      (right | place) >> 1, &temp_lists[idx]);
        poss &= ~place;
        idx++;
    }
    cilk_sync;
    
    // After sync: merge all temp lists into board_list
    for (int i = 0; i < count; i++) {
        merge_lists(board_list, &temp_lists[i]);
    }
    
    free(temp_lists);
}
```

**Problems with this approach:**

1. **Memory allocation overhead**: Must allocate an array of `BoardList`
   structures at each recursive level
2. **Manual initialization**: Must explicitly initialize each temporary list
3. **Manual merging**: Must sequentially merge all temporary lists after
   `cilk_sync`
4. **Memory management**: Must explicitly free the temporary list array
5. **Code complexity**: Additional code for counting positions, allocating,
   initializing, and merging

### With BoardListReducer

A `BoardListReducer` would eliminate all of these issues:

```c
// Register reducer (once, at the start)
CILK_C_REDUCER_BOARDLIST(r, BoardList, identity);

void try(int row, int left, int right) {
    // ...
    
    // No need to count positions or allocate arrays!
    while (poss != 0) {
        place = poss & -poss;
        // Each strand automatically gets its own view
        cilk_spawn try(row | place, (left | place) << 1, 
                      (right | place) >> 1);
        poss &= ~place;
    }
    cilk_sync;
    // Views are automatically merged after sync - no manual merging needed!
    
    // Access the merged result
    BoardList* result = REDUCER_VIEW(r);
}
```

**Benefits:**

1. **No memory allocation**: The reducer automatically manages per-strand
   views internally
2. **Automatic initialization**: Each view is automatically initialized to the
   identity element
3. **Automatic merging**: Views are automatically merged after `cilk_sync`
   using the monoid's binary operator
4. **No manual cleanup**: The reducer handles all memory management
5. **Simpler code**: No need for counting, allocation, initialization, or
   merging logic
6. **Better performance**: Reducer implementation is optimized (hash-table
   lookup per access, efficient merging)

### Key Insight

The reducer eliminates temporary lists by:

- **Per-strand views**: Each Cilk strand gets its own view of the reducer,
  allowing safe concurrent access without locks
- **Automatic merging**: After `cilk_sync`, the Cilk runtime automatically
  merges all views using the monoid's binary operator
- **Serial semantics**: The final result is guaranteed to be the same as a
  serial execution, regardless of scheduling or number of processors

This means we can write code as if we're using a single shared `BoardList`,
but the reducer ensures thread-safety and correct merging behind the scenes.

---

## Monoid Definition

To implement a `BoardListReducer`, we must define a monoid structure on
`BoardList` objects.

### Type of Objects

**Type T**: `BoardList`

The reducer operates on `BoardList` structures, which represent linked lists
of `Board` values (solutions to the N-Queens problem).

```c
typedef struct BoardList {
    BoardNode* head;  // First node in the list
    BoardNode* tail;  // Last node in the list
    int size;         // Number of nodes in the list
} BoardList;
```

### Associative Binary Operator

**Operator ⊕**: `merge_lists`

The binary operator is the `merge_lists` function, which concatenates two
`BoardList` structures in O(1) time by linking the tail of the first list to
the head of the second list.

```c
void merge_lists(BoardList* list1, BoardList* list2) {
    if (list2->head == NULL) {
        return;  // list2 is empty, nothing to merge
    }
    
    if (list1->head == NULL) {
        // list1 is empty, move all nodes from list2
        list1->head = list2->head;
        list1->tail = list2->tail;
        list1->size = list2->size;
    } else {
        // Both non-empty: concatenate list2 to end of list1
        list1->tail->next = list2->head;
        list1->tail = list2->tail;
        list1->size += list2->size;
    }
    
    // Reset list2 to empty
    list2->head = NULL;
    list2->tail = NULL;
    list2->size = 0;
}
```

**Associativity**: The `merge_lists` operation is associative because
concatenating lists is associative. For any three `BoardList` objects A, B,
and C:

```
(A ⊕ B) ⊕ C = A ⊕ (B ⊕ C)
```

Both sides result in a single list containing all elements from A, followed by
all elements from B, followed by all elements from C, in that order.

**Note**: Since the list of boards generated by `queens()` is not ordered (as
stated in the problem), the order of concatenation doesn't matter for
correctness. The associativity property ensures that parallel execution
produces the same set of solutions as serial execution, regardless of merge
order.

### Identity Element

**Identity e**: An empty `BoardList`

The identity element is an empty `BoardList` structure where:
- `head = NULL`
- `tail = NULL`
- `size = 0`

**Identity property**: For any `BoardList` X:

```
X ⊕ e = e ⊕ X = X
```

Merging any list with an empty list (in either order) produces the original
list unchanged. This is exactly what `merge_lists` does when one of the lists
is empty.

```c
void init_list(BoardList* list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}
```

The `init_list` function creates the identity element.

---

## Monoid Summary

**Monoid**: (T, ⊕, e)

- **T**: `BoardList` - The set of all possible linked lists of boards
- **⊕**: `merge_lists` - The associative binary operator that concatenates
  two lists
- **e**: Empty `BoardList` (`head=NULL`, `tail=NULL`, `size=0`) - The
  identity element

**Properties**:

1. **Closure**: For any two `BoardList` objects A and B, `A ⊕ B` is also a
   `BoardList`
2. **Associativity**: `(A ⊕ B) ⊕ C = A ⊕ (B ⊕ C)` for any `BoardList` A, B, C
3. **Identity**: `A ⊕ e = e ⊕ A = A` for any `BoardList` A

These properties guarantee that a parallel execution using the reducer will
produce the same result as a serial execution, regardless of how the work is
scheduled or how many processors are used.

---

## Example: How the Reducer Works

Consider a parallel execution with three spawned tasks:

```
Initial: reducer = e (empty list)

Task 1 finds solutions: [board1, board2]
Task 2 finds solutions: [board3]
Task 3 finds solutions: [board4, board5, board6]

After cilk_sync, the reducer automatically merges:
  reducer = e ⊕ [board1, board2] ⊕ [board3] ⊕ [board4, board5, board6]
         = [board1, board2, board3, board4, board5, board6]
```

The Cilk runtime ensures that:
- Each task gets its own view (no races)
- Views are merged after sync (using the associative operator)
- The final result is equivalent to serial execution

---

## Conclusion

A `BoardListReducer` eliminates the need for temporary lists in Section 3.2
by:

1. **Automatic view management**: Each strand gets its own view automatically
2. **Automatic merging**: Views are merged after `cilk_sync` using the monoid
   operator
3. **No manual allocation**: No need to allocate arrays of temporary lists
4. **Simpler code**: Eliminates counting, allocation, initialization, and
   merging logic
5. **Better performance**: Optimized reducer implementation with minimal
   overhead

The monoid structure is:

- **Type**: `BoardList`
- **Binary operator**: `merge_lists` (concatenation)
- **Identity**: Empty `BoardList` (`head=NULL`, `tail=NULL`, `size=0`)

This monoid satisfies all required properties (closure, associativity,
identity), ensuring that parallel execution with the reducer produces the
same results as serial execution while eliminating the need for temporary
lists and manual synchronization.

