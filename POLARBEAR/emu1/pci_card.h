#ifndef _PCI_CARD_H
#define _PCI_CARD_H

#define PCVID 0x9710
#define PCDID 0x9912

struct pci_c_softc {
    struct task     log_task;
    struct cdev     *cdev;
    struct resource *irq;
    void            *icookie;
    device_t        device;
    int             irq_rid;
};

d_open_t     pci_c_open;
d_close_t    pci_c_close;
d_read_t     pci_c_read;
d_write_t    pci_c_write;

int pci_c_probe(device_t);
int pci_c_attach(device_t);
int pci_c_detach(device_t);

void pci_c_intr(void*);

devclass_t pci_c_devclass;

#endif
