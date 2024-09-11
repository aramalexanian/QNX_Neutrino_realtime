#include <stdlib.h>
#include <stdio.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

//Function prototypes for io and connection funcs
int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra);
int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb);
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb);

//Var for data
char data[255];
//var for connection id
int server_coid;

typedef union {
	struct _pulse pulse;
	char msg[255];
} my_message_t;

int main(int arc, char **argv) {
	//Initial vars
	dispatch_t *dpp;
	resmgr_io_funcs_t io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	iofunc_attr_t ioattr;
	dispatch_context_t *ctp;
	int id;

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

	//Fills the device attributes structure
	iofunc_attr_init(&ioattr, S_IFCHR | 0666, NULL, NULL);

	//Attaches the resource manager to a desired pathname
	//Uses the dispatch structure, pathname, filetype, connect funcs, io funcs, and device attributes
	if ((id = resmgr_attach(dpp, NULL, "/dev/local/mydevice", _FTYPE_ANY, 0,
			&connect_funcs, &io_funcs, &ioattr)) == -1) {
		fprintf(stderr, "%s:  Unable to attach name.\n", argv[0]);
		return (EXIT_FAILURE);
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

//When opening a connection with the resource manager
int io_open(resmgr_context_t *ctp, io_open_t *msg, RESMGR_HANDLE_T *handle,
		void *extra) {
	//Creates a connection to a namespace called mydevice
	if ((server_coid = name_open("mydevice", 0)) == -1) {
		perror("name_open failed.");
		return EXIT_FAILURE;
	}
	//Creates an iofunc_ocb_t structure (Open Control Block)
	return (iofunc_open_default(ctp, msg, handle, extra));
}

//Handles client calls to read() and readdir()
int io_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb) {
	//Number bytes
	int nb;
	//If no data exit
	if (data == NULL)
		return 0;

	//Number bytes set to the length of data
	nb = strlen(data);

	//test to see if we have already sent the whole message.
	if (ocb->offset == nb)
		return 0;

	//We will return which ever is smaller the size of our data or the size of the buffer
	nb = min(nb, msg->i.nbytes);

	//Set the number of bytes we will return
	_IO_SET_READ_NBYTES(ctp, nb);

	//Copy data into reply buffer.
	SETIOV(ctp->iov, data, nb);

	//update offset into our data used to determine start position for next read.
	ocb->offset += nb;

	//If we are going to send any bytes update the access time for this resource.
	if (nb > 0)
		ocb->attr->flags |= IOFUNC_ATTR_ATIME;

	return (_RESMGR_NPARTS(1));
}

//Handles client calls to write(), fwrite() and more, this includes console commands
int io_write(resmgr_context_t *ctp, io_write_t *msg, RESMGR_OCB_T *ocb) {
	//Number bytes
	int nb = 0;

	if (msg->i.nbytes == ctp->info.msglen - (ctp->offset + sizeof(*msg))) {
		/* have all the data */
		char *buf;
		char *alert_msg;
		int i, small_integer;
		buf = (char*) (msg + 1);

		//Checks if the buffer contains alert
		if (strstr(buf, "alert") != NULL) {
			//Pulls the second string from buf
			for (i = 0; i < 2; i++) {
				alert_msg = strsep(&buf, " ");
			}
			//Assigns the integer value of the second string to small_integer
			small_integer = atoi(alert_msg);
			//Checks if small_integer follows our constraints
			if (small_integer >= 1 && small_integer <= 99) {
				//Sends a pulse to the server using the namespace
				MsgSendPulse(server_coid, SchedGet(0, 0, NULL),
				_PULSE_CODE_MINAVAIL, small_integer);
			} else {
				printf("Integer is not between 1 and 99.\n");
			}
			//If not alert adds the data to the buffer
		} else {
			strcpy(data, buf);
		}

		nb = msg->i.nbytes;
	}
	//Writes the data to the file
	_IO_SET_WRITE_NBYTES(ctp, nb);

	//
	if (msg->i.nbytes > 0)
		ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_CTIME;

	return (_RESMGR_NPARTS(0));
}
