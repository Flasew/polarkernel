#ifndef _PCI_PARALLEL_H
#define _PCI_PARALLEL_H

#define PPVID 0x9710
#define PPDID 0x9912

#define ARR_SIZE 8192
#define BUF_SIZE 512

MALLOC_DECLARE(M_ILOG);
MALLOC_DEFINE(M_ILOG, "ilog_buffer", "individual log")

struct pci_p_softc {
    struct task     log_task;
    struct cdev     *cdev;
    struct cdev     *log_cdev;
    struct resource *irq;
    void            *icookie;
    device_t        device;
    int             irq_rid;
};

struct logq {
    int             nlogs;
    char            logs[ARR_SIZE][BUF_SIZE];
};

struct intr_log_softc {
    int             which;
    struct logq     logArr[2];
};


#endif