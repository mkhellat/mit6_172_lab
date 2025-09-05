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

// Thresholds for using different LUT sizes
#define WORD_LUT_THRESHOLD (10000u)    // 10K bits for 16-bit LUT
#define LARGE_LUT_THRESHOLD (100000u)  // 100K bits for 32-bit LUT
#define HUGE_LUT_THRESHOLD (1000000u)  // 1M bits for 64-bit LUT

// Lookup tables for various bit widths
static const uint8_t bit_reverse_table256[256] = {
#define R2(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#define R6(n) R4(n), R4(n + 2*4), R4(n + 1*4), R4(n + 3*4)
    R6(0), R6(2), R6(1), R6(3)
#undef R2
#undef R4
#undef R6
};

// 16-bit word reversal LUT (generated at compile time)
static const uint16_t bit_reverse_table16k[65536] = {
#define R2_16(n) n, n + 2*16384, n + 1*16384, n + 3*16384
#define R4_16(n) R2_16(n), R2_16(n + 2*4096), R2_16(n + 1*4096), R2_16(n + 3*4096)
#define R6_16(n) R4_16(n), R4_16(n + 2*1024), R4_16(n + 1*1024), R4_16(n + 3*1024)
#define R8_16(n) R6_16(n), R6_16(n + 2*256), R6_16(n + 1*256), R6_16(n + 3*256)
#define R10_16(n) R8_16(n), R8_16(n + 2*64), R8_16(n + 1*64), R8_16(n + 3*64)
#define R12_16(n) R10_16(n), R10_16(n + 2*16), R10_16(n + 1*16), R10_16(n + 3*16)
#define R14_16(n) R12_16(n), R12_16(n + 2*4), R12_16(n + 1*4), R12_16(n + 3*4)
#define R16_16(n) R14_16(n), R14_16(n + 2*1), R14_16(n + 1*1), R14_16(n + 3*1)
    R16_16(0), R16_16(2), R16_16(1), R16_16(3)
#undef R2_16
#undef R4_16
#undef R6_16
#undef R8_16
#undef R10_16
#undef R12_16
#undef R14_16
#undef R16_16
};

// 32-bit word reversal LUT (256 entries for 8-bit chunks, generated at compile time)
static const uint32_t bit_reverse_table32[256] = {
#define R2_32(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4_32(n) R2_32(n), R2_32(n + 2*16), R2_32(n + 1*16), R2_32(n + 3*16)
#define R6_32(n) R4_32(n), R4_32(n + 2*4), R4_32(n + 1*4), R4_32(n + 3*4)
    R6_32(0), R6_32(2), R6_32(1), R6_32(3)
#undef R2_32
#undef R4_32
#undef R6_32
};

// 64-bit word reversal LUT (256 entries for 8-bit chunks, generated at compile time)
static const uint64_t bit_reverse_table64[256] = {
#define R2_64(n) n, n + 2*64, n + 1*64, n + 3*64
#define R4_64(n) R2_64(n), R2_64(n + 2*16), R2_64(n + 1*16), R2_64(n + 3*16)
#define R6_64(n) R4_64(n), R4_64(n + 2*4), R4_64(n + 1*4), R4_64(n + 3*4)
    R6_64(0), R6_64(2), R6_64(1), R6_64(3)
#undef R2_64
#undef R4_64
#undef R6_64
};


// ********************************* Types **********************************

// Concrete data type representing an array of bits.

// ********************************* Function Prototypes **********************************

static inline uint8_t read_u8_bits(const uint8_t* const buf,
                                   const size_t num_bytes,
                                   const size_t bit_pos);
static inline void write_u8_bits(uint8_t* const buf,
                                 const size_t num_bytes,
                                 const size_t bit_pos,
                                 const uint8_t value);
static inline uint16_t read_u16_bits(const uint8_t* const buf,
                                     const size_t num_bytes,
                                     const size_t bit_pos);
