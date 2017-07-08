#ifndef _PCI_PARALLEL_H
#define _PCI_PARALLEL_H

#define PPVID 0x9710
#define PPDID 0x9912

#define ARR_SIZE 8192
#define BUF_SIZE 512

struct pci_p_softc {
    device_t        device;
    struct cdev     *cdev;
    struct resource *irq;
    int             irq_rid;
    void            *icookie;
};

struct intr_log_softc {
    int             which;
    struct logq     logArr[2];
}

struct logq {
    int             nlogs;
    char            logs[ARR_SIZE][BUF_SIZE];
}

#endif