#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

#define MY_PULSE_CODE   _PULSE_CODE_MINAVAIL
//Function to check and print the status of the device driver
int checkStatus();

//Message received during pulses
typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;
my_message_t msg;

//pointer for namespace
name_attach_t *attach;

int main(void) {
	//Creates the namespace
	if ((attach = name_attach(NULL, "mydevice", 0)) == NULL) {
		return EXIT_FAILURE;
	}
	//Check the current status exits on closed
	if (checkStatus() == 0)
		return EXIT_SUCCESS;

	while (1) {
		//Waits for a pulse from the device
		int rcvid = MsgReceivePulse(attach->chid, &msg, sizeof(msg), NULL);
		//When a pulse is received
		if (rcvid == 0) {
			//Prints the small int sent with the pulse
			printf("Small Integer: %d\n", msg.pulse.value.sival_int);
			switch (msg.pulse.code) {
			//On MY_PULSE_CODE check the status
			case MY_PULSE_CODE:
				if (checkStatus() == 0)
					return EXIT_SUCCESS;
				break;
			}
			//On receiving anything other than a pulse the program exits
		} else {
			printf("Messaged Received is not a pulse... Exiting");
			return EXIT_FAILURE;
		}
	}
	return EXIT_SUCCESS;
}

//Function used to check the status of a device driver
int checkStatus() {
	//Opens the file
	FILE *file = fopen("/dev/local/mydevice", "r");
	//Vars to store the status and value
	char status[255];
	char value[255];
	//Pulls the status from the driver
	fscanf(file, "%s", status);
	//Checks that it is status
	if (strstr(status, "status") != NULL) {
		//Pulls the value
		fscanf(file, "%s", value);
		printf("Status: %s\n", value);
		//If the value is closed then the program exits
		if (strcmp(value, "closed") == 0) {
			name_detach(attach, 0);
			return EXIT_SUCCESS;
		}
	}
	fclose(file);
	return 1;
}
