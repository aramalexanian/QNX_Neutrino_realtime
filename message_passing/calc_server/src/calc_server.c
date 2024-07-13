#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include "calc_message.h"
/*
 * Program used to calculate equations sent to it by the calc_client
 * @author Aram Alexanian
 */

int main(void) {
	/*
	 * Initial Variables
	 * client_id: The client connecting's id
	 * channel_id: The channel created's id
	 * clientStruct: The struct received from the client
	 * serverStruct: The struct sent by the server
	 * message: The error message added to the serverStruct
	 */
	int client_id;
	int channel_id;
	client_send_t clientStruct;
	server_response_t serverStruct;
	char message[128];

	//Creates channel
	channel_id = ChannelCreate(0);
	//Check for channel creation
	if (channel_id == -1) {
		perror("Channel creation fail.");
		exit(EXIT_FAILURE);
	}

	//Prints the server PID and Channel id
	printf("Server PID: %d\n Channel ID: %d\n", getpid(), channel_id);

	while (1) {
		//Blocks until a message is received
		client_id = MsgReceive(channel_id, (client_send_t*) &clientStruct,
				sizeof(clientStruct), NULL);

		//Pulls the operator from the sent message
		char operator = clientStruct.operator;
		//Double to store the result of the operation
		double result = 0;
		//Initialize the status of the operation
		int status = SRVR_OK;

		switch (operator) {
		//Multiplication
		case 'x':
			result = (double) clientStruct.left_hand * clientStruct.right_hand;
			break;
		//Addition
		case '+':
			result = (double) clientStruct.left_hand + clientStruct.right_hand;
			//Check for overflow
			if ((clientStruct.left_hand > 0 && clientStruct.right_hand > 0 && result < 0)
					|| result > INT_MAX) {
				//Sets status to overflow
				status = SRVR_OVERFLOW;
				//Adds overflow error message to message
				snprintf(message, sizeof(message),
						"Error message from the server: OVERFLOW %d + %d\n",
						clientStruct.left_hand, clientStruct.right_hand);
			}
			break;
		//Division
		case '/':
			//Checks for division by 0
			if (clientStruct.right_hand == 0) {
				//Sets status to undefined
				status = SRVR_UNDEFINED;
				//Adds undefined error message to message
				snprintf(message, sizeof(message),
						"Error message from the server: undefined %d / 0\n", clientStruct.left_hand);
				break;
			}
			result = (double) clientStruct.left_hand / clientStruct.right_hand;
			break;
		//Subtraction
		case '-':
			result = (double) clientStruct.left_hand - clientStruct.right_hand;
			break;
		//Invalid operator
		default:
			//Sets status to invalid operator
			status = SRVR_INVALID_OPERATOR;
			//Adds invalid operator error message to message
			snprintf(message, sizeof(message),
					"Error message from the server: INVALID OPERATOR: %c\n",
					clientStruct.operator);
			break;
		}
		//Sets the server struct variables
		serverStruct.answer = result;
		serverStruct.statusCode = status;
		strcpy(serverStruct.errorMsg, message);

		//Sends the server struct back with a reply
		MsgReply(client_id, EOK, (const char*) &serverStruct, sizeof(serverStruct));
	}

	//Destroy channel on end
	ChannelDestroy(channel_id);

	return EXIT_SUCCESS;

}
