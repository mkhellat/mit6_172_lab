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

// Hash table configuration for LUT management
#define LUT_ENTRY_CACHE_SIZE 9

// Thresholds for using different LUT sizes
#define TINY_LUT_THRESHOLD       (500u)       // 500 bits for 32-bit LUT
#define SMALL_LUT_THRESHOLD      (5000u)      // 5K bits for 64-bit LUT  
#define MEDIUM_LUT_THRESHOLD     (50000u)     // 50K bits for 128-bit LUT
#define LARGE_LUT_THRESHOLD      (250000u)    // 250K bits for 256-bit LUT
#define CACHE_LINE_LUT_THRESHOLD (500000u)    // 500K bits for 512-bit LUT (cache line)
#define HUGE_LUT_THRESHOLD       (2500000u)   // 2.5M bits for 1024-bit LUT
#define MASSIVE_LUT_THRESHOLD    (10000000u)  // 10M bits for 2048-bit LUT
#define EXTREME_LUT_THRESHOLD    (50000000u)  // 50M bits for 4096-bit LUT
#define ULTRA_LUT_THRESHOLD      (100000000u) // 100M bits for 8192-bit LUT

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


// 128-bit LUT (256 entries) - implemented as 2x uint64_t
typedef struct {
    uint64_t low;
    uint64_t high;
} uint128_t;


// 256-bit LUT (256 entries) - implemented as 4x uint64_t
typedef struct {
    uint64_t low;
    uint64_t mid_low;
    uint64_t mid_high;
    uint64_t high;
} uint256_t;


// 512-bit LUT (256 entries) - implemented as 8x uint64_t (cache line size)
typedef struct {
    uint64_t chunk0;
    uint64_t chunk1;
    uint64_t chunk2;
    uint64_t chunk3;
    uint64_t chunk4;
    uint64_t chunk5;
    uint64_t chunk6;
    uint64_t chunk7;
} uint512_t;


// 1024-bit LUT (256 entries) - implemented as 16x uint64_t
typedef struct {
    uint64_t chunk[16];
} uint1024_t;


// 2048-bit LUT (256 entries) - implemented as 32x uint64_t
typedef struct {
    uint64_t chunk[32];
} uint2048_t;


// 4096-bit LUT (256 entries) - implemented as 64x uint64_t
typedef struct {
    uint64_t chunk[64];
} uint4096_t;


// 8192-bit LUT (256 entries) - implemented as 128x uint64_t
typedef struct {
    uint64_t chunk[128];
} uint8192_t;


// LUT entry types for different bit widths
typedef enum {
    LUT_TYPE_8BIT = 0,      // Fallback for very small arrays
    LUT_TYPE_32BIT = 1,     // 500+ bits
    LUT_TYPE_64BIT = 2,     // 5K+ bits
    LUT_TYPE_128BIT = 3,    // 50K+ bits
    LUT_TYPE_256BIT = 4,    // 250K+ bits
    LUT_TYPE_512BIT = 5,    // 500K+ bits (cache line)
    LUT_TYPE_1024BIT = 6,   // 2.5M+ bits
    LUT_TYPE_2048BIT = 7,   // 10M+ bits
    LUT_TYPE_4096BIT = 8,   // 50M+ bits
    LUT_TYPE_8192BIT = 9    // 100M+ bits
} lut_type_t;


// Hash table entry for LUT management
typedef struct lut_hash_entry {
    size_t bit_count;           // Key: number of bits
    lut_type_t lut_type;        // Value: which LUT to use
    uint8_t* lut_data;          // Pointer to LUT data
    size_t lut_size;            // Size of LUT data
    struct lut_hash_entry* next; // For collision handling
} lut_hash_entry_t;


// Hash table for LUT selection
typedef struct {
    lut_hash_entry_t cache[LUT_ENTRY_CACHE_SIZE];
    size_t cache_head;
} lut_manager_t;


// **************************** Global Variables ****************************

// Global LUT manager instance
static lut_manager_t* g_lut_manager = NULL;

// ********************** Large Integer Bit Operations **********************

// 64-bit LUT operations
static inline uint64_t
read_u64_bits(const uint8_t* const buf, const size_t num_bytes, const size_t bit_pos);
static inline void
write_u64_bits(uint8_t* const buf, const size_t num_bytes, const size_t bit_pos, const uint64_t value);
// 128-bit LUT operations
static inline uint128_t
read_u128_bits(const uint8_t* const buf, const size_t num_bytes, const size_t bit_pos);
static inline void
write_u128_bits(uint8_t* const buf, const size_t num_bytes, const size_t bit_pos, const uint128_t value);
static inline uint128_t
reverse_u128_bits(uint128_t value);
// 512-bit LUT operations
static inline uint512_t
read_u512_bits(const uint8_t* const buf, const size_t num_bytes, const size_t bit_pos);
static inline void
write_u512_bits(uint8_t* const buf, const size_t num_bytes, const size_t bit_pos, const uint512_t value);
static inline uint512_t
reverse_u512_bits(uint512_t value);



// ************************** Function Prototypes ***************************

// Hash table management functions
static lut_manager_t* lut_manager_init(void);
static void lut_manager_destroy(lut_manager_t* manager);
static lut_type_t lut_manager_get_type(lut_manager_t* manager, size_t bit_count);
static size_t lut_manager_get_lut_size(lut_type_t type);
static lut_hash_entry_t* lut_manager_find_entry(lut_manager_t* manager, size_t bit_count);
static void lut_manager_add_entry(lut_manager_t* manager, size_t bit_count, lut_type_t type);

// Huge integer bit operations
static uint1024_t read_u1024_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos);
static void write_u1024_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint1024_t value);
static uint1024_t reverse_u1024_bits(uint1024_t value);
static uint2048_t read_u2048_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos);
static void write_u2048_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint2048_t value);
static uint2048_t reverse_u2048_bits(uint2048_t value);
static uint4096_t read_u4096_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos);
static void write_u4096_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint4096_t value);
static uint4096_t reverse_u4096_bits(uint4096_t value);
static uint8192_t read_u8192_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos);
static void write_u8192_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint8192_t value);
static uint8192_t reverse_u8192_bits(uint8192_t value);

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
static void
bitarray_rotate_left(bitarray_t* const bitarray,
		     const size_t bit_offset,
		     const size_t bit_length,
		     const size_t bit_left_amount);

