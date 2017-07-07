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

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

#include <sys/time.h>

#define PPVID 0x9710
#define PPDID 0x9912

struct pci_p_softc {
	device_t	    device;
	struct cdev	    *cdev;
	struct resource *irq;
    int             irq_rid;
	void		    *icookie;
};

static d_open_t		pci_p_open;
static d_close_t	pci_p_close;
static d_read_t		pci_p_read;
static d_write_t	pci_p_write;
// static d_poll_t		pci_p_poll;

static int pci_p_probe(device_t);
static int pci_p_attach(device_t);
static int pci_p_detach(device_t);

static void pci_p_intr(void*);

static struct cdevsw pci_p_cdevsw = {
	.d_version = 	D_VERSION,
	.d_open    = 	pci_p_open,
	.d_close   = 	pci_p_close,
	.d_read    =    pci_p_read,
	.d_write   =    pci_p_write,
	// .d_poll	   =    pci_p_poll,
	.d_name	   =    "pci_p_card"
};
	 
static devclass_t pci_p_devclass;

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

// static void
// pci_p_identify(driver_t * driver, device_t parent)
// {
//     device_t dev;
// 
//     dev = device_find_child(parent, "pcip", -1);
//     if (!dev)
//         BUS_ADD_CHILD(parent, 0, "pcip", -1);
// }

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

    bus_teardown_intr(dev, sc->irq, sc->icookie);
    bus_release_resource(dev, SYS_RES_IRQ, sc->irq_rid, sc->irq);

    return (0);
}

static void
pci_p_intr(void * arg)
{
    struct pci_p_softc * sc = arg;
    device_t dev = sc -> device;

    struct timespec tsp;
    nanouptime(&tsp);

    device_printf(dev, "[%ld: %ld] INTERRUPT FROM PCI-PARALLEL CARD CAUGHT.\n", 
            tsp.tv_sec, tsp.tv_nsec);
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

DRIVER_MODULE(pcip, pci, pci_p_driver, pci_p_devclass, 0, 0);

