/******************************************************************************
* FILE: hello_arg2.c
* DESCRIPTION:
*   A "hello world" Pthreads program which demonstrates another safe way
*   to pass arguments to threads during thread creation.  In this case,
*   a structure is used to pass multiple arguments.
* AUTHOR: Blaise Barney
* LAST REVISED: 01/29/09
******************************************************************************/
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_THREADS	8

//char           *messages[NUM_THREADS]; //global
//[ptr][][][]
//ptr -> ola mundo

struct thread_data { 
    int		thread_id; 
	int		sum; 
	char           *message; 
};


void           *
PrintHello(void *threadarg)
{
	int		taskid    , sum; //escopo local
	char           *hello_msg;
	struct thread_data *my_data;

	sleep(2); //dorme 2 segundos
	my_data = (struct thread_data *)threadarg; //void * --> struct *  // transforma void * em struct com os dados
	taskid = my_data->thread_id; //(*my_data).thread_id
	sum = my_data->sum;
	hello_msg = my_data->message;
	printf("Thread %d: %s  Sum=%d\n", taskid, hello_msg, sum);
	pthread_exit(NULL);
}

int 
main(int argc, char *argv[])
{
	pthread_t	threads[NUM_THREADS];       //[0][1][2][3][4]...[7] threads
    //int vetor[NUM_THREADS];
    struct thread_data thread_data_array[NUM_THREADS]; //[0][1][2][3]..[7]
    //[0] = thread_id, sum, char *
    //[1] = int, int, char *
    char           *messages[NUM_THREADS]; //[pont][pont]
    //char messages[NUM};
    //[c][c][c]
	int		rc        , t, sum;

	sum = 0; //sum esta visivel para as threads? NAO. Esta no escopo da main
	messages[0] = "English: Hello World!"; //SIM. global
	messages[1] = "French: Bonjour, le monde!";
	messages[2] = "Spanish: Hola al mundo";
	messages[3] = "Klingon: Nuq neH!";
	messages[4] = "German: Guten Tag, Welt!";
	messages[5] = "Russian: Zdravstvytye, mir!";
	messages[6] = "Japan: Sekai e konnichiwa!";
	messages[7] = "Latin: Orbis, te saluto!";

	for (t = 0; t < NUM_THREADS; t++) { //para cada thread
		sum = sum + t; // interna na main
        //aqui chegou o exemplo!
		thread_data_array[t].thread_id = t; //informacao foi copiada ou ponteiro (ref)?
		thread_data_array[t].sum = sum; //copia
		thread_data_array[t].message = messages[t]; //ponteiro 
        //fim dados

		printf("Creating thread %d\n", t);
		rc = pthread_create(&threads[t], NULL, PrintHello, (void *)
				    &thread_data_array[t]);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
	pthread_exit(NULL);
}