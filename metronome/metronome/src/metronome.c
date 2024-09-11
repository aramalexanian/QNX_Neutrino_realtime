struct devattr_s;
#define IOFUNC_ATTR_T struct devattr_s
struct devocb_s;
#define IOFUNC_OCB_T struct devocb_s

#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

//Used to define an metronome call or a help call
#define METRO 0
#define HELP 1

//Device names
char *devnames[2] = { "/dev/local/metronome", "/dev/local/metronome-help" };

//Struct to define each device
typedef struct devattr_s {
	iofunc_attr_t attr;
	int device;
} devattr_t;

//Struct used to create different device ocbs
typedef struct devocb_s {
	iofunc_ocb_t ocb;
	char buffer[400];
} devocb_t;

//Stores the different device attrs
devattr_t devattrs[2];

//Function prototypes
void metronome_thread();
void find_interval();
void set_expire(struct itimerspec *expire, timer_t timer_id, float startSeconds,
		float startNano, float intervalSeconds, float intervalNano);
void device_ocb_free(IOFUNC_OCB_T *docb);
IOFUNC_OCB_T* device_ocb_calloc(resmgr_context_t *ctp, devattr_t *dattr);

//var for connection id
int server_coid;

//Message received during pulses
typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;
my_message_t msg;

//pointer for namespace
name_attach_t *attach;

//Diffrent pulses sent to the thread
enum {
	_PULSE_METRONOME,
	_PULSE_PAUSE,
	_PULSE_STOP,
	_PULSE_START,
	_PULSE_SET,
	_PULSE_QUIT
};

//Different intervals
int intervals[8][4] = { { 2, 4, 4, 0 }, { 3, 4, 6, 1 }, { 4, 4, 8, 2 }, { 5, 4,
		10, 3 }, { 3, 8, 6, 4 }, { 6, 8, 6, 5 }, { 9, 8, 9, 6 },
		{ 12, 8, 12, 7 } };

//Different tempos
char *tempos[] = { "|1&2&", "|1&2&3&", "|1&2&3&4&", "|1&2&3&4-5-", "|1-2-3-",
		"|1&a2&a", "|1&a2&a3&a", "|1&a2&a3&a4&a" };

//1 second in nanoseconds
#define NANOSECONDS 1000000000
//initial variables
char *current_tempo;
int beats_per_minute, top, bottom, nano_secs, current_interval;
float seconds_per_interval;

//When opening a connection with the resource manager
int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	//Creates a connection to a namespace called metronome
	if ((server_coid = name_open("metronome", 0)) == -1) {
		perror("name_open failed.");
		return EXIT_FAILURE;
	}
	//Creates an iofunc_ocb_t structure (Open Control Block)
	return (iofunc_open_default(ctp, msg, handle, extra));
}

//Handles client calls to read() and readdir()
int io_read(resmgr_context_t *ctp, io_read_t *msg, devocb_t *docb) {
	//Number bytes
	int nb;

	//Checks if there is a call to metronome or metronome-help
	if (docb->ocb.attr->device == METRO) {
		snprintf(docb->buffer, sizeof(docb->buffer),
				"[metronome: %d beats/min, time signature %d/%d, secs-per-interval: %.2f, nanoSecs: %d]\n",
				beats_per_minute, top, bottom, seconds_per_interval, nano_secs);
	} else if (docb->ocb.attr->device == HELP) {
		snprintf(docb->buffer, sizeof(docb->buffer),
				"Metronome Resource Manager (ResMgr)\n\n\n  API:\n    pause[1-9]\t\t\t\t-pause the metronome for 1-9 seconds\n    quit\t\t\t\t-quit the metronome\n    set <bpm> <ts-top> <ts-bottom>\t-set the motronome to <bpm> ts-top/ts-bottom\n    start\t\t\t\t-start the metronome from teh stopped state\n    stop\t\t\t\t-stop the metronome, use 'start' to resume\n");
	}

	//Number bytes set to the length of data
	nb = strlen(docb->buffer);

	//test to see if we have already sent the whole message.
	if (docb->ocb.offset == nb)
		return 0;

	//We will return which ever is smaller the size of our data or the size of the buffer
	nb = min(nb, msg->i.nbytes);

	//Set the number of bytes we will return
	_IO_SET_READ_NBYTES(ctp, nb);

	//Copy data into reply buffer.
	SETIOV(ctp->iov, docb->buffer, nb);

	//update offset into our data used to determine start position for next read.
	docb->ocb.offset += nb;

	//If we are going to send any bytes update the access time for this resource.
	if (nb > 0)
		docb->ocb.attr->attr.flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}