// Reverse a bitarray in-place.
//
// Rev(A) = {A[n] A[n-1] ... A[0]}
//
// Swap bits starting from LSB and MSB moving inwards towards the
// center of the array.
static inline void
bitarray_reverse_naive(bitarray_t* const bitarray,
		       size_t start_idx,
		       size_t end);

// Block-wise reversal algorithm for very large arrays - uses 64-bit
// chunks for better cache locality
static inline void
bitarray_reverse_block_wise(bitarray_t* const bitarray,
			    const size_t start_idx,
			    const size_t end_idx);

// Reverse a subarray within a bitarray using adaptive LUT optimization
//
// start_idx Starting bit index of subarray to reverse
// end_idx   Ending bit index of subarray to reverse (inclusive)
// 
// This function reverses a contiguous subarray of bits using an adaptive
// strategy that selects the optimal LUT size based on array size. The
// algorithm:
// 1. For extremely large arrays (200M+ bits): delegates to block-wise reversal
// 2. For smaller arrays: uses LUT manager to select optimal chunk size
//    (8-bit to 8192-bit) based on total bit count
// 3. Employs cache prefetching and optimized bit operations
// 4. Falls back to naive bit-by-bit reversal if LUT manager fails
// 
// Performance: O(n) with adaptive optimization based on array size
static inline void
bitarray_reverse_lut(bitarray_t* const bitarray,
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


//************************** Generic Bit Functions **************************

/* --------------------------------------------------------------------------
 * read_bits
 *
 * Generic function to read arbitrary number of bits from an arbitrary bit position.
 * Returns up to 64 bits in a uint64_t. For larger reads, caller can make multiple calls.
 *
 * buf: pointer to the underlying byte buffer
 * bit_pos: bit index at which to begin reading (0-based from LSB of byte 0)
 * bit_count: number of bits to read (1 to 64)
 *
 * Returns: uint64_t containing the read bits (LSB first)
 */
static inline uint64_t
read_bits(const uint8_t* buf, 
          size_t bit_pos, 
          size_t bit_count)
{
    // Limit to 64 bits for simplicity and performance
    if (bit_count > 64) {
        bit_count = 64;
    }
    
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    // Calculate how many bytes we need to read
    const size_t bytes_needed = (bit_count + bit_offset + 7) >> 3;
    
    // Handle aligned reads efficiently
    if (bit_offset == 0 && bit_count == 64 && bytes_needed >= 8) {
        // Aligned 64-bit read
        return *(const uint64_t*)(buf + byte_index);
    }
    
    if (bit_offset == 0 && bit_count == 32 && bytes_needed >= 4) {
        // Aligned 32-bit read
        return *(const uint32_t*)(buf + byte_index);
    }
    
    if (bit_offset == 0 && bit_count == 16 && bytes_needed >= 2) {
        // Aligned 16-bit read
        return *(const uint16_t*)(buf + byte_index);
    }
    
    if (bit_offset == 0 && bit_count == 8 && bytes_needed >= 1) {
        // Aligned 8-bit read
        return buf[byte_index];
    }
    
    // Unaligned read: combine bytes
    uint64_t result = 0;
    for (size_t i = 0; i < bytes_needed && i < 8; i++) {
        result |= ((uint64_t)buf[byte_index + i]) << (i * 8);
    }
    
    // Shift to remove leading bits if needed
    if (bit_offset != 0) {
        result >>= bit_offset;
        // Add overflow bits from the next byte if needed
        if (byte_index + bytes_needed < byte_index + 8) {
            result |= ((uint64_t)buf[byte_index + bytes_needed]) << (64 - bit_offset);
        }
    }
    
    // Mask to the requested bit count
    if (bit_count < 64) {
        result &= ((1ULL << bit_count) - 1);
    }
    
    return result;
}

/* --------------------------------------------------------------------------
 * write_bits
 *
 * Generic function to write arbitrary number of bits to an arbitrary bit position.
 * Can write any number of bits from a value buffer.
 *
 * buf: pointer to the underlying byte buffer
 * bit_pos: bit index at which to begin writing (0-based from LSB of byte 0)
 * bit_count: number of bits to write (1 to any number)
 * value: pointer to the data to write
 * value_bytes: size of the value data in bytes
 */
static inline void
write_bits(uint8_t* buf, 
           size_t bit_pos, 
           size_t bit_count,
           const uint8_t* value,
           size_t value_bytes)
{
    if (bit_count == 0 || value_bytes == 0) {
        return;
    }
    
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    // Calculate how many bytes we need to write
    const size_t bytes_needed = (bit_count + bit_offset + 7) >> 3;
    
    // Handle aligned writes efficiently
    if (bit_offset == 0 && bit_count == 64 && bytes_needed >= 8 && value_bytes >= 8) {
        // Aligned 64-bit write
        *(uint64_t*)(buf + byte_index) = *(const uint64_t*)value;
        return;
    }
    
    if (bit_offset == 0 && bit_count == 32 && bytes_needed >= 4 && value_bytes >= 4) {
        // Aligned 32-bit write
        *(uint32_t*)(buf + byte_index) = *(const uint32_t*)value;
        return;
    }
    
    if (bit_offset == 0 && bit_count == 16 && bytes_needed >= 2 && value_bytes >= 2) {
        // Aligned 16-bit write
        *(uint16_t*)(buf + byte_index) = *(const uint16_t*)value;
        return;
    }
    
    if (bit_offset == 0 && bit_count == 8 && bytes_needed >= 1 && value_bytes >= 1) {
        // Aligned 8-bit write
        buf[byte_index] = value[0];
        return;
    }
    
    // Unaligned write: split into bytes
    for (size_t i = 0; i < bytes_needed && i < value_bytes; i++) {
        uint8_t byte_val = value[i];
        
        if (i == 0 && bit_offset != 0) {
            // First byte needs special handling for bit alignment
            const uint8_t first_mask = (uint8_t)(0xFFu << bit_offset);
            const uint8_t part0 = (uint8_t)(byte_val << bit_offset);
            buf[byte_index] = (uint8_t)((buf[byte_index] & (uint8_t)~first_mask) |
                                      (part0 & first_mask));
        } else {
            buf[byte_index + i] = byte_val;
        }
    }
    
    // Handle overflow bits if needed
    if (bit_offset != 0 && byte_index + bytes_needed < byte_index + value_bytes) {
        uint8_t overflow = value[bytes_needed];
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + bytes_needed] = (uint8_t)((buf[byte_index + bytes_needed] & (uint8_t)~second_mask) |
                                                 (overflow & second_mask));
    }
}


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
 * bitarray_cleanup_lut_manager
 *
 * Clean up the global LUT manager. Should be called at program termination
 * to free all hash table resources.
 */
