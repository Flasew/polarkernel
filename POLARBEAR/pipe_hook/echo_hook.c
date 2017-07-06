#include <sys/param.h> 
#include <sys/proc.h> 
#include <sys/module.h>

#include <sys/kernel.h> 
#include <sys/systm.h> 
#include <sys/conf.h> 
#include <sys/queue.h> 
#include <sys/lock.h> 
#include <sys/mutex.h>

#include <fs/devfs/devfs_int.h>

#include <sys/bus.h>
#include <sys/interrupt.h>

#include <dev/atkbdc/atkbdreg.h>
#include <dev/atkbdc/atkbdc_subr.h>
#include <dev/atkbdc/atkbdcreg.h>
#include <psm.h>
#include <machine/bus.h>
#include <machine/resource.h>
#include <sys/rman.h>
#include <sys/kbio.h>
#include <dev/kbd/kbdreg.h>

//extern TAILQ_HEAD(cdev_priv_list,cdev_priv) cdevp_list;

device_t kbd;
device_t * devList;
int devcount = 0;

static int atkbdattach(device_t);

d_open_t open_hook;
d_open_t * open;

d_close_t close_hook;
d_close_t * close;

d_read_t read_hook;
d_read_t * read;

d_write_t write_hook;
d_write_t * write;

d_ioctl_t ioctl_hook;
d_ioctl_t * ioctl;

static int
atkbdattach(device_t dev)
{
    atkbd_softc_t *sc;
    keyboard_t *kbd;
    u_long irq;
    int flags;
    int rid;
    int error;

    sc = device_get_softc(dev);

    rid = KBDC_RID_KBD;
    irq = bus_get_resource_start(dev, SYS_RES_IRQ, rid);
    flags = device_get_flags(dev);
    error = atkbd_attach_unit(dev, &kbd, irq, flags);
    if (error)
        return error;

    /* declare our interrupt handler */
    sc->intr = bus_alloc_resource_any(dev, SYS_RES_IRQ, &rid, RF_ACTIVE);
    if (sc->intr == NULL)
        return ENXIO;
    error = bus_setup_intr(dev, sc->intr, INTR_TYPE_TTY, NULL, atkbdintr,
            kbd, &sc->ih);
    if (error)
        bus_release_resource(dev, SYS_RES_IRQ, rid, sc->intr);

    return error;
}

int 
open_hook(struct cdev * dev, int oflags, int devtype, struct thread * td)
{
    uprintf("Openhook\n");
    return (*open)(dev, oflags, devtype, td);
}

    int 
close_hook(struct cdev * dev, int fflag, int devtype, struct thread * td)
{
    uprintf("Closehook\n");
    return (*close)(dev, fflag, devtype, td);
}

    int
ioctl_hook(struct cdev * dev, u_long cmd, caddr_t data, int fflag,
        struct thread * td)
{
    uprintf("IOctlhook\n");
    return (*ioctl)(dev, cmd, data, fflag, td);
}

int
write_hook(struct cdev * dev, struct uio * uio, int ioflag)
{
    uprintf("Writehook\n");

    return (*write)(dev, uio, ioflag);
}

int 
read_hook(struct cdev * dev, struct uio * uio, int ioflag)
{
	uprintf("Readhook\n");
	return (*read)(dev, uio, ioflag);
}


static int 
load(struct module * module, int cmd, void * arg)
{
	int error = 0;
	struct cdev_priv * cdp;

	switch(cmd){
		case MOD_LOAD:
            device_get_children(root_bus, &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            device_get_children(devList[0], &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }
            device_get_children(devList[4], &devList, &devcount);
            uprintf("Count: %d\n", devcount);
            for (int i = 0; i < devcount; i++){
                device_print_prettyname(devList[i]);
            }

            // kbd = decList[20];
            // int rid = KBDC_RID_KBD;
            
           
			mtx_lock(&devmtx);
            // TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
            //     uprintf("%s\n", cdp->cdp_c.si_name);
            // }
			TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
				if (strcmp(cdp->cdp_c.si_name, "echo") == 0) {
                    //open = cdp->cdp_c.si_devsw->d_open;
                    //cdp->cdp_c.si_devsw-> d_open = open_hook;
                    //close = cdp->cdp_c.si_devsw->d_close;
                    //cdp->cdp_c.si_devsw-> d_open = close_hook;
					read = cdp->cdp_c.si_devsw->d_read;
					cdp->cdp_c.si_devsw-> d_read = read_hook;
                    write = cdp->cdp_c.si_devsw->d_write;
                    cdp->cdp_c.si_devsw-> d_write = write_hook;
                    ioctl = cdp->cdp_c.si_devsw->d_ioctl;
                    cdp->cdp_c.si_devsw-> d_ioctl = ioctl_hook;
					break;
				}
			}
			mtx_unlock(&devmtx);
			break;
		case MOD_UNLOAD:

			mtx_lock(&devmtx);
			TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
				if (strcmp(cdp->cdp_c.si_name, "echo") == 0) {

					//cdp->cdp_c.si_devsw->d_open = open;
                    //cdp->cdp_c.si_devsw->d_close = close;
					cdp->cdp_c.si_devsw->d_read = read;
                    cdp->cdp_c.si_devsw->d_ioctl = ioctl;
                    cdp->cdp_c.si_devsw->d_write = write;
					break;
				}
			}
			mtx_unlock(&devmtx);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}
	return error;
}


static moduledata_t echo_hook_mod = {
	"echo_hook",
	load,
	NULL
};

DECLARE_MODULE(echo_hook, echo_hook_mod, SI_SUB_DRIVERS, 
	SI_ORDER_MIDDLE);


