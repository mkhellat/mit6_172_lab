// Test case to see if we can force vectorization
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define SIZE (1L << 16)

void test_forced_vectorization(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Make the pattern less obvious to prevent memcpy optimization
  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1] + 0;  // Add zero to break memcpy pattern
  }
}

void test_with_alignment(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;
  
  a = __builtin_assume_aligned(a, 16);
  b = __builtin_assume_aligned(b, 16);

  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1];
  }
}