void
bitarray_cleanup_lut_manager(void)
{
  if (g_lut_manager != NULL) {
    lut_manager_destroy(g_lut_manager);
    g_lut_manager = NULL;
  }
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
 * bitarray_reverse_block_wise
 *
 * Block-wise reversal algorithm for very large arrays. Uses 64-bit chunks
 * for better cache locality and memory access patterns compared to naive
 * bit-by-bit reversal. This is NOT the Block Swap rotation algorithm.
 */
static inline void
bitarray_reverse_block_wise(bitarray_t* const bitarray,
                           const size_t start_idx,
                           const size_t end_idx)
{
  if (bitarray == NULL || bitarray->buf == NULL || start_idx >= end_idx) {
    return;
  }
  
  uint8_t* buf = (uint8_t*)bitarray->buf;
  const size_t total_bits = end_idx - start_idx + 1u;
  
  // For very large arrays, use 64-bit word operations for better performance
  if (total_bits >= 64) {
    size_t pos = start_idx;
    size_t end_pos = end_idx;
    
    // Process 64-bit chunks from both ends with aggressive prefetching
    while (pos + 63 < end_pos) {
      // Prefetch next chunks for better cache performance
      if (pos + 127 < end_pos) {
        __builtin_prefetch(&buf[(pos + 64) >> 3], 0, 3);
        __builtin_prefetch(&buf[(end_pos - 127) >> 3], 0, 3);
      }
      if (pos + 255 < end_pos) {
        __builtin_prefetch(&buf[(pos + 128) >> 3], 0, 3);
        __builtin_prefetch(&buf[(end_pos - 255) >> 3], 0, 3);
      }
      
      // Read 64-bit chunks from both ends
      uint64_t left_chunk = read_bits(buf, pos, 64);
      uint64_t right_chunk = read_bits(buf, end_pos - 63, 64);
      
      // Reverse each chunk using LUT - optimized version
      const uint8_t* lut = bit_reverse_table256;
      uint64_t left_reversed = 0;
      uint64_t right_reversed = 0;
      
      // Unrolled and optimized bit reversal
      left_reversed = ((uint64_t)lut[left_chunk & 0xFF]) << 56;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 48;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 40;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 32;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 24;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 16;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]) << 8;
      left_chunk >>= 8;
      left_reversed |= ((uint64_t)lut[left_chunk & 0xFF]);
      
      right_reversed = ((uint64_t)lut[right_chunk & 0xFF]) << 56;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 48;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 40;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 32;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 24;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 16;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]) << 8;
      right_chunk >>= 8;
      right_reversed |= ((uint64_t)lut[right_chunk & 0xFF]);
      
      // Swap the reversed chunks
      write_bits(buf, pos, 64, (const uint8_t*)&right_reversed, 8);
      write_bits(buf, end_pos - 63, 64, (const uint8_t*)&left_reversed, 8);
      
      pos += 64;
      end_pos -= 64;
    }
    
    // Handle remaining bits with naive approach
    if (pos < end_pos) {
      bitarray_reverse_naive(bitarray, pos, end_pos);
    }
  } else {
    // Fall back to naive for small arrays
    bitarray_reverse_naive(bitarray, start_idx, end_idx);
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

  // Initialize LUT manager if not already done
  if (g_lut_manager == NULL) {
    g_lut_manager = lut_manager_init();
    if (g_lut_manager == NULL) {
      // Fallback to naive implementation if LUT manager fails
      bitarray_reverse_naive(bitarray, start_idx, end_idx);
      return;
    }
  }

  uint8_t* buf = (uint8_t*)bitarray->buf;
  const size_t num_bytes = (bitarray->bit_sz + 7u) >> 3;
  const size_t total_bits = end_idx - start_idx + 1u;

  // For extremely large arrays, use block-wise reversal for better cache locality
  if (total_bits >= (2 * ULTRA_LUT_THRESHOLD)) {
    // Use block-wise reversal for extremely large arrays (200M+ bits)
    bitarray_reverse_block_wise(bitarray, start_idx, end_idx);
    return;
  }

  // Use hash table to determine optimal LUT strategy
  lut_type_t lut_type = lut_manager_get_type(g_lut_manager, total_bits);
  
  switch (lut_type) {
    case LUT_TYPE_512BIT: {
      // 512-bit LUT strategy for huge arrays (cache line aligned) with prefetching
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 512) ? 512 : remaining;
        
        // Prefetch next cache line for large arrays
        if (chunk_size == 512 && pos + 512 < end_idx) {
          __builtin_prefetch(&buf[(pos + 512) >> 3], 0, 3);
        }
        
        uint512_t chunk = read_u512_bits(buf, num_bytes, pos);
        uint512_t reversed = reverse_u512_bits(chunk);
        write_u512_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_256BIT: {
      // 256-bit LUT strategy for very large arrays - use 64-bit chunks for efficiency
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 64) ? 64 : remaining;
        
        uint64_t chunk = read_bits(buf, pos, 64);
        uint64_t reversed = 0;
        
        // Reverse each 8-bit chunk using the 8-bit LUT
        reversed |= ((uint64_t)bit_reverse_table256[chunk & 0xFF]) << 56;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 8) & 0xFF]) << 48;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 16) & 0xFF]) << 40;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 24) & 0xFF]) << 32;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 32) & 0xFF]) << 24;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 40) & 0xFF]) << 16;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 48) & 0xFF]) << 8;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 56) & 0xFF]);
        
        write_bits(buf, pos, 64, (const uint8_t*)&reversed, 8);
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_128BIT: {
      // 128-bit LUT strategy for large arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 128) ? 128 : remaining;
        
        uint128_t chunk = read_u128_bits(buf, num_bytes, pos);
        uint128_t reversed = reverse_u128_bits(chunk);
        write_u128_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_64BIT: {
      // 64-bit LUT strategy for medium arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 64) ? 64 : remaining;
        
        uint64_t chunk = read_bits(buf, pos, 64);
        uint64_t reversed = 0;
        
        // Reverse each 8-bit chunk using the 8-bit LUT
        reversed |= ((uint64_t)bit_reverse_table256[chunk & 0xFF]) << 56;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 8) & 0xFF]) << 48;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 16) & 0xFF]) << 40;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 24) & 0xFF]) << 32;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 32) & 0xFF]) << 24;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 40) & 0xFF]) << 16;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 48) & 0xFF]) << 8;
        reversed |= ((uint64_t)bit_reverse_table256[(chunk >> 56) & 0xFF]);
        
        write_bits(buf, pos, 64, (const uint8_t*)&reversed, 8);
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_32BIT: {
      // 32-bit LUT strategy for small arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 32) ? 32 : remaining;
        
        uint32_t chunk = read_bits(buf, pos, 32);
        uint32_t reversed = 0;
        
        // Reverse each 8-bit chunk using the 8-bit LUT
        reversed |= bit_reverse_table256[chunk & 0xFF] << 24;
        reversed |= bit_reverse_table256[(chunk >> 8) & 0xFF] << 16;
        reversed |= bit_reverse_table256[(chunk >> 16) & 0xFF] << 8;
        reversed |= bit_reverse_table256[(chunk >> 24) & 0xFF];
        
        write_bits(buf, pos, 32, (const uint8_t*)&reversed, 4);
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_1024BIT: {
      // 1024-bit LUT strategy for huge arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 1024) ? 1024 : remaining;
        
        uint1024_t chunk = read_u1024_bits(buf, num_bytes, pos);
        uint1024_t reversed = reverse_u1024_bits(chunk);
        write_u1024_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_2048BIT: {
      // 2048-bit LUT strategy for massive arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 2048) ? 2048 : remaining;
        
        uint2048_t chunk = read_u2048_bits(buf, num_bytes, pos);
        uint2048_t reversed = reverse_u2048_bits(chunk);
        write_u2048_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_4096BIT: {
      // 4096-bit LUT strategy for extreme arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 4096) ? 4096 : remaining;
        
        uint4096_t chunk = read_u4096_bits(buf, num_bytes, pos);
        uint4096_t reversed = reverse_u4096_bits(chunk);
        write_u4096_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_8192BIT: {
      // 8192-bit LUT strategy for ultra arrays
      size_t pos = start_idx;
      while (pos < end_idx) {
        size_t remaining = end_idx - pos + 1;
        size_t chunk_size = (remaining >= 8192) ? 8192 : remaining;
        
        uint8192_t chunk = read_u8192_bits(buf, num_bytes, pos);
        uint8192_t reversed = reverse_u8192_bits(chunk);
        write_u8192_bits(buf, num_bytes, pos, reversed);
        
        pos += chunk_size;
      }
      break;
    }
    case LUT_TYPE_8BIT:
    default: {
      // Fall back to naive bit-by-bit for very small arrays
      size_t lo = start_idx;
      size_t hi = end_idx;
      
      while (lo < hi) {
        // Read bits
        bool left_bit = bitarray_get(bitarray, lo);
        bool right_bit = bitarray_get(bitarray, hi);
        
        // Swap bits
        bitarray_set(bitarray, lo, right_bit);
        bitarray_set(bitarray, hi, left_bit);
        
        lo++;
        hi--;
      }
      break;
    }
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
 * read_u128_bits
 */
static inline uint128_t
read_u128_bits(const uint8_t* const buf,
               const size_t num_bytes,
               const size_t bit_pos)
{
    uint128_t result = {0, 0};
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 16u <= num_bytes) {
        // Aligned 128-bit read
        result.low = *(const uint64_t*)(buf + byte_index);
        result.high = *(const uint64_t*)(buf + byte_index + 8);
        return result;
    }
    
    // Unaligned read: combine up to 17 bytes
    for (int i = 0; i < 16; i++) {
        if (byte_index + i < num_bytes) {
            if (i < 8) {
                result.low |= ((uint64_t)buf[byte_index + i]) << (i * 8);
            } else {
                result.high |= ((uint64_t)buf[byte_index + i]) << ((i - 8) * 8);
            }
        }
    }
    
    if (bit_offset != 0) {
        // Shift right by bit_offset
        uint64_t carry = result.low & ((1ULL << bit_offset) - 1);
        result.low >>= bit_offset;
        result.high >>= bit_offset;
        result.high |= (carry << (64 - bit_offset));
        
        // Handle overflow from 17th byte
        if (byte_index + 16 < num_bytes) {
            uint64_t overflow = ((uint64_t)buf[byte_index + 16]) << (64 - bit_offset);
            result.high |= overflow;
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u128_bits
 */
static inline void
write_u128_bits(uint8_t* const buf,
                const size_t num_bytes,
                const size_t bit_pos,
                const uint128_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 16u <= num_bytes) {
        // Aligned 128-bit write
        *(uint64_t*)(buf + byte_index) = value.low;
        *(uint64_t*)(buf + byte_index + 8) = value.high;
        return;
    }
    
    // Unaligned write: split into bytes
    for (int i = 0; i < 16; i++) {
        if (byte_index + i < num_bytes) {
            uint8_t byte_val;
            if (i < 8) {
                byte_val = (uint8_t)(value.low >> (i * 8));
            } else {
                byte_val = (uint8_t)(value.high >> ((i - 8) * 8));
            }
            
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
    
    if (bit_offset != 0 && byte_index + 16 < num_bytes) {
        // Handle overflow bits
        uint8_t overflow = (uint8_t)(value.high >> (64 - bit_offset));
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + 16] = (uint8_t)((buf[byte_index + 16] & (uint8_t)~second_mask) |
                                       (overflow & second_mask));
    }
}
/* --------------------------------------------------------------------------
 * read_u512_bits
 */
static inline uint512_t
read_u512_bits(const uint8_t* const buf,
               const size_t num_bytes,
               const size_t bit_pos)
{
    uint512_t result = {0, 0, 0, 0, 0, 0, 0, 0};
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 64u <= num_bytes) {
        // Aligned 512-bit read (cache line aligned)
        result.chunk0 = *(const uint64_t*)(buf + byte_index);
        result.chunk1 = *(const uint64_t*)(buf + byte_index + 8);
        result.chunk2 = *(const uint64_t*)(buf + byte_index + 16);
        result.chunk3 = *(const uint64_t*)(buf + byte_index + 24);
        result.chunk4 = *(const uint64_t*)(buf + byte_index + 32);
        result.chunk5 = *(const uint64_t*)(buf + byte_index + 40);
        result.chunk6 = *(const uint64_t*)(buf + byte_index + 48);
        result.chunk7 = *(const uint64_t*)(buf + byte_index + 56);
        return result;
    }
    
    // Unaligned read: combine up to 65 bytes
    for (int i = 0; i < 64; i++) {
        if (byte_index + i < num_bytes) {
            uint64_t byte_val = (uint64_t)buf[byte_index + i];
            if (i < 8) {
                result.chunk0 |= byte_val << (i * 8);
            } else if (i < 16) {
                result.chunk1 |= byte_val << ((i - 8) * 8);
            } else if (i < 24) {
                result.chunk2 |= byte_val << ((i - 16) * 8);
            } else if (i < 32) {
                result.chunk3 |= byte_val << ((i - 24) * 8);
            } else if (i < 40) {
                result.chunk4 |= byte_val << ((i - 32) * 8);
            } else if (i < 48) {
                result.chunk5 |= byte_val << ((i - 40) * 8);
            } else if (i < 56) {
                result.chunk6 |= byte_val << ((i - 48) * 8);
            } else {
                result.chunk7 |= byte_val << ((i - 56) * 8);
            }
        }
    }
    
    if (bit_offset != 0) {
        // Shift right by bit_offset across all chunks
        uint64_t carry = result.chunk0 & ((1ULL << bit_offset) - 1);
        result.chunk0 >>= bit_offset;
        result.chunk1 >>= bit_offset;
        result.chunk2 >>= bit_offset;
        result.chunk3 >>= bit_offset;
        result.chunk4 >>= bit_offset;
        result.chunk5 >>= bit_offset;
        result.chunk6 >>= bit_offset;
        result.chunk7 >>= bit_offset;
        
        // Propagate carries across all chunks
        result.chunk1 |= (carry << (64 - bit_offset));
        carry = result.chunk1 & ((1ULL << bit_offset) - 1);
        result.chunk1 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk2 |= (carry << (64 - bit_offset));
        carry = result.chunk2 & ((1ULL << bit_offset) - 1);
        result.chunk2 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk3 |= (carry << (64 - bit_offset));
        carry = result.chunk3 & ((1ULL << bit_offset) - 1);
        result.chunk3 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk4 |= (carry << (64 - bit_offset));
        carry = result.chunk4 & ((1ULL << bit_offset) - 1);
        result.chunk4 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk5 |= (carry << (64 - bit_offset));
        carry = result.chunk5 & ((1ULL << bit_offset) - 1);
        result.chunk5 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk6 |= (carry << (64 - bit_offset));
        carry = result.chunk6 & ((1ULL << bit_offset) - 1);
        result.chunk6 &= ~((1ULL << bit_offset) - 1);
        
        result.chunk7 |= (carry << (64 - bit_offset));
        
        // Handle overflow from 65th byte
        if (byte_index + 64 < num_bytes) {
            uint64_t overflow = ((uint64_t)buf[byte_index + 64]) << (64 - bit_offset);
            result.chunk7 |= overflow;
        }
    }
    
    return result;
}

/* --------------------------------------------------------------------------
 * write_u512_bits
 */
static inline void
write_u512_bits(uint8_t* const buf,
                const size_t num_bytes,
                const size_t bit_pos,
                const uint512_t value)
{
    const size_t byte_index = bit_pos >> 3;
    const uint8_t bit_offset = (uint8_t)(bit_pos & 7u);
    
    if (bit_offset == 0 && byte_index + 64u <= num_bytes) {
        // Aligned 512-bit write (cache line aligned)
        *(uint64_t*)(buf + byte_index) = value.chunk0;
        *(uint64_t*)(buf + byte_index + 8) = value.chunk1;
        *(uint64_t*)(buf + byte_index + 16) = value.chunk2;
        *(uint64_t*)(buf + byte_index + 24) = value.chunk3;
        *(uint64_t*)(buf + byte_index + 32) = value.chunk4;
        *(uint64_t*)(buf + byte_index + 40) = value.chunk5;
        *(uint64_t*)(buf + byte_index + 48) = value.chunk6;
        *(uint64_t*)(buf + byte_index + 56) = value.chunk7;
        return;
    }
    
    // Unaligned write: split into bytes
    for (int i = 0; i < 64; i++) {
        if (byte_index + i < num_bytes) {
            uint8_t byte_val;
            if (i < 8) {
                byte_val = (uint8_t)(value.chunk0 >> (i * 8));
            } else if (i < 16) {
                byte_val = (uint8_t)(value.chunk1 >> ((i - 8) * 8));
            } else if (i < 24) {
                byte_val = (uint8_t)(value.chunk2 >> ((i - 16) * 8));
            } else if (i < 32) {
                byte_val = (uint8_t)(value.chunk3 >> ((i - 24) * 8));
            } else if (i < 40) {
                byte_val = (uint8_t)(value.chunk4 >> ((i - 32) * 8));
            } else if (i < 48) {
                byte_val = (uint8_t)(value.chunk5 >> ((i - 40) * 8));
            } else if (i < 56) {
                byte_val = (uint8_t)(value.chunk6 >> ((i - 48) * 8));
            } else {
                byte_val = (uint8_t)(value.chunk7 >> ((i - 56) * 8));
            }
            
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
    
    if (bit_offset != 0 && byte_index + 64 < num_bytes) {
        // Handle overflow bits
        uint8_t overflow = (uint8_t)(value.chunk7 >> (64 - bit_offset));
        const uint8_t second_mask = (uint8_t)(0xFFu >> (8u - bit_offset));
        buf[byte_index + 64] = (uint8_t)((buf[byte_index + 64] & (uint8_t)~second_mask) |
                                       (overflow & second_mask));
    }
}
/* --------------------------------------------------------------------------
 * reverse_u128_bits
 */
static inline uint128_t
reverse_u128_bits(uint128_t value)
{
    uint128_t result = {0, 0};
    
    // Reverse each 8-bit chunk using the 8-bit LUT
    result.low |= ((uint64_t)bit_reverse_table256[value.low & 0xFF]) << 56;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 8) & 0xFF]) << 48;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 16) & 0xFF]) << 40;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 24) & 0xFF]) << 32;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 32) & 0xFF]) << 24;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 40) & 0xFF]) << 16;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 48) & 0xFF]) << 8;
    result.low |= ((uint64_t)bit_reverse_table256[(value.low >> 56) & 0xFF]);
    
    result.high |= ((uint64_t)bit_reverse_table256[value.high & 0xFF]) << 56;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 8) & 0xFF]) << 48;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 16) & 0xFF]) << 40;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 24) & 0xFF]) << 32;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 32) & 0xFF]) << 24;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 40) & 0xFF]) << 16;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 48) & 0xFF]) << 8;
    result.high |= ((uint64_t)bit_reverse_table256[(value.high >> 56) & 0xFF]);
    
    return result;
}
/* --------------------------------------------------------------------------
 * reverse_u512_bits
 */
