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
#include "./isort.h"

// Coarsening threshold (experimentally optimized)
#define THRESHOLD 64

// Function prototypes
static void merge_c(data_t* A, int p, int q, int r);
static void copy_c(data_t* source, data_t* dest, int n);

// Inline memory management
static inline void inline_mem_alloc(data_t** space, int size) {
  *space = (data_t*)malloc(sizeof(data_t) * size);
  if (*space == NULL) {
    printf("out of memory...\n");
    exit(EXIT_FAILURE);
  }
}
static inline void inline_mem_free(data_t** space) {
  free(*space);
  *space = NULL;
}



// Coarsened sort by a combination of isort for small blocks and
// bottom-up merge sort for larger blocks.
void sort_c(data_t* A, int p, int r) {
  assert(A);
  const int n = r - p + 1;
  
  // Step 1: Sort ALL blocks of size <= THRESHOLD
  for (int i = p; i <= r; i += THRESHOLD) {
    int block_end = i + THRESHOLD - 1;
    if (block_end > r) block_end = r;  // Handle last partial block
    isort(A + i, A + block_end + 1);   // Sort block [i, block_end]
  }

  // Step 2: Bottom-up merge for larger blocks
  for (int width = THRESHOLD; width < n; width *= 2) {
    for (int i = p; i <= r - width; i += 2 * width) {
      int left_end = i + width - 1;
      int right_end = i + 2 * width - 1;
      if (right_end > r) right_end = r;
      merge_c(A, i, left_end, right_end);
    }
  }
}


 static void merge_c(data_t* A, int p, int q, int r) {
  assert(A && p <= q && (q + 1) <= r);
  const int n1 = q - p + 1;
  const int n2 = r - q;

  data_t *left = NULL, *right = NULL;
  inline_mem_alloc(&left, n1 + 1);
  inline_mem_alloc(&right, n2 + 1);

  copy_c(A + p, left, n1);
  copy_c(A + q + 1, right, n2);
  *(left  + n1) = UINT_MAX;
  *(right + n2) = UINT_MAX;

  int i = 0, j = 0;
  for (int k = p; k <= r; k++) {
    *(A + k) = (*(left + i) <= *(right + j)) ? *(left + i++) : *(right + j++);
  }

  inline_mem_free(&left);
  inline_mem_free(&right);
}

static void copy_c(data_t* source, data_t* dest, int n) {
  assert(source && dest);
  for (int i = 0; i < n; i++) {
    *(dest + i) = *(source + i);
  }
}
