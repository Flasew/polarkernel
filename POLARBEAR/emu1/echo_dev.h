#ifndef _ECHO_DEV_H
#define _ECHO_DEV_H

// MALLOC_DECLARE(M_ECHO);
// MALLOC_DEFINE(M_ECHO, "echo_buffer", "buffer for echo driver");

// #define ECHO_CLEAR_BUFFER       _IO('E', 1)

d_open_t	echo_open;
d_close_t	echo_close;
d_write_t	echo_write;
d_ioctl_t	echo_ioctl;

// typedef struct echo {
// 	int buffer_size;
// 	char * buffer;
// 	int length;
// } echo_t;

extern struct nameidata nd;

#endif