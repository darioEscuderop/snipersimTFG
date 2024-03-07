#include "stdio.h"
#include "stdlib.h"
#include <iostream>
#include <chrono>
#include <sim_api.h>

#define SIZE_DEF 10000000000
#define TOP_ITERATIONS 1

__attribute__((noinline))
void _kernel_(int* A, int* B, uint64_t top_iterations, uint64_t inner_iterations, int tile_id, int num_tiles) {
  int tmp = 0;
#pragma nounroll
  for (uint64_t j = 0; j < top_iterations; j++) {
#pragma nounroll
    for (uint64_t i = 0; i < inner_iterations; i++) {
      A[B[i]]+=1;
//      A[i]=1;
//      B[i]=2;
    }
  }
}

int main() {
  uint64_t i, SIZE = SIZE_DEF;
  int * A = (int *) malloc(SIZE * sizeof(int));
  int * B = (int *) malloc(SIZE * sizeof(int));
  
  printf("%ld\n", SIZE);
  srand(0);
  for (i = 0; i < SIZE; i++) {
    A[i] = 0;
//    B[i] = rand() % 16; // Add to make accesses regular
    B[i] = rand() % SIZE;
  }
  auto start = std::chrono::system_clock::now();
  printf("Go!\n");
//SimRoiStart();
  _kernel_(A,B, TOP_ITERATIONS, SIZE, 0, 1);
//SimRoiEnd();

  auto end = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = end-start;
  std::cout << "Time: " << elapsed_seconds.count() << "s\n";
  free(A);
  free(B);
  printf("Finished!\n");
  return 0;
}