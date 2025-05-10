/******************************************************************************
* FILE: hello.c
* DESCRIPTION:
*   A "hello world" Pthreads program.  Demonstrates thread creation and
*   termination.
* AUTHOR: Blaise Barney
* LAST REVISED: 08/09/11
******************************************************************************/
#include <pthread.h> //lib para as threads
#include <stdio.h>
#include <stdlib.h>


void           *
PrintHello(void *threadid)
{
	long		tid;
	tid = (long)threadid;
	printf("Hello World! It's me, thread #%ld!\n", tid);
	pthread_exit(NULL);
}

int 
main(int argc, char *argv[])
{
    int NUM_THREADS = atoi(argv[1]); //quantidade de threads

	pthread_t	threads[NUM_THREADS]; //[ X ] [  ] [  ] [  ]
	int		rc;
	long		t;
	for (t = 0; t < NUM_THREADS; t++) { // para cada thread
		printf("In main: creating thread %ld\n", t); //main executando

        //pthread_create(&thread, NULL, funcao, argumentos da funcao);)
		rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
        //aqui ja tem uma thread executando
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	/* Last thing that main() should do */
	pthread_exit(NULL); //quantos fluxos de execucao temos? poscomp (2 anos)
}