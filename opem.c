#include <stdio.h>
#include <omp.h>

int main(void) {
    int a = 10;
    #pragma omp parallel // clausa
    {
        printf("d/n", a);
    }

    return 0;
}