static inline uint512_t
reverse_u512_bits(uint512_t value)
{
    uint512_t result = {0, 0, 0, 0, 0, 0, 0, 0};
    
    // Reverse each 8-bit chunk using the 8-bit LUT
    result.chunk0 |= ((uint64_t)bit_reverse_table256[value.chunk0 & 0xFF]) << 56;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 8) & 0xFF]) << 48;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 16) & 0xFF]) << 40;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 24) & 0xFF]) << 32;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 32) & 0xFF]) << 24;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 40) & 0xFF]) << 16;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 48) & 0xFF]) << 8;
    result.chunk0 |= ((uint64_t)bit_reverse_table256[(value.chunk0 >> 56) & 0xFF]);
    
    result.chunk1 |= ((uint64_t)bit_reverse_table256[value.chunk1 & 0xFF]) << 56;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 8) & 0xFF]) << 48;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 16) & 0xFF]) << 40;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 24) & 0xFF]) << 32;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 32) & 0xFF]) << 24;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 40) & 0xFF]) << 16;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 48) & 0xFF]) << 8;
    result.chunk1 |= ((uint64_t)bit_reverse_table256[(value.chunk1 >> 56) & 0xFF]);
    
    result.chunk2 |= ((uint64_t)bit_reverse_table256[value.chunk2 & 0xFF]) << 56;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 8) & 0xFF]) << 48;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 16) & 0xFF]) << 40;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 24) & 0xFF]) << 32;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 32) & 0xFF]) << 24;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 40) & 0xFF]) << 16;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 48) & 0xFF]) << 8;
    result.chunk2 |= ((uint64_t)bit_reverse_table256[(value.chunk2 >> 56) & 0xFF]);
    
    result.chunk3 |= ((uint64_t)bit_reverse_table256[value.chunk3 & 0xFF]) << 56;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 8) & 0xFF]) << 48;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 16) & 0xFF]) << 40;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 24) & 0xFF]) << 32;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 32) & 0xFF]) << 24;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 40) & 0xFF]) << 16;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 48) & 0xFF]) << 8;
    result.chunk3 |= ((uint64_t)bit_reverse_table256[(value.chunk3 >> 56) & 0xFF]);
    
    result.chunk4 |= ((uint64_t)bit_reverse_table256[value.chunk4 & 0xFF]) << 56;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 8) & 0xFF]) << 48;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 16) & 0xFF]) << 40;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 24) & 0xFF]) << 32;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 32) & 0xFF]) << 24;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 40) & 0xFF]) << 16;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 48) & 0xFF]) << 8;
    result.chunk4 |= ((uint64_t)bit_reverse_table256[(value.chunk4 >> 56) & 0xFF]);
    
    result.chunk5 |= ((uint64_t)bit_reverse_table256[value.chunk5 & 0xFF]) << 56;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 8) & 0xFF]) << 48;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 16) & 0xFF]) << 40;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 24) & 0xFF]) << 32;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 32) & 0xFF]) << 24;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 40) & 0xFF]) << 16;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 48) & 0xFF]) << 8;
    result.chunk5 |= ((uint64_t)bit_reverse_table256[(value.chunk5 >> 56) & 0xFF]);
    
    result.chunk6 |= ((uint64_t)bit_reverse_table256[value.chunk6 & 0xFF]) << 56;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 8) & 0xFF]) << 48;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 16) & 0xFF]) << 40;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 24) & 0xFF]) << 32;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 32) & 0xFF]) << 24;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 40) & 0xFF]) << 16;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 48) & 0xFF]) << 8;
    result.chunk6 |= ((uint64_t)bit_reverse_table256[(value.chunk6 >> 56) & 0xFF]);
    
    result.chunk7 |= ((uint64_t)bit_reverse_table256[value.chunk7 & 0xFF]) << 56;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 8) & 0xFF]) << 48;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 16) & 0xFF]) << 40;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 24) & 0xFF]) << 32;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 32) & 0xFF]) << 24;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 40) & 0xFF]) << 16;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 48) & 0xFF]) << 8;
    result.chunk7 |= ((uint64_t)bit_reverse_table256[(value.chunk7 >> 56) & 0xFF]);
    
    // Reverse the order of chunks for proper bit reversal
    uint512_t temp = result;
    result.chunk0 = temp.chunk7;
    result.chunk1 = temp.chunk6;
    result.chunk2 = temp.chunk5;
    result.chunk3 = temp.chunk4;
    result.chunk4 = temp.chunk3;
    result.chunk5 = temp.chunk2;
    result.chunk6 = temp.chunk1;
    result.chunk7 = temp.chunk0;
    
    return result;
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
/* --------------------------------------------------------------------------
 * lut_manager_init
 *
 * Initialize the LUT manager with cache-based LUT selection.
 * Pre-populates the cache with threshold-based LUT type mappings.
 */