static inline void write_u16_bits(uint8_t* const buf,
                                  const size_t num_bytes,
                                  const size_t bit_pos,
                                  const uint16_t value);
static inline uint32_t read_u32_bits(const uint8_t* const buf,
                                     const size_t num_bytes,
                                     const size_t bit_pos);
static inline void write_u32_bits(uint8_t* const buf,
                                  const size_t num_bytes,
                                  const size_t bit_pos,
                                  const uint32_t value);
static inline uint64_t read_u64_bits(const uint8_t* const buf,
                                     const size_t num_bytes,
                                     const size_t bit_pos);
static inline void write_u64_bits(uint8_t* const buf,
                                  const size_t num_bytes,
                                  const size_t bit_pos,
                                  const uint64_t value);
struct bitarray {
  // The number of bits represented by this bit array.
  // Need not be divisible by 8.
  size_t bit_sz;

  // The underlying memory buffer that stores the bits in
  // packed form (8 per byte).
  char* buf;
};


// ******************** Prototypes for static functions *********************

// Read 8 bits starting at an arbitrary bit position (may span 2 bytes)
/* --------------------------------------------------------------------------
 * read_u8_bits
 *
 * Reads eight consecutive bits starting at an arbitrary bit position.
 * The window may straddle two adjacent bytes. Bits outside the physical
 * buffer are treated as zeros.
 */
static inline uint8_t
read_u8_bits(const uint8_t* const buf,
             const size_t num_bytes,
             const size_t bit_pos);

// Write 8 bits to an arbitrary bit position (may span 2 bytes),
// preserving other bits.
/* --------------------------------------------------------------------------
 * write_u8_bits
 *
 * Writes eight consecutive bits to an arbitrary bit position while preserving
 * other bits not covered by the write. The window may straddle two adjacent
 * bytes.
 */
static inline void write_u8_bits(uint8_t* const buf,
				 const size_t num_bytes,
				 const size_t bit_pos,
				 const uint8_t value);

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

// Reads eight consecutive bits starting at an arbitrary bit position.
//
// buf is a pointer to the underlying byte buffer.
// num_bytes is the length of the buffer in bytes.
// bit_pos is the bit index at which to begin reading (0-based from LSB of
//          byte 0). The 8-bit window may straddle two adjacent bytes.
//
// Returns the 8-bit value formed by bits [bit_pos, bit_pos+7], where bit_pos
// corresponds to the least-significant bit of the returned byte.
static inline uint8_t read_u8_bits(const uint8_t* const buf,
                                   const size_t num_bytes,
                                   const size_t bit_pos);

