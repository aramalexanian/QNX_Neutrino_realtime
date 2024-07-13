#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include "../../des_controller/src/des.h"

/**
 * This program us used to receive messages and display them
 * @author Aram Alexanian
 */

int main(void) {
	//The channel id, client id and confirmation to reply with
	int channel_id, client_id, confirmation_id;

	//Message that will be displayed
	char message[40], lastMessage[40];

	//Creates channel on fail will exit
	if ((channel_id = ChannelCreate(0)) == -1) {
		perror("Failed Channel Creation");
		exit(EXIT_FAILURE);
	}

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
		client_id = MsgReceive(channel_id, &message, sizeof(message), NULL);
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
			ChannelDestroy(channel_id);
			exit(EXIT_SUCCESS);
		}

	}


	return EXIT_SUCCESS;
}
