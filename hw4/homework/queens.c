/**
 * Determine number of ways to place N queens on a NxN chess board so
 * that no queen can attack another (i.e., no two queens in any row,
 * column, or diagonal).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

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

void try(int row, int left, int right, BoardList* board_list) {
    int poss, place;
    // If row bitvector is all 1s, a valid ordering of the queens exists.
    if (row == 0xFF) {
        // Found a solution: convert row bitvector to Board representation
        // For now, we'll use row as the board representation
        append_board(board_list, (Board)row);
    } else {
        poss = ~(row | left | right) & 0xFF;
        while (poss != 0) {
            place = poss & -poss;
            // Create a temporary list for this recursive branch
            BoardList temp_list;
            init_list(&temp_list);
            
            // Recursive call (serial for now - will re-parallelize later)
            try(row | place, (left | place) << 1, (right | place) >> 1, &temp_list);
            
            // Merge the temporary list into the main list
            merge_lists(board_list, &temp_list);
            
            poss &= ~place;
        }
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
