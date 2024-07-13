#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

//Value to determine if the signal has been received
volatile sig_atomic_t usr1Happened = 0;

//Function called when SIGUSR1 has been received
void sigusr1_handler(int sig);

int main(void) {
	signal(SIGUSR2, SIG_IGN);

	//parentPID:	parent's pid
	//forkNumber:	Number of forks to make
	int parentPID = getpid(), forkNumber = 0;

	//Prints parent pid when starting to run
	printf("PID = %d : Parent Running... \n", parentPID);

	printf("Enter the number of children:");
	//Enter the number of forks
	scanf("%d", &forkNumber);

	//Creates all the children
	for(int i = 0; i < forkNumber; i++){
		//adds each child fork to the pid array
		fork();

		//When a child does child actions
		if(getpid() != parentPID){
			//Setup for sigaction
			struct sigaction sigusr1;
			sigusr1.sa_handler = sigusr1_handler;
			sigusr1.sa_flags = 0;

			//Sets sigusr1 with sigaction
			if (sigaction(SIGUSR1, &sigusr1, NULL) == -1) {
				perror("sigaction");
				exit(1);
			}

			printf("PID = %d: Child Running...\n", getpid());
			//Loops until SIGUSR1 signal comes through
			while(!usr1Happened){
				sleep(1);
			}

			printf("PID = %d : Child Received USR1 \n", getpid());

			printf("PID = %d : Child Exiting USR1 \n", getpid());
			//Finishes fork process
			exit(0);
		}
	}

	signal(SIGUSR1, SIG_IGN);

	//Parent waits for all child processes to end
	if(getpid() == parentPID){
		while(forkNumber){
			wait(NULL);
			forkNumber--;
		}
	}

	printf("PID = %d : Children finished. Parent exiting \n", getpid());

	//end of parent process
	return EXIT_SUCCESS;

}

//Changes the value of usr1Happened to 1 when SIGUSR1 signal comes through
void sigusr1_handler(int sig){
	usr1Happened = 1;
}