//Handles client calls to write(), fwrite() and more, this includes console commands
int io_write(resmgr_context_t *ctp, io_write_t *msg, devocb_t *docb) {
	//Checks if there is a write call to metronome-help and exits
	if (docb->ocb.attr->device == HELP) {
		fprintf(stderr,
				"Error! You cannot write to /dev/local/metronome-help.\nTo access /dev/local/metronome-help call cat '/dev/local/metronome-help'\n");
		return 1;
	}
	//Number bytes
	int nb = 0;

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		/* have all the data */
		char *buf;
		char *value;
		int i, pause_value;
		buf = (char*) (msg + 1);

		//Checks if the buffer contains pause
		if (strstr(buf, "pause") != NULL) {
			//Pulls the second string from buf
			for (i = 0; i < 2; i++) {
				value = strsep(&buf, " ");
			}
			//Assigns the integer value of the second string to pause_value
			pause_value = atoi(value);
			//Checks if pause_value follows our constraints
			if (pause_value >= 1 && pause_value <= 9) {
				//Sends a pulse to the server using the namespace
				MsgSendPulse(server_coid, SchedGet(0, 0, NULL), _PULSE_PAUSE,
						pause_value);
			} else {
				printf("\nPause is not between 1 and 9.\n");
			}
			//If not alert adds the data to the buffer
			//Sends pulses for each different message
		} else if (strstr(buf, "stop") != NULL) {
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), _PULSE_STOP, 1);
		} else if (strstr(buf, "start") != NULL) {
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), _PULSE_START, 1);
		} else if (strstr(buf, "set") != NULL) {
			//Assigns a new value to beats_per_minute, top and bottom before sending the pulse
			for (i = 0; i < 2; i++) {
				beats_per_minute = atoi(strsep(&buf, " "));
			}
			top = atoi(strsep(&buf, " "));
			bottom = atoi(strsep(&buf, " "));
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), _PULSE_SET, 0);
		} else if (strstr(buf, "quit") != NULL) {
			//Exits the program
			MsgSendPulse(server_coid, SchedGet(0, 0, NULL), _PULSE_QUIT, 0);
			name_close(server_coid);
			return EXIT_SUCCESS;
		} else {
			//Message for invalid input
			buf[strcspn(buf, "\n")] = 0;
			fprintf(stderr, "\nError %s an invalid input...\n", buf);
		}

		nb = msg->i.nbytes;
	}
	//Writes the data to the file
	_IO_SET_WRITE_NBYTES(ctp, nb);

	//
	if (msg->i.nbytes > 0)
		docb->ocb.attr->attr.flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS(0));
}

int main(int arc, char **argv) {
	//Checks for proper usage
	if (arc != 4) {
		printf("Usage: ./metronome <bpm> <ts-top> <ts-bottom>\n");
		return EXIT_FAILURE;
	}

	//Assigns variables the arguments
	beats_per_minute = atoi(argv[1]);
	top = atoi(argv[2]);
	bottom = atoi(argv[3]);
	//Finds the interval
	find_interval(top, bottom);

	//Creates a thread running metronome_thread
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	//Creates threads
	pthread_create(NULL, &attr, &metronome_thread, NULL);
	//destroys the attributes
	pthread_attr_destroy(&attr);

	//Initial vars
	dispatch_t *dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	//iofunc_attr_t ioattr;
	dispatch_context_t *ctp;
	int id;

	iofunc_funcs_t dev_ocb_funcs = { _IOFUNC_NFUNCS, device_ocb_calloc,
			device_ocb_free };

	iofunc_mount_t dev_mount = { 0, 0, 0, 0, &dev_ocb_funcs };

	//Creating the dispatch structure
	if ((dpp = dispatch_create()) == NULL) {
		fprintf(stderr, "%s:  Unable to allocate dispatch context.\n", argv[0]);
		return (EXIT_FAILURE);
	}

	//Fills tables with default resource manager io functions
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs, _RESMGR_IO_NFUNCS,
			&io_funcs);
	//Overrides the open, read, and write functions with our own
	connect_funcs.open = io_open;
	io_funcs.read = io_read;
	io_funcs.write = io_write;

	//Creates multiple devices for the resource manager to manage
	for (int i = 0; i < 2; i++) {
		iofunc_attr_init(&devattrs[i].attr, S_IFCHR | 666, NULL, NULL);
		devattrs[i].device = i;
		devattrs[i].attr.mount = &dev_mount;
		if ((id = resmgr_attach(dpp, NULL, devnames[i], _FTYPE_ANY, 0,
				&connect_funcs, &io_funcs, &devattrs[i])) == -1) {
			fprintf(stderr, "%s:  Unable to attach name.\n");
			return (EXIT_FAILURE);
		}
	}

	//Creates a dispatch context, needed for message receive loop
	ctp = dispatch_context_alloc(dpp);
	while (1) {
		//Performs message receive
		ctp = dispatch_block(ctp);
		//Calls the appropriate function (read, write, open)
		dispatch_handler(ctp);
	}
	return EXIT_SUCCESS;
}

