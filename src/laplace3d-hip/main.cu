//
// Program to solve Laplace equation on a regular 3D grid
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <algorithm>
#include <chrono>
#include <hip/hip_runtime.h>
#include "kernel.h"
#include "reference.h"

// declaration, forward

void reference(int NX, int NY, int NZ, float* h_u1, float* h_u2);

void printHelp(void);

// Main program

int main(int argc, char **argv){

  // check command line inputs
  if(argc != 6) {
    printHelp();
    return 1;
  }

  const int NX = atoi(argv[1]);
  const int NY = atoi(argv[2]);
  const int NZ = atoi(argv[3]);
  const int REPEAT = atoi(argv[4]);
  const int verify = atoi(argv[5]);

  // NX is a multiple of 32
  if (NX <= 0 || NX % 32 != 0 || NY <= 0 || NZ <= 0 || REPEAT <= 0) return 1;

  printf("\nGrid dimensions: %d x %d x %d\n", NX, NY, NZ);
  printf("Result verification %s \n", verify ? "enabled" : "disabled");
 
  // allocate memory for arrays

  const size_t grid3D_size = NX * NY * NZ ;
  const size_t grid3D_bytes = grid3D_size * sizeof(float);

  float *h_u1 = (float *) malloc (grid3D_bytes);
  float *h_u2 = (float *) malloc (grid3D_bytes);
  float *h_u3 = (float *) malloc (grid3D_bytes);

  float *d_u1, *d_u2;
  hipMalloc((void **)&d_u1, grid3D_bytes);
  hipMalloc((void **)&d_u2, grid3D_bytes);

  const int pitch = NX;

  // initialise u1
  int i, j, k;
    
  for (k=0; k<NZ; k++) {
    for (j=0; j<NY; j++) {
      for (i=0; i<NX; i++) {
        int ind = i + j*NX + k*NX*NY;
        if (i==0 || i==NX-1 || j==0 || j==NY-1|| k==0 || k==NZ-1)
          h_u1[ind] = 1.0f;           // Dirichlet b.c.'s
        else
          h_u1[ind] = 0.0f;
      }
    }
  }

  // copy u1 to device
  hipMemcpy(d_u1, h_u1, grid3D_bytes, hipMemcpyHostToDevice);

  // Set up the execution configuration
  const int bx = 1 + (NX-1)/BLOCK_X;
  const int by = 1 + (NY-1)/BLOCK_Y;

  dim3 dimGrid(bx,by);
  dim3 dimBlock(BLOCK_X,BLOCK_Y);

  printf("\ndimGrid  = %d %d %d \n", bx, by, 1);
  printf("dimBlock = %d %d %d \n", BLOCK_X, BLOCK_Y, 1);

  // Warmup
  hipLaunchKernelGGL(laplace3d, dimGrid, dimBlock, 0, 0, NX, NY, NZ, pitch, d_u1, d_u2);
  hipDeviceSynchronize();

  // Execute GPU kernel
  auto start = std::chrono::steady_clock::now();

  for (i = 1; i <= REPEAT; ++i) {
    hipLaunchKernelGGL(laplace3d, dimGrid, dimBlock, 0, 0, NX, NY, NZ, pitch, d_u1, d_u2);
    std::swap(d_u1, d_u2);
  }

  hipDeviceSynchronize();
  auto end = std::chrono::steady_clock::now();
  auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
  printf("Average kernel execution time: %f (s)\n", (time * 1e-9f) / REPEAT);
  printf("Total kernel execution time: %f (s)\n", (time * 1e-9f));

  // Read back GPU results
  hipMemcpy(h_u2, d_u1, grid3D_bytes, hipMemcpyDeviceToHost);

  if (verify) {
    // Reference
    for (i = 1; i <= REPEAT; ++i) {
      reference(NX, NY, NZ, h_u1, h_u3);
      std::swap(h_u1, h_u3);
    }

    // verify (may take long for large grid sizes)
    float err = 0.f;
    for (k=0; k<NZ; k++) {
      for (j=0; j<NY; j++) {
        for (i=0; i<NX; i++) {
          int ind = i + j*NX + k*NX*NY;
          err += (h_u1[ind]-h_u2[ind])*(h_u1[ind]-h_u2[ind]);
        }
      }
    }
    printf("\n rms error = %f \n", sqrtf(err/ NX*NY*NZ));
  }

 // Release GPU and CPU memory
  hipFree(d_u1);
  hipFree(d_u2);
  free(h_u1);
  free(h_u2);
  free(h_u3);

  return 0;
}

//Print help screen
void printHelp(void)
{
  printf("Usage:  laplace3d [OPTION]...\n");
  printf("6-point stencil 3D Laplace test \n");
  printf("\n");
  printf("Example: run 100 iterations on a 256x128x128 grid\n");
  printf("./main 256 128 128 100 1\n");

  printf("\n");
  printf("Options:\n");
  printf("Grid width\n");
  printf("Grid height\n");
  printf("Grid depth\n");
  printf("Number of repetitions\n");
  printf("verify the result\n");
}
