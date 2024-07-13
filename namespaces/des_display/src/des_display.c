#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include "../../des_controller/src/des.h"

/**
 * This program us used to receive messages and display them
 */

int main(void) {
	//The channel id, client id and confirmation to reply with
	int client_id, confirmation_id;

	name_attach_t *attach;

	//Message that will be displayed
	char message[40], lastMessage[40];

	attach = name_attach(NULL, "display_pipeline", 0);

	//Prints what the display is running on
	printf("The Display is running as PID: %d\n", getpid());

	while (1) {
		//Resets last message
		memset(lastMessage, 0, strlen(lastMessage));
		//Records the last message
		strcpy(lastMessage, message);
		//Resets the message
		memset(message, 0, strlen(message));
		//Receives the message
		client_id = MsgReceive(attach->chid, &message, sizeof(message), NULL);
		//Only prints the status if the message has changed
		if (strcmp(message, lastMessage) != 0) {
			printf("%s\n", message);
		}

		//Sets confirmation ID
		confirmation_id = OK;

		//Replies with confirmation
		MsgReply(client_id, EOK, (const char*) &confirmation_id,
				sizeof(confirmation_id));

		if(strcmp(message, "Exit Display") == 0){
			name_detach(attach, 0);
			exit(EXIT_SUCCESS);
		}

	}


	return EXIT_SUCCESS;
}
