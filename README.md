# Hardware


# Arquitetura:                          x86_64
Modo(s) operacional da CPU:           32-bit, 64-bit
Ordem dos bytes:                      Little Endian
Address sizes:                        39 bits physical, 48 bits virtual
CPU(s):                               8
Lista de CPU(s) on-line:              0-7
Thread(s) per núcleo:                 2
Núcleo(s) por soquete:                4

## Software inicial

Multiplicação igual linear linha de A por coluna de  B 

make: Nada a ser feito para 'all'.

real    0m28,142s
user    0m28,088s
sys     0m0,036s
=== Running test with 1 threads ===

real    0m29,198s
user    0m29,130s
sys     0m0,052s
=== Running test with 2 threads ===

real    0m23,251s
user    0m32,567s
sys     0m0,040s
=== Running test with 3 threads ===

real    0m21,038s
user    0m34,073s
sys     0m0,044s
=== Running test with 4 threads ===

real    0m20,737s
user    0m38,956s
sys     0m0,040s
=== Running test with 5 threads ===

real    0m28,703s
user    0m56,062s
sys     0m0,096s
=== Running test with 6 threads ===

real    0m34,964s
user    1m13,219s
sys     0m0,112s
=== Running test with 7 threads ===

real    0m31,871s
user    1m24,868s
sys     0m0,176s
=== Running test with 8 threads ===

real    0m30,494s
user    1m24,554s
sys     0m0,195s
=== Running test with 9 threads ===

real    0m29,595s
user    1m22,242s
sys     0m0,163s
=== Running test with 10 threads ===

real    0m28,431s
user    1m22,437s
sys     0m0,156s



Performance counter stats for './main 1000 4 1':

          8.037,22 msec task-clock                #    1,576 CPUs utilized          
               522      context-switches          #   64,948 /sec                   
                36      cpu-migrations            #    4,479 /sec                   
             9.870      page-faults               #    1,228 K/sec                  
    34.302.945.965      cycles                    #    4,268 GHz                    
    69.489.499.393      instructions              #    2,03  insn per cycle         
     2.086.098.037      branches                  #  259,555 M/sec                  
         2.284.499      branch-misses             #    0,11% of all branches        
   155.715.353.080      slots                     #   19,374 G/sec                  
    65.507.701.726      topdown-retiring          #     40,0% retiring              
    17.809.705.441      topdown-bad-spec          #     10,9% bad speculation       
     5.747.694.705      topdown-fe-bound          #      3,5% frontend bound        
    74.594.846.357      topdown-be-bound          #     45,6% backend bound         

       5,099770677 seconds time elapsed

       8,009641000 seconds user
       0,028019000 seconds sys