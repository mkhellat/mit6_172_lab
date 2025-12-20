# Efficient Linked List Concatenation

> "The most efficient algorithm is often the simplest one that achieves
> the goal." - Performance Engineering Principle

---

## Overview

This write-up analyzes the most efficient way to concatenate two singly
linked lists and provides the asymptotic running time analysis of the
`merge_lists` function implemented for the N-Queens problem.

---

## Implementation

### BoardList Structure

```c
typedef struct BoardNode {
    Board board;
    struct BoardNode* next;
} BoardNode;

typedef struct BoardList {
    BoardNode* head;  // First node in the list
    BoardNode* tail;  // Last node in the list
    int size;         // Number of nodes in the list
} BoardList;
```

The `BoardList` structure maintains:
- **`head`**: Pointer to the first node
- **`tail`**: Pointer to the last node
- **`size`**: Count of nodes in the list

### merge_lists Function

```c
void merge_lists(BoardList* list1, BoardList* list2) {
    // Handle case where list2 is empty
    if (list2->head == NULL) {
        return;
    }
    
    // Handle case where list1 is empty
    if (list1->head == NULL) {
        // Move all nodes from list2 to list1
        list1->head = list2->head;
        list1->tail = list2->tail;
        list1->size = list2->size;
    } else {
        // Both lists are non-empty: concatenate list2 to the end of list1
        list1->tail->next = list2->head;
        list1->tail = list2->tail;
        list1->size += list2->size;
    }
    
    // Reset list2 to be empty
    list2->head = NULL;
    list2->tail = NULL;
    list2->size = 0;
}
```

---

## Most Efficient Concatenation Method

### The Optimal Approach

The **most efficient way to concatenate two singly linked lists** is to:

1. **Use a tail pointer**: Maintain a pointer to the last node of the first
   list
2. **Direct pointer manipulation**: Link the tail of list1 directly to the
   head of list2
3. **Update metadata**: Update the tail pointer and size in constant time

This is exactly what our `merge_lists` function does.

### Why This is Optimal

**Time Complexity**: **O(1)** - Constant time

The operation requires only:
- **1 pointer assignment**: `list1->tail->next = list2->head`
- **2 pointer updates**: Update `list1->tail` and reset `list2->tail`
- **1 arithmetic operation**: `list1->size += list2->size`

All of these are **constant-time operations**, regardless of the size of
either list.

### Alternative Approaches (Less Efficient)

#### 1. Traversing to Find the Tail

```c
// Inefficient: O(n) where n is length of list1
BoardNode* current = list1->head;
while (current->next != NULL) {
    current = current->next;  // Traverse entire list1
}
current->next = list2->head;
```

**Time Complexity**: **O(n)** where n is the length of list1
- Must traverse the entire first list to find the tail
- Inefficient for long lists

#### 2. Copying Nodes

```c
// Very inefficient: O(m) where m is length of list2
BoardNode* current = list2->head;
while (current != NULL) {
    append_board(list1, current->board);  // Allocate new nodes
    current = current->next;
}
```

**Time Complexity**: **O(m)** where m is the length of list2
- Allocates new memory for each node
- Copies data unnecessarily
- Much slower and uses more memory

---

## Asymptotic Running Time Analysis

### Our Implementation: O(1)

**Best Case**: O(1)
- When list2 is empty: immediate return
- When list1 is empty: 3 pointer assignments
- When both are non-empty: 1 pointer assignment + 2 updates

**Average Case**: O(1)
- Always constant time, regardless of list sizes

**Worst Case**: O(1)
- Always constant time, regardless of list sizes

### Space Complexity: O(1)

No additional memory is allocated. The function only manipulates existing
pointers and updates metadata.

### Why O(1) is Achievable

The key insight is that we **maintain a tail pointer** in the `BoardList`
structure. Without this, concatenation would require:

1. **Traversing list1** to find its last node: O(n)
2. **Linking to list2**: O(1)
3. **Total**: O(n)

With the tail pointer:
1. **Direct access to tail**: O(1)
2. **Linking to list2**: O(1)
3. **Total**: O(1)

---

## Comparison with Other Data Structures

### Array Concatenation

```c
// Concatenate two arrays
int* new_array = malloc((size1 + size2) * sizeof(int));
memcpy(new_array, array1, size1 * sizeof(int));
memcpy(new_array + size1, array2, size2 * sizeof(int));
```

**Time Complexity**: O(n + m) - must copy all elements
**Space Complexity**: O(n + m) - must allocate new memory

### Vector/Dynamic Array Concatenation

Similar to arrays: O(n + m) time and space

### Doubly Linked List Concatenation

With tail pointers: **O(1)** - same as our implementation

---

## Practical Considerations

### Memory Management

Our implementation **transfers ownership** of nodes from list2 to list1:
- Nodes are not copied or freed
- list2 becomes empty but nodes remain allocated
- This is efficient but requires careful memory management

### Thread Safety

For parallel execution:
- Each thread can maintain its own `BoardList`
- After parallel work completes, lists can be merged in O(1) per merge
- Multiple merges can be done sequentially or in parallel

### Edge Cases Handled

1. **list2 is empty**: Immediate return, no work needed
2. **list1 is empty**: Transfer all nodes from list2 to list1
3. **Both non-empty**: Standard concatenation
4. **Both empty**: list2 reset (no-op)

---

## Verification

The implementation correctly handles all cases:

```c
// Test case 1: list1 has 3 nodes, list2 has 5 nodes
BoardList list1, list2;
init_list(&list1);
init_list(&list2);
// ... add 3 boards to list1, 5 boards to list2 ...
merge_lists(&list1, &list2);
// Result: list1 has 8 nodes, list2 is empty ✓

// Test case 2: list2 is empty
merge_lists(&list1, &list2);
// Result: list1 unchanged, list2 remains empty ✓

// Test case 3: list1 is empty
init_list(&list1);
merge_lists(&list1, &list2);
// Result: list1 gets all nodes from list2, list2 is empty ✓
```

---

## Conclusion

**What is the most efficient way to concatenate two singly linked lists?**

**Answer**: Use a **tail pointer** to maintain direct access to the last
node, then perform a constant-time pointer manipulation to link the lists
together.

**Asymptotic running time of merge_lists function**:

**O(1)** - Constant time

The function achieves optimal O(1) time complexity by:
1. Maintaining a tail pointer in the `BoardList` structure
2. Performing direct pointer manipulation (no traversal needed)
3. Updating metadata (tail, size) in constant time

This is the **theoretically optimal** approach for concatenating singly
linked lists, as it requires no traversal and no data copying.

