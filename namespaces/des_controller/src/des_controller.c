#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include "des.h"

/*
 * Program used to receive commands from an input program and process
 * those commands which will result in a change in state.
 *
 * On a change in state
 */
//Function prototypes
void idle();
void rscan();
void lscan();
void runlock();
void lunlock();
void ropen();
void lopen();
void weight();
void rclose();
void lclose();
void rlock();
void llock();

volatile void (*statefunc)() = idle;

//Structs used to send data
data_client send_data;
data_server reply_data;

//Message sent to the display
char displayMessage[40] = "Waiting for person...";

int currentState = IDLE, endState = IDLE;
int main(int argc, char *argv[]) {
	//PID used to connect to the display, storing the channel id, storing the connection id, and response to the message sent
	int client_id, display_connection_id, confirmation_id;

	name_attach_t *attach;

	//Creates a channel for connection, on fail exits
	attach = name_attach(NULL, "input_pipeline", 0);

	//Printed message showing the PID
	printf("The controller is running as PID: %d\n", getpid());

	//Connects to the display channel
	display_connection_id = name_open("display_pipeline", 0);

	//Loops until the exit command is received
	while (1) {

		//Send display message to des_display
		if (MsgSend(display_connection_id, &displayMessage,
				sizeof(displayMessage), (char*) &confirmation_id,
				sizeof(confirmation_id)) == -1L) {
			fprintf(stderr, "MsgSend had an error\n");
			exit(EXIT_FAILURE);
		}

		if (strcmp("exit", send_data.code) == 0) {
			printf("Exiting controller\n");
			name_detach(attach, 0);
			name_close(display_connection_id);
			exit(EXIT_SUCCESS);
		}

		//Check for end of cycle
		if (currentState == endState && endState != IDLE) {
			statefunc = idle;
			currentState = IDLE;
			endState = IDLE;
			strcpy(displayMessage, "Waiting for person...");
		} else {
			//Receive data from des_input
			client_id = MsgReceive(attach->chid, (data_client*) &send_data,
					sizeof(send_data), NULL);
			reply_data.statusCode = OK;
		}

		(*statefunc)();

		//There is no received message to respond to at the end of the process
		MsgReply(client_id, EOK, (const char*) &reply_data, sizeof(reply_data));

		if (strcmp("exit", send_data.code) == 0) {
			strcpy(displayMessage, "Exit Display");
		}
	}

	return EXIT_SUCCESS;
}

void idle() {
	if (strcmp("rs", send_data.code) == 0) {
		if (send_data.data > 0) {
			statefunc = rscan;
			currentState = RSCAN;
			endState = LLOCK;
			snprintf(displayMessage, sizeof(displayMessage),
					"Person Scanned ID, ID = %d", send_data.data);
		} else {
			reply_data.statusCode = FAIL;
			strcpy(reply_data.errorMsg, "Improper Person ID sent");
		}
	}

	if (strcmp("ls", send_data.code) == 0) {
		if (send_data.data > 0) {
			statefunc = lscan;
			currentState = LSCAN;
			endState = RLOCK;
			snprintf(displayMessage, sizeof(displayMessage),
					"Person Scanned ID, ID = %d", send_data.data);
		} else {
			reply_data.statusCode = FAIL;
			strcpy(reply_data.errorMsg, "Improper Person ID sent");
		}
	}
}

void rscan() {
	if (strcmp("gru", send_data.code) == 0) {
		statefunc = runlock;
		currentState = RUNLOCK;
		strcpy(displayMessage, "Right door unlocked by guard");
	}
}

void lscan() {
	if (strcmp("glu", send_data.code) == 0) {
		statefunc = lunlock;
		currentState = LUNLOCK;
		strcpy(displayMessage, "Left door unlocked by guard");
	}
}

//When the state is UNLOCK the person will open the corresponding door
void runlock() {
	if (strcmp("ro", send_data.code) == 0) {
		statefunc = ropen;
		currentState = ROPEN;
		strcpy(displayMessage, "Person opened right door");
	}
}

void lunlock() {
	if (strcmp("lo", send_data.code) == 0) {
		statefunc = lopen;
		currentState = LOPEN;
		strcpy(displayMessage, "Person opened left door");
	}
}

/*
 * When the state is OPEN we need to check whether the end state to determine
 * if the door needs to be closed or to receive the weight
 */
void ropen() {
	if ((endState == RLOCK) && (strcmp("rc", send_data.code) == 0)) {
		statefunc = rclose;
		currentState = RCLOSE;
		strcpy(displayMessage, "Right door closed (automatically)");
	}
	if ((endState == LLOCK) && strcmp("ws", send_data.code) == 0) {
		statefunc = weight;
		currentState = WEIGHT;
		snprintf(displayMessage, sizeof(displayMessage),
				"Person weighed, weight =  %d", send_data.data);
	}
}

void lopen() {
	if ((endState == LLOCK) && (strcmp("lc", send_data.code) == 0)) {
		statefunc = lclose;
		currentState = LCLOSE;
		strcpy(displayMessage, "Left door closed (automatically)");
	}

	if ((endState == RLOCK) && strcmp("ws", send_data.code) == 0) {
		if (send_data.data > 0) {
			statefunc = weight;
			currentState = WEIGHT;
			snprintf(displayMessage, sizeof(displayMessage),
					"Person weighed, weight =  %d", send_data.data);
		} else {
			reply_data.statusCode = FAIL;
			strcpy(reply_data.errorMsg, "Improper weight data sent");
		}
	}
}

//When the state is WEIGHT we need to check the end state to close the correct door
void weight() {
	if ((endState == LLOCK) && (strcmp("rc", send_data.code) == 0)) {
		statefunc = rclose;
		currentState = RCLOSE;
		strcpy(displayMessage, "Right door closed (automatically)");
	}

	if ((endState == RLOCK) && (strcmp("lc", send_data.code) == 0)) {
		statefunc = lclose;
		currentState = LCLOSE;
		strcpy(displayMessage, "Left door closed (automatically)");
	}
}

//When the state is CLOSE the guard will lock the corresponding door
void rclose() {
	if (strcmp("grl", send_data.code) == 0) {
		statefunc = rlock;
		currentState = RLOCK;
		strcpy(displayMessage, "Right door locked by guard");
	}
}

void lclose() {
	if (strcmp("gll", send_data.code) == 0) {
		statefunc = llock;
		currentState = LLOCK;
		strcpy(displayMessage, "Left door locked by guard");
	}
}

/*
 * When the state is LOCK the guard will open the opposing door
 * The state will only remain in LOCK during the first door locking
 * When the second door is locked the state is returned to IDLE
 */
void rlock() {
	if (strcmp("glu", send_data.code) == 0) {
		statefunc = lunlock;
		currentState = LUNLOCK;
		strcpy(displayMessage, "Left door unlocked by guard");
	}
}

void llock() {
	if (strcmp("gru", send_data.code) == 0) {
		statefunc = runlock;
		currentState = RUNLOCK;
		strcpy(displayMessage, "Right door unlocked by guard");
	}
}

