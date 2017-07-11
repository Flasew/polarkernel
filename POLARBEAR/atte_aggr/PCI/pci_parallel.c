#include <sys/param.h>
#include <sys/module.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <sys/conf.h>
#include <sys/uio.h>
#include <sys/bus.h>

#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/interrupt.h>
#include <sys/rman.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/condvar.h>
#include <sys/malloc.h>
#include <sys/queue.h>
#include <sys/taskqueue.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <sys/time.h>

#include "pci_parallel.h"

// pci-p stuff
static d_open_t     pci_p_open;
static d_close_t    pci_p_close;
static d_read_t     pci_p_read;
static d_write_t    pci_p_write;

static int pci_p_probe(device_t);
static int pci_p_attach(device_t);
static int pci_p_detach(device_t);

static void pci_p_intr(void*);

static struct cdevsw pci_p_cdevsw = {
    .d_version =    D_VERSION,
    .d_open    =    pci_p_open,
    .d_close   =    pci_p_close,
    .d_read    =    pci_p_read,
    .d_write   =    pci_p_write,
    .d_name    =    "pci_p_card"
};
     
static devclass_t pci_p_devclass;

// ilog stuff
struct mtx          ilog_mtx;

static d_open_t     ilog_open;
static d_close_t    ilog_close;
static d_read_t     ilog_read;

static struct cdevsw ilog_cdevsw = {
    .d_version =    D_VERSION,
    .d_open    =    ilog_open,
    .d_close   =    ilog_close,
    .d_read    =    ilog_read,
    .d_name    =    "ilog_device"
};

struct intr_log_softc ilog_sc = { 0 };

static int
pci_p_open(struct cdev * dev, int oflags, int devtype, struct thread * td)
{
    struct pci_p_softc * sc;
    
    sc = dev -> si_drv1;
    device_printf(sc->device, "opened P2P successfully.\n");
    return (0);
}

static int
pci_p_close(struct cdev * dev, int fflag, int devtype, struct thread * td)
{
    struct pci_p_softc * sc;
    
    sc = dev -> si_drv1;
    device_printf(sc->device, "closed P2P successfully.\n");
    return (0);
}

static int
pci_p_read(struct cdev * dev, struct uio * uio, int ioflag)
{
    struct pci_p_softc * sc;

    sc = dev -> si_drv1;
    device_printf(sc->device,"read request = %zdB\n", uio->uio_resid);
    return (0);
}


static int
pci_p_write(struct cdev * dev, struct uio * uio, int ioflag)
{
    struct pci_p_softc * sc;

    sc = dev -> si_drv1;
    device_printf(sc->device,"write request = %zdB\n", uio->uio_resid);
    return (0);
}

static void 
pci_p_log(void * arg, int pending)
{
    struct timespec tsp;
    nanouptime(&tsp);

    mtx_lock(&ilog_mtx);

    int which = ilog_sc.which;
    ++ilog_sc.logArr[which].nlogs;    
    if (ilog_sc.logArr[which].nlogs >= ARR_SIZE) {
        printf("Buffer overflowed - cleared.\n");
        ilog_sc.logArr[which].nlogs = 1;
    }

    snprintf(ilog_sc.logArr[which].logs[ilog_sc.logArr[which].nlogs - 1],
        BUF_SIZE - 1, "[%ld: %ld]: Interrupt caught.\n", 
        tsp.tv_sec, tsp.tv_nsec );

    mtx_unlock(&ilog_mtx);
}

static void
pci_p_intr(void * arg)
{
    struct pci_p_softc * sc = arg;
    // device_t dev = sc->device;
    // output...

    // struct timespec tsp;
    // nanouptime(&tsp);
    // device_printf(dev, "INTR: [%ld: %ld]: Interrupt caught.\n", 
    //     tsp.tv_sec, tsp.tv_nsec );

    taskqueue_enqueue(taskqueue_swi, &sc->log_task);

}

static int 
ilog_open(struct cdev * dev, int oflags, int devtype, struct thread * td)
{
    uprintf("Opening ilog device...\n");
    return 0;
}

static int
ilog_close(struct cdev * dev, int fflag, int devtype, struct thread * td)
{
    uprintf("Closing ilog device...\n");
    return 0;
}

