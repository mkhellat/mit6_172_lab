// Test case to force vectorization by breaking memcpy pattern
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#define SIZE (1L << 16)

void test_complex_pattern(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Break the memcpy pattern with a more complex operation
  for (i = 0; i < SIZE; i++) {
    a[i] = b[i + 1] ^ 0;  // XOR with 0 (identity) but breaks memcpy pattern
  }
}

void test_with_condition(uint8_t * restrict a, uint8_t * restrict b) {
  uint64_t i;

  // Add a condition that prevents memcpy optimization
  for (i = 0; i < SIZE; i++) {
    if (i < SIZE) {  // Always true, but breaks memcpy pattern
      a[i] = b[i + 1];
    }
  }
}
