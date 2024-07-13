#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

//Variable used to tell if SIGUSR1 signal has been detected
volatile sig_atomic_t usr1Happened = 0;

//function to change usr1Happened to 1
void sigusr1_handler(int sig);

/*******************************************************************************
 * main( )
 ******************************************************************************/
int main(void) {
	//Sets up sigaction to use sigusr1_handler
	struct sigaction sigusr1;
	sigusr1.sa_handler = sigusr1_handler;
	sigusr1.sa_flags = 0;

	if (sigaction(SIGUSR1, &sigusr1, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	//Prints the pid when starting
	printf("PID = %d : Running \n", getpid());

	//Loops until SIGUSR1 signal detected
	while (usr1Happened == 0){
		sleep(1);
	}

	printf("PID = %d : Received USR1 \n", getpid());

	printf("PID = %d : Exiting USR1 \n", getpid());

	//Exits the function
	return EXIT_SUCCESS;
}

//Function to change usr1Happened to 1
void sigusr1_handler(int sig) {
	usr1Happened = 1;
}
