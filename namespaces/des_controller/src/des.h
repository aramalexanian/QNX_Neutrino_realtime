/*
 * Header file for door control
 * @author Aram Alexanian
 */

//Enum to describe the state of the machine
enum state {
	IDLE,
	RSCAN,
	LSCAN,
	RUNLOCK,
	LUNLOCK,
	ROPEN,
	LOPEN,
	WEIGHT,
	RCLOSE,
	LCLOSE,
	RLOCK,
	LLOCK
};

//Enum to describe status
enum status_codes {
	OK,
	FAIL,
	END
};

//Struct for sending data
struct data_send
{
	char code[5];
	int data;
} typedef data_client;

//Struct for replying
struct data_reply
{
	int statusCode;
	char errorMsg[128];
} typedef data_server;
