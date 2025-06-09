#include <unistd.h>
#include <ctype.h>
#include <stdio.h>

#define N 16
#define CORES 16 

__global__ void hello(char* s) {
  if (isupper(s[blockIdx.x])) {
    s[blockIdx.x] = toupper(s[blockIdx.x]);
  }
}

int main( ) {
  char cpu_string[N] = "hello world!";

  char* gpu_string;
  cudaMalloc((void**) &gpu_string, N * sizeof(char));

  cudaMemcpy(gpu_string, cpu_string, N * sizeof(char), cudaMemcpyHostToDevice);
  
  hello<<<CORES, 1>>>(gpu_string);

  cudaMemcpy(cpu_string, gpu_string, N * sizeof(char), cudaMemcpyDeviceToHost);
  
  cudaFree(gpu_string);
  
  printf("%s\n", cpu_string);

  return 0;
}