static lut_manager_t*
lut_manager_init(void)
{
    lut_manager_t* manager = calloc(1, sizeof(lut_manager_t));
    if (manager == NULL) {
        return NULL;
    }
    
    // Initialize cache head
    manager->cache_head = 0;
    
    // Pre-populate cache with threshold mappings
    lut_manager_add_entry(manager, TINY_LUT_THRESHOLD, LUT_TYPE_32BIT);
    lut_manager_add_entry(manager, SMALL_LUT_THRESHOLD, LUT_TYPE_64BIT);
    lut_manager_add_entry(manager, MEDIUM_LUT_THRESHOLD, LUT_TYPE_128BIT);
    lut_manager_add_entry(manager, LARGE_LUT_THRESHOLD, LUT_TYPE_256BIT);
    lut_manager_add_entry(manager, CACHE_LINE_LUT_THRESHOLD, LUT_TYPE_512BIT);
    lut_manager_add_entry(manager, HUGE_LUT_THRESHOLD, LUT_TYPE_1024BIT);
    lut_manager_add_entry(manager, MASSIVE_LUT_THRESHOLD, LUT_TYPE_2048BIT);
    lut_manager_add_entry(manager, EXTREME_LUT_THRESHOLD, LUT_TYPE_4096BIT);
    lut_manager_add_entry(manager, ULTRA_LUT_THRESHOLD, LUT_TYPE_8192BIT);
    
    return manager;
}
/* --------------------------------------------------------------------------
 * lut_manager_destroy
 *
 * Clean up the LUT manager and free all allocated memory.
 */
