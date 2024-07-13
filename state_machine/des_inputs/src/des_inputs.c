#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include "../../des_controller/src/des.h"

/*
 * This program is used for sending commands to a controller
 * @author Aram Alexanian
 */

int main(int argc, char *argv[]) {
	//Checks for the appropriate number of arguments
	if (argc != 2) {
		printf("Usage: ./des_inputs.c <Controller-PID>");
		exit(EXIT_FAILURE);
	}

	//Structs to send data
	data_client send_data;
	data_server reply_data;

	//Pid to connect to the controller, id of the connection
	int controllerPID = atoi(argv[1]), connection_id;

	//Connecting to the controller channel
	connection_id = ConnectAttach(ND_LOCAL_NODE, controllerPID, 1,
	_NTO_SIDE_CHANNEL, 0);

	//Error if connection fails
	if (connection_id == -1) {
		fprintf(stderr, "ConnectAttach had an error\n");
		exit(EXIT_FAILURE);
	}

	//Command entered to send
	char entry[4];
	//Used to store weight and person id
	int data;

	//Loops until the exit command is received
	while (strcmp("exit", entry) != 0) {
		//List of entries
		printf("\n");
		printf(
				"Enter the event type (ls=left scan, rs=right scan, ws=weight scale, lo=left open, ro=right open, lc=left closed, rc=right closed , gru=guard right unlock, grl=guard right lock, gll=guard left lock, glu=guard left unlock, exit=Exit programs)\n");
		//Takes a code
		scanf("%s", entry);

		//Assigns code to send data
		strcpy(send_data.code, entry);

		//Checks if the code is rs or ls
		if ((strcmp(entry, "rs") == 0) || (strcmp(entry, "ls") == 0)) {
			printf("Enter Person's ID:\n");
			//Enter the person id
			scanf("%d", &data);
			//Assigns person id to data
			send_data.data = data;
		}

		//Checks if the code is ws
		if ((strcmp(entry, "ws") == 0)) {
			printf("Enter Person's weight:\n");
			//Enter the weight of the person
			scanf("%d", &data);
			//Assigns the weight to data
			send_data.data = data;
		}

		/*
		 * Sends the data using the send_data struct and wait for a response
		 * with the reply_data struct
		 */
		if (MsgSend(connection_id, (const char*) &send_data, sizeof(send_data),
				(char*) &reply_data, sizeof(reply_data)) == -1L) {
			fprintf(stderr, "MsgSend had an error\n");
			exit(EXIT_FAILURE);
		}

		//Checks if the status code is FAIL and prints errorMsg
		switch (reply_data.statusCode) {
		case FAIL:
			printf("%s\n", reply_data.errorMsg);
			break;
		}

		if (strcmp("exit", entry) == 0) {
			//Removes connection
			ConnectDetach(connection_id);
			exit(EXIT_SUCCESS);
		}
	}

	return EXIT_SUCCESS;
}
