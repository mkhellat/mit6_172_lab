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

#define THRESHOLD 64

// Function prototypes
static void merge_f(data_t* A, int p, int q, int r, data_t* temp);



void sort_f(data_t* A, int p, int r) {
  assert(A);
  const int n = r - p + 1;
  
  // Compute maximum buffer size needed
  int max_buf_size = 0;
  for (int width = THRESHOLD; width < n; width *= 2) {
    max_buf_size = width;  // Max right half size in any merge
  }

  data_t* global_temp = NULL;
  if (max_buf_size > 0) {
    global_temp = (data_t*)malloc(max_buf_size * sizeof(data_t));
    if (!global_temp) {
      fprintf(stderr, "Memory allocation failed\n");
      exit(EXIT_FAILURE);
    }
  }

  // Sort THRESHOLD-sized blocks
  for (int i = p; i <= r; i += THRESHOLD) {
    int block_end = i + THRESHOLD - 1;
    if (block_end > r) block_end = r;
    isort(A + i, A + block_end + 1);
  }

  // Bottom-up merge using pre-allocated buffer
  for (int width = THRESHOLD; width < n; width *= 2) {
    for (int i = p; i <= r - width; i += 2 * width) {
      int left_end = i + width - 1;
      int right_end = i + 2 * width - 1;
      if (right_end > r) right_end = r;
      merge_f(A, i, left_end, right_end, global_temp);
    }
  }
  
  free(global_temp);
}



static void merge_f(data_t* A, int p, int q, int r, data_t* temp) {
  assert(A && p <= q && (q + 1) <= r);
  const int n2 = r - q;  // Right part size

  // Copy only right half to temporary buffer
  for (int i = 0; i < n2; i++) {
    *(temp + i) = *(A + q + 1 + i);
  }

  int i = q;        // Left part index (starts at end)
  int j = n2 - 1;   // Temp buffer index (starts at end)
  int k = r;        // Write index (starts at end)

  // Merge backwards
  while (i >= p && j >= 0) {
    *(A + k--) = (*(A + i) > *(temp + j)) ? *(A + i--) : *(temp + j--);
  }
  
  // Copy remaining right elements
  while (j >= 0) {
    *(A + k--) = *(temp + j--);
  }
}

