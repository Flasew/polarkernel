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

#include <sys/namei.h>
#include <sys/vnode.h>
#include <sys/fcntl.h>
#include <sys/proc.h>

#include <sys/time.h>

#include "pci_card.h"
#include "ilog_dev.h"

#define TAR_DEV "/dev/echo"
     
int
pci_c_open(struct cdev * dev, int oflags, int devtype, struct thread * td)
{
    struct pci_c_softc * sc;
    
    sc = dev -> si_drv1;
    device_printf(sc->device, "opened PCI-cvt card successfully.\n");
    return (0);
}

int
pci_c_close(struct cdev * dev, int fflag, int devtype, struct thread * td)
{
    struct pci_c_softc * sc;
    
    sc = dev -> si_drv1;
    device_printf(sc->device, "closed PCI-cvt successfully.\n");
    return (0);
}

int
pci_c_read(struct cdev * dev, struct uio * uio, int ioflag)
{
    struct pci_c_softc * sc;

    sc = dev -> si_drv1;
    device_printf(sc->device,"read request = %zdB\n", uio->uio_resid);
    return (0);
}


int
pci_c_write(struct cdev * dev, struct uio * uio, int ioflag)
{
    struct pci_c_softc * sc;

    sc = dev -> si_drv1;
    device_printf(sc->device,"write request = %zdB\n", uio->uio_resid);
    return (0);
}

static void 
pci_c_log(void * arg, int pending)
{
    struct timespec tsp;
    nanouptime(&tsp);

    struct intr_log_softc * ilog_sc_p = arg;

    mtx_lock(ilog_sc_p->ilog_mtx);

    int which = ilog_sc_p->which;
    ++ilog_sc_p->logArr[which].nlogs;

    if (ilog_sc_p->logArr[which].nlogs >= ARR_SIZE) {
        printf("Buffer overflowed - cleared.\n");
        ilog_sc_p->logArr[which].nlogs = 1;
    }

    snprintf(ilog_sc_p->logArr[which].logs[ilog_sc_p->logArr[which].nlogs - 1],
        BUF_SIZE - 1, "[%ld: %ld]: Interrupt caught.\n", 
        tsp.tv_sec, tsp.tv_nsec );

    mtx_unlock(ilog_sc_p->ilog_mtx);
}

static void
pci_c_intr(void * arg)
{
    struct pci_c_softc * sc = arg;
    
    // output...
    struct timespec tsp;
    nanouptime(&tsp);

    char buffer[BUF_SIZE] = {0};
    snprintf(buffer,
        BUF_SIZE - 1, "[%ld: %ld]: Interrupt caught.\n", 
        tsp.tv_sec, tsp.tv_nsec );

    int err = write_to_cdev(TAR_DEV, buffer);
    if (err != 0){
        uprintf("ERROR writing to target\n");
        return;
    }

    taskqueue_enqueue(taskqueue_swi, &sc->log_task);

}

static int write_to_cdev(const char *cdev_path, char * mes) {

    struct nameidata nd;
    int error, flags;

    NDINIT(&nd, LOOKUP, FOLLOW, UIO_SYSSPACE, cdev_path, curthread);
    flags = FWRITE;

    error = vn_open(&nd, &flags, 0, NULL);
    if (error) {
        uprintf("error open\n");
        return (error);
    }

    NDFREE(&nd, NDF_ONLY_PNBUF);

    struct iovec iodata = {
        mes,
        strlen(mes)
    };

    struct uio fuio = {
        &iodata, 
        1,
        0,
        strlen(mes),
        UIO_SYSSPACE,
        UIO_WRITE,
        curthread
    };

    error = nd.ni_vp->v_un.vu_cdev->si_devsw->
        d_write(nd.ni_vp->v_un.vu_cdev, &fuio, 0);

    // if (!error)
    //     uprintf("write succeed\n");

    VOP_UNLOCK(nd.ni_vp, 0);
    vn_close(nd.ni_vp, FWRITE, curthread->td_ucred, curthread);
    return error;
}

int
pci_c_probe(device_t dev)
{
    if (pci_read_config(dev, PCIR_VENDOR, 2) == PCVID &&
        pci_read_config(dev, PCIR_DEVICE, 2) == PCDID)
        return BUS_PROBE_SPECIFIC;
    else
        return (ENXIO);
}

int 
pci_c_attach(device_t dev)
{
    struct pci_c_softc * sc = device_get_softc(dev);
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
                NULL, pci_c_intr, sc, &sc->icookie);
    if (error) {
        device_printf(dev, "unable to register IRQ.\n");
        bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);
        return (error);
    }

    TASK_INIT(&sc->log_task, 0, pci_c_log, &ilog_sc);

    sc->cdev = make_dev(&pci_c_cdevsw, unit, 1001, GID_WHEEL, 0600, 
            "pcip%d", unit);
    sc->cdev->si_drv1 = sc;
    
    return (0);
}

int 
pci_c_detach(device_t dev)
{
    struct pci_c_softc * sc = device_get_softc(dev);

    destroy_dev(sc->cdev);
    // destroy_dev(sc->log_cdev);

    bus_teardown_intr(dev, sc->irq, sc->icookie);
    bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);

    taskqueue_drain(taskqueue_swi, &sc->log_task);

    return (0);
}


