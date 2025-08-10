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
static void merge_p(data_t* A, int p, int q, int r);
static void copy_p(data_t* source, data_t* dest, int n);

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

void sort_p(data_t* A, int p, int r) {
  assert(A);
  const int n = r - p + 1;
  if (n <= 1) return;  // Already sorted

  // Bottom-up iterative merge sort
  for (int width = 1; width < n; width *= 2) {
    for (int i = p; i <= r - width; i += 2 * width) {
      int left_end = i + width - 1;
      int right_end = i + 2 * width - 1;
      if (right_end > r) {
	right_end = r;
      }
      merge_p(A, i, left_end, right_end);
    }
  }
}

// A merge routine. Merges the sub-arrays A [p..q] and A [q + 1..r].
// Uses two arrays 'left' and 'right' in the merge operation.
static void merge_p(data_t* A, int p, int q, int r) {
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

  copy_p(A + p, left, n1);
  copy_p(A + q + 1, right, n2);
  *(left  + n1) = UINT_MAX;
  *(right + n2) = UINT_MAX;

  int i = 0;
  int j = 0;

  for (int k = p; k <= r; k++) {
    if (*(left + i) <= *(right + j)) {
      *(A + k) = *(left + i);
      i++;
    } else {
      *(A + k) = *(right + j);
      j++;
    }
  }
  inline_mem_free(&left);
  inline_mem_free(&right);
}


static void copy_p(data_t* source, data_t* dest, int n) {
  assert(dest);
  assert(source);

  for (int i = 0 ; i < n ; i++) {
    *(dest + i) = *(source + i);
  }
}
