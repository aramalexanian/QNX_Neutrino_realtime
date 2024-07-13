#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>

/*
 * Program used to send sem_post to an existing semaphore
 * The sem_post are used to wake up threads that are waiting on the semaphore to be unlocked
 * Works in conjunction with thread_factory.c
 * @author Aram Alexanian
 */

//Default value of the semaphore
#define SEMVALUE 0

//The semaphore
sem_t *sem;

int main(void) {
	//Number of threads to wake up
	int numWakeup = 1;
	//Opens the semaphore
	//Assigns it to the sem variable
	sem = sem_open("SEMVALUE", 0);

	//Message to display the pid
	printf("thread-waker: %d\n", getpid());

	//Loops until numWakeup is 0
	while (numWakeup != 0) {
		//Enter the number of threads to wakeup
		printf("How many threads do you want to wake up (enter 0 to exit)?\n");
		scanf("%d", &numWakeup);
		//Sends a sem_post for however many you entered
		for (int i = 0; i < numWakeup; i++) {
			printf("Sem Post %d\n", (i + 1));
			sem_post(sem);
		}
	}

	//Closes the semaphore and unlinks
	sem_close(sem);
	sem_unlink("SEMVALUE");

	printf("Exiting thread waker");

	return EXIT_SUCCESS;
}
