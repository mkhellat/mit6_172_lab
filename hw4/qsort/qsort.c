#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <cilk/cilk.h>

#ifdef CILKSCALE
#include <cilk/cilkscale.h>
#endif

typedef uint32_t data_t;

// A utility function to swap two elements
void swap(data_t* a, data_t* b) {
    data_t t = *a;
    *a = *b;
    *b = t;
}

/* This function is same in both iterative and recursive*/
int partition(data_t arr[], int l, int h) {
    data_t x = arr[h];
    int i = (l - 1);

    for (int j = l; j <= h - 1; j++) {
        if (arr[j] <= x) {
            i++;
            swap(&arr[i], &arr[j]);
        }
    }
    swap(&arr[i + 1], &arr[h]);
    return (i + 1);
}

/* arr[] --> Array to be sorted,
   l  --> Starting index,
   h  --> Ending index */
void quickSort(data_t arr[], int l, int h) {
    if (l < h) {
        // Set pivot element at its correct position
        int p = partition(arr, l, h);

        // Parallelize both recursive calls - no race condition
        // Each call operates on disjoint array regions
        cilk_spawn quickSort(arr, l, p - 1);
        quickSort(arr, p + 1, h);
        cilk_sync;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <size> [seed]\n", argv[0]);
        return 1;
    }

    int size = atoi(argv[1]);
    int seed = (argc > 2) ? atoi(argv[2]) : 1;
    srand(seed);

    data_t* arr = (data_t*)malloc(size * sizeof(data_t));
    if (arr == NULL) {
        printf("Failed to allocate memory\n");
        return 1;
    }

    // Initialize array with random values
    for (int i = 0; i < size; i++) {
        arr[i] = rand() % 1000;
    }

    // Sort the array
    quickSort(arr, 0, size - 1);

#ifdef CILKSCALE
    // Print Cilkscale scalability information
    print_total();
#endif

    // Verify sorted (optional check)
    int sorted = 1;
    for (int i = 1; i < size; i++) {
        if (arr[i - 1] > arr[i]) {
            sorted = 0;
            break;
        }
    }

    if (sorted) {
        printf("Array is sorted\n");
    } else {
        printf("Array is NOT sorted!\n");
    }

    free(arr);
    return 0;
}

