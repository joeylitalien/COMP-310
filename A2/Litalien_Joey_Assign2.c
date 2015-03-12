/* ===================================================================================== */
/*                                                                                       */
/*                ECSE 427 / COMP 310 OPERATING SYSTEMS --- Assignment 2                 */
/*                             Joey LITALIEN | ID: 260532617                             */
/*                                                                                       */
/* ===================================================================================== */
 
#include <stdlib.h>			/* --------------------------------------------------------- */
#include <stdio.h>			/* This program implements and solves the readers-writers    */
#include <semaphore.h>		/* problem using threads. The threads access a single global */
#include <pthread.h>		/* integer n times, where n is a parameter passed to main(). */
#include <time.h>			/* --------------------------------------------------------- */

#define W_THREADS 10										 /* number of writer threads */
#define R_THREADS 100										 /* number of reader threads */
#define INC 10 							 /* writer increments the shared variable by INC */
#define SLEEP_RANGE 100 	   			  /* each thread sleeps from 0 to SLEEP_RANGE ms */
#define BOLD "\033[1m"										     		 /* text in bold */
#define RST "\033[0m"												 /* reset text style */

static sem_t rw_mutex, mutex;     /* semaphores for read-write and read_count permission */
static int glob = 0;							  /* shared variable accessed by threads */
static int read_count = 0;							   /* number of threads reading glob */

long wMin = 0, wMax = 0, wAvg = 0,			    	 /* waiting times for writer threads */
	 rMin = 0, rMax = 0, rAvg = 0;		 			 /* waiting times for reader threads */

/* ------------------------------------------------------------------------------------- */
/* FUNCTION  writer:                                                                     */
/*    This function is the start_routine() of a writer thread. It reads the shared       */
/* variable glob and incrememt its value by INC.			       						 */
/* ------------------------------------------------------------------------------------- */
static void *writer(void *arg)
{
	int loops = *(int *) arg;								     /* number of iterations */
	int j;
	struct timeval t0, t1;			   /* initial and final time to compute elapsed time */
	long waitTime;											/* total time a thread waits */
	srand(time(NULL));							   /* initialize random number generator */

	for (j = 0; j < loops; j++) {
		gettimeofday(&t0, NULL);									/* take initial time */
		if (sem_wait(&rw_mutex) == -1) exit(2);			    /* wait for write permission */
		gettimeofday(&t1, NULL);									  /* take final time */
		waitTime += (t1.tv_sec - t0.tv_sec) * 1e06L; 	  	   		/* compute wait time */
   		waitTime += (t1.tv_usec - t0.tv_usec);

		glob += INC;									     /* WRITE to shared variable */

	    if (sem_post(&rw_mutex) == -1) exit(2);	  /* resume execution of threads waiting */
	   	usleep((rand() % SLEEP_RANGE) * 1000);			/* sleep a random amount of time */
	}
	return (void *)waitTime;
}

/* ------------------------------------------------------------------------------------- */
/* FUNCTION  reader:                                                                     */
/*    This function is the start_routine() of a reader thread. It only reads the shared  */
/* variable glob.																		 */
/* ------------------------------------------------------------------------------------- */
static void *reader(void *arg)
{
	int loops = *(int *) arg;									 /* number of iterations */
	int j, peek;
	struct timeval t0, t1;			   /* initial and final time to compute elapsed time */
	long waitTime = 0;								  		/* total time a thread waits */
	srand(time(NULL));							   /* initialize random number generator */

	for (j = 0; j < loops; j++) {
		gettimeofday(&t0, NULL);								    /* take initial time */
		if (sem_wait(&mutex) == -1) exit(2);		   /* wait for read_count permission */
	   	read_count++;				    /* when permission is granted, update read_count */
	   	if (read_count == 1) {									   /* if thread is first */
	   		if (sem_wait(&rw_mutex) == -1) exit(2);								 /* wait */
	   	}
	   	gettimeofday(&t1, NULL);									  /* take final time */
	   	waitTime += (t1.tv_sec - t0.tv_sec) * 1e06L; 	  	 	    /* compute wait time */
   		waitTime += (t1.tv_usec - t0.tv_usec);
	   	if (sem_post(&mutex) == -1) exit(2);      /* resume execution of threads waiting */

	   	peek = glob;									      /* READ to shared variable */

	    if (sem_wait(&mutex) == -1) exit(2);		   /* wait for read_count permission */
	    read_count--;											    /* update read_count */
	    if (read_count == 0) {									 /* if no one is waiting */
	    	if (sem_post(&rw_mutex) == -1) exit(2);		  /* resume execution of threads */
	    }
	   	if (sem_post(&mutex) == -1) exit(2);	  /* resume execution of threads waiting */
	   	usleep((rand() % SLEEP_RANGE) * 1000);			/* sleep a random amount of time */
	}
	return (void *)waitTime;
}

