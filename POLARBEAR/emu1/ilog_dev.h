#ifndef _ILOG_DEV_H
#define _ILOG_DEV_H

#define ARR_SIZE 4096
#define BUF_SIZE 512

#include <sys/malloc.h>

MALLOC_DECLARE(M_ILOG)
MALLOC_DEFINE(M_ILOG, "ilog_buffer", "log messages")

struct logq {
    int             nlogs;
    char            logs[ARR_SIZE][BUF_SIZE];
};

struct intr_log_softc {
	struct mtx		*ilog_mtx;
    int             which;
    struct logq     logArr[2];
};

extern struct intr_log_softc ilog_sc;

d_open_t     ilog_open;
d_close_t    ilog_close;
d_read_t     ilog_read;

#endif