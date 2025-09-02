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

// Implements the ADT specified in bitarray.h as a packed array of bits; a bit
// array containing bit_sz bits will consume roughly bit_sz/8 bytes of
// memory.


#include "./bitarray.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <sys/types.h>


// ******************************** Macros **********************************

//  Generate bit reversal table using recursive macros; Copied from
//  Bit Twiddling Hacks.
static const uint8_t bit_reverse_table256[256] = {
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4), R4(n + 1*4), R4(n + 3*4)
    R6(0), R6(2), R6(1), R6(3)
#undef R2
#undef R4
#undef R6
};


// ********************************* Types **********************************

// Concrete data type representing an array of bits.
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* buf;
};


// ******************** Prototypes for static functions *********************

// Rotates a subarray left by an arbitrary number of bits.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
// bit_left_amount is the number of places to rotate the
//                    subarray left
//
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static void bitarray_rotate_left(bitarray_t* const bitarray,
                                 const size_t bit_offset,
                                 const size_t bit_length,
                                 const size_t bit_left_amount);

// Reverse a bitarray in-place.
//
// Rev(A) = {A[n] A[n-1] ... A[0]}
//
// Swap bits starting from LSB and MSB moving inwards towards the
// center of the array.
static inline void bitarray_reverse_naive(bitarray_t* const bitarray,
					  size_t start_idx,
					  size_t end);

// Reverse a subarray within a bitarray using LUT optimization
//
// start_idx Starting bit index of subarray to reverse
// end Ending bit index of subarray to reverse (inclusive)
// 
// This function reverses a contiguous subarray of bits within a
// bitarray using a combination of bit-level and byte-level operations
// for optimal performance. The algorithm:
// 1. Handles partial bytes at subarray boundaries with bit-level
// operations
// 2. Uses LUT-based byte reversal for complete bytes in the middle
// section
// 3. Maintains the original bitarray structure outside the subarray
// 
// Performance: O(n) time complexity with optimized byte operations
static inline void bitarray_reverse_lut(bitarray_t* const bitarray,
					const size_t start_idx,
					const size_t end_idx);

// Rotates a subarray left by one bit.
//
// bit_offset is the index of the start of the subarray
// bit_length is the length of the subarray, in bits
//g
// The subarray spans the half-open interval
// [bit_offset, bit_offset + bit_length)
// That is, the start is inclusive, but the end is exclusive.
static void bitarray_rotate_left_one(bitarray_t* const bitarray,
                                     const size_t bit_offset,
                                     const size_t bit_length);

// Portable modulo operation that supports negative dividends.
//
// Many programming languages define modulo in a manner incompatible with its
// widely-accepted mathematical definition.
// http://stackoverflow.com/questions/1907565/c-python-different-behaviour-of-the-modulo-operation
// provides details; in particular, C's modulo
// operator (which the standard calls a "remainder" operator) yields a result
// signed identically to the dividend e.g., -1 % 10 yields -1.
// This is obviously unacceptable for a function which returns size_t, so we
// define our own.
//
// n is the dividend and m is the divisor
//
// Returns a positive integer r = n (mod m), in the range
// 0 <= r < m.
static size_t modulo(const ssize_t n, const size_t m);

// Produces a mask which, when ANDed with a byte, retains only the
// bit_index th byte.
//
// Example: bitmask(5) produces the byte 0b00100000.
//
// (Note that here the index is counted from right
// to left, which is different from how we represent bitarrays in the
// tests.  This function is only used by bitarray_get and bitarray_set,
// however, so as long as you always use bitarray_get and bitarray_set
// to access bits in your bitarray, this reverse representation should
// not matter.
static char bitmask(const size_t bit_index);


// ******************************* Functions ********************************

/* --------------------------------------------------------------------------
 * bitarray_new
 */
bitarray_t*
bitarray_new(const size_t bit_sz)
{
  // Allocate an underlying buffer of ceil(bit_sz/8) bytes.
  char* const buf = calloc(1, (bit_sz+7) / 8);
  if (buf == NULL) {
    return NULL;
  }

  // Allocate space for the struct.
  bitarray_t* const bitarray = malloc(sizeof(struct bitarray));
  if (bitarray == NULL) {
    free(buf);
    return NULL;
  }

  bitarray->buf = buf;
  bitarray->bit_sz = bit_sz;
  return bitarray;
}
/* --------------------------------------------------------------------------
 * bitarray_free
 */
void
bitarray_free(bitarray_t* const bitarray)
{
  if (bitarray == NULL) {
    return;
  }
  free(bitarray->buf);
  bitarray->buf = NULL;
  free(bitarray);
}
/* --------------------------------------------------------------------------
 * bitarray_get_bit_sz
 */
size_t
bitarray_get_bit_sz(const bitarray_t* const bitarray)
{
  return bitarray->bit_sz;
}
/* --------------------------------------------------------------------------
 * bitarray_get
 */
bool
bitarray_get(const bitarray_t* const bitarray, const size_t bit_index)
{
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to get the nth
  // bit, we want to look at the (n mod 8)th bit of the (floor(n/8)th)
  // byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to produce either a zero byte (if the bit was 0) or a nonzero byte
  // (if it wasn't).  Finally, we convert that to a boolean.
  return (bitarray->buf[bit_index / 8] & bitmask(bit_index)) ?
         true : false;
}
/* --------------------------------------------------------------------------
 * bitarray_set
 */