static int
ilog_read(struct cdev * dev, struct uio * uio, int ioflag)
{
    uprintf("uio_resid is %zd.\n", uio->uio_resid);
    mtx_lock(&ilog_mtx);
    int which = ilog_sc.which;
    ilog_sc.which = !which;
    mtx_unlock(&ilog_mtx);

    uio->uio_resid = 8192;
    int error = 0;
    int amount;
    // uprintf("%d logs on hold\n", ilog_sc.logArr[which].nlogs);
    for (int i = 0; i < ilog_sc.logArr[which].nlogs; i++) {
        // uprintf("LOOP: %d\n", i);
        amount = MIN(uio->uio_resid,
            (strlen(ilog_sc.logArr[which].logs[i]) - uio->uio_offset > 0) ?
             strlen(ilog_sc.logArr[which].logs[i]) - uio->uio_offset : 0) ;

        error = uiomove(ilog_sc.logArr[which].logs[i] + uio->uio_offset,
                 amount, uio);
        uprintf("uio_resid is %zd.\n", uio->uio_resid);

        if (error != 0) {
            uprintf("Read failed.\n");
            break;
        }
        uio->uio_offset = 0;
    }
    ilog_sc.logArr[which].nlogs = 0;
    return error;
}

static int
pci_p_probe(device_t dev)
{
    if (pci_read_config(dev, PCIR_VENDOR, 2) == PPVID &&
        pci_read_config(dev, PCIR_DEVICE, 2) == PPDID)
        return BUS_PROBE_SPECIFIC;
    else
        return (ENXIO);
}

static int 
pci_p_attach(device_t dev)
{
    struct pci_p_softc * sc = device_get_softc(dev);
    int unit = device_get_unit(dev);

    int error = 0;

    sc->device = dev;
    
    // allocate irq...
    sc->irq = bus_alloc_resource_any(dev, SYS_RES_IRQ, &sc->irq_rid, 
                RF_ACTIVE | RF_SHAREABLE);
    if (!sc->irq) {
        device_printf(dev, "unable to allocate interrupt resource.\n");
        return (ENXIO);
    }

    // register irq
    error = bus_setup_intr(dev, sc->irq, INTR_TYPE_TTY | INTR_MPSAFE, 
                NULL, pci_p_intr, sc, &sc->icookie);
    if (error) {
        device_printf(dev, "unable to register IRQ.\n");
        bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);
        return (error);
    }
    mtx_init(&ilog_mtx, "ilog_mtx", NULL, MTX_DEF);
    TASK_INIT(&sc->log_task, 0, pci_p_log, dev);

    sc->cdev = make_dev(&pci_p_cdevsw, unit, 1001, GID_WHEEL, 0600, 
            "pcip%d", unit);
    sc->cdev->si_drv1 = sc;
    

    return (0);
}

static int 
pci_p_detach(device_t dev)
{
    struct pci_p_softc * sc = device_get_softc(dev);

    destroy_dev(sc->cdev);
    // destroy_dev(sc->log_cdev);

    bus_teardown_intr(dev, sc->irq, sc->icookie);
    bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);

    taskqueue_drain(taskqueue_swi, &sc->log_task);
    mtx_destroy(&ilog_mtx);

    return (0);
}

static int 
pci_p_load(struct module *m, int event, void *arg) {
    
    static struct cdev * ilog_cdev;

    switch (event) {
        case MOD_LOAD:
            ilog_cdev = make_dev(&ilog_cdevsw, 0, 
                    1001, GID_WHEEL, 0600, "p2plog");
            uprintf("ILOG LOADED\n");
            break;
        case MOD_UNLOAD:
            destroy_dev(ilog_cdev);
            uprintf("ILOG UNLOADED\n");
            break;
        default: 
            return (EINVAL);
    }
    return 0;
}


static device_method_t pci_p_methods[] = {
    // DEVMETHOD(device_identify,  pci_p_identify),
    DEVMETHOD(device_probe,     pci_p_probe),
    DEVMETHOD(device_attach,    pci_p_attach),
    DEVMETHOD(device_detach,    pci_p_detach),
    {0, 0}
};

static driver_t pci_p_driver = {
    "pcip",
    pci_p_methods,
    sizeof(struct pci_p_softc)
};

DRIVER_MODULE(pcip, pci, pci_p_driver, pci_p_devclass, pci_p_load, 0);

