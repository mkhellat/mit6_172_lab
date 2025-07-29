// Copyright (c) 2012 MIT License by 6.172 Staff

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

void swap(int * i, int * j) {
  int temp = *i;
  //printf("-------------------- temp = *i --------------------\n");
  //printf("    i ( address - value ) --> ( %p - %d ) \n", i, *i);
  //printf("    j ( address - value ) --> ( %p - %d ) \n", j, *j);
  //printf(" temp ( address - value ) --> ( %p - %d ) \n", &temp, temp);
  *i = *j;
  //printf("---------------------- *i = *j ---------------------\n");
  //printf("->> i ( address - value ) --> ( %p - %d ) \n", i, *i);
  //printf("->> j ( address - value ) --> ( %p - %d ) \n", j, *j);
  //printf(" temp ( address - value ) --> ( %p - %d ) \n", &temp, temp);
  *j = temp;
  //printf("-------------------- *j = temp --------------------\n");
  //printf("<<- i ( address - value ) --> ( %p - %d ) \n", i, *i);
  //printf("<<- j ( address - value ) --> ( %p - %d ) \n", j, *j);
  /printf(" temp ( address - value ) --> ( %p - %d ) \n", &temp, temp);
}

int main() {
  int k = 1;
  int m = 2;
  swap(&k, &m);
  // What does this print?
  printf("k = %d, m = %d\n", k, m);

  return 0;
}