void
bitarray_set(bitarray_t* const bitarray,
	     const size_t bit_index,
	     const bool value)
{
  assert(bit_index < bitarray->bit_sz);

  // We're storing bits in packed form, 8 per byte.  So to set the nth
  // bit, we want to set the (n mod 8)th bit of the (floor(n/8)th) byte.
  //
  // In C, integer division is floored explicitly, so we can just do it to
  // get the byte; we then bitwise-and the byte with an appropriate mask
  // to clear out the bit we're about to set.  We bitwise-or the result
  // with a byte that has either a 1 or a 0 in the correct place.
  bitarray->buf[bit_index / 8] =
    (bitarray->buf[bit_index / 8] & ~bitmask(bit_index)) |
    (value ? bitmask(bit_index) : 0);
}
/* --------------------------------------------------------------------------
 * bitarray_randfill
 */
void
bitarray_randfill(bitarray_t* const bitarray)
{
  int32_t *ptr = (int32_t *)bitarray->buf;
  for (int64_t i=0; i<bitarray->bit_sz/32 + 1; i++){
    ptr[i] = rand();
  }
}
/* --------------------------------------------------------------------------
 * bitarray_rotate
 */
void
bitarray_rotate(bitarray_t* const bitarray,
		const size_t bit_offset,
		const size_t bit_length,
		const ssize_t bit_right_amount)
{
  assert(bit_offset + bit_length <= bitarray->bit_sz);

  if (bit_length == 0) {
    return;
  }

  // Convert a rotate left or right to a left rotate only, and eliminate
  // multiple full rotations.
  bitarray_rotate_left(bitarray, bit_offset, bit_length,
                       modulo(-bit_right_amount, bit_length));
}
/* --------------------------------------------------------------------------
 * bitarray_rotate_left
 */
static void
bitarray_rotate_left(bitarray_t* const bitarray,
		     const size_t bit_offset,
		     const size_t bit_length,
		     const size_t bit_left_amount)
{
  // Triple Reverse Method
  size_t k = bit_left_amount % bit_length;
  if (k == 0) return;
  // BA = rev(rev(BA)) = rev(rev(A) rev(B))
  bitarray_reverse_lut(bitarray, bit_offset,
		       bit_offset + k - 1);
  bitarray_reverse_lut(bitarray, bit_offset + k,
		       bit_offset + bit_length - 1);
  bitarray_reverse_lut(bitarray, bit_offset,
		       bit_offset + bit_length - 1);
}
/* --------------------------------------------------------------------------
 * bitarray_revers_naive
 */
static inline void
bitarray_reverse_naive(bitarray_t* const bitarray,
		       size_t start_idx,
		       size_t end_idx)
{
  /* Validate parameters */
  if (bitarray == NULL || bitarray->buf == NULL || start_idx >= end_idx) {
    return;
  }
  
  bool temp;
  while (start_idx < end_idx) {
    temp = bitarray_get(bitarray, start_idx);
    bitarray_set(bitarray, start_idx, bitarray_get(bitarray, end_idx));
    bitarray_set(bitarray, end_idx, temp);
    start_idx++;
    end_idx--;
  }
}
/* --------------------------------------------------------------------------
 * bitarray_reverse_lut
 */
static inline void
bitarray_reverse_lut(bitarray_t* const bitarray,
		     const size_t start_idx,
		     const size_t end_idx)
{
  /* Validate parameters */
  if (bitarray == NULL || bitarray->buf == NULL || start_idx >= end_idx) {
    return;
  }

  /* Calculate byte indices and bit positions */
  const size_t start_chunk = start_idx ? 8 - start_idx % 8 : 0;
  const size_t   end_chunk = (end_idx + 1) % 8;
  const size_t  start_byte = start_chunk ? start_idx / 8 + 1 : start_idx / 8;
  const size_t    end_byte = end_idx / 8;
  const size_t   num_bytes = end_byte - start_byte + 1;

  /* Handle single-byte case and with bit-level reversal */
  if (num_bytes < 2 || start_chunk || end_chunk ) {
    bitarray_reverse_naive(bitarray, start_idx, end_idx);
    return;
  }
  
  /* Reverse byte order using XOR swap (no temporary variable) */
  for (size_t i = start_byte; i < start_byte + num_bytes / 2; i++) {
    size_t j = end_byte - start_byte - 1 - i;
    bitarray->buf[i] ^= bitarray->buf[j];
    bitarray->buf[j] ^= bitarray->buf[i];
    bitarray->buf[i] ^= bitarray->buf[j];
  }

  /* Reverse bits within each byte using precomputed LUT */
  for (size_t i = start_byte; i < end_byte; i++) {
    bitarray->buf[i] = bit_reverse_table256[(uint8_t)bitarray->buf[i]];
  }
}
/* --------------------------------------------------------------------------
 * bitarray_rotate_left_one
 */
static void
bitarray_rotate_left_one(bitarray_t* const bitarray,
                                     const size_t bit_offset,
                                     const size_t bit_length)
{
  // Grab the first bit in the range, shift everything left by one, and
  // then stick the first bit at the end.
  const bool first_bit = bitarray_get(bitarray, bit_offset);
  size_t i;
  for (i = bit_offset; i + 1 < bit_offset + bit_length; i++) {
    bitarray_set(bitarray, i, bitarray_get(bitarray, i + 1));
  }
  bitarray_set(bitarray, i, first_bit);
}
/* --------------------------------------------------------------------------
 * modulo
 */
static size_t
modulo(const ssize_t n, const size_t m)
{
  const ssize_t signed_m = (ssize_t)m;
  assert(signed_m > 0);
  const ssize_t result = ((n % signed_m) + signed_m) % signed_m;
  assert(result >= 0);
  return (size_t)result;
}
/* --------------------------------------------------------------------------
 * bitmask
 */
static char
bitmask(const size_t bit_index)
{
  return 1 << (bit_index % 8);
}

