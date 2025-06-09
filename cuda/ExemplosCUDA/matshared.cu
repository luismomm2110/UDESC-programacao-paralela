__global__ void matrixMulKernel (float *m, float *n, float *p, int width)
{
    __shared__ float sm[TILE_WIDTH][TILE_WIDTH];
    __shared__ float sn[TILE_WIDTH][TILE_WIDTH];

    int row = blockIdx.y * TILE_WIDTH + threadIdx.y;
    int col = blockIdx.x * TILE_WIDTH + threadIdx.x;

    float pvalue = 0;

    for (int i = 0; i < width/TILE_WIDTH; i++) {
        sm[threadIdx.y][threadIdx.x] = m[row * width + (i * TILE_WIDTH + threadIdx.x)];
        sn[threadIdx.y][threadIdx.x] = n[col + (i * TILE_WIDTH + threadIdx.y) * width];
        __syncthreads();

        for (int k = 0; k < TILE_WIDTH; k++) {
            pvalue += sm[threadIdx.y][k] * sn[k][threadIdx.x];
        }
        __syncthreads();

    }
    p[row * width + col] = pvalue;
}
