/**
 * Copyright (c) 2012 MIT License by 6.172 Staff
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/


#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include "./util.h"

// Function prototypes
static void merge_i(data_t* A, int p, int q, int r);
static void copy_i(data_t* source, data_t* dest, int n);

// Inline mem_alloc and mem_free definitions
static inline void inline_mem_alloc(data_t** space, int size) {
  *space = (data_t*)malloc(sizeof(data_t) * size);
  if (*space == NULL) {
    printf("out of memory...\n");
  }
}
static inline void inline_mem_free(data_t** space) {
  free(*space);
  *space = 0;
}



// Frame structure to simulate recursion
typedef struct {
    int p;          // Left index
    int r;          // Right index
    int q;          // Midpoint (computed once)
    char state;     // 0: need left sort, 1: left done, 2: both done
} Frame;

void sort_i(data_t* A, int p, int r) {
    assert(A);
    if (p >= r) return;  // Base case handling

    // Conservative stack depth (64 handles 2^64 elements)
    #define MAX_DEPTH 64
    Frame stack[MAX_DEPTH];
    int top = 0;

    // Initial frame (entire array)
    stack[top++] = (Frame){p, r, 0, 0};

    while (top > 0) {
        Frame* f = &stack[top - 1];  // Current frame

        switch (f->state) {
            case 0:  // Need to sort left half
                if (f->p >= f->r) {
                    top--;  // Pop trivial segments
                    break;
                }
                
                // Compute and store midpoint
                f->q = (f->p + f->r) / 2;
                
                // Push left half (preserves original order)
                if (top >= MAX_DEPTH) {
                    fprintf(stderr, "Stack overflow in sort_i\n");
                    exit(EXIT_FAILURE);
                }
                stack[top++] = (Frame){f->p, f->q, 0, 0};
                f->state = 1;  // Next: sort right half
                break;
                
            case 1:  // Left sorted, need to sort right
                // Push right half
                if (top >= MAX_DEPTH) {
                    fprintf(stderr, "Stack overflow in sort_i\n");
                    exit(EXIT_FAILURE);
                }
                stack[top++] = (Frame){f->q + 1, f->r, 0, 0};
                f->state = 2;  // Next: merge
                break;
                
            case 2:  // Both halves sorted, merge
                merge_i(A, f->p, f->q, f->r);
                top--;  // Pop completed frame
                break;
        }
    }
    #undef MAX_DEPTH
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
// Uses two arrays 'left' and 'right' in the merge operation.
static void merge_i(data_t* A, int p, int q, int r) {
  assert(A);
  assert(p <= q);
  assert((q + 1) <= r);
  int n1 = q - p + 1;
  int n2 = r - q;

  data_t* left = 0, * right = 0;
  inline_mem_alloc(&left, n1 + 1);
  inline_mem_alloc(&right, n2 + 1);
  if (left == NULL || right == NULL) {
    inline_mem_free(&left);
    inline_mem_free(&right);
    return;
  }

  copy_i(&(A[p]), left, n1);
  copy_i(&(A[q + 1]), right, n2);
  left[n1] = UINT_MAX;
  right[n2] = UINT_MAX;

  int i = 0;
  int j = 0;

  for (int k = p; k <= r; k++) {
    if (left[i] <= right[j]) {
      A[k] = left[i];
      i++;
    } else {
      A[k] = right[j];
      j++;
    }
  }
  inline_mem_free(&left);
  inline_mem_free(&right);
}


static void copy_i(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);

  for (int i = 0 ; i < n ; i++) {
    dest[i] = source[i];
  }
}

