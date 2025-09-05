#include <iostream>
using namespace std;

const int N = 16;
const int CORES = 16;

__global__ void hello(char* s) {
  if ((s[blockIdx.x] >= 'a') && (s[blockIdx.x] <= 'z')) {
    s[blockIdx.x] -= 32;
  }
}

int main( ) {
  char cpu_string[N] = "hello world!";

  char* gpu_string;
  cudaMalloc((void**) &gpu_string, N * sizeof(char));

  cudaMemcpy(gpu_string, cpu_string, N * sizeof(char), cudaMemcpyHostToDevice);
  
  hello<<<CORES, 1>>>(gpu_string);
  //hello<<<1, CORES>>>(gpu_string);

  cudaMemcpy(cpu_string, gpu_string, N * sizeof(char), cudaMemcpyDeviceToHost);
  
  cudaFree(gpu_string);
  
  cout << cpu_string << endl;

  return 0;
}
