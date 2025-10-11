#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "./bitarray.h"

// Function to print bit array as binary string
void print_bitarray(bitarray_t* ba, char* buffer) {
    size_t bit_sz = bitarray_get_bit_sz(ba);
    for (size_t i = 0; i < bit_sz; i++) {
        buffer[i] = bitarray_get(ba, i) ? '1' : '0';
    }
    buffer[bit_sz] = '\0';
}

// Generate test case for large arrays
void generate_large_array_test(FILE* f, int test_num, size_t bit_size, ssize_t rotation_amount) {
    fprintf(f, "# Test %d: Large array (%zu bits) rotation by %zd\n", test_num, bit_size, rotation_amount);
    fprintf(f, "t %d\n", test_num);
    
    // Create bitarray
    bitarray_t* ba = bitarray_new(bit_size);
    bitarray_randfill(ba);
    
    // Print initial state
    char* buffer = malloc(bit_size + 1);
    print_bitarray(ba, buffer);
    fprintf(f, "n %s\n", buffer);
    
    // Perform rotation
    bitarray_rotate(ba, 0, bit_size, rotation_amount);
    
    // Print expected result
    print_bitarray(ba, buffer);
    fprintf(f, "e %s\n", buffer);
    fprintf(f, "\n");
    
    free(buffer);
    bitarray_free(ba);
}

// Generate edge case tests
void generate_edge_case_tests(FILE* f) {
    fprintf(f, "# Advanced Edge Cases for Triple Reverse Rotation\n");
    fprintf(f, "# Generated automatically\n\n");
    
    int test_num = 0;
    
    // Test 1: Single bit rotation
    fprintf(f, "# Test %d: Single bit rotation\n", test_num);
    fprintf(f, "t %d\n", test_num++);
    fprintf(f, "n 1\n");
    fprintf(f, "r 0 1 1\n");
    fprintf(f, "e 1\n\n");
    
    // Test 2: Two bits rotation
    fprintf(f, "# Test %d: Two bits rotation\n", test_num);
    fprintf(f, "t %d\n", test_num++);
    fprintf(f, "n 10\n");
    fprintf(f, "r 0 2 1\n");
    fprintf(f, "e 01\n\n");
    
    // Test 3: Prime length arrays (edge cases for triple reverse)
    size_t prime_lengths[] = {3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
    for (int i = 0; i < 10; i++) {
        generate_large_array_test(f, test_num++, prime_lengths[i], 1);
    }
    
    // Test 4: Powers of 2 (cache-friendly sizes)
    size_t power2_lengths[] = {8, 16, 32, 64, 128, 256, 512, 1024};
    for (int i = 0; i < 8; i++) {
        generate_large_array_test(f, test_num++, power2_lengths[i], 3);
    }
    
    // Test 5: Large arrays using our LUT thresholds
    size_t lut_thresholds[] = {500, 5000, 50000, 250000, 500000, 2500000, 10000000, 50000000, 100000000};
    for (int i = 0; i < 9; i++) {
        generate_large_array_test(f, test_num++, lut_thresholds[i], 7);
    }
    
    // Test 6: Cross-boundary rotations
    fprintf(f, "# Test %d: Cross-byte boundary rotation\n", test_num);
    fprintf(f, "t %d\n", test_num++);
    fprintf(f, "n 1111111100000000\n");
    fprintf(f, "r 0 16 8\n");
    fprintf(f, "e 0000000011111111\n\n");
    
    // Test 7: Multiple rotations
    fprintf(f, "# Test %d: Multiple rotations\n", test_num);
    fprintf(f, "t %d\n", test_num++);
    fprintf(f, "n 10101010\n");
    fprintf(f, "r 0 8 2\n");
    fprintf(f, "e 10101010\n");
    fprintf(f, "r 0 8 4\n");
    fprintf(f, "e 10101010\n\n");
}

int main() {
    srand(time(NULL));
    
    FILE* f = fopen("tests/advanced_edge_cases", "w");
    if (!f) {
        printf("Error: Could not create test file\n");
        return 1;
    }
    
    fprintf(f, "# Copyright (c) 2012 MIT License by 6.172 Staff\n");
    fprintf(f, "# Advanced Edge Cases for Triple Reverse Rotation\n");
    fprintf(f, "# Generated automatically using metaprogramming\n\n");
    
    generate_edge_case_tests(f);
    
    fclose(f);
    printf("Generated advanced test cases in tests/advanced_edge_cases\n");
    return 0;
}