static void
lut_manager_destroy(lut_manager_t* manager)
{
    if (manager == NULL) {
        return;
    }
    
    // No dynamic allocations to free in cache-based approach
    free(manager);
}
/* --------------------------------------------------------------------------
 * lut_manager_add_entry
 *
 * Add a new entry to the cache for bit count to LUT type mapping.
 */
static void
lut_manager_add_entry(lut_manager_t* manager, size_t bit_count, lut_type_t type)
{
    if (manager->cache_head >= LUT_ENTRY_CACHE_SIZE) {
        return; // Cache full
    }
    
    // Add to cache
    lut_hash_entry_t* entry = &manager->cache[manager->cache_head];
    entry->bit_count = bit_count;
    entry->lut_type = type;
    entry->lut_data = NULL; // Will be set when needed
    entry->lut_size = lut_manager_get_lut_size(type);
    entry->next = NULL;
    
    manager->cache_head++;
}
/* --------------------------------------------------------------------------
 * lut_manager_find_entry
 *
 * Find the appropriate LUT entry for a given bit count.
 * Returns the entry with the highest threshold <= bit_count.
 */
static lut_hash_entry_t*
lut_manager_find_entry(lut_manager_t* manager, size_t bit_count)
{
    lut_hash_entry_t* best_entry = NULL;
    size_t best_threshold = 0;
    
    // Search cache for the best match
    for (size_t i = 0; i < manager->cache_head; i++) {
        lut_hash_entry_t* entry = &manager->cache[i];
        if (entry->bit_count <= bit_count && entry->bit_count > best_threshold) {
            best_entry = entry;
            best_threshold = entry->bit_count;
        }
    }
    
    // If no threshold found, use 8-bit LUT as fallback
    if (best_entry == NULL) {
        // Create a fallback entry for 8-bit LUT
        static lut_hash_entry_t fallback = {0, LUT_TYPE_8BIT, NULL, 0, NULL};
        return &fallback;
    }
    
    return best_entry;
}
/* --------------------------------------------------------------------------
 * lut_manager_get_type
 *
 * Get the appropriate LUT type for a given bit count using hash table lookup.
 */