// Writes eight consecutive bits to an arbitrary bit position while preserving
// neighboring bits not covered by the write.
//
// buf is a pointer to the underlying byte buffer.
// num_bytes is the length of the buffer in bytes.
// bit_pos is the bit index at which to begin writing (0-based from LSB of
//          byte 0). The 8-bit window may straddle two adjacent bytes.
// value is the 8-bit value to write such that its least-significant bit is
//        written at position bit_pos.
//
// This function masks and updates only the relevant bit ranges in the one or
// two affected bytes, leaving all other bits untouched.
static inline void write_u8_bits(uint8_t* const buf,
                                 const size_t num_bytes,
                                 const size_t bit_pos,
                                 const uint8_t value);


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

  uint8_t* buf = (uint8_t*)bitarray->buf;

  size_t lo = start_idx;
  size_t hi = end_idx;

  // Fast path: subrange within one byte
  if ((lo >> 3) == (hi >> 3)) {
    size_t byte_index = lo >> 3;
    uint8_t start_bit = lo & 7;
    uint8_t end_bit = hi & 7;
    uint8_t width = (uint8_t)(end_bit - start_bit + 1);
    uint8_t mask = (uint8_t)(((1u << width) - 1u) << start_bit);
    uint8_t bits = (uint8_t)((buf[byte_index] & mask) >> start_bit);
    bits = (uint8_t)(bit_reverse_table256[bits] >> (8 - width));
    buf[byte_index] = (uint8_t)((buf[byte_index] & ~mask) | (uint8_t)(bits << start_bit));
    return;
  }

  // Four-tier LUT optimization: 8-bit, 16-bit, 32-bit, and 64-bit LUTs
  const size_t num_bytes = (bitarray->bit_sz + 7u) >> 3;
  const size_t total_bits = hi - lo + 1u;
  
  if (total_bits >= HUGE_LUT_THRESHOLD) {
    // Use 64-bit LUT for huge arrays
    while (lo < hi) {
      // Process 64-bit chunks when possible
      if (hi - lo + 1u >= 128u) {
        uint64_t left64  = read_u64_bits(buf, num_bytes, lo);
        uint64_t right64 = read_u64_bits(buf, num_bytes, hi - 63u);
        write_u64_bits(buf, num_bytes, lo,       bit_reverse_table64[right64 & 0xFF]);
        write_u64_bits(buf, num_bytes, hi - 63u, bit_reverse_table64[left64 & 0xFF]);
        lo += 64u;
        hi -= 64u;
      }
      // Process 32-bit chunks
      else if (hi - lo + 1u >= 64u) {
        uint32_t left32  = read_u32_bits(buf, num_bytes, lo);
        uint32_t right32 = read_u32_bits(buf, num_bytes, hi - 31u);
        write_u32_bits(buf, num_bytes, lo,       bit_reverse_table32[right32 & 0xFF]);
        write_u32_bits(buf, num_bytes, hi - 31u, bit_reverse_table32[left32 & 0xFF]);
        lo += 32u;
        hi -= 32u;
      }
      // Process 16-bit chunks
      else if (hi - lo + 1u >= 32u) {
        uint16_t left16  = read_u16_bits(buf, num_bytes, lo);
        uint16_t right16 = read_u16_bits(buf, num_bytes, hi - 15u);
        write_u16_bits(buf, num_bytes, lo,       bit_reverse_table16k[right16]);
        write_u16_bits(buf, num_bytes, hi - 15u, bit_reverse_table16k[left16]);
        lo += 16u;
        hi -= 16u;
      }
      // Process 8-bit chunks
      else if (hi - lo + 1u >= 16u) {
        uint8_t left8  = read_u8_bits(buf, num_bytes, lo);
        uint8_t right8 = read_u8_bits(buf, num_bytes, hi - 7u);
        write_u8_bits(buf, num_bytes, lo,       bit_reverse_table256[right8]);
        write_u8_bits(buf, num_bytes, hi - 7u,  bit_reverse_table256[left8]);
        lo += 8u;
        hi -= 8u;
      } else {
        // Fallback: bit-by-bit swap for remaining < 16 bits
        bool tmp = bitarray_get(bitarray, lo);
        bitarray_set(bitarray, lo, bitarray_get(bitarray, hi));
        bitarray_set(bitarray, hi, tmp);
        lo++;
        hi--;
      }
    }
  } else if (total_bits >= LARGE_LUT_THRESHOLD) {
    // Use 32-bit LUT for very large arrays
    while (lo < hi) {
      // Process 32-bit chunks when possible
      if (hi - lo + 1u >= 64u) {
        uint32_t left32  = read_u32_bits(buf, num_bytes, lo);
        uint32_t right32 = read_u32_bits(buf, num_bytes, hi - 31u);
        write_u32_bits(buf, num_bytes, lo,       bit_reverse_table32[right32 & 0xFF]);
        write_u32_bits(buf, num_bytes, hi - 31u, bit_reverse_table32[left32 & 0xFF]);
        lo += 32u;
        hi -= 32u;
      }
      // Process 16-bit chunks
      else if (hi - lo + 1u >= 32u) {
        uint16_t left16  = read_u16_bits(buf, num_bytes, lo);
        uint16_t right16 = read_u16_bits(buf, num_bytes, hi - 15u);
        write_u16_bits(buf, num_bytes, lo,       bit_reverse_table16k[right16]);
        write_u16_bits(buf, num_bytes, hi - 15u, bit_reverse_table16k[left16]);
        lo += 16u;
        hi -= 16u;
      }
      // Process 8-bit chunks
      else if (hi - lo + 1u >= 16u) {
        uint8_t left8  = read_u8_bits(buf, num_bytes, lo);
        uint8_t right8 = read_u8_bits(buf, num_bytes, hi - 7u);
        write_u8_bits(buf, num_bytes, lo,       bit_reverse_table256[right8]);
        write_u8_bits(buf, num_bytes, hi - 7u,  bit_reverse_table256[left8]);
        lo += 8u;
        hi -= 8u;
      } else {
        // Fallback: bit-by-bit swap for remaining < 16 bits
        bool tmp = bitarray_get(bitarray, lo);
        bitarray_set(bitarray, lo, bitarray_get(bitarray, hi));
        bitarray_set(bitarray, hi, tmp);
        lo++;
        hi--;
      }
    }
  } else if (total_bits >= WORD_LUT_THRESHOLD) {
    // Use 16-bit LUT for medium arrays
    while (lo < hi) {
      // Process 16-bit chunks when possible
      if (hi - lo + 1u >= 32u) {
        uint16_t left16  = read_u16_bits(buf, num_bytes, lo);
        uint16_t right16 = read_u16_bits(buf, num_bytes, hi - 15u);
        write_u16_bits(buf, num_bytes, lo,       bit_reverse_table16k[right16]);
        write_u16_bits(buf, num_bytes, hi - 15u, bit_reverse_table16k[left16]);
        lo += 16u;
        hi -= 16u;
      }
      // Process 8-bit chunks
      else if (hi - lo + 1u >= 16u) {
        uint8_t left8  = read_u8_bits(buf, num_bytes, lo);
        uint8_t right8 = read_u8_bits(buf, num_bytes, hi - 7u);
        write_u8_bits(buf, num_bytes, lo,       bit_reverse_table256[right8]);
        write_u8_bits(buf, num_bytes, hi - 7u,  bit_reverse_table256[left8]);
        lo += 8u;
        hi -= 8u;
      } else {
        // Fallback: bit-by-bit swap for remaining < 16 bits
        bool tmp = bitarray_get(bitarray, lo);
        bitarray_set(bitarray, lo, bitarray_get(bitarray, hi));
        bitarray_set(bitarray, hi, tmp);
        lo++;
        hi--;
      }
    }
  } else {
    // Use 8-bit LUT for small arrays
    while (lo < hi) {
      // If we have at least 16 bits total, try to process 8-bit chunks
      if (hi - lo + 1u >= 16u) {
        uint8_t left8  = read_u8_bits(buf, num_bytes, lo);
        uint8_t right8 = read_u8_bits(buf, num_bytes, hi - 7u);
        write_u8_bits(buf, num_bytes, lo,       bit_reverse_table256[right8]);
        write_u8_bits(buf, num_bytes, hi - 7u,  bit_reverse_table256[left8]);
        lo += 8u;
        hi -= 8u;
      } else {
        // Fallback: bit-by-bit swap for remaining < 16 bits
        bool tmp = bitarray_get(bitarray, lo);
        bitarray_set(bitarray, lo, bitarray_get(bitarray, hi));
        bitarray_set(bitarray, hi, tmp);
        lo++;
        hi--;
      }
    }
  }
}
/* --------------------------------------------------------------------------
 * read_u8_bits
 */
