#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <semaphore.h>

/*
 * Program used to create threads
 * Threads are woken by the thread_waker
 * @author Aram Alexanian
 */

//initialized semaphore value
#define SEMVALUE 0

//Value to determine if the signal has been received
volatile sig_atomic_t usr1Happened = 0;

//Semaphore variable
sem_t *sem;

//Function called when SIGUSR1 has been received
void handler(int sig);

//Function to be run when threads are made
void childThread();

int main(void) {
	//Setup for sigaction
	struct sigaction sigusr1;
	sigset_t set;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);

	sigusr1.sa_handler = &handler;
	sigusr1.sa_flags = 0;
	sigusr1.sa_mask = set;

	//Sets sigusr1 with sigaction
	if (sigaction(SIGUSR1, &sigusr1, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	//parentPID:	parent's pid
	printf("Parent process: %d\n", getpid());
	//forkNumber:	Number of threads to make
	int threadNumber = 0;

	printf("Enter the number of threads:");
	//Enter the number of threads
	scanf("%d", &threadNumber);

	//Opens the semaphore and creates it
	//assigns it to the sem variable
	sem = sem_open("SEMVALUE", O_CREAT, S_IWOTH, 0);

	//Variable to assign the default thread attributes
	pthread_attr_t attr;
	//Creates all the children
	for (int i = 0; i < threadNumber; i++) {
		//creates default thread attributes
		pthread_attr_init(&attr);
		//Creates threads
		pthread_create(NULL, &attr, &childThread, NULL);
		//destroys the attributes
		pthread_attr_destroy(&attr);
	}

	//Stuck waiting for usr1Happened to change value. Done with SIGUSR1
	while (!usr1Happened){
		sleep(1);
	}


	//When finished destroys the semaphore and unlinks it
	sem_destroy(sem);
	sem_unlink("SEMVALUE");

	printf("Parent process ended.");
	//end of parent process
	return EXIT_SUCCESS;

}

//Changes the value of usr1Happened to 1 when SIGUSR1 signal comes through
void handler(int sig) {
	usr1Happened = 1;
}

//Function that the threads run. Waits for a sem_post command and displays a message
void childThread() {
	//Message to display the thread tid
	printf("Thread created! tid = %d\n", pthread_self());

	while (1) {
		//Performs a sem wait until the semaphore is unlocked
		//On a fail the error is displayed
		while (sem_wait(sem) < 0) {
			perror("Failed to lock semaphore\n");
		}
		//Message when the thread is unlocked with the tid
		printf("Thread has been unblocked! tid = %d\n", pthread_self());
		//Pauses for 5 seconds
		sleep(5);
	}
}