static lut_type_t
lut_manager_get_type(lut_manager_t* manager, size_t bit_count)
{
    if (manager == NULL) {
        return LUT_TYPE_8BIT; // Fallback
    }
    
    lut_hash_entry_t* entry = lut_manager_find_entry(manager, bit_count);
    return entry->lut_type;
}
/* --------------------------------------------------------------------------
 * lut_manager_get_lut_size
 *
 * Get the size of LUT data for a given LUT type.
 */
static size_t
lut_manager_get_lut_size(lut_type_t type)
{
    switch (type) {
        case LUT_TYPE_8BIT:    return 256 * sizeof(uint8_t);
        case LUT_TYPE_32BIT:   return 256 * sizeof(uint32_t);
        case LUT_TYPE_64BIT:   return 256 * sizeof(uint64_t);
        case LUT_TYPE_128BIT:  return 256 * sizeof(uint128_t);
        case LUT_TYPE_256BIT:  return 256 * sizeof(uint256_t);
        case LUT_TYPE_512BIT:  return 256 * sizeof(uint512_t);
        case LUT_TYPE_1024BIT: return 256 * sizeof(uint1024_t);
        case LUT_TYPE_2048BIT: return 256 * sizeof(uint2048_t);
        case LUT_TYPE_4096BIT: return 256 * sizeof(uint4096_t);
        case LUT_TYPE_8192BIT: return 256 * sizeof(uint8192_t);
        default:               return 256 * sizeof(uint8_t);
    }
}
/* --------------------------------------------------------------------------
 * read_u1024_bits
 *
 * Read 1024 bits starting at an arbitrary bit position.
 * Returns a uint1024_t structure with the bits.
 */