static inline uint8_t
read_u8_bits(const uint8_t* const buf,
             const size_t num_bytes,
             const size_t bit_pos)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    const uint8_t b0 = buf[byte_index];
    const uint8_t b1 = (byte_index + 1u < num_bytes) ?
      buf[byte_index + 1u] : 0u;
    if (bit_offset == 0) {
        return b0;
    }
    return (uint8_t)((b0 >> bit_offset) |
		     (uint8_t)(b1 << (8u - bit_offset)));
}

/* --------------------------------------------------------------------------
 * read_u16_bits
 */
static inline uint16_t
read_u16_bits(const uint8_t* const buf,
              const size_t num_bytes,
              const size_t bit_pos)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 2u <= num_bytes) {
        // Aligned 16-bit read
        return *(const uint16_t*)(buf + byte_index);
    }
    
    // Unaligned read: combine up to 3 bytes
    uint16_t result = 0;
    for (int i = 0; i < 2; i++) {
        if (byte_index + i < num_bytes) {
            result |= ((uint16_t)buf[byte_index + i]) << (i * 8);
        }
    }
    
    if (bit_offset != 0) {
        result >>= bit_offset;
        if (byte_index + 2 < num_bytes) {
            result |= ((uint16_t)buf[byte_index + 2]) << (16 - bit_offset);
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u8_bits
 */
static inline void
write_u8_bits(uint8_t* const buf,
              const size_t num_bytes,
              const size_t bit_pos,
              const uint8_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    if (bit_offset == 0) {
        buf[byte_index] = value;
        return;
    }
    const uint8_t first_mask = (uint8_t)(0xFFu << bit_offset);
    const uint8_t part0 = (uint8_t)(value << bit_offset);
    buf[byte_index] = (uint8_t)((buf[byte_index] & (uint8_t)~first_mask) |
				(part0 & first_mask));

    const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
    const uint8_t part1 = (uint8_t)(value >> (8u - bit_offset));
    if (byte_index + 1u < num_bytes) {
        buf[byte_index + 1u] = (uint8_t)((buf[byte_index + 1u] &
					  (uint8_t)~second_mask) |
					 (part1 & second_mask));
    }
}

/* --------------------------------------------------------------------------
 * write_u16_bits
 */
static inline void
write_u16_bits(uint8_t* const buf,
               const size_t num_bytes,
               const size_t bit_pos,
               const uint16_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 2u <= num_bytes) {
        // Aligned 16-bit write
        *(uint16_t*)(buf + byte_index) = value;
        return;
    }
    
    // Unaligned write: split into bytes
    for (int i = 0; i < 2; i++) {
        if (byte_index + i < num_bytes) {
            uint8_t byte_val = (uint8_t)(value >> (i * 8));
            if (i == 0 && bit_offset != 0) {
                // First byte needs special handling
                const uint8_t first_mask = (uint8_t)(0xFFu << bit_offset);
                const uint8_t part0 = (uint8_t)(byte_val << bit_offset);
                buf[byte_index] = (uint8_t)((buf[byte_index] & (uint8_t)~first_mask) |
                                          (part0 & first_mask));
            } else {
                buf[byte_index + i] = byte_val;
            }
        }
    }
    
    if (bit_offset != 0 && byte_index + 2 < num_bytes) {
        // Handle overflow bits
        uint8_t overflow = (uint8_t)(value >> (16 - bit_offset));
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + 2] = (uint8_t)((buf[byte_index + 2] & (uint8_t)~second_mask) |
                                       (overflow & second_mask));
    }
}

