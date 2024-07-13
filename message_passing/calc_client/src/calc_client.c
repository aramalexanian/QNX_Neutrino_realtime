#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/neutrino.h>
#include <sys/netmgr.h>
#include "../../calc_server/src/calc_message.h"

/*
 * Program used to send equations to calc_server
 * @author Aram Alexanian
 */

int main(int argc, char *argv[]) {
	//Checks for the appropriate number of arguments
	if (argc != 5) {
		printf("Usage: ./calc_client <Calc-Server-PID> left_operand operator right_operand");
		exit(EXIT_FAILURE);
	}

	//Prints the client PID
	printf("Client PID: %d\n", getpid());

	/*
	 * Initial Variables
	 * connection_id: The id of the connection made to the channel
	 * clientStruct: The struct received from the client
	 * serverStruct: The struct sent by the server
	 */
	int connection_id;
	client_send_t clientStruct;
	server_response_t serverStruct;

	//The server PID passed as the first argument

	/*
	 * Data for operation passed as arguments
	 * arg1: PID of the calc_server
	 * arg2: Left hand value
	 * arg3: operator (+ - x /)
	 * arg4: Right hand value
	 */
	int serverPID = atoi(argv[1]);
	clientStruct.left_hand = atoi(argv[2]);
	clientStruct.operator = *argv[3];
	clientStruct.right_hand = atoi(argv[4]);

	//Creating a connection to the calc_server channel
	connection_id = ConnectAttach(ND_LOCAL_NODE, serverPID, 1, _NTO_SIDE_CHANNEL, 0);

	//If connection fails exit
	if (connection_id == -1) {
		fprintf(stderr, "ConnectAttach had an error\n");
		exit(EXIT_FAILURE);
	}

	//Sends a message through the channel to calc_server with the clientStruct. On failure exits
	if (MsgSend(connection_id, (const char*) &clientStruct,
			sizeof(clientStruct), (char*) &serverStruct, sizeof(serverStruct))
			== -1L) {
		fprintf(stderr, "MsgSend had an error\n");
		exit(EXIT_FAILURE);
	}

	//Result depends on status of the server operation
	switch (serverStruct.statusCode) {
	//On okay status prints the data
	case SRVR_OK:
		printf("The server has calculated the result of %d %c %d as %.2f\n",
				clientStruct.left_hand, clientStruct.operator,
				clientStruct.right_hand, serverStruct.answer);
		break;
		//On any status that is not okay prints the passed error message
	case SRVR_INVALID_OPERATOR:
	case SRVR_UNDEFINED:
	case SRVR_OVERFLOW:
		printf("%s", serverStruct.errorMsg);
		break;
		//Prints when the reply fails
	default:
		fprintf(stderr, "Failed to receive a reply\n");
		break;
	}

	//Detaches the connection
	ConnectDetach(connection_id);

	return EXIT_SUCCESS;
}
