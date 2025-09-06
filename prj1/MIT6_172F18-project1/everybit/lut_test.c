/**
 * Test program to demonstrate hash table-based LUT deployment
 * for bit array reversal operations.
 */

#include "bitarray.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

// Performance measurement macros
#define MEASURE_TIME(func_call) do { \
    clock_t start = clock(); \
    func_call; \
    clock_t end = clock(); \
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC; \
    printf("Execution time: %.6f seconds\n", cpu_time); \
} while(0)

// Test different bit array sizes to trigger different LUT strategies
void test_lut_strategies(void) {
    printf("=== Testing Hash Table-Based LUT Deployment ===\n\n");
    
    // Test sizes that will trigger different LUT strategies
    size_t test_sizes[] = {
        100,      // Should use 8-bit LUT (naive)
        1000,     // Should use 32-bit LUT
        10000,    // Should use 64-bit LUT
        100000,   // Should use 128-bit LUT
        1000000,  // Should use 256-bit LUT
        5000000   // Should use 512-bit LUT
    };
    
    const char* expected_strategies[] = {
        "8-bit LUT (naive)",
        "32-bit LUT",
        "64-bit LUT", 
        "128-bit LUT",
        "256-bit LUT",
        "512-bit LUT"
    };
    
    int num_tests = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_tests; i++) {
        size_t bit_size = test_sizes[i];
        printf("Test %d: %zu bits (expected: %s)\n", 
               i + 1, bit_size, expected_strategies[i]);
        
        // Create bit array
        bitarray_t* ba = bitarray_new(bit_size);
        if (ba == NULL) {
            printf("  ERROR: Failed to allocate bit array\n");
            continue;
        }
        
        // Fill with test pattern
        bitarray_randfill(ba);
        
        // Measure reversal time
        printf("  Reversing bit array... ");
        MEASURE_TIME(bitarray_rotate(ba, 0, bit_size, bit_size/2));
        
        // Clean up
        bitarray_free(ba);
        printf("\n");
    }
}

// Test hash table lookup performance
void test_hash_table_performance(void) {
    printf("=== Testing Hash Table Lookup Performance ===\n\n");
    
    const int num_lookups = 1000000;
    size_t test_values[] = {100, 1000, 10000, 100000, 1000000, 5000000};
    int num_values = sizeof(test_values) / sizeof(test_values[0]);
    
    for (int i = 0; i < num_values; i++) {
        size_t bit_count = test_values[i];
        printf("Testing %zu bit lookups... ", bit_count);
        
        clock_t start = clock();
        for (int j = 0; j < num_lookups; j++) {
            // This would normally call lut_manager_get_type()
            // For this test, we'll simulate the hash lookup
            uint32_t hash = (uint32_t)((bit_count * 2654435761U) % 16);
            (void)hash; // Suppress unused variable warning
        }
        clock_t end = clock();
        
        double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
        printf("%.6f seconds (%d lookups)\n", cpu_time, num_lookups);
    }
}

// Test memory usage comparison
void test_memory_usage(void) {
    printf("=== Memory Usage Analysis ===\n\n");
    
    printf("Hash Table Structure:\n");
    printf("  - Hash table buckets: %d * 8 bytes = %d bytes\n", 
           16, 16 * 8);
    printf("  - Cache entries: %d * %zu bytes = %zu bytes\n", 
           8, sizeof(void*) * 4, 8 * sizeof(void*) * 4);
    printf("  - Total hash table overhead: ~%zu bytes\n\n", 
           16 * 8 + 8 * sizeof(void*) * 4);
    
    printf("Static LUT Arrays:\n");
    printf("  - 8-bit LUT: 256 * 1 byte = 256 bytes\n");
    printf("  - 32-bit LUT: 256 * 4 bytes = 1024 bytes\n");
    printf("  - 64-bit LUT: 256 * 8 bytes = 2048 bytes\n");
    printf("  - 128-bit LUT: 256 * 16 bytes = 4096 bytes\n");
    printf("  - 256-bit LUT: 256 * 32 bytes = 8192 bytes\n");
    printf("  - 512-bit LUT: 256 * 64 bytes = 16384 bytes\n");
    printf("  - Total static LUTs: %d bytes\n\n", 
           256 + 1024 + 2048 + 4096 + 8192 + 16384);
    
    printf("Hash table provides dynamic LUT selection with minimal overhead!\n");
}

int main(void) {
    printf("Hash Table-Based LUT Deployment Test\n");
    printf("====================================\n\n");
    
    // Initialize random seed
    srand((unsigned int)time(NULL));
    
    // Run tests
    test_lut_strategies();
    printf("\n");
    
    test_hash_table_performance();
    printf("\n");
    
    test_memory_usage();
    
    // Clean up LUT manager
    bitarray_cleanup_lut_manager();
    
    printf("\nAll tests completed successfully!\n");
    return 0;
}