/* --------------------------------------------------------------------------
 * read_u32_bits
 */
static inline uint32_t
read_u32_bits(const uint8_t* const buf,
              const size_t num_bytes,
              const size_t bit_pos)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 4u <= num_bytes) {
        // Aligned 32-bit read
        return *(const uint32_t*)(buf + byte_index);
    }
    
    // Unaligned read: combine up to 5 bytes
    uint32_t result = 0;
    for (int i = 0; i < 4; i++) {
        if (byte_index + i < num_bytes) {
            result |= ((uint32_t)buf[byte_index + i]) << (i * 8);
        }
    }
    
    if (bit_offset != 0) {
        result >>= bit_offset;
        if (byte_index + 4 < num_bytes) {
            result |= ((uint32_t)buf[byte_index + 4]) << (32 - bit_offset);
        }
    }
    
    return result;
}

/* --------------------------------------------------------------------------
 * write_u32_bits
 */
static inline void
write_u32_bits(uint8_t* const buf,
               const size_t num_bytes,
               const size_t bit_pos,
               const uint32_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 4u <= num_bytes) {
        // Aligned 32-bit write
        *(uint32_t*)(buf + byte_index) = value;
        return;
    }
    
    // Unaligned write: split into bytes
    for (int i = 0; i < 4; i++) {
        if (byte_index + i < num_bytes) {
            uint8_t byte_val = (uint8_t)(value >> (i * 8));
            if (i == 0 && bit_offset != 0) {
                // First byte needs special handling
                const uint8_t first_mask = (uint8_t)(0xFFu << bit_offset);
                const uint8_t part0 = (uint8_t)(byte_val << bit_offset);
                buf[byte_index] = (uint8_t)((buf[byte_index] & (uint8_t)~first_mask) |
                                          (part0 & first_mask));
            } else {
                buf[byte_index + i] = byte_val;
            }
        }
    }
    
    if (bit_offset != 0 && byte_index + 4 < num_bytes) {
        // Handle overflow bits
        uint8_t overflow = (uint8_t)(value >> (32 - bit_offset));
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + 4] = (uint8_t)((buf[byte_index + 4] & (uint8_t)~second_mask) |
                                       (overflow & second_mask));
    }
}