/* ------------------------------------------------------------------------------------- */
/*                                Main program starts here                               */
/* ------------------------------------------------------------------------------------- */
int  main(int argc, char *argv[])
{
	pthread_t  writers[W_THREADS];				   /* initialize array of writer threads */
	pthread_t  readers[R_THREADS];				   /* initialize array of reader threads */
	int loops = atoi(argv[1]);                     /* get number of iterations from user */
	void *wStatus, *rStatus;
	double wSum = 0, rSum = 0;

	if ((sem_init(&rw_mutex, 0, 1) == -1) || 				    /* initialize semaphores */
		(sem_init(&mutex, 0, 1) == -1)) { 
    	printf("error: init semaphores\n");
    	exit(1);
    }

    int i;
    for (i = 0; i < W_THREADS; i++) {						    /* create writer threads */
    	if (pthread_create(&writers[i], NULL, writer, &loops) != 0) {
    		printf("error: creating writer threads\n");
    		exit(1);
    	}
    }
    for (i = 0; i < R_THREADS; i++) {							/* create reader threads */
    	if (pthread_create(&readers[i], NULL, reader, &loops) != 0) {
    		printf("error: creating reader threads\n");
    		exit(1);
    	}
    }

	for (i = 0; i < W_THREADS; i++)	{	 	    /*suspend execution of the threads until */
    	if (pthread_join(writers[i], &wStatus) != 0) { 		/* threads return wait times */
        	printf("error: joining writer threads\n");
          	exit(1);
        }
    	long wTime = (long)wStatus;						    /* convert wait time to long */
    	wSum += wTime;									/* update sum to compute average */
    	if (i == 0) {						         /* initialize min and max variables */
   			wMin = wTime;
   			wMax = wTime;
   		}
    	if (wTime < wMin) wMin = wTime;						 /* update minimum wait time */
    	if (wTime > wMax) wMax = wTime;						 /* update maximum wait time */
    }
    wAvg = wSum / W_THREADS;						    /* compute average of wait times */

    for (i = 0; i < R_THREADS; i++) {
    	if (pthread_join(readers[i], &rStatus) != 0) { 		 /* threads return wait time */
    		printf("error: joining reader threads\n");
          	exit(1);
        }
    	long rTime = (long)rStatus;							/* convert wait time to long */
    	rSum += rTime;									/* update sum to compute average */
    	if (i == 0) {								 /* initialize min and max variables */
   			rMin = rTime;
   			rMax = rTime;
   		}
    	if (rTime < rMin) rMin = rTime;					     /* update minimum wait time */
    	if (rTime > rMax) rMax = rTime;						 /* update maximum wait time */
    }
    rAvg = rSum / R_THREADS;						    /* compute average of wait times */

    printf(BOLD "\nReaders-Writers Problem\n" RST);			  /* print the values nicely */
    printf("%d writers, %d readers, %d iterations\n", W_THREADS, R_THREADS,  loops);
    printf("Time displayed in microseconds (ms)\n");
    printf("-------------------------------------------------\n");
    printf(BOLD "Threads       Minimum       Maximum       Average\n" RST);
    printf("Writers    %10lu    %10lu    %10lu\n", wMin, wMax, wAvg);
    printf("Readers    %10lu    %10lu    %10lu\n", rMin, rMax, rAvg);
    printf("-------------------------------------------------\n");
    printf("Shared variable value: %d\n\n", glob);

  	exit(0);													    /* exit successfully */
}