//Function to find the interval and tempo and assign them to global variables
void find_interval(int keyOne, int keyTwo) {
	for (int i = 0; i < (sizeof(intervals) / sizeof(intervals[0])); i++) {
		if (intervals[i][0] == keyOne && intervals[i][1] == keyTwo) {
			current_interval = intervals[i][2];
			current_tempo = tempos[intervals[i][3]];
		}
	}
}

//Thread that runs the metronome. This is run by a thread
void metronome_thread() {
	//Creates the namespace
	if ((attach = name_attach(NULL, "metronome", 0)) == NULL) {
		exit(EXIT_FAILURE);
	}
	//Calculations
	float beats_per_second = 60.0 / (float) beats_per_minute;
	float seconds_per_measure = (float) top * beats_per_second;
	seconds_per_interval = seconds_per_measure / (float) current_interval;

	//Creating a timer
	timer_t timer_id;
	struct sigevent event;
	event.sigev_notify = SIGEV_PULSE;
	event.sigev_coid = name_open("metronome", 0);
	event.sigev_priority = 10;
	event.sigev_code = _PULSE_METRONOME;
	timer_create(CLOCK_REALTIME, &event, &timer_id);

	//Start the timer
	struct itimerspec expire;
	nano_secs = seconds_per_interval * NANOSECONDS;
	set_expire(&expire, timer_id, 0, nano_secs, 0, nano_secs);

	int counter = 0;
	while (1) {
		//Waits for a pulse from the device
		int rcvid = MsgReceivePulse(attach->chid, &msg, sizeof(msg), NULL);
		//When a pulse is received
		if (rcvid == 0) {
			switch (msg.pulse.code) {
			//Performs the metronome
			case _PULSE_METRONOME:
				if (counter == 0) {
					printf("\n%c%c", current_tempo[0], current_tempo[1]);
					counter += 2;
				} else {
					printf("%c", current_tempo[counter]);
					counter++;
				}
				fflush(stdout);
				if (counter == strlen(current_tempo)) {
					counter = 0;
				}
				break;
				//Pauses the metronome
			case _PULSE_PAUSE:
				set_expire(&expire, timer_id, msg.pulse.value.sival_int, 0, 0,
						0);
				break;
				//Stops the metronome
			case _PULSE_STOP:
				set_expire(&expire, timer_id, 0, 0, 0, 0);
				break;
				//Starts the metronome
			case _PULSE_START:
				set_expire(&expire, timer_id, 0, nano_secs, 0, nano_secs);
				break;
				//Changes the speed and tempo of the metronome
			case _PULSE_SET:
				find_interval(top, bottom);
				beats_per_second = 60.0 / (float) beats_per_minute;
				seconds_per_measure = (float) top * beats_per_second;
				seconds_per_interval = seconds_per_measure
						/ (float) current_interval;
				nano_secs = seconds_per_interval * NANOSECONDS;
				set_expire(&expire, timer_id, 0, nano_secs, 0, nano_secs);
				counter = 0;
				break;
				//Quits the metronome
			case _PULSE_QUIT:
				timer_delete(timer_id);
				name_detach(attach, 0);
				name_close(event.sigev_coid);
				exit(EXIT_SUCCESS);
				break;
			}
			//On receiving anything other than a pulse the program exits
		} else {
			printf("Messaged Received is not a pulse... Exiting");
			exit(EXIT_FAILURE);
		}
	}
}

//Sets the timer
void set_expire(struct itimerspec *expire, timer_t timer_id, float startSeconds,
		float startNano, float intervalSeconds, float intervalNano) {
	expire->it_value.tv_sec = startSeconds;
	expire->it_value.tv_nsec = startNano;
	expire->it_interval.tv_sec = intervalSeconds;
	expire->it_interval.tv_nsec = intervalNano;
	timer_settime(timer_id, 0, expire, NULL);
}

//allocates memory for the device ocb
devocb_t* device_ocb_calloc(resmgr_context_t *ctp, devattr_t *dattr) {
	devocb_t *docb;
	docb = calloc(1, sizeof(devocb_t));
	docb->ocb.offset = 0;
	return (docb);
}

//Frees memory for the device ocb
void device_ocb_free(devocb_t *docb) {
	free(docb);
}