/* --------------------------------------------------------------------------
 * read_u64_bits
 */
static inline uint64_t
read_u64_bits(const uint8_t* const buf,
              const size_t num_bytes,
              const size_t bit_pos)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 8u <= num_bytes) {
        // Aligned 64-bit read
        return *(const uint64_t*)(buf + byte_index);
    }
    
    // Unaligned read: combine up to 9 bytes
    uint64_t result = 0;
    for (int i = 0; i < 8; i++) {
        if (byte_index + i < num_bytes) {
            result |= ((uint64_t)buf[byte_index + i]) << (i * 8);
        }
    }
    
    if (bit_offset != 0) {
        result >>= bit_offset;
        if (byte_index + 8 < num_bytes) {
            result |= ((uint64_t)buf[byte_index + 8]) << (64 - bit_offset);
        }
    }
    
    return result;
}

/* --------------------------------------------------------------------------
 * write_u64_bits
 */
static inline void
write_u64_bits(uint8_t* const buf,
               const size_t num_bytes,
               const size_t bit_pos,
               const uint64_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 8u <= num_bytes) {
        // Aligned 64-bit write
        *(uint64_t*)(buf + byte_index) = value;
        return;
    }
    
    // Unaligned write: split into bytes
    for (int i = 0; i < 8; i++) {
        if (byte_index + i < num_bytes) {
            uint8_t byte_val = (uint8_t)(value >> (i * 8));
            if (i == 0 && bit_offset != 0) {
                // First byte needs special handling
                const uint8_t first_mask = (uint8_t)(0xFFu << bit_offset);
                const uint8_t part0 = (uint8_t)(byte_val << bit_offset);
                buf[byte_index] = (uint8_t)((buf[byte_index] & (uint8_t)~first_mask) |
                                          (part0 & first_mask));
            } else {
                buf[byte_index + i] = byte_val;
            }
        }
    }
    
    if (bit_offset != 0 && byte_index + 8 < num_bytes) {
        // Handle overflow bits
        uint8_t overflow = (uint8_t)(value >> (64 - bit_offset));
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + 8] = (uint8_t)((buf[byte_index + 8] & (uint8_t)~second_mask) |
                                       (overflow & second_mask));
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

