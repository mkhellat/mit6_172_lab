// Copyright (c) 2012 MIT License by 6.172 Staff

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Macro to print size of types
#define PRINT_SIZE(type) \
  printf("size of %s : %zu bytes \n", #type, sizeof(type))

#define TYPE_LIST    \
    X(int)   X(int *)   X(short) X(short *)   \
    X(long)  X(long *)  X(char)  X(char *)    \
    X(float) X(float *) X(double) X(double *) \
    X(unsigned int)     X(unsigned int *)     \
    X(long long)     X(long long *)           \
    X(uint8_t)       X(uint8_t *)             \
    X(uint16_t)      X(uint16_t *)            \
    X(uint32_t)      X(uint32_t *)            \
    X(uint64_t)      X(uint64_t *)            \
    X(uint_fast8_t)  X(uint_fast8_t *)        \
    X(uint_fast16_t) X(uint_fast16_t *)       \
    X(uintmax_t) X(uintmax_t *)               \
    X(intmax_t)  X(intmax_t *)                \
    X(__int128) X(__int128 *)

#define X(type) PRINT_SIZE(type);
void print_sizes () {
  TYPE_LIST;
}
#undef X

int main() {
  // Please print the sizes of the following types:
  // int, short, long, char, float, double, unsigned int, long long
  // uint8_t, uint16_t, uint32_t, and uint64_t, uint_fast8_t,
  // uint_fast16_t, uintmax_t, intmax_t, __int128, and student

  // Here's how to show the size of one type. See if you can define a macro
  // to avoid copy pasting this code.
  //printf("size of %s : %zu bytes \n", "int", sizeof(int));
  // e.g. PRINT_SIZE("int", int);
  //      PRINT_SIZE("short", short);
  

  // Alternatively, you can use stringification
  // (https://gcc.gnu.org/onlinedocs/cpp/Stringification.html) so that
  // you can write
  // e.g. PRINT_SIZE(int);
  //      PRINT_SIZE(short);
  print_sizes();

  // Composite types have sizes too.
  typedef struct {
    int id;
    int year;
  } student;

  student you;
  you.id = 12345;
  you.year = 4;


  // Array declaration. Use your macro to print the size of this.
  int x[5];
  PRINT_SIZE(x);
  PRINT_SIZE(&x);

  // You can just use your macro here instead: PRINT_SIZE("student", you);
  PRINT_SIZE(student);
  PRINT_SIZE(&you);

  return 0;
}