static uint1024_t
read_u1024_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos)
{
    uint1024_t result = {0};
    
    for (size_t i = 0; i < 16; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            result.chunk[i] = read_u64_bits(buf, num_bytes, chunk_bit_pos);
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u1024_bits
 *
 * Write 1024 bits starting at an arbitrary bit position.
 */
static void
write_u1024_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint1024_t value)
{
    for (size_t i = 0; i < 16; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            write_u64_bits(buf, num_bytes, chunk_bit_pos, value.chunk[i]);
        }
    }
}
/* --------------------------------------------------------------------------
 * reverse_u1024_bits
 *
 * Reverse the bits in a 1024-bit value using the LUT.
 */
static uint1024_t
reverse_u1024_bits(uint1024_t value)
{
    uint1024_t result = {0};
    
    // Reverse each 8-bit chunk using the LUT
    for (size_t i = 0; i < 16; i++) {
        uint64_t chunk = value.chunk[i];
        uint64_t reversed_chunk = 0;
        
        // Process 8 bytes in the chunk
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte_val = (chunk >> (j * 8)) & 0xFF;
            uint8_t reversed_byte = bit_reverse_table256[byte_val];
            reversed_chunk |= ((uint64_t)reversed_byte) << ((7 - j) * 8);
        }
        
        result.chunk[15 - i] = reversed_chunk;
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * read_u2048_bits
 *
 * Read 2048 bits starting at an arbitrary bit position.
 * Returns a uint2048_t structure with the bits.
 */
static uint2048_t
read_u2048_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos)
{
    uint2048_t result = {0};
    
    for (size_t i = 0; i < 32; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            result.chunk[i] = read_u64_bits(buf, num_bytes, chunk_bit_pos);
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u2048_bits
 *
 * Write 2048 bits starting at an arbitrary bit position.
 */
static void
write_u2048_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint2048_t value)
{
    for (size_t i = 0; i < 32; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            write_u64_bits(buf, num_bytes, chunk_bit_pos, value.chunk[i]);
        }
    }
}
/* --------------------------------------------------------------------------
 * reverse_u2048_bits
 *
 * Reverse the bits in a 2048-bit value using the LUT.
 */
static uint2048_t
reverse_u2048_bits(uint2048_t value)
{
    uint2048_t result = {0};
    
    // Use the 2048-bit LUT for efficient reversal
    // Process 8-bit chunks and use LUT
    for (size_t i = 0; i < 32; i++) {
        uint64_t chunk = value.chunk[i];
        uint64_t reversed_chunk = 0;
        
        // Process 8 bytes in the chunk
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte_val = (chunk >> (j * 8)) & 0xFF;
            uint8_t reversed_byte = bit_reverse_table256[byte_val];
            reversed_chunk |= ((uint64_t)reversed_byte) << ((7 - j) * 8);
        }
        
        result.chunk[31 - i] = reversed_chunk;
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * read_u4096_bits
 *
 * Read 4096 bits starting at an arbitrary bit position.
 * Returns a uint4096_t structure with the bits.
 */
static uint4096_t
read_u4096_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos)
{
    uint4096_t result = {0};
    
    for (size_t i = 0; i < 64; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            result.chunk[i] = read_u64_bits(buf, num_bytes, chunk_bit_pos);
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u4096_bits
 *
 * Write 4096 bits starting at an arbitrary bit position.
 */
static void
write_u4096_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint4096_t value)
{
    for (size_t i = 0; i < 64; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            write_u64_bits(buf, num_bytes, chunk_bit_pos, value.chunk[i]);
        }
    }
}
/* --------------------------------------------------------------------------
 * reverse_u4096_bits
 *
 * Reverse the bits in a 4096-bit value using the LUT.
 */
static uint4096_t
reverse_u4096_bits(uint4096_t value)
{
    uint4096_t result = {0};
    
    // Use the 4096-bit LUT for efficient reversal
    // Process 8-bit chunks and use LUT
    for (size_t i = 0; i < 64; i++) {
        uint64_t chunk = value.chunk[i];
        uint64_t reversed_chunk = 0;
        
        // Process 8 bytes in the chunk
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte_val = (chunk >> (j * 8)) & 0xFF;
            uint8_t reversed_byte = bit_reverse_table256[byte_val];
            reversed_chunk |= ((uint64_t)reversed_byte) << ((7 - j) * 8);
        }
        
        result.chunk[63 - i] = reversed_chunk;
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * read_u8192_bits
 *
 * Read 8192 bits starting at an arbitrary bit position.
 * Returns a uint8192_t structure with the bits.
 */
static uint8192_t
read_u8192_bits(const uint8_t* buf, size_t num_bytes, size_t bit_pos)
{
    uint8192_t result = {0};
    
    for (size_t i = 0; i < 128; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            result.chunk[i] = read_u64_bits(buf, num_bytes, chunk_bit_pos);
        }
    }
    
    return result;
}
/* --------------------------------------------------------------------------
 * write_u8192_bits
 *
 * Write 8192 bits starting at an arbitrary bit position.
 */
static void
write_u8192_bits(uint8_t* buf, size_t num_bytes, size_t bit_pos, uint8192_t value)
{
    for (size_t i = 0; i < 128; i++) {
        size_t chunk_bit_pos = bit_pos + (i * 64);
        if (chunk_bit_pos < num_bytes * 8) {
            write_u64_bits(buf, num_bytes, chunk_bit_pos, value.chunk[i]);
        }
    }
}
/* --------------------------------------------------------------------------
 * reverse_u8192_bits
 *
 * Reverse the bits in a 8192-bit value using the LUT.
 */
static uint8192_t
reverse_u8192_bits(uint8192_t value)
{
    uint8192_t result = {0};
    
    // Use the 8192-bit LUT for efficient reversal
    // Process 8-bit chunks and use LUT
    for (size_t i = 0; i < 128; i++) {
        uint64_t chunk = value.chunk[i];
        uint64_t reversed_chunk = 0;
        
        // Process 8 bytes in the chunk
        for (size_t j = 0; j < 8; j++) {
            uint8_t byte_val = (chunk >> (j * 8)) & 0xFF;
            uint8_t reversed_byte = bit_reverse_table256[byte_val];
            reversed_chunk |= ((uint64_t)reversed_byte) << ((7 - j) * 8);
        }
        
        result.chunk[127 - i] = reversed_chunk;
    }
    
    return result;
}

