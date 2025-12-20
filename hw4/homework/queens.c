/**
 * Determine number of ways to place N queens on a NxN chess board so
 * that no queen can attack another (i.e., no two queens in any row,
 * column, or diagonal).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cilk/cilk.h>

// Coarsening threshold: execute serially when this many queens are placed
// For N=8, threshold of 6 means: if 6+ queens placed, execute serially (2-3 rows left)
// This reduces spawn overhead for near-base-case recursion
#define COARSENING_THRESHOLD 6

// Count number of bits set in a value (popcount)
static inline int popcount(uint8_t x) {
    int count = 0;
    while (x != 0) {
        count++;
        x &= x - 1;  // Clear least significant bit
    }
    return count;
}

// Board representation: each solution is stored as a uint64_t
// For N=8, we use the lower 8 bits to represent queen positions in each row
typedef uint64_t Board;

// Node in the linked list
typedef struct BoardNode {
    Board board;
    struct BoardNode* next;
} BoardNode;

// Linked list structure with head, tail, and size
typedef struct BoardList {
    BoardNode* head;  // First node in the list
    BoardNode* tail;  // Last node in the list
    int size;         // Number of nodes in the list
} BoardList;

// Initialize an empty BoardList
void init_list(BoardList* list) {
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

// Add a board to the end of the list
void append_board(BoardList* list, Board board) {
    BoardNode* node = (BoardNode*)malloc(sizeof(BoardNode));
    if (node == NULL) {
        fprintf(stderr, "Failed to allocate memory for BoardNode\n");
        exit(1);
    }
    node->board = board;
    node->next = NULL;
    
    if (list->head == NULL) {
        // Empty list
        list->head = node;
        list->tail = node;
    } else {
        // Non-empty list: append to tail
        list->tail->next = node;
        list->tail = node;
    }
    list->size++;
}

// Merge list2 into list1, then reset list2 to be empty
void merge_lists(BoardList* list1, BoardList* list2) {
    // Handle case where list2 is empty
    if (list2->head == NULL) {
        // Nothing to merge, list2 is already empty
        return;
    }
    
    // Handle case where list1 is empty
    if (list1->head == NULL) {
        // list1 is empty, so just move all nodes from list2 to list1
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

// Free all nodes in the list
void free_list(BoardList* list) {
    BoardNode* current = list->head;
    while (current != NULL) {
        BoardNode* next = current->next;
        free(current);
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->size = 0;
}

// Serial version (original implementation without parallelization)
void try_serial(int row, int left, int right, BoardList* board_list) {
    int poss, place;
    // If row bitvector is all 1s, a valid ordering of the queens exists.
    if (row == 0xFF) {
        // Found a solution: convert row bitvector to Board representation
        append_board(board_list, (Board)row);
    } else {
        poss = ~(row | left | right) & 0xFF;
        while (poss != 0) {
            place = poss & -poss;
            // Create a temporary list for this recursive branch
            BoardList temp_list;
            init_list(&temp_list);
            
            // Serial recursive call
            try_serial(row | place, (left | place) << 1, (right | place) >> 1, &temp_list);
            
            // Merge the temporary list into the main list
            merge_lists(board_list, &temp_list);
            
            poss &= ~place;
        }
    }
}

// Parallel version with coarsening
void try(int row, int left, int right, BoardList* board_list) {
    int poss, place;
    // If row bitvector is all 1s, a valid ordering of the queens exists.
    if (row == 0xFF) {
        // Found a solution: convert row bitvector to Board representation
        append_board(board_list, (Board)row);
    } else {
        // Coarsening: if we're close to the base case, use serial execution
        int queens_placed = popcount((uint8_t)row);
        if (queens_placed >= COARSENING_THRESHOLD) {
            // Use serial version for near-base-case recursion
            try_serial(row, left, right, board_list);
            return;
        }
        
        // Parallel execution for earlier recursion levels
        poss = ~(row | left | right) & 0xFF;
        
        // Count number of valid positions to allocate array
        int count = 0;
        int temp_poss = poss;
        while (temp_poss != 0) {
            temp_poss &= temp_poss - 1;  // Clear least significant bit
            count++;
        }
        
        // Allocate array of temporary lists
        BoardList* temp_lists = (BoardList*)malloc(count * sizeof(BoardList));
        if (temp_lists == NULL) {
            fprintf(stderr, "Failed to allocate memory for temp_lists\n");
            exit(1);
        }
        
        // Initialize all temp lists
        for (int i = 0; i < count; i++) {
            init_list(&temp_lists[i]);
        }
        
        // Spawn all recursive calls with their own temp lists
        int idx = 0;
        while (poss != 0) {
            place = poss & -poss;
            
            // Parallel recursive call - each branch gets its own list
            cilk_spawn try(row | place, (left | place) << 1, (right | place) >> 1, &temp_lists[idx]);
            
            poss &= ~place;
            idx++;
        }
        cilk_sync;
        
        // After all spawned tasks complete, merge all temp lists into board_list
        for (int i = 0; i < count; i++) {
            merge_lists(board_list, &temp_lists[i]);
        }
        
        // Free the array (lists themselves are already merged, so nodes are in board_list)
        free(temp_lists);
    }
}

int main() {
    BoardList board_list;
    init_list(&board_list);
    
    try(0, 0, 0, &board_list);
    
    printf("There are %d solutions.\n", board_list.size);
    
    free_list(&board_list);
    return 0;
}
